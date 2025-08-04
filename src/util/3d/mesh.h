#pragma once
#include <qvectornd.h>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLDebugLogger>
#include <QOpenGLVertexArrayObject>
#include <vector>

namespace PMP
{
	enum WALLSIDE {
		POS_X,
		NEG_X,
		POS_Y,
		NEG_Y,
		POS_Z,
		NEG_Z,
        NONE
	};

    struct Vertex {
        QVector3D Position;
        QVector3D Normal;
    };

    struct Quad {
        std::array<Vertex,4> verts;
        std::array<unsigned int, 6> indices;
    };

    struct Mesh{
        std::vector<GLfloat> vertices;
        std::vector<GLint> indices;
    };

    QVector3D GetWallNormal(WALLSIDE side);
    QVector3D GetWallUp(WALLSIDE side);
    bool WallFacesDirection(WALLSIDE side, const QVector3D& dir);

    Quad generateCubeFace(WALLSIDE side, float size=128.0f);
    Quad generateQuadAt(QVector3D center, WALLSIDE side, float size);

    Mesh CreateMesh(std::vector<Vertex> vertices, std::vector<unsigned int> indexBuffer);
    Mesh CreateMesh(std::vector<Quad> quads);
}