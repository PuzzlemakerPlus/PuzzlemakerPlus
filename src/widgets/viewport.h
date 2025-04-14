// viewport.h
// Declares the main editor viewport as an extension of QOpenGLWidget
#ifndef PMP_VIEWPORT_H
#pragma once

#include <QOpenGLWidget>

namespace PMP
{
	class Viewport : public QOpenGLWidget
	{
	public:
		Viewport(QWidget *parent) : QOpenGLWidget(parent) { }

	protected:
		void paintGL() override;
		void resizeGL( int width, int height ) override;
		void initializeGL() override;
	};
}



#endif // !PMP_VIEWPORT_H
