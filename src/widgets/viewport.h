// viewport.h
// Declares the main editor viewport as an extension of QOpenGLWidget
#include <qevent.h>
#include <qvectornd.h>
#include <qtimer.h>
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
		void loadLines(std::vector<QVector3D> vertices);
		void loadTestChamberMesh(const TestChamber& tc);
		
		void setFog(bool enable) {m_fog = enable;};
		void setFogStart(float x) {m_fogStart = x;};
		void setFogEnd(float x) {m_fogEnd = x;};
		void setFogColor(QVector3D col) {m_fogCol = col;};
		
	protected:
		void loadShaders();
		void paintGL() override;
		void resizeGL( int width, int height ) override;
		void initializeGL() override;

		void onFrameUpdate();
		void keyPressEvent(QKeyEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void wheelEvent(QWheelEvent* event) override;

		void getViewMatrix(QMatrix4x4& out);
		void drawDots(QMatrix4x4 view);
		void drawLines(QMatrix4x4 view);
		void drawTestChamber(QMatrix4x4 view);

	private:
		// Camera transformation matrices
		// (Chamber geometry won't use a model matrix, item model matrices will be stored in the item class)
		QMatrix4x4 m_projection;
		float m_fov;
		
		QVector3D m_cameraTarget;
		// Not making this a quaternion for now because I don't have a good understanding of them yet
		// and it doesn't really seem to be necessary (camera only rotates around yaw/pitch) - Kelsey
		QVector3D m_cameraAngles;
		float m_cameraDistance;
		
		bool m_isOrbiting;
		bool m_shouldOrbitInertia;
		QVector3D m_OrbitVelocity;
		bool m_isPanning;
		QVector2D m_lastMousePos;
		
		QTimer* m_frameTimer = new QTimer(this);
		
		bool m_fog = false;
		float m_fogStart = 2500.0f;
		float m_fogEnd = 25000.0f;
		QVector3D m_fogCol = {0.1,0.13,0.2};
		
		// TODO: These will probably be moved elsewhere - Kelsey
		QOpenGLShaderProgram m_wallShader;
		QOpenGLVertexArrayObject m_VAO;
		QOpenGLBuffer m_vertexBuffer{QOpenGLBuffer::Type::VertexBuffer};
		QOpenGLBuffer m_indexBuffer{QOpenGLBuffer::Type::IndexBuffer};
		GLsizei m_indicesCount;
		
		QOpenGLShaderProgram m_dotShader;
		QOpenGLVertexArrayObject m_dotVAO;
		QOpenGLBuffer m_dotVBO{QOpenGLBuffer::Type::VertexBuffer};
		GLsizei m_dotCount;
		
		QOpenGLShaderProgram m_lineShader;
		QOpenGLVertexArrayObject m_lineVAO;
		QOpenGLBuffer m_lineVBO{QOpenGLBuffer::Type::VertexBuffer};
		GLsizei m_lineVertCount;

#ifndef NDEBUG
		QOpenGLDebugLogger m_debugLogger;
#endif
	};
}



#endif // !PMP_VIEWPORT_H
