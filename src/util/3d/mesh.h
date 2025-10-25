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
	enum class WallSide
	{
		POS_X,
		NEG_X,
		POS_Y,
		NEG_Y,
		POS_Z,
		NEG_Z,
		NONE
	};

	struct Vertex
	{
		QVector3D Position;
		QVector3D Normal;
	};

	struct TestChamberVertex
	{
		QVector3D Position;
		QVector3D Normal;
		unsigned int surfaceID;
		unsigned int parentNodeID;
		unsigned int wallSide;
	};

	struct Quad
	{
		std::array<Vertex,4> verts;
		std::array<unsigned int, 6> indices;
	};

	struct TestChamberQuad
	{
		std::array<TestChamberVertex, 4> verts;
		std::array<unsigned int, 6> indices;
	};

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<GLuint> indices;
	};

	struct TestChamberMesh
	{
		std::vector<TestChamberVertex> vertices;
		std::vector<GLuint> indices;
	};

	struct LineVertex{
		QVector3D pos;
		QVector3D col = {1.0f,0.1f,0.1f};
		LineVertex(QVector3D position) : pos(position) {}
		LineVertex(QVector3D position, QVector3D color) : pos(position), col(color) {}
	};

	inline const char* to_string(WallSide s) {
		switch (s) {
			case WallSide::POS_X:  return "POS_X";
			case WallSide::NEG_X:   return "NEG_X";
			case WallSide::POS_Y:   return "POS_Y";
			case WallSide::NEG_Y:  return "NEG_Y";
			case WallSide::POS_Z:    return "POS_Z";
			case WallSide::NEG_Z: return "NEG_Z";
			default:               return "None";
		}
	}

	bool WallFacesDirection(WallSide side, const QVector3D& dir);

	QVector3D Lerp(const QVector3D& a, const QVector3D& b, float t);
	QVector3D getWallNormal(WallSide side);
	QVector3D getWallUp(WallSide side);

	Quad generateCubeFace(WallSide side, float size=128.0f);
	Quad generateQuadAt(QVector3D center, WallSide side, float size);
	TestChamberQuad generateTCQuadAt(QVector3D center, WallSide side, float size, unsigned int surfaceID, unsigned int parentID);

	void CreateMesh(std::vector<Quad> quads, Mesh& out);
	void CreateTCMesh(std::vector<TestChamberQuad> quads, TestChamberMesh& out);

	void QuadCornersToLineWireframe(const std::vector<QVector3D>& corners, std::vector<LineVertex>& out, const QVector3D& color=QVector3D(0.0f,0.0f,0.0f), int start = 0);
	void MultiQuadCornersToLineWireframe(const std::vector<QVector3D>& corners, std::vector<LineVertex>& out, const QVector3D& color=QVector3D(0.0f,0.0f,0.0f));
}