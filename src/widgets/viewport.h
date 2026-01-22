// viewport.h
// Declares the main editor viewport as an extension of QOpenGLWidget
#pragma once

#include <QLabel>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLDebugLogger>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QQuaternion>
#include <QVector3D>
#include <initializer_list>
#include <qevent.h>
#include <qtimer.h>
#include <qvectornd.h>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "../selection.h"
#include "../test_chamber.h"
#include "../util/3d/ray.h"

#define AOKERNEL_SIZE 64

namespace PMP
{
struct DebugPoint;

// These defaults roughly match PTI
const float cameraDistDefault = 5000.0f;
const float cameraDistMin = 1400.0f;
const float cameraDistMax = 25000.0f;

const float fovDefault = 15.0f;
const float nearClipDefault = 7.0f;
const float farClipDefault = 65536.0f;

const float fogStartDefault = 2000.0f;
const float fogEndDefault = 5000.0f;

const QVector3D camAngleDefault(-64.0f, 0.0f, 22.0f);

template <typename T> bool allDifferent(std::initializer_list<T> values)
{
	std::unordered_set<T> unique(values.begin(), values.end());
	return unique.size() == values.size();
}

struct ScreenSurface
{
	unsigned int parentID;
	unsigned int surfaceID;
	WallSide wallSide;

	std::string toString() const
	{
		std::stringstream ss;
		ss << "ScreenSurface{ parentID=" << parentID << ", surfaceID=" << surfaceID << ", wallSide=" << to_string(wallSide)
		   << " }";
		return ss.str();
	}

	// Needed for unordered_* key equality
	friend bool operator==(const ScreenSurface& a, const ScreenSurface& b) noexcept
	{
		return a.parentID == b.parentID && a.surfaceID == b.surfaceID && a.wallSide == b.wallSide;
	}
};

// Robust, fast hasher (splitmix64-style mixing + hash_combine)
struct ScreenSurfaceHash
{
	static inline std::uint64_t mix(std::uint64_t x) noexcept
	{
		x ^= x >> 30;
		x *= 0xbf58476d1ce4e5b9ULL;
		x ^= x >> 27;
		x *= 0x94d049bb133111ebULL;
		x ^= x >> 31;
		return x;
	}

	std::size_t operator()(const ScreenSurface& s) const noexcept
	{
		std::uint64_t seed = 0;
		auto combine = [&](std::uint64_t v) {
			std::uint64_t h = mix(v);
			seed ^= h + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
		};
		combine(s.parentID);
		combine(s.surfaceID);
		combine(static_cast<std::uint32_t>(s.wallSide));
		return static_cast<std::size_t>(seed);
	}
};

struct ScreenSurfaceEq
{
	bool operator()(const ScreenSurface& a, const ScreenSurface& b) const noexcept
	{
		return a.parentID == b.parentID && a.surfaceID == b.surfaceID && a.wallSide == b.wallSide;
	}
};

using ScreenSurfaceSet = std::unordered_set<ScreenSurface, ScreenSurfaceHash, ScreenSurfaceEq>;

class Viewport : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
	Q_OBJECT

public:
	Viewport(QWidget* parent)
	    : QOpenGLWidget(parent), QOpenGLFunctions_3_3_Core(),
#ifndef NDEBUG
	      m_debugLogger(this),
#endif
	      m_fov(fovDefault), m_near(nearClipDefault), m_far(farClipDefault),
	      // FIXME: Position/angle handling here is reversed from what's expected - Kelsey
	      m_cameraTarget(0.0f, 0.0f, 0.0f), m_cameraAngles(camAngleDefault), m_cameraDistance(cameraDistDefault),
	      m_isOrbiting(false), m_isPanning(false), m_lastMousePos(0.0f, 0.0f), m_isDragging(false),
	      m_shouldOrbitInertia(false), m_gridSizeLabel(this)
	{
		connect(m_frameTimer, &QTimer::timeout, this, &Viewport::onFrameUpdate);
		m_frameTimer->start(10); // ~100 FPS

		generateAOKernel();
		setFocusPolicy(Qt::FocusPolicy::StrongFocus);
	}

	void loadDots(std::vector<DebugPoint> vertices);
	void loadLines(std::vector<LineVertex> vertices);
	// Load a single ray to draw
	void loadLines(Ray ray, float length = 20000.0f, QVector3D colStart = QVector3D(1.0f, 0.0f, 0.0f),
	               QVector3D colEnd = QVector3D(0.0f, 0.0f, 1.0f));
	void loadSelectionLines();
	void loadTestChamberMesh();
	void reloadChamberMeshAndSelection();

	void setFog(bool enable) { m_fog = enable; };

	void setFogStart(float x) { m_fogStart = x; };

	void setFogEnd(float x) { m_fogEnd = x; };

	void setFogColor(QVector3D col) { m_fogCol = col; };

	void setTestChamber(TestChamber* tc)
	{
		m_testChamber = tc;
		updateGridSizeLabel();
	};

	void drawTestChamberSurfaceAttributes(QMatrix4x4 view);

	void setSelector(Selector* s) { m_selector = s; };

	GLuint blurTexture(GLuint texture);

	QMatrix4x4 getProjectionMatrix() { return m_projection; };

	void getViewMatrix(QMatrix4x4& out);

	void getGlobalLightViewMatrix(QVector3D lightDir, QMatrix4x4& out);

	void resetCamera();

protected:
	void createGraphicsObjects();
	void createBlurFBO(const int W, const int H);
	void createSSAOFBO(const int W, const int H);
	void createGBufferFBO(const int W, const int H);
	void createShadowFBO();
	void createSelectionFBO();
	void resizeSelectionFBO(GLsizei W, GLsizei H);
	void loadShaders();
	void paintGL() override;
	void resizeGL(int width, int height) override;
	void getTextureSize(GLuint tex, int& width, int& height);
	void initializeGL() override;
	bool readSelectionRectCPU(ScreenSurfaceSet& out);

	void onFrameUpdate();
	void keyPressEvent(QKeyEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;

	void drawDots(QMatrix4x4 view);
	void drawLines(QMatrix4x4 view);
	void drawTestChamber(QMatrix4x4 view, QMatrix4x4 light, QVector3D lightDir);
	void drawGBuffer(QMatrix4x4 view);
	void drawDeferred(QMatrix4x4 view, QMatrix4x4 light, QVector3D lightDir, GLuint blurredAO);
	void drawShadow(QMatrix4x4 light);
	void drawAO(QMatrix4x4 view);
	void drawDragBox();

	void setSelecting(bool on, const QRect& r = {})
	{
		m_isDragging = on;
		m_rect = r;
		update();
	}

	void updateGridSizeLabel();

	void generateAOKernel();

private:
	TestChamber* m_testChamber = nullptr;
	Selector* m_selector = nullptr;

	// Camera transformation matrices
	// (Chamber geometry won't use a model matrix, item model matrices will be stored in the item class)
	QMatrix4x4 m_projection;
	float m_fov;
	QVector3D m_cameraTarget;
	// Not making this a quaternion for now because I don't have a good understanding of them yet
	// and it doesn't really seem to be necessary (camera only rotates around yaw/pitch) - Kelsey
	QVector3D m_cameraAngles;
	float m_cameraDistance;
	float m_near;
	float m_far;

	// Mouse based control
	bool m_isOrbiting;
	bool m_isPanning;
	bool m_isDragging;

	bool m_shouldOrbitInertia;
	QVector2D m_lastMousePos;
	QVector2D m_lastMouseClickPos;
	QVector2D m_dragStartPos;
	QVector2D m_dragEndPos;
	QVector3D m_orbitVelocity;

	// Timer for onFrameUpdate()
	QTimer* m_frameTimer = new QTimer(this);

	// Fog control
	bool m_fog = false;
	float m_fogStart = fogStartDefault;
	float m_fogEnd = fogEndDefault;
	QVector3D m_fogCol = {0.1, 0.13, 0.2};

	// Shadow control
	bool m_drawShadow = false;

	// Debug view control
	bool m_debugSurface = false;

	// Drag box drawing
	QPoint m_origin;
	QRect m_rect;
	QRect m_lastDragRect;

	// Graphics objects
	// TODO: These will probably be moved elsewhere - Kelsey
	QOpenGLShaderProgram m_wallShader;
	QOpenGLShaderProgram m_selectionShader;
	QOpenGLVertexArrayObject m_chamberVAO;
	QOpenGLBuffer m_chamberVBO;
	QOpenGLBuffer m_chamberIBO;
	GLsizei m_indicesCount = 0;
	// Make sure we don't try to do operations with opengl objects until initGL has been called by Qt
	bool m_glInitialized = false;

	QOpenGLShaderProgram m_dotShader;
	QOpenGLVertexArrayObject m_dotVAO;
	QOpenGLBuffer m_dotVBO;
	GLsizei m_dotCount = 0;

	QOpenGLShaderProgram m_lineShader;
	QOpenGLVertexArrayObject m_lineVAO;
	QOpenGLBuffer m_lineVBO;
	GLsizei m_lineVertCount = 0;

	GLuint m_selectionFBO = 0;
	GLuint m_selectionTex = 0;
	GLuint m_selectionDep = 0;
	const float m_selectionTextureScale = 0.25f;

	QOpenGLShaderProgram m_shadowShader;
	QOpenGLVertexArrayObject m_chamberShadowVAO;
	QOpenGLBuffer m_chamberShadowVBO;
	QOpenGLBuffer m_chamberShadowIBO;
	GLsizei m_shadowIndicesCount = 0;
	GLuint m_shadowFBO;
	GLuint m_shadowMap;
	const int SHADOW_RES = 2048;

	QOpenGLVertexArrayObject m_fullscreenVAO;

	QOpenGLShaderProgram m_deferredShader;
	QOpenGLShaderProgram m_gBufferShader;
	GLuint m_gBufferFBO;
	GLuint m_gNormal;
	GLuint m_gAlbedo;
	GLuint m_gDepth;

	QOpenGLShaderProgram m_aoShader;
	GLuint m_ssaoFBO;
	GLuint m_aoTex;
	std::array<QVector3D, AOKERNEL_SIZE> m_aokernel;

	QOpenGLShaderProgram m_blurShader;
	GLuint m_blurFBO;
	GLuint m_blurTex;

	QLabel m_gridSizeLabel;

#ifndef NDEBUG
	QOpenGLDebugLogger m_debugLogger;
#endif
};
} // namespace PMP
