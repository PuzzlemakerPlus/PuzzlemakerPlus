// viewport.h
// Declares the main editor viewport as an extension of QOpenGLWidget
#pragma once

#include <vector>
#include <qevent.h>
#include <qtimer.h>
#include <qvectornd.h>
#include <unordered_set>
#include <initializer_list>

#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLDebugLogger>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_3_Core>

#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>

#include "../selection.h"
#include "../util/3d/ray.h"
#include "../test_chamber.h"

namespace PMP
{
	
	const float cameraDistDefault = 512.0f;
	const float cameraDistMin = 256.0f;
	const float cameraDistMax = 14000.0f;

	template <typename T>
	bool allDifferent(std::initializer_list<T> values) {
		std::unordered_set<T> unique(values.begin(), values.end());
		return unique.size() == values.size();
	}	

	struct ScreenSurface
	{
		unsigned int parentID;
		unsigned int surfaceID;
		WALLSIDE wallSide;
		std::string toString() const {
			std::ostringstream oss;
			oss << "ScreenSurface{ parentID=" << parentID
				<< ", surfaceID=" << surfaceID
				<< ", wallSide="  << to_string(wallSide)
				<< " }";
			return oss.str();
		}
		// Needed for unordered_* key equality
		friend bool operator==(const ScreenSurface& a, const ScreenSurface& b) noexcept {
			return a.parentID  == b.parentID &&
				a.surfaceID == b.surfaceID &&
				a.wallSide  == b.wallSide;
		}
	};
	// Robust, fast hasher (splitmix64-style mixing + hash_combine)
	struct ScreenSurfaceHash {
		static inline std::uint64_t mix(std::uint64_t x) noexcept {
			x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
			x ^= x >> 27; x *= 0x94d049bb133111ebULL;
			x ^= x >> 31; return x;
		}
		std::size_t operator()(const ScreenSurface& s) const noexcept {
			std::uint64_t seed = 0;
			auto combine = [&](std::uint64_t v){
				std::uint64_t h = mix(v);
				seed ^= h + 0x9e3779b97f4a7c15ULL + (seed<<6) + (seed>>2);
			};
			combine(s.parentID);
			combine(s.surfaceID);
			combine(static_cast<std::uint32_t>(s.wallSide));
			return static_cast<std::size_t>(seed);
		}
	};
	struct ScreenSurfaceEq {
		bool operator()(const ScreenSurface& a, const ScreenSurface& b) const noexcept {
			return a.parentID  == b.parentID &&
				a.surfaceID == b.surfaceID &&
				a.wallSide  == b.wallSide;
		}
	};
	using ScreenSurfaceSet = std::unordered_set<ScreenSurface, ScreenSurfaceHash, ScreenSurfaceEq>;

	class Viewport : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
	{
	public:
		Viewport(QWidget *parent) : 
			QOpenGLWidget(parent), 
			QOpenGLFunctions_3_3_Core(),
#ifndef NDEBUG
			m_debugLogger(this),
#endif
			m_fov(90.0f),
			// FIXME: Position/angle handling here is reversed from what's expected - Kelsey
			m_cameraTarget(0.0f, 0.0f, 0.0f),
			m_cameraAngles(0.0f, 0.0f, 0.0f),
			m_cameraDistance(cameraDistDefault),
			m_isOrbiting(false),
			m_isPanning(false),
			m_lastMousePos(0.0f, 0.0f),
			m_isDragging(false),
			m_shouldOrbitInertia(false)
		{ 
			connect(m_frameTimer, &QTimer::timeout, this, &Viewport::onFrameUpdate);
			m_frameTimer->start(10); // ~100 FPS
		}

		void loadDots(std::vector<QVector3D> vertices);
		void loadLines(std::vector<LineVertex> vertices);
		//Load a single ray to draw
		void loadLines(Ray ray, float length=20000.0f, QVector3D colStart=QVector3D(1.0f,0.0f,0.0f), QVector3D colEnd=QVector3D(0.0f,0.0f,1.0f));
		void loadSelectionLines();
		void loadTestChamberMesh();
		
		void setFog(bool enable) {m_fog = enable;};
		void setFogStart(float x) {m_fogStart = x;};
		void setFogEnd(float x) {m_fogEnd = x;};
		void setFogColor(QVector3D col) {m_fogCol = col;};

		void setTestChamber(TestChamber* tc) {m_testChamber = tc;};
		void drawTestChamberSurfaceAttributes(QMatrix4x4 view);
		void setSelector(Selector* s){m_selector = s;m_selector->setViewPortDim(width(), height());};

		QMatrix4x4 getProjectionMatrix() {return m_projection;};
		void getViewMatrix(QMatrix4x4& out);
		
	protected:
		void createGraphicsObjects();
		void createSelectionFBO();
		void resizeSelectionFBO(GLsizei W, GLsizei H);
		void createSelectionPBO(GLsizeiptr needBytes);
		void loadShaders();
		void paintGL() override;
		void resizeGL( int width, int height ) override;
		void reloadChamberMeshAndSelection();
		void initializeGL() override;
		bool readSelectionRectCPU(ScreenSurfaceSet& out);

		void onFrameUpdate();
		void keyPressEvent(QKeyEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void wheelEvent(QWheelEvent* event) override;

		void drawDots(QMatrix4x4 view);
		void drawLines(QMatrix4x4 view);
		void drawTestChamber(QMatrix4x4 view);
		void drawDragBox();

		void setSelecting(bool on, const QRect& r = {}) { m_isDragging = on; m_rect = r; update(); }

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
		
		//Mouse based control
		bool m_isOrbiting;
		bool m_isPanning;
		bool m_isDragging;

		bool m_shouldOrbitInertia;
		QVector2D m_lastMousePos;
		QVector2D m_lastMouseClickPos;
		QVector2D m_dragStartPos;
		QVector2D m_dragEndPos;
		QVector3D m_orbitVelocity;
		
		//Timer for onFrameUpdate()
		QTimer* m_frameTimer = new QTimer(this);

		// Fog control
		bool m_fog = false;
		float m_fogStart = 2500.0f;
		float m_fogEnd = 25000.0f;
		QVector3D m_fogCol = {0.1,0.13,0.2};

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
		
		QOpenGLShaderProgram m_dotShader;
		QOpenGLVertexArrayObject m_dotVAO;
		QOpenGLBuffer m_dotVBO;
		GLsizei m_dotCount = 0;
		
		QOpenGLShaderProgram m_lineShader;
		QOpenGLVertexArrayObject m_lineVAO;
		QOpenGLBuffer m_lineVBO;
		GLsizei m_lineVertCount = 0;

		GLuint selectionFBO=0;
		GLuint selectionTex=0;
		GLuint selectionDep=0;
		const float selectionTextureScale = 0.25f;

#ifndef NDEBUG
		QOpenGLDebugLogger m_debugLogger;
#endif
	};
}
