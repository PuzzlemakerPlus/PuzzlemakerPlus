#include "viewport.h"
#include <iostream>
namespace PMP
{
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
		bool vaoCheck = allDifferent({ vertexPosLocation, vertexNormalLocation, surfaceIDLocation, parentIDLocation, wallSideLocation });
		Q_ASSERT(vaoCheck && "All positions of VAO must have unique position");
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

		const int W = int(width() * devicePixelRatioF());
		const int H = int(height() * devicePixelRatioF());
		createGBufferFBO(W, H);
		createSSAOFBO(W, H);
		createBlurFBO(W, H);
		createSelectionFBO();
		createShadowFBO();


		const std::string_view verStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
		const std::string_view vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		const std::string_view renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		qDebug() << "Version: " << verStr;
		qDebug() << "Adapter: " << renderer;
		qDebug() << "Vendor: " << vendor;
	}

	void Viewport::createBlurFBO(const int W, const int H)
	{
		if (m_blurFBO) glDeleteFramebuffers(1, &m_blurFBO);
		if (m_blurTex) glDeleteTextures(1, &m_blurTex);

		glGenFramebuffers(1, &m_blurFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO);

		glGenTextures(1, &m_blurTex);
		glBindTexture(GL_TEXTURE_2D, m_blurTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_blurTex, 0);

		// set normal and albedo buffers as draw buffers
		GLenum attachments = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &attachments);

		// Completeness check
		const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			qWarning() << "Selection FBO incomplete, status =" << Qt::hex << status;
		}

		// Back to Qt's onscreen FBO
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
	}

	void Viewport::createGBufferFBO(const int W, const int H)
	{
		//delete old
		if (m_gBufferFBO) glDeleteFramebuffers(1, &m_gBufferFBO);
		if (m_gNormal) glDeleteTextures(1, &m_gNormal);
		if (m_gAlbedo) glDeleteTextures(1, &m_gAlbedo);
		if (m_gDepth) glDeleteTextures(1, &m_gDepth);
		// start FBO
		glGenFramebuffers(1, &m_gBufferFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_gBufferFBO);

		// normal
		glGenTextures(1, &m_gNormal);
		glBindTexture(GL_TEXTURE_2D, m_gNormal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gNormal, 0);

		// albedo
		glGenTextures(1, &m_gAlbedo);
		glBindTexture(GL_TEXTURE_2D, m_gAlbedo);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_gAlbedo, 0);

		// depth
		glGenTextures(1, &m_gDepth);
		glBindTexture(GL_TEXTURE_2D, m_gDepth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, W, H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_gDepth, 0);

		// set normal and albedo buffers as draw buffers
		GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, attachments);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			qDebug() << "GBuffer FBO is NOT complete!";
		}
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
	}

	void Viewport::createShadowFBO() {
		glGenFramebuffers(1, &m_shadowFBO);
		if (m_shadowMap) glDeleteTextures(1, &m_shadowMap);
		glGenTextures(1, &m_shadowMap);

		glBindTexture(GL_TEXTURE_2D, m_shadowMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
			SHADOW_RES, SHADOW_RES, 0,
			GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		// make "outside the map" return lit
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_2D, m_shadowMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
	}

	void Viewport::createSelectionFBO()
	{
		makeCurrent(); // ensure context current if not in initializeGL/resizeGL

		const GLsizei baseW = GLsizei(width() * devicePixelRatioF());
		const GLsizei baseH = GLsizei(height() * devicePixelRatioF());
		const int W = std::max(1, int(std::floor(baseW * m_selectionTextureScale)));
		const int H = std::max(1, int(std::floor(baseH * m_selectionTextureScale)));

		// delete old if any
		if (m_selectionDep) glDeleteRenderbuffers(1, &m_selectionDep);
		if (m_selectionTex) glDeleteTextures(1, &m_selectionTex);
		if (m_selectionFBO) glDeleteFramebuffers(1, &m_selectionFBO);

		glGenFramebuffers(1, &m_selectionFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_selectionFBO);

		// Integer color texture
		glGenTextures(1, &m_selectionTex);
		glBindTexture(GL_TEXTURE_2D, m_selectionTex);
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
			GL_TEXTURE_2D, m_selectionTex, 0);

		// Depth renderbuffer
		glGenRenderbuffers(1, &m_selectionDep);
		glBindRenderbuffer(GL_RENDERBUFFER, m_selectionDep);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, H);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_RENDERBUFFER, m_selectionDep);

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

	void Viewport::createSSAOFBO(const int W, const int H)
	{
		// Guard: if the widget is 0x0 (e.g. not shown yet), skip
		if (W == 0 || H == 0) {
			qDebug() << "Skipping AO FBO creation, size is 0x0";
			return;
		}

		if (m_ssaoFBO) {
			glDeleteFramebuffers(1, &m_ssaoFBO);
			m_ssaoFBO = 0;
		}
		if (m_aoTex) {
			glDeleteTextures(1, &m_aoTex);
			m_aoTex = 0;
		}

		glGenFramebuffers(1, &m_ssaoFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);

		// Single-channel AO texture
		glGenTextures(1, &m_aoTex);
		glBindTexture(GL_TEXTURE_2D, m_aoTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, m_aoTex, 0);

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
		const int W = std::max(1, int(std::floor(baseW * m_selectionTextureScale)));
		const int H = std::max(1, int(std::floor(baseH * m_selectionTextureScale)));

		// If you haven¡¯t created the FBO yet, make it once.
		if (!m_selectionFBO) {
			createSelectionFBO();     // your existing function
			return;                   // createSelectionFBO already sized to current widget
		}

		// Reallocate attachments to the new size
		glBindFramebuffer(GL_FRAMEBUFFER, m_selectionFBO);

		// Color (integer) texture
		glBindTexture(GL_TEXTURE_2D, m_selectionTex);
		glTexImage2D(GL_TEXTURE_2D, 0,
			GL_RGBA32UI,           // integer internal format
			W, H, 0,
			GL_RGBA_INTEGER,       // integer external format
			GL_UNSIGNED_INT,       // integer type
			nullptr);

		// Depth renderbuffer
		glBindRenderbuffer(GL_RENDERBUFFER, m_selectionDep);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, H);

		// Make sure draw buffers are still set
		const GLenum db = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &db);

		// Check completeness
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			qWarning() << "Selection FBO incomplete after resize, status =" << Qt::hex << status;
		}

		// Return to Qt¡¯s default FBO
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
	}
	void Viewport::drawDots(QMatrix4x4 view)
	{
		if (m_dotCount == 0) return;
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
		if (m_lineVertCount == 0) return;
		glEnable(GL_POLYGON_OFFSET_LINE);
		//glPolygonOffset(-1000.0f, -1000.0f);
		m_lineShader.bind();
		m_lineShader.setUniformValue("u_view", view);
		m_lineShader.setUniformValue("u_proj", m_projection);
		m_lineShader.setUniformValue("u_screenWidth", W);
		m_lineShader.setUniformValue("u_screenHeight", H);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_gDepth);
		m_lineShader.setUniformValue("u_depth", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_gAlbedo);
		m_lineShader.setUniformValue("u_albedo", 1);
		m_lineVAO.bind();
		glDrawArrays(GL_LINES, 0, m_lineVertCount);
		m_lineVAO.release();
		m_lineShader.release();
		glDisable(GL_POLYGON_OFFSET_LINE);
	}


	// forward rendering
	void Viewport::drawTestChamber(QMatrix4x4 view, QMatrix4x4 light, QVector3D lightDir)
	{
		if (m_indicesCount == 0) return;
		m_wallShader.bind();
		m_wallShader.setUniformValue("u_view", view);
		m_wallShader.setUniformValue("u_proj", m_projection);
		m_wallShader.setUniformValue("u_fogColor", m_fogCol);
		m_wallShader.setUniformValue("u_debugView", static_cast<GLint>(m_debugSurface));
		if (m_fog)
		{
			m_wallShader.setUniformValue("u_fogStart", m_fogStart);
			m_wallShader.setUniformValue("u_fogEnd", m_fogEnd);
		}
		else
		{
			// set fog beyond the far clip plane
			m_wallShader.setUniformValue("u_fogStart", m_far * m_far);
			m_wallShader.setUniformValue("u_fogEnd", m_far * m_far + 1);
		}
		m_wallShader.setUniformValue("u_lightVP", light);
		m_wallShader.setUniformValue("u_lightDir", lightDir);
		m_wallShader.setUniformValue("u_drawShadow", static_cast<GLint>(m_drawShadow));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_shadowMap);
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
		glBindFramebuffer(GL_FRAMEBUFFER, m_gBufferFBO);
		glViewport(0, 0, W, H); // <- likely not necessary
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);
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

	void Viewport::drawDeferred(QMatrix4x4 view, QMatrix4x4 light, QVector3D lightDir, GLuint blurredAO)
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

		QMatrix4x4 customprojection;
		customprojection.setToIdentity();
		const float aspect = (float)width() / (float)(height() > 0 ? height() : 1);
		customprojection.perspective(m_fov, aspect, 1, 3000);

		m_deferredShader.setUniformValue("u_proj", m_projection);
		m_deferredShader.setUniformValue("u_invProj", m_projection.inverted());
		m_deferredShader.setUniformValue("u_screenSize", QVector2D(W, H));
		m_deferredShader.setUniformValue("u_near", m_near);
		m_deferredShader.setUniformValue("u_far", m_far);

		//global directional light
		QMatrix4x4 invView = view.inverted();
		QMatrix4x4 viewToLight = light * invView;
		m_deferredShader.setUniformValue("u_viewToLight", viewToLight);
		m_deferredShader.setUniformValue("u_lightVP", light);
		m_deferredShader.setUniformValue("u_lightDir", lightDir);
		m_deferredShader.setUniformValue("u_drawShadow", static_cast<GLint>(m_drawShadow));

		//fog
		m_deferredShader.setUniformValue("u_fogColor", m_fogCol);
		if (m_fog)
		{
			// PTI seems to keep the "location" of the fog consistent regardless of zoom level
			m_deferredShader.setUniformValue("u_fogStart", m_fogStart + m_cameraDistance);
			m_deferredShader.setUniformValue("u_fogEnd", m_fogEnd + m_cameraDistance);
		}
		else
		{
			// set fog beyond the far clip plane
			m_deferredShader.setUniformValue("u_fogStart", m_far + 1.0f);
			m_deferredShader.setUniformValue("u_fogEnd", m_far + 2.0f);
		}

		//textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_shadowMap);
		m_deferredShader.setUniformValue("u_shadowMap", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_gNormal);
		m_deferredShader.setUniformValue("u_normal", 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_gAlbedo);
		m_deferredShader.setUniformValue("u_albedo", 2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, m_gDepth);
		m_deferredShader.setUniformValue("u_depth", 3);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, blurredAO);
		m_deferredShader.setUniformValue("u_ao", 4);

		m_fullscreenVAO.bind();
		glDrawArrays(GL_TRIANGLES, 0, 3); // fullscreen triangle that is generated in the vertex shader
		m_fullscreenVAO.release();
	}

	void Viewport::drawShadow(QMatrix4x4 light)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
		glViewport(0, 0, SHADOW_RES, SHADOW_RES);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDisable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		m_shadowShader.bind();
		m_shadowShader.setUniformValue("u_lightVP", light);
		m_chamberShadowVAO.bind();
		glDrawElements(GL_TRIANGLES, m_shadowIndicesCount, GL_UNSIGNED_INT, 0);
		m_chamberShadowVAO.release();
		//switch back
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glViewport(0, 0, int(width() * devicePixelRatioF()), int(height() * devicePixelRatioF()));
	}

	void Viewport::drawAO(QMatrix4x4 view)
	{
		//AO pipeline
		// kernel is a uniformly random rotation turned into unit direction vectors
		// for each fragment, we randomly rotate the kernel 
		// this means we dont have to regenerate kernel every frame
		const int W = int(width() * devicePixelRatioF());
		const int H = int(height() * devicePixelRatioF());
		glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);
		glViewport(0, 0, W, H);
		glDisable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // AO = 1 (no occlusion)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_aoShader.bind();
		if (!m_aoShader.bind()) {
			qDebug() << "AO shader failed to bind:" << m_aoShader.log();
		}
		// Matrices
		m_aoShader.setUniformValue("u_view", view);
		m_aoShader.setUniformValue("u_invView", view.inverted());
		QMatrix4x4 aoprojection;
		aoprojection.setToIdentity();
		const float aspect = (float)width() / (float)(height() > 0 ? height() : 1);
		aoprojection.perspective(m_fov, aspect, 1, 3000);

		m_aoShader.setUniformValue("u_proj", aoprojection);
		m_aoShader.setUniformValue("u_invProj", aoprojection.inverted());
		m_aoShader.setUniformValueArray("u_kernel", m_aokernel.data(), (GLsizei)m_aokernel.size());

		// see ao.frag for explanation
		//m_aoShader.setUniformValue("u_range", 70.0f);
		//m_aoShader.setUniformValue("u_variableRange", 0.05f); 
		//m_aoShader.setUniformValue("u_cutOff", 0.4f);
		// G-buffer inputs
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_gDepth);
		m_aoShader.setUniformValue("u_depth", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_gNormal);
		m_aoShader.setUniformValue("u_normal", 1);

		m_fullscreenVAO.bind();
		glDrawArrays(GL_TRIANGLES, 0, 3);
		m_fullscreenVAO.release();
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
		glViewport(0, 0, W, H);
		glEnable(GL_DEPTH_TEST);
	}

	//access the blurred texture in m_blurTex
	// outputs m_blurTex which is the output of the blur
	GLuint Viewport::blurTexture(GLuint texture)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_blurFBO);
		const int W = int(width() * devicePixelRatioF());
		const int H = int(height() * devicePixelRatioF());
		glViewport(0, 0, W, H);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);

		m_blurShader.bind();
		m_blurShader.setUniformValue("u_radius", 10); //value terms of texels
		m_blurShader.setUniformValue("u_sigma", 4.0f);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		m_blurShader.setUniformValue("u_input", 0);

		m_fullscreenVAO.bind();
		glDrawArrays(GL_TRIANGLES, 0, 3);

		m_fullscreenVAO.release();
		glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
		glEnable(GL_DEPTH_TEST);
		glViewport(0, 0, W, H);
		return m_blurTex;
	}

	void Viewport::drawTestChamberSurfaceAttributes(QMatrix4x4 view)
	{

		if (m_indicesCount == 0) return;
		if (m_lastDragRect.width() == 0 || m_lastDragRect.height() == 0) return;
		// bind offscreen FBO
		m_selectionShader.bind();
		glBindFramebuffer(GL_FRAMEBUFFER, m_selectionFBO);
		glDisable(GL_BLEND);
		glDisable(GL_DITHER);
		glEnable(GL_DEPTH_TEST);
		const int W = int(width() * devicePixelRatioF() * m_selectionTextureScale);
		const int H = int(height() * devicePixelRatioF() * m_selectionTextureScale);
		glViewport(0, 0, W, H);

		// Do not implement scissors here
		// Since m_lastDragRect can change during gpu rendering, using scissors can lead to pixels not being drawn when it should be.

		// clear integer color + depth
		const GLuint zero[4] = { 0,0,0,0 };
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

		int rn = 100;
		if (m_testChamber)
		{
			rn = m_testChamber->getRootNodeSizeInt();
		}
		QVector3D minP(-rn / 2, -rn / 2, -rn / 2);
		QVector3D maxP(rn / 2, rn / 2, rn / 2);

		QVector3D center = 0.5f * (minP + maxP);
		QVector3D extent = 0.5f * (maxP - minP);

		float orthoSize = qMax(extent.x(), extent.z()) * 1.3f;   // padding
		float m_gDepth = extent.length() * 2.0f;                // enough to see caster + receiver

		//------------------------------------------------------------
		// Position the light camera:  
		//  Place it *behind* the geometry along -lightDir
		//------------------------------------------------------------
		QVector3D lightPos = center - lightDir * m_gDepth;

		//------------------------------------------------------------
		// Build the matrices
		//------------------------------------------------------------
		QMatrix4x4 lightView = lookAt(lightPos, center, QVector3D(0, 1, 0));

		QMatrix4x4 lightProj = ortho(
			-orthoSize, orthoSize,
			-orthoSize, orthoSize,
			1.0f,
			m_gDepth * 2.0f               // far plane
		);

		out = lightProj * lightView;
	}
	// QT Functions
	void Viewport::paintGL()
	{
		//int blurW, blurH;
		//getTextureSize(m_blurTex, blurW, blurH);
		//int aoW, aoH;
		//getTextureSize(m_aoTex, aoW, aoH);
		//std::cout << "Blur Size: " << blurW << "x" << blurH << std::endl;
		//std::cout << "AO Size: " << aoW << "x" << aoH << std::endl;
		//qDebug() << "Start drawing";
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_PROGRAM_POINT_SIZE);

		QMatrix4x4 view;
		getViewMatrix(view);
		//deferred rendering. draw normal and albedo as well as depth
		drawGBuffer(view);

		// directional light direction (world space)
		QVector3D lightDir = QVector3D(-1.0f, -1.0f, -5.0f).normalized();
		QMatrix4x4 lightVP;
		getGlobalLightViewMatrix(lightDir, lightVP);

		drawShadow(lightVP);
		drawAO(view);
		GLuint blurredAO = blurTexture(m_aoTex);

		drawDeferred(view, lightVP, lightDir, blurredAO);
		drawDots(view);
		drawLines(view);
		drawTestChamberSurfaceAttributes(view);

		// after your normal scene draw (default FBO bound)
		const int W = int(width() * devicePixelRatioF());
		const int H = int(height() * devicePixelRatioF());
		glViewport(0, 0, W, H);

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

}