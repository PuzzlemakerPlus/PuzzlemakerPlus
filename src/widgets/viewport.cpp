// viewport.cpp
// Implements the main editor viewport as an extension of QOpenGLWidget
#pragma once
#include <QMouseEvent>
#include <QWheelEvent>
#include <qnamespace.h>
#include <qvectornd.h>
#include "viewport.h"
#include "../util/3d/mesh.h"

#include "../test_chamber_quads.h"
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
		m_wallShader.removeAllShaders();
		m_wallShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/chamber_surface.vert");
		m_wallShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/chamber_surface.frag");
		m_wallShader.link();
	}

	void Viewport::loadTestChamberMesh(const TestChamber& tc)
	{
		this->makeCurrent();
		m_VAO.create();
		m_VAO.bind();

		m_vertexBuffer.create();
		m_vertexBuffer.bind();

		TestChamberQuads tcq = TestChamberQuads(tc);
		
		tcq.generateQuads();
		Mesh mesh = CreateMesh(tcq.GetQuads());

		// Disabling for now until we are more certain it's what we want - Kelsey
		//m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
		m_vertexBuffer.allocate(mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex)); // todo

		m_indexBuffer.create();
		m_indexBuffer.bind();
		m_indexBuffer.allocate(mesh.indices.data(), mesh.indices.size() * sizeof(unsigned int));
		m_indicesCount = mesh.indices.size();

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
		this->doneCurrent();
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
		//m_wallShader.setUniformValue("uVP", m_projection * view);
		//m_wallShader.setUniformValue("u_view", view);
		//m_wallShader.setUniformValue("u_proj", m_projection);
		m_wallShader.setUniformValue("u_view", view);
		m_wallShader.setUniformValue("u_proj", m_projection);
		m_wallShader.setUniformValue("u_fogColor", m_fogCol);
		if(m_fog)
		{
			m_wallShader.setUniformValue("u_fogStart", m_fogStart);
		}
		else {
			m_wallShader.setUniformValue("u_fogStart", m_fogEnd);
		}
		m_wallShader.setUniformValue("u_fogEnd", m_fogEnd);

		m_VAO.bind();
		// TODO: obviously this count needs to be fixed to reasonable number
		glDrawElements(GL_TRIANGLES, m_indicesCount, GL_UNSIGNED_INT, 0); 

		m_VAO.release();
		m_wallShader.release();
	}

	void Viewport::keyPressEvent(QKeyEvent* event)
	{
		if (event->key() == Qt::Key_F)
		{
			m_fog = !m_fog;
		}
		update();
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
