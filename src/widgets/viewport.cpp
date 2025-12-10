// viewport.cpp
// Implements the main editor viewport as an extension of QOpenGLWidget
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
#include <random>


#include "../undo.h"
#include "../selection.h"

#include "../util/3d/mesh.h"
#include "../util/debug.h"

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
		//glClearColor( 0.803, 0.815, 0.807, 1.0f ); set in deferred shader now

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
		m_shadowShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shadow.vert");
		m_shadowShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shadow.frag");
#ifndef NDEBUG
		if (!m_shadowShader.link()) {
			qDebug() << "Shader failed to link:" << m_shadowShader.log();
			return;
		}
#else
		m_shadowShader.link();
#endif
		m_gBufferShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/g_buffer.vert");
		m_gBufferShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/g_buffer.frag");
#ifndef NDEBUG
		if (!m_gBufferShader.link()) {
			qDebug() << "Shader failed to link:" << m_gBufferShader.log();
			return;
		}
#else
		m_gBufferShader.link();
#endif
		m_deferredShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/deferred.vert");
		m_deferredShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/deferred.frag");
#ifndef NDEBUG
		if (!m_deferredShader.link()) {
			qDebug() << "Shader failed to link:" << m_deferredShader.log();
			return;
		}
#else
		m_deferredShader.link();
#endif
		m_aoShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/ao.vert");
		m_aoShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/ao.frag");
#ifndef NDEBUG
		if (!m_aoShader.link()) {
			qDebug() << "Shader failed to link:" << m_aoShader.log();
			return;
		}
#else
		m_aoShader.link();
#endif
	}

	void Viewport::createGraphicsObjects()
	{
		std::cout << "Creating OpenGL Objects" << std::endl;
		Q_ASSERT(QOpenGLContext::currentContext());
		m_chamberVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		m_chamberIBO = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
		m_chamberShadowVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		m_chamberShadowIBO = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
		m_dotVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
		m_lineVBO = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);

		m_dotVAO.create();
		m_dotVBO.create();
		m_lineVAO.create();
		m_lineVBO.create();
		m_chamberVAO.create();
		m_chamberVBO.create();
		m_chamberIBO.create();
		m_chamberShadowVAO.create();
		m_chamberShadowVBO.create();
		m_chamberShadowIBO.create();

		m_chamberVAO.bind();
		m_chamberVBO.bind();
		m_chamberIBO.bind();

		const GLsizei stride = sizeof(TestChamberVertex);

		const int vertexPosLocation = 0;
		const int vertexNormalLocation = 1;
		const int surfaceIDLocation = 2;
		const int parentIDLocation = 3;
		const int wallSideLocation = 4;
		const int texCoordLocation = 5;
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
		
		glEnableVertexAttribArray(wallSideLocation);
		glVertexAttribPointer(texCoordLocation, 2, GL_FLOAT, GL_TRUE, stride, reinterpret_cast<const void*>(offsetof(TestChamberVertex, texCoord)));


		m_chamberVAO.release();
		m_chamberVBO.release();
		m_chamberIBO.release();

		m_chamberShadowVAO.bind();
		m_chamberShadowVBO.bind();
		m_chamberShadowIBO.bind();

		glEnableVertexAttribArray(vertexPosLocation);
		glVertexAttribPointer(vertexPosLocation, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(TestChamberVertex, Position)));

		glEnableVertexAttribArray(vertexNormalLocation);
		glVertexAttribPointer(vertexNormalLocation, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offsetof(TestChamberVertex, Normal)));

		glEnableVertexAttribArray(surfaceIDLocation);
		glVertexAttribIPointer(surfaceIDLocation, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(offsetof(TestChamberVertex, surfaceID)));

		glEnableVertexAttribArray(parentIDLocation);
		glVertexAttribIPointer(parentIDLocation, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(offsetof(TestChamberVertex, parentNodeID)));

		glEnableVertexAttribArray(wallSideLocation);
		glVertexAttribIPointer(wallSideLocation, 1, GL_UNSIGNED_INT, stride, reinterpret_cast<const void*>(offsetof(TestChamberVertex, wallSide)));
		
		m_chamberShadowVAO.release();
		m_chamberShadowVBO.release();
		m_chamberShadowIBO.release();

		// Fullscreen triangle setup for deferred rendering
		m_fullscreenVAO.create();
		m_fullscreenVAO.bind();
		// No VBO needed, we can generate the triangle in the vertex shader
		m_fullscreenVAO.release();

		createGBufferFBO();
		createSelectionFBO();
		createShadowFBO();
		createHBAOFBO();
		const std::string_view verStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		const std::string_view vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		const std::string_view renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		qDebug() << "Version: " << verStr;
		qDebug() << "Adapter: " << renderer;
		qDebug() << "Vendor: " << vendor;
	}

	void Viewport::createGBufferFBO()
	{
		const int W = int(width() * devicePixelRatioF());
		const int H = int(height() * devicePixelRatioF());
		//delete old
		if (gBufferFBO) glDeleteFramebuffers(1, &gBufferFBO);
		if (gNormal) glDeleteTextures(1, &gNormal);
		if (gAlbedo) glDeleteTextures(1, &gAlbedo);
		if (gDepth) glDeleteTextures(1, &gDepth);
		// start FBO
		glGenFramebuffers(1, &gBufferFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);

		// normal
		glGenTextures(1, &gNormal);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gNormal, 0);
		
		// albedo
		glGenTextures(1, &gAlbedo);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gAlbedo, 0);

		// depth
		glGenTextures(1, &gDepth);
		glBindTexture(GL_TEXTURE_2D, gDepth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, W, H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);
		
		// set normal and albedo buffers as draw buffers
		GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, attachments);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			qDebug() << "GBuffer FBO is NOT complete!";
		}
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
	}

	void Viewport::createShadowFBO() {
		glGenFramebuffers(1, &shadowFBO);
		if (shadowMap) glDeleteTextures(1, &shadowMap);
		glGenTextures(1, &shadowMap);

		glBindTexture(GL_TEXTURE_2D, shadowMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
			SHADOW_RES, SHADOW_RES, 0,
			GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		// make "outside the map" return lit
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_2D, shadowMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
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

	void Viewport::createHBAOFBO()
	{
		const int W = int(width() * devicePixelRatioF());
		const int H = int(height() * devicePixelRatioF());

		// Guard: if the widget is 0x0 (e.g. not shown yet), skip
		if (W == 0 || H == 0) {
			qDebug() << "Skipping AO FBO creation, size is 0x0";
			return;
		}

		if (hbaoFBO) {
			glDeleteFramebuffers(1, &hbaoFBO);
			hbaoFBO = 0;
		}
		if (aoTex) {
			glDeleteTextures(1, &aoTex);
			aoTex = 0;
		}

		glGenFramebuffers(1, &hbaoFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, hbaoFBO);

		// Single-channel AO texture
		glGenTextures(1, &aoTex);
		glBindTexture(GL_TEXTURE_2D, aoTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		


		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, aoTex, 0);

		GLenum drawBuf = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuf);

		// Now check completeness
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			qDebug() << "AO FBO incomplete! status =" << status;
		}

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
	void Viewport::loadDots(std::vector<DebugPoint> vertices)
	{
		m_dotCount = vertices.size();
		this->makeCurrent();
		m_dotVAO.bind();
		m_dotVBO.bind();

		m_dotVBO.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(DebugPoint)));

		// Position attribute
		int posLoc = m_dotShader.attributeLocation("aPos");
		m_dotShader.enableAttributeArray(posLoc);
		m_dotShader.setAttributeBuffer(
			posLoc,
			GL_FLOAT,
			offsetof(DebugPoint, position),
			3,
			sizeof(DebugPoint)
		);

		// Color attribute
		int colorLoc = m_dotShader.attributeLocation("aColor");
		m_dotShader.enableAttributeArray(colorLoc);
		m_dotShader.setAttributeBuffer(
			colorLoc,
			GL_FLOAT,
			offsetof(DebugPoint, color),
			3,
			sizeof(DebugPoint)
		);

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
		std::vector<TestChamberQuad> shadowQuads;
		m_testChamber->generateTCShadowQuads(shadowQuads);
		TestChamberMesh mesh;
		CreateTCMesh(quads, mesh);
		TestChamberMesh shadowMesh;
		CreateTCMesh(shadowQuads, shadowMesh);
		if (mesh.vertices.empty() || mesh.indices.empty()) throw std::runtime_error("Mesh has no vertices or indices to allocate");
		// if you got an error around here, it is likely because for some reason the surface generation failed so opengl tried to draw 0 surfaces
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

		m_chamberShadowVAO.bind();
		m_chamberShadowVBO.bind();
		m_chamberShadowIBO.bind();
		m_chamberShadowVBO.allocate(shadowMesh.vertices.data(), shadowMesh.vertices.size() * sizeof(TestChamberVertex));
		m_chamberShadowIBO.allocate(shadowMesh.indices.data(), shadowMesh.indices.size() * sizeof(unsigned int));
		m_chamberShadowVAO.release();
		m_chamberShadowVBO.release();
		m_chamberShadowIBO.release();

		this->doneCurrent();
		m_indicesCount = mesh.indices.size();
		m_shadowIndicesCount = shadowMesh.indices.size();
		update();
	}

	void Viewport::getViewMatrix(QMatrix4x4& out)
	{
		// Do camera offset from target point before rotating, easier that way
		out.translate(QVector3D(0, 0, -m_cameraDistance));
		out.rotate(QQuaternion::fromEulerAngles(m_cameraAngles));
		out.translate(m_cameraTarget);
	}
	//lightDir needs to be normalized
	void Viewport::getGlobalLightViewMatrix(QVector3D lightDir, QMatrix4x4& out)
	{
		//------------------------------------------------------------
		// Choose a reasonable AABB for your level region
		// (replace with your real level min/max bounds)
		//------------------------------------------------------------
		QVector3D minP(-512, -512, -512);
		QVector3D maxP(512, 512, 512);

		QVector3D center = 0.5f * (minP + maxP);
		QVector3D extent = 0.5f * (maxP - minP);

		float orthoSize = qMax(extent.x(), extent.z()) * 1.3f;   // padding
		float gDepth = extent.length() * 2.0f;                // enough to see caster + receiver

		//------------------------------------------------------------
		// Position the light camera:  
		//  Place it *behind* the geometry along -lightDir
		//------------------------------------------------------------
		QVector3D lightPos = center - lightDir * gDepth;

		//------------------------------------------------------------
		// Build the matrices
		//------------------------------------------------------------
		QMatrix4x4 lightView = lookAt(lightPos, center, QVector3D(0, 1, 0));

		QMatrix4x4 lightProj = ortho(
			-orthoSize, orthoSize,
			-orthoSize, orthoSize,
			1.0f,
			gDepth * 2.0f               // far plane
		);

		out = lightProj * lightView;
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
		const int W = int(width() * devicePixelRatioF());
		const int H = int(height() * devicePixelRatioF());
		if(m_lineVertCount == 0) return;
		glEnable(GL_POLYGON_OFFSET_LINE);
		//glPolygonOffset(-1000.0f, -1000.0f);
		m_lineShader.bind();
		m_lineShader.setUniformValue("u_view", view);
		m_lineShader.setUniformValue("u_proj", m_projection);
		m_lineShader.setUniformValue("u_screenWidth", W);
		m_lineShader.setUniformValue("u_screenHeight", H);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gDepth);
		m_lineShader.setUniformValue("u_depth", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		m_lineShader.setUniformValue("u_albedo", 1);
		m_lineVAO.bind();
		glDrawArrays(GL_LINES, 0, m_lineVertCount);
		m_lineVAO.release();
		m_lineShader.release();
		glDisable(GL_POLYGON_OFFSET_LINE);
	}

	// deprecated
	void Viewport::drawTestChamber(QMatrix4x4 view, QMatrix4x4 light, QVector3D lightDir)
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
		m_wallShader.setUniformValue("u_lightVP", light);
		m_wallShader.setUniformValue("u_lightDir", lightDir);
		m_wallShader.setUniformValue("u_drawShadow", static_cast<GLint>(m_drawShadow));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowMap);
		m_wallShader.setUniformValue("u_shadowMap", 0);

		m_chamberVAO.bind();
		glDrawElements(GL_TRIANGLES, m_indicesCount, GL_UNSIGNED_INT, 0);
		m_chamberVAO.release();
		m_wallShader.release();
	}

	void Viewport::drawGBuffer(QMatrix4x4 view)
	{
		const int W = int(width() * devicePixelRatioF());
		const int H = int(height() * devicePixelRatioF());
		glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
		glViewport(0, 0, W, H); // <- likely not necessary
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_gBufferShader.bind();
		m_gBufferShader.setUniformValue("u_view", view);
		m_gBufferShader.setUniformValue("u_proj", m_projection);

		// for test chamber that is already in world space, pass in the identity matrix
		m_gBufferShader.setUniformValue("u_model", QMatrix4x4());
		m_gBufferShader.setUniformValue("u_debugView", static_cast<GLint>(m_debugSurface));
		m_gBufferShader.setUniformValue("u_useTestChamberColor", 1);
		
		m_chamberVAO.bind();
		glDrawElements(GL_TRIANGLES, m_indicesCount, GL_UNSIGNED_INT, 0);
		m_chamberVAO.release();
		m_gBufferShader.setUniformValue("uUseTestChamberColor", 0);
		// draw other models on screen
		// making sure to pass in appropriate u_model every time

		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
	}

	void Viewport::drawDeferred(QMatrix4x4 view, QMatrix4x4 light, QVector3D lightDir)
	{
		const int W = int(width() * devicePixelRatioF());
		const int H = int(height() * devicePixelRatioF());
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
		glViewport(0, 0, W, H); // <- likely not necessary
		glDisable(GL_DEPTH_TEST); // depth information stored in depth buffer anyways

		m_deferredShader.bind();
		//essentials
		m_deferredShader.setUniformValue("u_view", view);
		m_deferredShader.setUniformValue("u_invView", view.inverted());
		m_deferredShader.setUniformValue("u_proj", m_projection);
		m_deferredShader.setUniformValue("u_invProj", m_projection.inverted());
		m_deferredShader.setUniformValue("u_screenSize", QVector2D(W, H));
		m_deferredShader.setUniformValue("u_near", m_near);
		m_deferredShader.setUniformValue("u_far", m_far);

		//global directional light
		m_deferredShader.setUniformValue("u_lightVP", light);
		m_deferredShader.setUniformValue("u_lightDir", lightDir);
		m_deferredShader.setUniformValue("u_drawShadow", static_cast<GLint>(m_drawShadow));

		//fog
		m_deferredShader.setUniformValue("u_fogColor", m_fogCol);
		if (m_fog) m_deferredShader.setUniformValue("u_fogStart", m_fogStart);
		else m_deferredShader.setUniformValue("u_fogStart", m_fogEnd);
		m_deferredShader.setUniformValue("u_fogEnd", m_fogEnd);

		//textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowMap);
		m_deferredShader.setUniformValue("u_shadowMap", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		m_deferredShader.setUniformValue("u_normal", 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		m_deferredShader.setUniformValue("u_albedo", 2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gDepth);
		m_deferredShader.setUniformValue("u_depth", 3);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, aoTex);
		m_deferredShader.setUniformValue("u_ao", 4);

		m_fullscreenVAO.bind();
		glDrawArrays(GL_TRIANGLES, 0, 3); // fullscreen triangle that is generated in the vertex shader
		m_fullscreenVAO.release();
	}

	void Viewport::drawShadow(QMatrix4x4 light)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glViewport(0, 0, SHADOW_RES, SHADOW_RES);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDisable(GL_CULL_FACE);

		m_shadowShader.bind();
		m_shadowShader.setUniformValue("u_lightVP", light);
		m_chamberShadowVAO.bind();
		glDrawElements(GL_TRIANGLES, m_shadowIndicesCount, GL_UNSIGNED_INT, 0);
		m_chamberShadowVAO.release();
		//switch back
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
		glEnable(GL_CULL_FACE);
		glViewport(0, 0, int(width() * devicePixelRatioF()), int(height() * devicePixelRatioF()));
	}

	

	void Viewport::drawAO(QMatrix4x4 view)
	{
		const int W = int(width() * devicePixelRatioF());
		const int H = int(height() * devicePixelRatioF());
		glBindFramebuffer(GL_FRAMEBUFFER, hbaoFBO);
		glViewport(0, 0, W, H);
		glDisable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.0f, 1.0f, 1.0f);  // AO = 1 (no occlusion)
		//glClear(GL_COLOR_BUFFER_BIT);

		m_aoShader.bind();
		if (!m_aoShader.bind()) {
			qDebug() << "AO shader failed to bind:" << m_aoShader.log();
		}
		// Matrices
		m_aoShader.setUniformValue("u_view", view);
		m_aoShader.setUniformValue("u_invView", view.inverted());
		m_aoShader.setUniformValue("u_proj", m_projection);
		m_aoShader.setUniformValue("u_invProj", m_projection.inverted());
		m_aoShader.setUniformValue("u_near", m_near);
		m_aoShader.setUniformValue("u_far", m_far);
		m_aoShader.setUniformValue("u_rand", 50.0f);
		m_aoShader.setUniformValueArray("u_kernel", m_aokernel.data(),(GLsizei)m_aokernel.size());

		// G-buffer inputs
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		m_aoShader.setUniformValue("u_normal", 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gDepth);
		m_aoShader.setUniformValue("u_depth", 1);

		m_fullscreenVAO.bind();
		glDrawArrays(GL_TRIANGLES, 0, 3);
		m_fullscreenVAO.release();
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
		glViewport(0, 0, W, H);
		glEnable(GL_DEPTH_TEST);
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
		//deferred rendering. draw normal and albedo as well as depth
		drawGBuffer(view);
		
		// directional light direction (world space)
		QVector3D lightDir = QVector3D(-1.0f, -1.0f, -5.0f).normalized();
		QMatrix4x4 light;
		getGlobalLightViewMatrix(lightDir, light);
		drawShadow(light);
		drawAO(view);

		drawDeferred(view, light, lightDir);
		drawDots(view);
		drawLines(view);


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
		m_projection.perspective(m_fov, aspect, m_near, m_far);
		createGBufferFBO();
		createHBAOFBO();
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
	void Viewport::generateAOKernel()
	{
		for (int i = 0; i < 64; ++i)
		{
			QVector3D sample = quaternionToUnitVector(uniformQuaternion());
			sample = sample.normalized();
			float var = 0.005f;
			float scale = (var * randf() + (1- var)); // bias closer to origin
			sample *= scale;
			m_aokernel[i] = sample;
		}
	}
	void Viewport::pertubateAOKernel(float strength)
	{
		for (int i = 0; i < 64; ++i)
		{
			QVector3D pertub(randf(), randf(), randf());
			m_aokernel[i] = (m_aokernel[i] + pertub * strength).normalized();
		}
	}
}
