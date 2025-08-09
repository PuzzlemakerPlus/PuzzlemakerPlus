// viewport.cpp
// Implements the main editor viewport as an extension of QOpenGLWidget
#pragma once
#include <stdexcept>
#include <vector>
//#include <GL/gl.h> //
#include <qmatrix4x4.h>
#include <qnamespace.h>
#include <QMouseEvent>
#include <QWheelEvent>
#include <qvectornd.h>
#include <QThread>
#include <iostream>

#include "viewport.h"
#include "../selection.h"
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
			m_debugLogger.enableMessages();
		}
		else
		{
			qDebug() << "WARNING: OpenGL debug output is not supported on this system.\n";
		}
#endif
		// Puzzlemaker background color
		glClearColor( 0.803, 0.815, 0.807, 1.0f );
		loadShaders();
		createGraphicsObjects();

		
		std::cout << "ViewportGL initialized." << std::endl;
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
		m_wallShader.link();
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
		m_lineShader.link();
#endif
	}
	void Viewport::createGraphicsObjects()
	{
		std::cout << "Creating OpenGL Objects" << std::endl;
		Q_ASSERT(QOpenGLContext::currentContext());
		m_chamberVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		m_chamberIBO = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
		m_dotVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		m_lineVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		m_dotVAO.create();
		m_dotVBO.create();
		m_lineVAO.create();
		m_lineVBO.create();

		m_chamberVAO.create();
		m_chamberVBO.create();
		m_chamberIBO.create();

		const std::string_view verStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		const std::string_view vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		const std::string_view renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		qDebug() << "Version: " << verStr;
		qDebug() << "Adapter: " << renderer;
		qDebug() << "Vendor: " << vendor;
	}

	// needs a list of points to draw
	void Viewport::loadDots(std::vector<QVector3D> vertices)
	{
		m_dotCount = vertices.size();
		this->makeCurrent();
		m_dotVAO.bind();

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
	void Viewport::loadLines(std::vector<LineVertex> vertices)
	{
		m_lineVertCount = vertices.size();

		this->makeCurrent();
		m_lineVAO.bind();

		m_lineVBO.bind();
		m_lineVBO.allocate(vertices.data(), vertices.size() * sizeof(LineVertex));

		int posLoc = m_lineShader.attributeLocation("aPos");
		m_lineShader.enableAttributeArray(posLoc);
		m_lineShader.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, sizeof(LineVertex));

		int vertexColorLocation = m_lineShader.attributeLocation("aColor");
		m_lineShader.enableAttributeArray(vertexColorLocation);
		m_lineShader.setAttributeBuffer(vertexColorLocation, GL_FLOAT, sizeof(QVector3D), 3, sizeof(LineVertex));

		m_lineVAO.release();
		m_lineVBO.release();
		this->doneCurrent();
	}
	void Viewport::loadLines(Ray ray, float length, QVector3D colStart, QVector3D colEnd)
	{
		loadLines({{ray.origin, colStart}, {ray.origin + ray.direction * length, colEnd}});
	}

	void Viewport::loadSelectionLines()
	{
		std::vector<QVector3D> corners;
		Q_ASSERT(m_selector);
		m_selector->getSelectionSurfaceCorners(corners);
		std::vector<LineVertex> lines;
		MultiQuadCornersToLineWireframe(corners,lines);
		loadLines(lines);
	}

	// Generate Test Chamber Mesh
	void Viewport::loadTestChamberMesh()
	{
		std::cout << "Loading test chamber mesh..." << std::endl;
		Q_ASSERT(m_testChamber);
		if(!m_testChamber) throw std::runtime_error("Can not load test chamber mesh without setting test chamber");
		
		std::vector<Quad> quads;
		m_testChamber->GenerateQuads(quads);
		Mesh mesh = CreateMesh(quads);
		if (mesh.vertices.empty() || mesh.indices.empty()) throw std::runtime_error("Mesh has no vertices or indices to allocate");

		if (!QOpenGLContext::currentContext()) {
			std::cout << "No OpenGL context! As expected" << std::endl;
		}
		else {
			std::cout << "OpenGL context is available." << std::endl;
		}
		this->makeCurrent();
		if (!QOpenGLContext::currentContext()) {
			std::cout << "No OpenGL context! Can't safely check isCreated." << std::endl;
			return;
		}
		else {
			 std::cout << "OpenGL context is available." << std::endl;
		}

		m_chamberVAO.bind();
		//qDebug() << "Creating m_chamberVBO";
		m_chamberVBO.bind();
		//qDebug() << "Creating m_chamberIBO";
		m_chamberIBO.bind();

		// Disabling for now until we are more certain it's what we want - Kelsey
		//m_chamberVBO.setUsagePattern(QOpenGLBuffer::StaticDraw);
		// I think this is correct
		//m_chamberVBO.setUsagePattern(QOpenGLBuffer::DynamicDraw);
		//qDebug() << "Allocating m_chamberVBO";
		m_chamberVBO.allocate(mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex)); // segfault here makse no sense

		//qDebug() << "Allocating m_chamberIBO";
		m_chamberIBO.allocate(mesh.indices.data(), mesh.indices.size() * sizeof(unsigned int));
		

		int vertexPosLocation = m_wallShader.attributeLocation("aPos");
		m_wallShader.enableAttributeArray(vertexPosLocation);
		m_wallShader.setAttributeBuffer(vertexPosLocation, GL_FLOAT, 0, 3, 6 * sizeof(GLfloat));

		int vertexNormalLocation = m_wallShader.attributeLocation("aNormal");
		m_wallShader.enableAttributeArray(vertexNormalLocation);
		m_wallShader.setAttributeBuffer(vertexNormalLocation, GL_FLOAT, 3 * sizeof(GLfloat), 3, 6 * sizeof(GLfloat));

		// must release VAO before index/vertex buffers, otherwise they get unbound from the VAO too!
		m_chamberVAO.release();

		m_chamberVBO.release(); // not necessary?
		m_chamberIBO.release(); // not necessary?
		this->doneCurrent();
		m_indicesCount = mesh.indices.size();
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
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1000.0f, -1000.0f);
		m_lineShader.bind();
		m_lineShader.setUniformValue("u_view", view);
		m_lineShader.setUniformValue("u_proj", m_projection);
		m_lineVAO.bind();
		glDrawArrays(GL_LINES, 0, m_lineVertCount);
		m_lineVAO.release();
		m_lineShader.release();
		glDisable(GL_POLYGON_OFFSET_LINE);
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
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			qDebug() << "OpenGL error: " << QString::number(err, 16);
		}
		m_chamberVAO.release();
		m_wallShader.release();
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
		Q_ASSERT(m_selector);
		// Selector needs to be aware of the viewport size.
		m_selector->setViewPortDim(width, height);
		
	}

	void Viewport::keyPressEvent(QKeyEvent* event)
	{
		Q_ASSERT(m_selector);
		Q_ASSERT(m_testChamber);
		if (event->matches(QKeySequence::Undo)) {
			UndoStack::instance().undo();
			loadTestChamberMesh();
			loadSelectionLines();
		}
		else if (event->matches(QKeySequence::Redo)) {
			UndoStack::instance().redo();
			loadTestChamberMesh();
			loadSelectionLines();
		}
		else if (event->key() == Qt::Key_F)
		{
			m_fog = !m_fog;
		}
		else if (event->key() == Qt::Key_Equal)
		{
			if(m_testChamber->ExtrudeSurface(m_selector))
			{
				// Prepare to draw the new chamber and the new selection
				loadTestChamberMesh();
				loadSelectionLines();
			}
		}
		else if (event->key() == Qt::Key_Minus)
		{
			if(m_testChamber->RecedeSurface(m_selector))
			{
				// Prepare to draw the new chamber and the new selection
				loadTestChamberMesh();
				loadSelectionLines();
			}
		}
		update();
	}
	void Viewport::mousePressEvent(QMouseEvent* event)
	{
		
		QMatrix4x4 viewMatrix;
		getViewMatrix(viewMatrix);
		Q_ASSERT(m_testChamber);
		std::optional<RayHitResult> rayCastChamber = m_testChamber->RayCast(
			ScreenPosToWorldRay(
				width(),
				height(),
				this->mapFromGlobal(QCursor::pos()), 
				m_projection, viewMatrix));

		if ( event->button() == Qt::MouseButton::RightButton )
		{
			if(rayCastChamber.has_value())
			{
				// Right click on surface/object
			}
			else {
				m_isPanning = true;
			}
			m_lastMouseClickPos = QVector2D(event->position());
			event->accept();
		}
		else if ( event->button() == Qt::MouseButton::LeftButton ||
				  event->button() == Qt::MouseButton::MiddleButton )
		{
			//Needs to be called after mainViewport.show()
			if(rayCastChamber.has_value())
			{
				//Start dragging
				m_isDragging = true;
			}
			else {
				//Orbit if mouse click outside test chamber
				m_isOrbiting = true;
			}
			m_lastMouseClickPos = QVector2D(event->position());
			event->accept();
		}

		m_lastMousePos = QVector2D(event->position());
	}
	void Viewport::mouseReleaseEvent(QMouseEvent* event)
	{
		if ( event->button() == Qt::MouseButton::RightButton )
		{
			if(m_isPanning)
			{
				m_isPanning = false;
			}
			event->accept();
		}
		else if ( event->button() == Qt::MouseButton::LeftButton ||
				  event->button() == Qt::MouseButton::MiddleButton )
		{
			if(m_isOrbiting)
			{
				m_isOrbiting = false;
			}
			if(m_isPanning)
			{
				m_isDragging = false;
			}
			// Surface Selection
			QVector2D newPos = QVector2D(event->position());
			if((newPos - m_lastMouseClickPos).length()<1)
			{
				//Selection
				QMatrix4x4 viewMatrix;
				getViewMatrix(viewMatrix);
				Q_ASSERT(m_selector);
				m_selector->clickSelectSurface(this->mapFromGlobal(QCursor::pos()), m_projection, viewMatrix);

				std::vector<QVector3D> corners;
				m_selector->getSelectionSurfaceCorners(corners);
				std::vector<LineVertex> lines;
				MultiQuadCornersToLineWireframe(corners,lines);
				loadLines(lines);
			}
			event->accept();
		}
		update();
	}
	void Viewport::wheelEvent( QWheelEvent* event )
	{
		float prev_dist = m_cameraDistance;
		m_cameraDistance -= event->angleDelta().y() / 2.0f; // todo make this customizable

		if ( m_cameraDistance < cameraDistMin )
			m_cameraDistance = cameraDistMin;
		else if ( m_cameraDistance > cameraDistMax )
			m_cameraDistance = cameraDistMax;
		
		if(prev_dist - m_cameraDistance != 0) update();
		event->accept();
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

			m_orbitVelocity = deltaOrbitAngle;
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
			m_cameraAngles += m_orbitVelocity;

			// Clamp again
			if (m_cameraAngles.x() > 0.0f)
				m_cameraAngles.setX(0.0f);
			else if (m_cameraAngles.x() < -180.0f)
				m_cameraAngles.setX(-180.0f);

			// Dampen inertia
			m_orbitVelocity *= 0.9f;

			// Stop when very slow
			if (m_orbitVelocity.lengthSquared() < 0.0001f) {
				m_orbitVelocity = QVector3D(0, 0, 0);
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
