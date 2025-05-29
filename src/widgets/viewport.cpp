// viewport.cpp
// Implements the main editor viewport as an extension of QOpenGLWidget

#include <QMouseEvent>
#include <QWheelEvent>

#include "viewport.h"

namespace PMP
{
	// FIXME: remove
	static GLfloat const tempVertices[] = {
		// bottom
		0.0f,   0.0f,   0.0f,   0.0f, 0.0f, 1.0f,
		128.0f, 0.0f,   0.0f,   0.0f, 0.0f, 1.0f,
		128.0f, 128.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		0.0f,   128.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		// south
		128.0f, 0.0f,   0.0f,   0.0f, 1.0f, 0.0f,
		0.0f,   0.0f,   0.0f,   0.0f, 1.0f, 0.0f,
		0.0f,   0.0f,   128.0f, 0.0f, 1.0f, 0.0f,
		128.0f, 0.0f,   128.0f, 0.0f, 1.0f, 0.0f,
		// east
		128.0f, 128.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		128.0f, 0.0f,   0.0f,   -1.0f, 0.0f, 0.0f,
		128.0f, 0.0f,   128.0f, -1.0f, 0.0f, 0.0f,
		128.0f, 128.0f, 128.0f, -1.0f, 0.0f, 0.0f,
		// north
		0.0f,   128.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		128.0f, 128.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		128.0f, 128.0f, 128.0f, 0.0f, -1.0f, 0.0f,
		0.0f,   128.0f, 128.0f, 0.0f, -1.0f, 0.0f,
		// west
		0.0f,   0.0f,   0.0f,   1.0f, 0.0f, 0.0f,
		0.0f,   128.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		0.0f,   128.0f, 128.0f, 1.0f, 0.0f, 0.0f,
		0.0f,   0.0f,   128.0f, 1.0f, 0.0f, 0.0f,
		// top
		128.0f, 0.0f,   128.0f, 0.0f, 0.0f, -1.0f,
		0.0f,   0.0f,   128.0f, 0.0f, 0.0f, -1.0f,
		0.0f,   128.0f, 128.0f, 0.0f, 0.0f, -1.0f,
		128.0f, 128.0f, 128.0f, 0.0f, 0.0f, -1.0f,
	};
	static GLint const tempIndices[] = {
		// bottom
		0, 1, 2,
		0, 2, 3,
		// south
		4, 5, 6,
		4, 6, 7,
		// east
		8, 9,  10,
		8, 10, 11,
		// north
		12, 13, 14,
		12, 14, 15,
		// west
		16, 17, 18,
		16, 18, 19,
		// top
		20, 21, 22,
		20, 22, 23,
	};

	void Viewport::initializeGL()
	{
		initializeOpenGLFunctions();

#ifndef NDEBUG
		if ( m_debugLogger.initialize() )
		{
			connect( &m_debugLogger, &QOpenGLDebugLogger::messageLogged, [=](const QOpenGLDebugMessage &debugMessage) {
						qDebug() << debugMessage;
					});
			m_debugLogger.startLogging();
		}
		else
		{
			qDebug() << "WARNING: OpenGL debug output is not supported on this system.\n";
		}
#endif

		// Puzzlemaker background color
		glClearColor( 0.803, 0.815, 0.807, 1.0f );

		m_wallShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/chamber_surface.vert");
		m_wallShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/chamber_surface.frag");
		m_wallShader.link();

		m_VAO.create();
		m_VAO.bind();

		m_vertexBuffer.create();
		m_vertexBuffer.bind();
		// Disabling for now until we are more certain it's what we want - Kelsey
		//m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
		m_vertexBuffer.allocate(tempVertices, sizeof(tempVertices)); // todo

		m_indexBuffer.create();
		m_indexBuffer.bind();
		m_indexBuffer.allocate(tempIndices, sizeof(tempIndices));

		int vertexPosLocation = m_wallShader.attributeLocation("aPos");
		m_wallShader.enableAttributeArray(vertexPosLocation);
		m_wallShader.setAttributeBuffer(vertexPosLocation, GL_FLOAT, 0, 3, 6 * sizeof(GLfloat));

		int vertexNormalLocation = m_wallShader.attributeLocation("aNormal");
		m_wallShader.enableAttributeArray(vertexNormalLocation);
		m_wallShader.setAttributeBuffer(vertexNormalLocation, GL_FLOAT, 3 * sizeof(GLfloat), 3, 6 * sizeof(GLfloat));

		// must release VAO before index/vertex buffers, otherwise they get unbound from the VAO too!
		m_VAO.release();

		m_indexBuffer.release();
		m_vertexBuffer.release();
	}

	void Viewport::resizeGL( int width, int height )
	{
		const float aspect = (float)width / (float)(height > 0 ? height : 1);
		m_projection.setToIdentity();
		// Source near clip plane value, might change later
		m_projection.perspective(m_fov, aspect, 7.0f, 32767.0f);
	}

	void Viewport::paintGL()
	{
		// Very temporary rendering code
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glEnable( GL_DEPTH_TEST );
		glEnable( GL_CULL_FACE );
		
		m_wallShader.bind();

		QMatrix4x4 view;
		// Do camera offset from target point before rotating, easier that way
		view.translate(QVector3D(0, 0, -m_cameraDistance));
		view.rotate(QQuaternion::fromEulerAngles(m_cameraAngles));
		view.translate(m_cameraTarget);
		m_wallShader.setUniformValue("uVP", m_projection * view);

		m_VAO.bind();

		glDrawElements(GL_TRIANGLES, sizeof(tempIndices) / sizeof(GLint), GL_UNSIGNED_INT, 0);

		m_VAO.release();
		m_wallShader.release();
	}

	void Viewport::mousePressEvent(QMouseEvent* event)
	{
		if ( event->button() == Qt::MouseButton::RightButton )
		{
			m_isPanning = true;
			event->accept();
		}
		else if ( event->button() == Qt::MouseButton::LeftButton ||
				  event->button() == Qt::MouseButton::MiddleButton )
		{
			m_isOrbiting = true;
			event->accept();
		}

		m_lastMousePos = QVector2D(event->position());
	}

	void Viewport::mouseReleaseEvent(QMouseEvent* event)
	{
		if ( event->button() == Qt::MouseButton::RightButton )
		{
			m_isPanning = false;
			event->accept();
		}
		else if ( event->button() == Qt::MouseButton::LeftButton ||
				  event->button() == Qt::MouseButton::MiddleButton )
		{
			m_isOrbiting = false;
			event->accept();
		}
	}

	void Viewport::mouseMoveEvent( QMouseEvent* event )
	{
		QVector2D delta = QVector2D(event->position()) - m_lastMousePos;
		if ( m_isOrbiting )
		{
			m_cameraAngles += QVector3D(delta.y() / 2.0f, 0, delta.x() / 2.0f); // todo make this customizable

			// Don't allow flipping the camera
			// 0 is straight down and -180 is straight up due to the way we handle the view matrix transform right now
			if ( m_cameraAngles.x() > 0.0f )
				m_cameraAngles.setX(0.0f);
			else if ( m_cameraAngles.x() < -180.0f )
				m_cameraAngles.setX(-180.0f);

			event->accept();
		}

		if ( m_isPanning )
		{
			QMatrix4x4 camAngle;
			camAngle.rotate(QQuaternion::fromEulerAngles(m_cameraAngles));
			camAngle = camAngle.transposed(); // Reverse the transformation

			QVector3D moveAmount(delta.x(), -delta.y(), 0.0f);
			moveAmount = camAngle * moveAmount;

			m_cameraTarget += moveAmount;
		}

		m_lastMousePos = QVector2D(event->position());
		update();
	}

	void Viewport::wheelEvent( QWheelEvent* event )
	{
		m_cameraDistance -= event->angleDelta().y() / 2.0f; // todo make this customizable

		if ( m_cameraDistance < cameraDistMin )
			m_cameraDistance = cameraDistMin;
		else if ( m_cameraDistance > cameraDistMax )
			m_cameraDistance = cameraDistMax;

		event->accept();
		update();
	}
}
