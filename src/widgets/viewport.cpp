// viewport.cpp
// Implements the main editor viewport as an extension of QOpenGLWidget
#pragma once
#include <qassert.h>
#include <vector>
#include <QPainter>
#include <QThread>
#include <iostream>
#include <stdexcept>
#include <qmatrix4x4.h>
#include <qnamespace.h>
#include <QMouseEvent>
#include <QWheelEvent>
#include <qvectornd.h>
#include <QOpenGLExtraFunctions>

#include "../undo.h"
#include "../selection.h"

#include "../util/3d/mesh.h"
#include "viewport.h"

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
		auto* ext = context()->extraFunctions();
		ext->glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
							   GL_DEBUG_SEVERITY_NOTIFICATION,
							   0, nullptr, GL_FALSE);
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
		m_selectionShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/chamber_selection.vert");
		m_selectionShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/chamber_selection.frag");
#ifndef NDEBUG
		if (!m_selectionShader.link()) {
			qDebug() << "Shader failed to link:" << m_selectionShader.log();
			return;
		}
#else
		m_selectionShader.link();
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

		m_chamberVAO.bind();
		m_chamberVBO.bind();
		m_chamberIBO.bind();

		const GLsizei stride = sizeof(TestChamberVertex);


		//Important to note: m_wallShader and m_selectionShader share the vao. 
		// const int vertexPosLocation = m_selectionShader.attributeLocation("aPos");
		// const int vertexNormalLocation = m_selectionShader.attributeLocation("aNormal");
		// const int surfaceIDLocation = m_selectionShader.attributeLocation("aSurfaceId");
		// const int parentIDLocation = m_selectionShader.attributeLocation("aParentId");
		// const int wallSideLocation = m_selectionShader.attributeLocation("aWallSide");
		// std::cout << vertexPosLocation << vertexNormalLocation << surfaceIDLocation << parentIDLocation << wallSideLocation<< "\n";
		const int vertexPosLocation = 0;
		const int vertexNormalLocation = 1;
		const int surfaceIDLocation = 2;
		const int parentIDLocation = 3;
		const int wallSideLocation = 4;
		bool vaoCheck = allDifferent({vertexPosLocation, vertexNormalLocation, surfaceIDLocation, parentIDLocation, wallSideLocation});
		Q_ASSERT(vaoCheck && "All positions of VAO must have unique position");
		glEnableVertexAttribArray(vertexPosLocation);
		glVertexAttribPointer(vertexPosLocation, 3, GL_FLOAT, GL_FALSE, stride,reinterpret_cast<const void*>(offsetof(TestChamberVertex, Position)));

		glEnableVertexAttribArray(vertexNormalLocation);
		glVertexAttribPointer(vertexNormalLocation, 3, GL_FLOAT, GL_FALSE, stride,reinterpret_cast<const void*>(offsetof(TestChamberVertex, Normal)));

		glEnableVertexAttribArray(surfaceIDLocation);
		glVertexAttribIPointer(surfaceIDLocation, 1, GL_UNSIGNED_INT, stride,reinterpret_cast<const void*>(offsetof(TestChamberVertex, surfaceID)));

		glEnableVertexAttribArray(parentIDLocation);
		glVertexAttribIPointer(parentIDLocation, 1, GL_UNSIGNED_INT, stride,reinterpret_cast<const void*>(offsetof(TestChamberVertex, parentNodeID)));

		glEnableVertexAttribArray(wallSideLocation);
		glVertexAttribIPointer(wallSideLocation, 1, GL_UNSIGNED_INT, stride,reinterpret_cast<const void*>(offsetof(TestChamberVertex, wallSide)));
		
		
		m_chamberVAO.release();
		m_chamberVBO.release();
		m_chamberIBO.release();
		createSelectionFBO();
		const std::string_view verStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		const std::string_view vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		const std::string_view renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		qDebug() << "Version: " << verStr;
		qDebug() << "Adapter: " << renderer;
		qDebug() << "Vendor: " << vendor;
	}

	void Viewport::createSelectionFBO()
	{
		makeCurrent(); // ensure context current if not in initializeGL/resizeGL
		
		const GLsizei baseW = GLsizei(width()  * devicePixelRatioF());
		const GLsizei baseH = GLsizei(height() * devicePixelRatioF());
		const int W = std::max(1, int(std::floor(baseW * selectionTextureScale)));
		const int H = std::max(1, int(std::floor(baseH * selectionTextureScale)));
		
		// delete old if any
		if (selectionDep) glDeleteRenderbuffers(1, &selectionDep);
		if (selectionTex) glDeleteTextures(1, &selectionTex);
		if (selectionFBO) glDeleteFramebuffers(1, &selectionFBO);

		glGenFramebuffers(1, &selectionFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, selectionFBO);

		// Integer color texture
		glGenTextures(1, &selectionTex);
		glBindTexture(GL_TEXTURE_2D, selectionTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexImage2D(GL_TEXTURE_2D, 0,
					GL_RGBA32UI,          // integer internal format
					W, H, 0,
					GL_RGBA_INTEGER,      // integer external format (NOT GL_RGBA)
					GL_UNSIGNED_INT,      // integer type
					nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
							GL_TEXTURE_2D, selectionTex, 0);

		// Depth renderbuffer
		glGenRenderbuffers(1, &selectionDep);
		glBindRenderbuffer(GL_RENDERBUFFER, selectionDep);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, H);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								GL_RENDERBUFFER, selectionDep);

		// Draw buffer list
		const GLenum db = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &db);

		// Completeness check
		const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			qWarning() << "Selection FBO incomplete, status =" << Qt::hex << status;
		}

		// Back to Qt's onscreen FBO
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
	}

	void Viewport::resizeSelectionFBO(GLsizei baseW, GLsizei baseH)
	{
		if (baseW <= 0 || baseH <= 0) return; // guard against minimized window
		const int W = std::max(1, int(std::floor(baseW * selectionTextureScale)));
		const int H = std::max(1, int(std::floor(baseH * selectionTextureScale)));

		// If you haven’t created the FBO yet, make it once.
		if (!selectionFBO) {
			createSelectionFBO();     // your existing function
			return;                   // createSelectionFBO already sized to current widget
		}

		// Reallocate attachments to the new size
		glBindFramebuffer(GL_FRAMEBUFFER, selectionFBO);

		// Color (integer) texture
		glBindTexture(GL_TEXTURE_2D, selectionTex);
		glTexImage2D(GL_TEXTURE_2D, 0,
					GL_RGBA32UI,           // integer internal format
					W, H, 0,
					GL_RGBA_INTEGER,       // integer external format
					GL_UNSIGNED_INT,       // integer type
					nullptr);

		// Depth renderbuffer
		glBindRenderbuffer(GL_RENDERBUFFER, selectionDep);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, H);

		// Make sure draw buffers are still set
		const GLenum db = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &db);

		// Check completeness
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			qWarning() << "Selection FBO incomplete after resize, status =" << Qt::hex << status;
		}

		// Return to Qt’s default FBO
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
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
		Q_ASSERT(m_selector);
		std::vector<QVector3D> corners;
		m_selector->getSelectionGeometry(corners);
		std::vector<LineVertex> lines;
		MultiQuadCornersToLineWireframe(corners,lines);
		loadLines(lines);
	}

	void Viewport::loadTestChamberMesh()
	{
		Q_ASSERT(m_testChamber);
		if(!m_testChamber) throw std::runtime_error("Can not load test chamber mesh without setting test chamber");
		
		std::vector<TestChamberQuad> quads;
		m_testChamber->generateTCQuads(quads);
		TestChamberMesh mesh;
		CreateTCMesh(quads, mesh);
		if (mesh.vertices.empty() || mesh.indices.empty()) throw std::runtime_error("Mesh has no vertices or indices to allocate");
		this->makeCurrent();
		m_chamberVAO.bind();
		m_chamberVBO.bind();
		m_chamberIBO.bind();
		// Disabling for now until we are more certain it's what we want - Kelsey
		//m_chamberVBO.setUsagePattern(QOpenGLBuffer::StaticDraw);
		
		m_chamberVBO.allocate(mesh.vertices.data(), mesh.vertices.size() * sizeof(TestChamberVertex));
		m_chamberIBO.allocate(mesh.indices.data(), mesh.indices.size() * sizeof(unsigned int));
		
		m_chamberVAO.release(); // must release VAO before index/vertex buffers, otherwise they get unbound from the VAO too!
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
		m_wallShader.setUniformValue("u_debugView", static_cast<GLint>(m_debugSurface));
		if(m_fog) m_wallShader.setUniformValue("u_fogStart", m_fogStart);
		else m_wallShader.setUniformValue("u_fogStart", m_fogEnd);
		m_wallShader.setUniformValue("u_fogEnd", m_fogEnd);

		m_chamberVAO.bind();
		glDrawElements(GL_TRIANGLES, m_indicesCount, GL_UNSIGNED_INT, 0);
		m_chamberVAO.release();
		m_wallShader.release();
	}

	void Viewport::drawTestChamberSurfaceAttributes(QMatrix4x4 view)
	{
		
		if(m_indicesCount == 0) return;
		if(m_lastDragRect.width() == 0 || m_lastDragRect.height() == 0) return;
		// bind offscreen FBO
		m_selectionShader.bind();
		glBindFramebuffer(GL_FRAMEBUFFER, selectionFBO);
		glDisable(GL_BLEND);
		glDisable(GL_DITHER);
		glEnable(GL_DEPTH_TEST);
		const int W = int(width() * devicePixelRatioF() * selectionTextureScale);
		const int H = int(height() * devicePixelRatioF() * selectionTextureScale);
		glViewport(0, 0, W, H);

		// Do not implement scissors here
		// Since m_lastDragRect can change during gpu rendering, using scissors can lead to pixels not being drawn when it should be.

		// clear integer color + depth
		const GLuint zero[4] = {0,0,0,0};
		glClearBufferuiv(GL_COLOR, 0, zero);
		glClear(GL_DEPTH_BUFFER_BIT);
		
		GLint curProg = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &curProg);
		Q_ASSERT(curProg == (GLint)m_selectionShader.programId());
		Q_ASSERT(m_selectionShader.isLinked());
		m_selectionShader.setUniformValue("u_view", view);
		m_selectionShader.setUniformValue("u_proj", m_projection);
		m_chamberVAO.bind();

		glDrawElements(GL_TRIANGLES, m_indicesCount, GL_UNSIGNED_INT, 0);
		m_chamberVAO.release();
		m_selectionShader.release();

		//Switch back to default framebuffer object
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
		glViewport(0, 0, int(width() * devicePixelRatioF()), int(height() * devicePixelRatioF()));
		glEnable(GL_BLEND);
		glEnable(GL_DITHER);
	}

	void Viewport::drawDragBox()
	{
		QPainter p(this); // safe in QOpenGLWidget
		p.setRenderHint(QPainter::Antialiasing, true);
		QColor edge(77,163,255);
		QColor fill(77,163,255, 40);
		p.fillRect(m_rect, fill);
		QPen pen(edge);
		pen.setStyle(Qt::DashLine);
		p.setPen(pen);
		p.drawRect(m_rect.adjusted(0,0,-1,-1));
	}

// QT Functions
	void Viewport::paintGL()
	{
		//qDebug() << "Start drawing";
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glEnable( GL_DEPTH_TEST );
		glEnable( GL_CULL_FACE );
		glEnable(GL_PROGRAM_POINT_SIZE);
		
		QMatrix4x4 view;
		getViewMatrix(view);
		drawDots(view);
		drawLines(view);
		//qDebug() << "Start drawing testchamber";
		drawTestChamber(view);
		//qDebug() << "Start drawing testchamber attribute";
		drawTestChamberSurfaceAttributes(view);

		// after your normal scene draw (default FBO bound)
		const int W = int(width()*devicePixelRatioF());
		const int H = int(height()*devicePixelRatioF());
		glViewport(0,0,W,H);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Important: neutral GL state before QPainter
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_CULL_FACE);
		glBindVertexArray(0);
		glUseProgram(0);
	}

	void Viewport::resizeGL( int width, int height )
	{
		Q_ASSERT(m_selector);
		const float aspect = (float)width / (float)(height > 0 ? height : 1);
		m_projection.setToIdentity();
		// Source near clip plane value, might change later
		m_projection.perspective(m_fov, aspect, 7.0f, 32767.0f);
		const int W = int(width * devicePixelRatioF());
		const int H = int(height * devicePixelRatioF());
		resizeSelectionFBO(W, H);
	}

	void Viewport::reloadChamberMeshAndSelection()
	{
		loadTestChamberMesh();
		loadSelectionLines();
	}

	bool Viewport::readSelectionRectCPU(ScreenSurfaceSet& out)
	{
		//TODO: this setting does not seem to work on my windows machine
		const int x = m_lastDragRect.left() * selectionTextureScale;
		const int y = height() * selectionTextureScale- m_lastDragRect.bottom() * selectionTextureScale;
		const int w = m_lastDragRect.width() * selectionTextureScale;
		const int h = m_lastDragRect.height() * selectionTextureScale;

		if (w <= 0 || h <= 0) return false;
		// 4 uints (RGBA) per pixel
		std::vector<uint32_t> buf(size_t(w) * h * 4);

		glBindFramebuffer(GL_FRAMEBUFFER, selectionFBO);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(x, y, w, h, GL_RGBA_INTEGER, GL_UNSIGNED_INT, buf.data());
		// Back to Qt’s default FBO so you don’t accidentally draw into the integer one
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

		out.clear();
		for (int i = 0; i < w * h; ++i) {
			const uint32_t parent  = buf[i*4 + 0];
			const uint32_t surface = buf[i*4 + 1];
			const WallSide side    = static_cast<WallSide>(buf[i*4 + 2]);
			out.emplace(parent, surface, side);
		}
		return true;
	}

	void Viewport::keyPressEvent(QKeyEvent* event)
	{
		Q_ASSERT(m_selector);
		Q_ASSERT(m_testChamber);
		bool actionPerformed = false;
		if (event->matches(QKeySequence::Undo)) {
			UndoStack::instance().undo();
			actionPerformed = true;
		}
		else if (event->matches(QKeySequence::Redo)) {
			UndoStack::instance().redo();
			actionPerformed = true;
		}
		else {
			switch (event->key()) {
			case Qt::Key_F:
				m_fog = !m_fog; // toggle fog. No need to reload chamber mesh and selection
				event->accept();
				update();
				break;
			case Qt::Key_G:
				m_debugSurface = !m_debugSurface; // toggle debug mode. No need to reload chamber mesh and selection
				event->accept();
				update();
				break;
			case Qt::Key_Equal: //Extrude surface
				actionPerformed = m_testChamber->extrudeSurfaces(m_selector);
				break;
			case Qt::Key_Minus: //Recede surface
				actionPerformed = m_testChamber->recedeSurfaces(m_selector);
				break;
			// Grid snap adjustment
			case Qt::Key_BracketLeft:
				m_testChamber->decreaseGridSize();
				updateGridSizeLabel();
				break;
			case Qt::Key_BracketRight:
				m_testChamber->increaseGridSize();
				updateGridSizeLabel();
				break;
			case Qt::Key_Escape:
				m_selector->clearSelection();
				actionPerformed = true;
				break;
			case Qt::Key_F5: // force visual reload for debugging
				actionPerformed = true;
				break;
			default:
				QWidget::keyPressEvent(event);
				break;
			}
		}
		if (actionPerformed)
		{
			event->accept();
			reloadChamberMeshAndSelection();
			update();
		}
	}

	void Viewport::mousePressEvent(QMouseEvent* event)
	{
		Q_ASSERT(m_testChamber);
		QMatrix4x4 viewMatrix;
		getViewMatrix(viewMatrix);
		std::optional<RayHitResult> rayCastChamber = m_testChamber->rayCast(screenPosToWorldRay(width(),height(),this->mapFromGlobal(QCursor::pos()), m_projection, viewMatrix));

		switch (event->button())
		{
			case Qt::MouseButton::LeftButton:
				//Needs to be called after mainViewport.show()
				if (rayCastChamber.has_value())
				{
#if 0
					m_origin = event->pos();
					//Start dragging
					setSelecting(true,QRect(m_origin, QSize()));
#endif
					m_selector->startSelection(rayCastChamber.value());
					m_isDragging = true; // We can consider a left click to always be dragging in the new selection system
					loadSelectionLines();
					update();
				}
				else {
					//Orbit if mouse click outside test chamber
					m_isOrbiting = true;
				}
				m_lastMouseClickPos = QVector2D(event->position());
				event->accept();
				break;
			case Qt::MouseButton::MiddleButton:
				// Middle click always orbits
				m_isOrbiting = true;
				m_lastMouseClickPos = QVector2D(event->position());
				event->accept();
				break;
			case Qt::MouseButton::RightButton:
				// To match PTI, rmb hold+drag needs to always pan, but releasing without moving opens context menus
				// For now, it always pans since we don't have context menus yet
				m_isPanning = true;
				m_lastMouseClickPos = QVector2D(event->position());
				event->accept();
				break;
			default:
				break;
		}
		m_lastMousePos = QVector2D(event->position());
	}

	void Viewport::mouseMoveEvent(QMouseEvent* event) 
	{
		if (m_isDragging) 
		{
#if 0
			m_selector->clearSurfaces();
			setSelecting(true, QRect(m_origin, event->pos()).normalized());
			m_lastDragRect = m_rect;
			ScreenSurfaceSet surfaces;
			readSelectionRectCPU(surfaces);
			for(const auto& s : surfaces)
			{
				//std::cout << s.toString() << " ";
				auto surf= m_testChamber->getSurface(s.parentID, s.wallSide);
				if(surf.has_value()) m_selector->addSelection(surf.value());
			}
#endif
			// TODO: Probably want to switch this to use the frame buffer sampling, ray casts work for now though
			QMatrix4x4 viewMatrix;
			getViewMatrix(viewMatrix);
			std::optional<RayHitResult> rayCastChamber = m_testChamber->rayCast(screenPosToWorldRay(width(),height(),this->mapFromGlobal(QCursor::pos()), m_projection, viewMatrix));

			if ( rayCastChamber.has_value() )
			{
				m_selector->updateSelection(rayCastChamber.value());
				loadSelectionLines();
				update();
			}
		}
	}

	void Viewport::mouseReleaseEvent(QMouseEvent* event)
	{
		QVector2D newPos = QVector2D(event->position());
		// detects whether the mouse moved since last frame.
		// used to determine click vs drag
		// so the 'click' actions are run when the mouse is released without moving it
		if ((newPos - m_lastMouseClickPos).length() > 1)
		{
			switch (event->button())
			{
			case Qt::MouseButton::LeftButton: //End Orbiting/Dragging
				if ( m_isDragging )
				{
					m_selector->endSelection();
					m_isDragging = false;
				}
				m_isOrbiting = false;
				event->accept();
				break;
			case Qt::MouseButton::MiddleButton: // End Orbiting
				m_isOrbiting = false;
				event->accept();
				break;
			case Qt::MouseButton::RightButton: //End Panning
				m_isPanning = false;
				event->accept();
				break;
			default:
				break;
			}
		}
		else //Mouse did not move
		{
			QMatrix4x4 viewMatrix;
			switch (event->button())
			{
			case Qt::MouseButton::LeftButton: //Click Select Surface
				if ( m_isDragging )
				{
					m_selector->endSelection();
					m_isDragging = false;
				}
				m_isOrbiting = false;
				event->accept();
				break;
			case Qt::MouseButton::MiddleButton:
				m_isOrbiting = false;
				event->accept();
				break;
			default:
				break;
			}
		}
		update();
	}

	void Viewport::wheelEvent( QWheelEvent* event )
	{
		float prev_dist = m_cameraDistance;
		m_cameraDistance -= event->angleDelta().y() / 2.0f; // todo make this customizable

		if ( m_cameraDistance < cameraDistMin ) m_cameraDistance = cameraDistMin;
		else if ( m_cameraDistance > cameraDistMax ) m_cameraDistance = cameraDistMax;
		
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

	void Viewport::resizeEvent( QResizeEvent* event )
	{
		m_gridSizeLabel.resize(width(), m_gridSizeLabel.height());
		m_gridSizeLabel.move(width() - m_gridSizeLabel.width(), height() - m_gridSizeLabel.height());
		m_gridSizeLabel.setAlignment(Qt::AlignmentFlag::AlignRight | Qt::AlignmentFlag::AlignBottom);
		updateGridSizeLabel();

		QOpenGLWidget::resizeEvent(event);
	}

	void Viewport::updateGridSizeLabel()
	{
		if ( !m_testChamber )
			return;

		m_gridSizeLabel.setText(QString("Grid Size: ") + QString::number(m_testChamber->getGridSize()));
	}
}
