// viewport.cpp
// Implements the main editor viewport as an extension of QOpenGLWidget
// See also: viewport_control.cpp and viewport_draw.cpp
#include "viewport.h"

#include <QMouseEvent>
#include <QOpenGLExtraFunctions>
#include <QPainter>
#include <QThread>
#include <QWheelEvent>
#include <iostream>
#include <qassert.h>
#include <qmatrix4x4.h>
#include <qnamespace.h>
#include <qvectornd.h>
#include <random>
#include <stdexcept>
#include <vector>

#include "../selection.h"
#include "../undo.h"
#include "../util/3d/mesh.h"
#include "../util/debug.h"

namespace PMP
{
void Viewport::getTextureSize(GLuint tex, int& width, int& height)
{
	glBindTexture(GL_TEXTURE_2D, tex);

	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
}

void Viewport::initializeGL()
{
	initializeOpenGLFunctions();

#ifndef NDEBUG
	if (m_debugLogger.initialize())
	{
		connect(&m_debugLogger, &QOpenGLDebugLogger::messageLogged,
		        [=](const QOpenGLDebugMessage& debugMessage) { qDebug() << debugMessage; });
		m_debugLogger.startLogging();
		m_debugLogger.enableMessages();
	}
	else
	{
		qDebug() << "WARNING: OpenGL debug output is not supported on this system.\n";
	}
#endif
	// Puzzlemaker background color
	// glClearColor( 0.803, 0.815, 0.807, 1.0f ); set in deferred shader now

	loadShaders();
	createGraphicsObjects();
	auto* ext = context()->extraFunctions();
	ext->glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

	m_glInitialized = true;
	// Load mesh data if the test chamber was set before this gets called
	if (m_testChamber)
		reloadChamberMeshAndSelection();

	std::cout << "ViewportGL initialized." << std::endl;
}

void Viewport::loadShaders()
{
	m_wallShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/chamber_surface.vert");
	m_wallShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/chamber_surface.frag");
#ifndef NDEBUG
	if (!m_wallShader.link())
	{
		qDebug() << "Shader failed to link:" << m_wallShader.log();
		return;
	}
#else
	m_wallShader.link();
#endif
	m_selectionShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/chamber_selection.vert");
	m_selectionShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/chamber_selection.frag");
#ifndef NDEBUG
	if (!m_selectionShader.link())
	{
		qDebug() << "Shader failed to link:" << m_selectionShader.log();
		return;
	}
#else
	m_selectionShader.link();
#endif
	m_dotShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/dot.vert");
	m_dotShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/dot.frag");
#ifndef NDEBUG
	if (!m_dotShader.link())
	{
		qDebug() << "Shader failed to link:" << m_dotShader.log();
		return;
	}
#else
	m_dotShader.link();
#endif
	m_lineShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/line.vert");
	m_lineShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/line.frag");
#ifndef NDEBUG
	if (!m_lineShader.link())
	{
		qDebug() << "Shader failed to link:" << m_lineShader.log();
		return;
	}
#else
	m_lineShader.link();
#endif
	m_shadowShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shadow.vert");
	m_shadowShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shadow.frag");
#ifndef NDEBUG
	if (!m_shadowShader.link())
	{
		qDebug() << "Shader failed to link:" << m_shadowShader.log();
		return;
	}
#else
	m_shadowShader.link();
#endif
	m_gBufferShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/g_buffer.vert");
	m_gBufferShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/g_buffer.frag");
#ifndef NDEBUG
	if (!m_gBufferShader.link())
	{
		qDebug() << "Shader failed to link:" << m_gBufferShader.log();
		return;
	}
#else
	m_gBufferShader.link();
#endif
	m_deferredShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/deferred.vert");
	m_deferredShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/deferred.frag");
#ifndef NDEBUG
	if (!m_deferredShader.link())
	{
		qDebug() << "Shader failed to link:" << m_deferredShader.log();
		return;
	}
#else
	m_deferredShader.link();
#endif
	m_aoShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/ao.vert");
	m_aoShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/ao.frag");
#ifndef NDEBUG
	if (!m_aoShader.link())
	{
		qDebug() << "Shader failed to link:" << m_aoShader.log();
		return;
	}
#else
	m_aoShader.link();
#endif
	m_blurShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/blur.vert");
	m_blurShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/blur.frag");
#ifndef NDEBUG
	if (!m_blurShader.link())
	{
		qDebug() << "Shader failed to link:" << m_blurShader.log();
		return;
	}
#else
	m_blurShader.link();
#endif
}

// needs a list of points to draw
void Viewport::loadDots(std::vector<DebugPoint> vertices)
{
	if (!m_glInitialized)
		return;

	m_dotCount = vertices.size();
	this->makeCurrent();
	m_dotVAO.bind();
	m_dotVBO.bind();

	m_dotVBO.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(DebugPoint)));

	// Position attribute
	int posLoc = m_dotShader.attributeLocation("aPos");
	m_dotShader.enableAttributeArray(posLoc);
	m_dotShader.setAttributeBuffer(posLoc, GL_FLOAT, offsetof(DebugPoint, position), 3, sizeof(DebugPoint));

	// Color attribute
	int colorLoc = m_dotShader.attributeLocation("aColor");
	m_dotShader.enableAttributeArray(colorLoc);
	m_dotShader.setAttributeBuffer(colorLoc, GL_FLOAT, offsetof(DebugPoint, color), 3, sizeof(DebugPoint));

	m_dotVAO.release();
	m_dotVBO.release();
	this->doneCurrent();
}

void Viewport::loadLines(std::vector<LineVertex> vertices)
{
	if (!m_glInitialized)
		return;

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
	MultiQuadCornersToLineWireframe(corners, lines);
	loadLines(lines);
}

void Viewport::loadTestChamberMesh()
{
	if (!m_glInitialized)
		return;

	Q_ASSERT(m_testChamber);
	if (!m_testChamber)
		throw std::runtime_error("Can not load test chamber mesh without setting test chamber");

	std::vector<TestChamberQuad> quads;
	m_testChamber->generateTCQuads(quads);
	std::vector<TestChamberQuad> shadowQuads;
	m_testChamber->generateTCShadowQuads(shadowQuads);
	TestChamberMesh mesh;
	CreateTCMesh(quads, mesh);
	TestChamberMesh shadowMesh;
	CreateTCMesh(shadowQuads, shadowMesh);
	if (mesh.vertices.empty() || mesh.indices.empty())
		throw std::runtime_error("Mesh has no vertices or indices to allocate");
	// if you got an error around here, it is likely because for some reason the surface generation failed so opengl
	// tried to draw 0 surfaces
	this->makeCurrent();
	m_chamberVAO.bind();
	m_chamberVBO.bind();
	m_chamberIBO.bind();
	// Disabling for now until we are more certain it's what we want - Kelsey
	// m_chamberVBO.setUsagePattern(QOpenGLBuffer::StaticDraw);
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

void Viewport::resizeGL(int _width, int _height)
{
	const float aspect = (float)_width / (float)(_height > 0 ? _height : 1);
	m_projection.setToIdentity();
	// Source near clip plane value, might change later
	m_projection.perspective(m_fov, aspect, m_near, m_far);
	const int W = int(width() * devicePixelRatioF());
	const int H = int(height() * devicePixelRatioF());
	createGBufferFBO(W, H);
	createSSAOFBO(W, H);
	createBlurFBO(W, H);
}

void Viewport::reloadChamberMeshAndSelection()
{
	if (!m_glInitialized)
		return;

	loadTestChamberMesh();
	loadSelectionLines();
}

bool Viewport::readSelectionRectCPU(ScreenSurfaceSet& out)
{
	// TODO: this setting does not seem to work on my windows machine
	const int x = m_lastDragRect.left() * m_selectionTextureScale;
	const int y = height() * m_selectionTextureScale - m_lastDragRect.bottom() * m_selectionTextureScale;
	const int w = m_lastDragRect.width() * m_selectionTextureScale;
	const int h = m_lastDragRect.height() * m_selectionTextureScale;

	if (w <= 0 || h <= 0)
		return false;
	// 4 uints (RGBA) per pixel
	std::vector<uint32_t> buf(size_t(w) * h * 4);

	glBindFramebuffer(GL_FRAMEBUFFER, m_selectionFBO);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, w, h, GL_RGBA_INTEGER, GL_UNSIGNED_INT, buf.data());
	// Back to Qt’s default FBO so you don’t accidentally draw into the integer one
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

	out.clear();
	for (int i = 0; i < w * h; ++i)
	{
		const uint32_t parent = buf[i * 4 + 0];
		const uint32_t surface = buf[i * 4 + 1];
		const WallSide side = static_cast<WallSide>(buf[i * 4 + 2]);
		out.emplace(parent, surface, side);
	}
	return true;
}

void Viewport::resizeEvent(QResizeEvent* event)
{
	m_gridSizeLabel.resize(width(), m_gridSizeLabel.height());
	m_gridSizeLabel.move(width() - m_gridSizeLabel.width(), height() - m_gridSizeLabel.height());
	m_gridSizeLabel.setAlignment(Qt::AlignmentFlag::AlignRight | Qt::AlignmentFlag::AlignBottom);
	updateGridSizeLabel();
	QOpenGLWidget::resizeEvent(event);
}

void Viewport::drawDragBox()
{
	QPainter p(this); // safe in QOpenGLWidget
	p.setRenderHint(QPainter::Antialiasing, true);
	QColor edge(77, 163, 255);
	QColor fill(77, 163, 255, 40);
	p.fillRect(m_rect, fill);
	QPen pen(edge);
	pen.setStyle(Qt::DashLine);
	p.setPen(pen);
	p.drawRect(m_rect.adjusted(0, 0, -1, -1));
}

void Viewport::updateGridSizeLabel()
{
	if (!m_testChamber)
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
		float scale = (var * randf() + (1 - var)); // bias closer to origin
		sample *= scale;
		m_aokernel[i] = sample;
	}
}

void Viewport::resetCamera()
{
	m_cameraTarget = QVector3D(0.0f, 0.0f, 0.0f);
	m_cameraAngles = camAngleDefault;
	m_cameraDistance = cameraDistDefault;
}
} // namespace PMP
