// pmp_viewport.h
// Declares the main editor viewport as an extension of QOpenGLWidget
#ifndef PMP_VIEWPORT_H
#pragma once

#include <QOpenGLWidget>

class PMPViewport : public QOpenGLWidget
{
	virtual void paintGL();
	virtual void resizeGL();
	virtual void initializeGL();
};

#endif // !PMP_VIEWPORT_H
