// viewport.cpp
// Implements the main editor viewport as an extension of QOpenGLWidget
#pragma once
#include <stdexcept>
#include <vector>
#include <GL/gl.h>
#include <QMouseEvent>
#include <QWheelEvent>
#include <qvectornd.h>
#include <qmatrix4x4.h>
#include <qnamespace.h>

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
		loadShaders();
		createVAOs();
		loadDots({QVector3D(128.0f,128.0f,128.0f)}); //Need to call these to create VAOS
		loadLines(std::vector<QVector3D>()); //Need to call these to create VAOS

		connect(m_frameTimer, &QTimer::timeout, this, &Viewport::onFrameUpdate);
		m_frameTimer->start(16); // ~60 FPS
	}

	void Viewport::loadShaders()
	{
		m_wallShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/chamber_surface.vert");
		m_wallShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/chamber_surface.frag");
#ifndef NDEBUG
		if (!m_wallShader.link()) {
			qDebug() << "Shader failed to link:" << m_wallShader.log();
			return;
		}
#else
		m_wallShader.link()
#endif
		m_dotShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/dot.vert");
		m_dotShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/dot.frag");
#ifndef NDEBUG
		if (!m_dotShader.link()) {
			qDebug() << "Shader failed to link:" << m_dotShader.log();
			return;
		}
#else
		m_dotShader.link();
#endif
		m_lineShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/line.vert");
		m_lineShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/line.frag");
#ifndef NDEBUG
		if (!m_lineShader.link()) {
			qDebug() << "Shader failed to link:" << m_lineShader.log();
			return;
		}
#else
		m_lineShader.link()
#endif
	}
	void Viewport::createVAOs()
	{
		m_dotVAO.create();
		m_lineVAO.create();
		m_chamberVAO.create();
	}

	// needs a list of points to draw
	void Viewport::loadDots(std::vector<QVector3D> vertices)
	{
		m_dotCount = vertices.size();
		this->makeCurrent();
		m_dotVAO.bind();
		m_dotVBO.create();
		m_dotVBO.bind();
		m_dotVBO.allocate(vertices.data(),vertices.size() * sizeof(QVector3D));

		int posLoc = m_dotShader.attributeLocation("aPos");
		m_dotShader.enableAttributeArray(posLoc);
		m_dotShader.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, sizeof(QVector3D));

		m_dotVAO.release();
		m_dotVBO.release();

		this->doneCurrent();
	}
	// needs at least 2 verts to form a line
	void Viewport::loadLines(std::vector<QVector3D> vertices)
	{
		m_lineVertCount = vertices.size();

		this->makeCurrent();
		m_lineVAO.bind();
		m_lineVBO.create();
		m_lineVBO.bind();
		m_lineVBO.allocate(vertices.data(), vertices.size() * sizeof(QVector3D));

		int posLoc = m_lineShader.attributeLocation("aPos");
		m_lineShader.enableAttributeArray(posLoc);
		m_lineShader.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, sizeof(QVector3D));

		m_lineVAO.release();
		m_lineVBO.release();
		this->doneCurrent();
	}
	void Viewport::loadLines(Ray ray, float length)
	{
		std::vector<QVector3D> vertices = {ray.origin, ray.origin + ray.direction * length};
		loadLines(vertices);
	}
	// Generate Test Chamber Mesh
	void Viewport::loadTestChamberMesh()
	{
		if(!m_testChamber) throw std::runtime_error("Can not load test chamber mesh without setting test chamber");
		
		std::vector<Quad> quads;
		m_testChamber->GenerateQuads(quads);
		Mesh mesh = CreateMesh(quads);

		this->makeCurrent();
		m_chamberVAO.bind();
		m_chamberVBO.create();
		m_chamberVBO.bind();
		// Disabling for now until we are more certain it's what we want - Kelsey
		//m_chamberVBO.setUsagePattern(QOpenGLBuffer::StaticDraw);
		m_chamberVBO.allocate(mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex)); // todo

		m_chamberIBO.create();
		m_chamberIBO.bind();
		m_chamberIBO.allocate(mesh.indices.data(), mesh.indices.size() * sizeof(unsigned int));
		m_indicesCount = mesh.indices.size();

		int vertexPosLocation = m_wallShader.attributeLocation("aPos");
		m_wallShader.enableAttributeArray(vertexPosLocation);
		m_wallShader.setAttributeBuffer(vertexPosLocation, GL_FLOAT, 0, 3, 6 * sizeof(GLfloat));

		int vertexNormalLocation = m_wallShader.attributeLocation("aNormal");
		m_wallShader.enableAttributeArray(vertexNormalLocation);
		m_wallShader.setAttributeBuffer(vertexNormalLocation, GL_FLOAT, 3 * sizeof(GLfloat), 3, 6 * sizeof(GLfloat));

		// must release VAO before index/vertex buffers, otherwise they get unbound from the VAO too!
		m_chamberVAO.release();

		m_chamberIBO.release();
		m_chamberVBO.release();
		this->doneCurrent();
		update();
	}
	void Viewport::getViewMatrix(QMatrix4x4& out)
	{
		// Do camera offset from target point before rotating, easier that way
		out.translate(QVector3D(0, 0, -m_cameraDistance));
		out.rotate(QQuaternion::fromEulerAngles(m_cameraAngles));
		out.translate(m_cameraTarget);
	}
	void Viewport::drawDots(QMatrix4x4 view)
	{
		if(m_dotCount == 0) return;
		m_dotShader.bind();
		m_dotShader.setUniformValue("u_view", view);
		m_dotShader.setUniformValue("u_proj", m_projection);
		m_dotShader.setUniformValue("u_size", 10.0f);
		m_dotVAO.bind();
		glDrawArrays(GL_POINTS, 0, m_dotCount);
		m_dotVAO.release();
		m_dotShader.release();
	}
	void Viewport::drawLines(QMatrix4x4 view)
	{
		if(m_lineVertCount == 0) return;
		m_lineShader.bind();
		m_lineShader.setUniformValue("u_view", view);
		m_lineShader.setUniformValue("u_proj", m_projection);
		m_lineShader.setUniformValue("u_color", QVector3D(1.0f, 0.0f, 0.0f));
		m_lineVAO.bind();
		glDrawArrays(GL_LINES, 0, m_lineVertCount);
		m_lineVAO.release();
		m_lineShader.release();
	}
	void Viewport::drawTestChamber(QMatrix4x4 view)
	{
		if(m_indicesCount == 0) return;
		m_wallShader.bind();
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

		m_chamberVAO.bind();
		glDrawElements(GL_TRIANGLES, m_indicesCount, GL_UNSIGNED_INT, 0); 
		m_chamberVAO.release();
		m_wallShader.release();
	}

	Ray Viewport::ScreenPosToWorldRay(QPoint mousePos)
	{
		float x = (2.0f * mousePos.x()) / width() - 1.0f;
		float y = 1.0f - (2.0f * mousePos.y()) / height();
		float z = 1.0f; // Forward direction

		QVector4D rayClip(x, y, -1.0f, 1.0f); // NDC near plane

		QMatrix4x4 proj = m_projection;
		QMatrix4x4 view;
		getViewMatrix(view);
		QMatrix4x4 invVP = (proj * view).inverted();

		QVector4D rayEye = proj.inverted() * rayClip;
		rayEye = QVector4D(rayEye.x(), rayEye.y(), -1.0, 0.0); // Direction in eye space

		QVector3D rayWorld = (view.inverted() * rayEye).toVector3D().normalized();

		QVector3D rayOrigin = (view.inverted() * QVector4D(0, 0, 0, 1)).toVector3D();

		return Ray{rayOrigin, rayWorld}; // Define your Ray struct as needed
	}


// QT Functions
	void Viewport::paintGL()
	{
		// Very temporary rendering code
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glEnable( GL_DEPTH_TEST );
		glEnable( GL_CULL_FACE );
		glEnable(GL_PROGRAM_POINT_SIZE);
		
		QMatrix4x4 view;
		getViewMatrix(view);
		drawDots(view);
		drawLines(view);
		drawTestChamber(view);
	}

	void Viewport::resizeGL( int width, int height )
	{
		const float aspect = (float)width / (float)(height > 0 ? height : 1);
		m_projection.setToIdentity();
		// Source near clip plane value, might change later
		m_projection.perspective(m_fov, aspect, 7.0f, 32767.0f);
	}

	void Viewport::keyPressEvent(QKeyEvent* event)
	{
		if (event->key() == Qt::Key_F)
		{
			m_fog = !m_fog;
		}
		if (event->key() == Qt::Key_Q)
		{
			Ray ray = ScreenPosToWorldRay(this->mapFromGlobal(QCursor::pos()));
			loadLines(ray);
			if(m_testChamber)
			{
				std::optional<RayHitResult> cast = m_testChamber->RayCast(ray);
				if(cast.has_value())
				{
					loadDots({cast->hitPoint});
				}
			}
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

	// This function gets called every 16ms
	void Viewport::onFrameUpdate()
	{
		bool updateGraphics = false;
		QVector2D currentMouseGlobal = QVector2D(this->mapFromGlobal(QCursor::pos()));
		QVector2D delta = currentMouseGlobal - m_lastMousePos;
		if ( m_isOrbiting )
		{
			QVector3D deltaOrbitAngle = 0.2f * QVector3D(delta.y() / 2.0f, 0, delta.x() / 2.0f); // todo make this customizable
			m_cameraAngles += deltaOrbitAngle;

			// Don't allow flipping the camera
			// 0 is straight down and -180 is straight up due to the way we handle the view matrix transform right now
			if ( m_cameraAngles.x() > 0.0f )
				m_cameraAngles.setX(0.0f);
			else if ( m_cameraAngles.x() < -180.0f )
				m_cameraAngles.setX(-180.0f);

			m_OrbitVelocity = deltaOrbitAngle;
			m_shouldOrbitInertia = true;
			updateGraphics = true;
		}

		if ( m_isPanning )
		{
			QMatrix4x4 camAngle;
			camAngle.rotate(QQuaternion::fromEulerAngles(m_cameraAngles));
			camAngle = camAngle.transposed(); // Reverse the transformation

			QVector3D moveAmount(delta.x(), -delta.y(), 0.0f);
			moveAmount = camAngle * moveAmount * 1.4f;

			m_cameraTarget += moveAmount;
			updateGraphics = true;
		}

		m_lastMousePos = currentMouseGlobal;
		
		if (m_shouldOrbitInertia && !m_isOrbiting) {
			m_cameraAngles += m_OrbitVelocity;

			// Clamp again
			if (m_cameraAngles.x() > 0.0f)
				m_cameraAngles.setX(0.0f);
			else if (m_cameraAngles.x() < -180.0f)
				m_cameraAngles.setX(-180.0f);

			// Dampen inertia
			m_OrbitVelocity *= 0.9f;

			// Stop when very slow
			if (m_OrbitVelocity.lengthSquared() < 0.0001f) {
				m_OrbitVelocity = QVector3D(0, 0, 0);
				m_shouldOrbitInertia = false;
			}
			updateGraphics = true;
		}
		if(updateGraphics)
		{
			update();//Draws the frame again because the view changed
		}
	}
}
