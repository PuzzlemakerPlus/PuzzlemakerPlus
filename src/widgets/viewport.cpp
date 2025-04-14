// viewport.cpp
// Implements the main editor viewport as an extension of QOpenGLWidget

#include <QOpenGLFunctions>
#include "viewport.h"

namespace PMP
{
	void Viewport::initializeGL()
	{
		QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
		// Puzzlemaker background color
		f->glClearColor( 0.803, 0.815, 0.807, 1.0f );
	}

	void Viewport::resizeGL( int width, int height )
	{
		// Update projection matrix and other size related settings:
		//m_projection.setToIdentity();
		//m_projection.perspective(45.0f, w / float(h), 0.01f, 100.0f);
	}

	void Viewport::paintGL()
	{
		QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
		f->glClear( GL_COLOR_BUFFER_BIT );

	}
}
