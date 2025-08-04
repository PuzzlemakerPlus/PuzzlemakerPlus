// viewport.h
// Declares the main editor viewport as an extension of QOpenGLWidget
#include <qevent.h>
#include <qvectornd.h>
#include <qtimer.h>
#include <vector>
#ifndef PMP_VIEWPORT_H
#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLDebugLogger>
#include <QOpenGLVertexArrayObject>

#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>

#include "../test_chamber.h"
#include "../util/3d/ray.h"
#include "../selection.h"

namespace PMP
{
	const float cameraDistDefault = 512.0f;
	const float cameraDistMin = 256.0f;
	const float cameraDistMax = 10000.0f;


	class Viewport : public QOpenGLWidget, protected QOpenGLFunctions_3_2_Core
	{
	public:
		Viewport(QWidget *parent) : 
			QOpenGLWidget(parent), 
			QOpenGLFunctions_3_2_Core(),
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
			m_lastMousePos(0.0f, 0.0f)
		{ }

		void loadDots(std::vector<QVector3D> vertices);
		void loadLines(std::vector<LineVertex> vertices);
		//Load a single ray to draw
		void loadLines(Ray ray, float length=20000.0f, QVector3D colStart=QVector3D(1.0f,0.0f,0.0f), QVector3D colEnd=QVector3D(0.0f,0.0f,1.0f));
		void loadTestChamberMesh();
		
		void setFog(bool enable) {m_fog = enable;};
		void setFogStart(float x) {m_fogStart = x;};
		void setFogEnd(float x) {m_fogEnd = x;};
		void setFogColor(QVector3D col) {m_fogCol = col;};

		void setTestChamber(TestChamber* tc) {m_testChamber = tc;};
		void setSelector(Selector* s){m_selector = s;m_selector->setViewPortDim(width(), height());};

		QMatrix4x4 getProjectionMatrix() {return m_projection;};
		void getViewMatrix(QMatrix4x4& out);
		
	protected:
		void createVAOs();
		void loadShaders();
		void paintGL() override;
		void resizeGL( int width, int height ) override;
		void initializeGL() override;

		void onFrameUpdate();
		void keyPressEvent(QKeyEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void wheelEvent(QWheelEvent* event) override;

		void drawDots(QMatrix4x4 view);
		void drawLines(QMatrix4x4 view);
		void drawTestChamber(QMatrix4x4 view);

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

		// Graphics objects
		// TODO: These will probably be moved elsewhere - Kelsey
		QOpenGLShaderProgram m_wallShader;
		QOpenGLVertexArrayObject m_chamberVAO;
		QOpenGLBuffer m_chamberVBO{QOpenGLBuffer::Type::VertexBuffer};
		QOpenGLBuffer m_chamberIBO{QOpenGLBuffer::Type::IndexBuffer};
		GLsizei m_indicesCount = 0;
		
		QOpenGLShaderProgram m_dotShader;
		QOpenGLVertexArrayObject m_dotVAO;
		QOpenGLBuffer m_dotVBO{QOpenGLBuffer::Type::VertexBuffer};
		GLsizei m_dotCount = 0;
		
		QOpenGLShaderProgram m_lineShader;
		QOpenGLVertexArrayObject m_lineVAO;
		QOpenGLBuffer m_lineVBO{QOpenGLBuffer::Type::VertexBuffer};
		GLsizei m_lineVertCount = 0;


#ifndef NDEBUG
		QOpenGLDebugLogger m_debugLogger;
#endif
	};
}



#endif // !PMP_VIEWPORT_H
