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

namespace PMP
{
	class Viewport : public QOpenGLWidget, protected QOpenGLFunctions_3_2_Core
	{
	public:
		Viewport(QWidget *parent) : QOpenGLWidget(parent), QOpenGLFunctions_3_2_Core(), m_debugLogger(this) { }

	protected:
		void paintGL() override;
		void resizeGL( int width, int height ) override;
		void initializeGL() override;

	private:
		QOpenGLShaderProgram m_wallShader;
		QOpenGLBuffer m_vertexBuffer{QOpenGLBuffer::Type::VertexBuffer};
		QOpenGLVertexArrayObject m_VAO;

#ifdef _DEBUG
		QOpenGLDebugLogger m_debugLogger;
#endif
	};
}



#endif // !PMP_VIEWPORT_H
