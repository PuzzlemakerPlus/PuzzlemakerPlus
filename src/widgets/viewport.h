// viewport.h
// Declares the main editor viewport as an extension of QOpenGLWidget
#ifndef PMP_VIEWPORT_H
#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLDebugLogger>
#include <QOpenGlVertexArrayObject>

#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>

namespace PMP
{
	class Viewport : public QOpenGLWidget, protected QOpenGLFunctions_3_2_Core
	{
	public:
		Viewport(QWidget *parent) : 
			QOpenGLWidget(parent), 
			QOpenGLFunctions_3_2_Core(), 
			m_fov(90.0f),
			m_cameraTarget(0.0f, 0.0f, 0.0f),
			m_cameraAngles(0.0f, 0.0f, 0.0f),
			m_cameraDistance(256.0f),
			m_leftMouseDown(false),
			m_rightMouseDown(false),
			m_lastMousePos(0.0f, 0.0f),
			m_debugLogger(this)
		{ }

	protected:
		void paintGL() override;
		void resizeGL( int width, int height ) override;
		void initializeGL() override;

		void mousePressEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void wheelEvent(QWheelEvent* event) override;

	private:
		QOpenGLShaderProgram m_wallShader;

		// Camera transformation matrices
		// (Chamber geometry won't use a model matrix, item model matrices will be stored in the item class)
		QMatrix4x4 m_projection;
		float m_fov;

		QVector3D m_cameraTarget;
		// Not making this a quaternion for now because I don't have a good understanding of them yet
		// and it doesn't really seem to be necessary (camera only rotates around yaw/pitch) - Kelsey
		QVector3D m_cameraAngles;
		float m_cameraDistance;

		bool m_leftMouseDown;
		bool m_rightMouseDown;
		QVector2D m_lastMousePos;

		// TODO: These will probably be moved elsewhere - Kelsey
		QOpenGLBuffer m_vertexBuffer{QOpenGLBuffer::Type::VertexBuffer};
		QOpenGLBuffer m_indexBuffer{QOpenGLBuffer::Type::IndexBuffer};

		QOpenGLVertexArrayObject m_VAO;

#ifdef _DEBUG
		QOpenGLDebugLogger m_debugLogger;
#endif
	};
}



#endif // !PMP_VIEWPORT_H
