// viewport.cpp
// Implements the main editor viewport as an extension of QOpenGLWidget

#include <QList>

#include "viewport.h"

namespace PMP
{
	// FIXME: remove
	static GLfloat const tempVertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f,  0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f
	};
	static GLint const tempIndices[] = {
		0, 1, 2,
		0, 2, 3
	};

	void Viewport::initializeGL()
	{
		initializeOpenGLFunctions();

#ifdef _DEBUG
		if ( m_debugLogger.initialize() )
		{
			connect( &m_debugLogger, &QOpenGLDebugLogger::messageLogged, [=](const QOpenGLDebugMessage &debugMessage) {
						qDebug() << debugMessage;
					});
			m_debugLogger.startLogging();
		}
		else
		{
			qDebug() << "Debug extension not available!\n";
		}
#endif

		// Puzzlemaker background color
		glClearColor( 0.803, 0.815, 0.807, 1.0f );

		// FIXME: Un-hardcode shaders, this is just temporary so I can get it rendering something - Kelsey
		const char* vertexShaderSource = "#version 330 core\n"
			"layout (location = 0) in vec3 aPos;\n"
			"void main()\n"
			"{\n"
			"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
			"}\0";

		const char* fragmentShaderSource = "#version 330 core\n"
			"out vec4 FragColor;\n"
			"void main()\n"
			"{\n"
			"	FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
			"}\0";

		m_wallShader.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
		m_wallShader.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
		m_wallShader.link();

		m_VAO.create();
		m_VAO.bind();

		m_vertexBuffer.create();
		m_vertexBuffer.bind();
		// Disabling for now until we are more certain it's what we want - Kelsey
		//m_vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
		m_vertexBuffer.allocate(tempVertices, sizeof(tempVertices)); // todo

		m_indexBuffer.create();
		m_indexBuffer.bind();
		m_indexBuffer.allocate(tempIndices, sizeof(tempIndices));

		int vertexPosLocation = m_wallShader.attributeLocation("aPos");
		m_wallShader.enableAttributeArray(vertexPosLocation);
		m_wallShader.setAttributeBuffer(vertexPosLocation, GL_FLOAT, 0, 3, 3 * sizeof(GLfloat));

		m_VAO.release();

		m_indexBuffer.release();
		m_vertexBuffer.release();
	}

	void Viewport::resizeGL( int width, int height )
	{
		// Update projection matrix and other size related settings
	}

	void Viewport::paintGL()
	{
		// Very temporary rendering code
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		glEnable( GL_DEPTH_TEST );
		//glEnable( GL_CULL_FACE );
		
		m_wallShader.bind();
		m_VAO.bind();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		m_VAO.release();
		m_wallShader.release();
	}
}
