// viewport.cpp
// Implements the main editor viewport as an extension of QOpenGLWidget
#include <QMouseEvent>
#include <QWheelEvent>
#include <qvectornd.h>

#include "viewport.h"
#include "../util/3d/mesh.h"
namespace PMP
{
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

		std::vector<GLfloat> vertexBufferData;
		std::vector<GLint> indexBufferData;
		std::array<Quad, 6> quads = {
			generateCubeFace(WALLSIDE::POS_X, 128.0f),
			generateCubeFace(WALLSIDE::NEG_X, 128.0f),
			generateCubeFace(WALLSIDE::POS_Y, 128.0f),
			generateCubeFace(WALLSIDE::NEG_Y, 128.0f),
			generateCubeFace(WALLSIDE::POS_Z, 128.0f),
			generateCubeFace(WALLSIDE::NEG_Z, 128.0f),
		};
		unsigned int vertexOffset = 0;

		for (const auto& quad : quads) {
			// Add vertices
			//vertexBufferData.insert(vertexBufferData.end(), quad.verts.begin(), quad.verts.end());
			for (const auto& v : quad.verts) {
				vertexBufferData.push_back(v.Position.x());
				vertexBufferData.push_back(v.Position.y());
				vertexBufferData.push_back(v.Position.z());

				vertexBufferData.push_back(v.Normal.x());
				vertexBufferData.push_back(v.Normal.y());
				vertexBufferData.push_back(v.Normal.z());
			}

			// Add indices (adjusted)
			for (unsigned int idx : quad.indices) {
				indexBufferData.push_back(idx + vertexOffset);
			}

			vertexOffset += static_cast<unsigned int>(quad.verts.size());
		}

		// Disabling for now until we are more certain it's what we want - Kelsey
		//m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
		m_vertexBuffer.allocate(vertexBufferData.data(), vertexBufferData.size() * sizeof(Vertex)); // todo

		m_indexBuffer.create();
		m_indexBuffer.bind();
		m_indexBuffer.allocate(indexBufferData.data(), indexBufferData.size() * sizeof(unsigned int));

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

		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0); //Because you need 36 indices (6 per face) to draw a cube

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
