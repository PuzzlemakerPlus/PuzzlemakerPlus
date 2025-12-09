
#include "mesh.h"
#include <qvectornd.h>
#include <stdexcept>
#include <iostream>

namespace PMP
{
	QVector3D Lerp(const QVector3D& a, const QVector3D& b, float t) {return a + t * (b - a);}

	bool WallFacesDirection(WallSide side, const QVector3D& dir) 
	{
		IntVector3D gNormal = getWallNormal(side);
		return gNormal.dot(dir) < 0;
	}

	IntVector3D getWallNormal(WallSide side)
	{
		using enum WallSide;
		switch (side) {
			case POS_X:   return IntVector3D(-1,  0,  0); // wall on a positive x side would have negative x normal
			case NEG_X:  return IntVector3D( 1,  0,  0);
			case POS_Y: return IntVector3D( 0, -1,  0);
			case NEG_Y:    return IntVector3D( 0,  1,  0);
			case POS_Z:   return IntVector3D( 0,  0, -1);
			case NEG_Z:  return IntVector3D( 0,  0,  1);
			default: return IntVector3D(0, 0, 0); // default fallback
		}
	}

	IntVector3D getWallUp(WallSide side) {
		using enum WallSide;
		switch (side) {
			case POS_X: return IntVector3D(0, 1, 0); // +Y is up on +X face
			case NEG_X: return IntVector3D(0, 1, 0); // same for -X face
			case POS_Y: return IntVector3D(0, 0, 1); // +Z is out, so up is -Z
			case NEG_Y: return IntVector3D(0, 0, 1);  // -Z is out, up is +Z
			case POS_Z: return IntVector3D(0, 1, 0); // +Y is up on +Z face
			case NEG_Z: return IntVector3D(0, 1, 0); // +Y is up on -Z face
			default: return IntVector3D(0, 0, 0); // default fallback
		}
	}

	Quad generateCubeFace(WallSide side, float size) {
		const QVector3D p0(0.0f, 0.0f, 0.0f);
		const QVector3D p1(size, 0.0f, 0.0f);
		const QVector3D p2(size, size, 0.0f);
		const QVector3D p3(0.0f, size, 0.0f);
		const QVector3D p4(0.0f, 0.0f, size);
		const QVector3D p5(size, 0.0f, size);
		const QVector3D p6(size, size, size);
		const QVector3D p7(0.0f, size, size);

		std::array<QVector3D, 4> quad;
		IntVector3D gNormal = getWallNormal(side);

		switch (side) {
			case WallSide::NEG_Z: // Bottom
				quad = { p0, p1, p2, p3 };
				break;
			case WallSide::NEG_Y: // South
				quad = { p1, p0, p4, p5 };
				break;
			case WallSide::POS_X: // East
				quad = { p2, p1, p5, p6 };
				break;
			case WallSide::POS_Y: // North
				quad = { p3, p2, p6, p7 };
				break;
			case WallSide::NEG_X: // West
				quad = { p0, p3, p7, p4 };
				break;
			case WallSide::POS_Z: // Top
				quad = { p5, p4, p7, p6 };
				break;
			default:
				throw std::out_of_range("Invalid side index");
		}

		std::array<Vertex, 4> face;
		for (int i = 0; i < 4; ++i) {
			face[i] = Vertex { quad[i], gNormal.toQVector3D()};
		}
		return {face, {0,1,2,0,2,3}};
	}

	Quad generateQuadAt(QVector3D center, WallSide side, float size)
	{
		float h = size / 2.0f;

		QVector3D p0 = center + QVector3D(-h, -h, -h);
		QVector3D p1 = center + QVector3D(+h, -h, -h);
		QVector3D p2 = center + QVector3D(+h, +h, -h);
		QVector3D p3 = center + QVector3D(-h, +h, -h);
		QVector3D p4 = center + QVector3D(-h, -h, +h);
		QVector3D p5 = center + QVector3D(+h, -h, +h);
		QVector3D p6 = center + QVector3D(+h, +h, +h);
		QVector3D p7 = center + QVector3D(-h, +h, +h);

		std::array<QVector3D, 4> quad;
		QVector3D gNormal;

		switch (side) {
			case WallSide::NEG_Z: // Back
				quad = { p0, p1, p2, p3 };
				gNormal = QVector3D(0.0f, 0.0f, 1.0f);
				break;
			case WallSide::NEG_Y: // Bottom
				quad = { p1, p0, p4, p5 };
				gNormal = QVector3D(0.0f, -1.0f, 0.0f);
				break;
			case WallSide::POS_X: // Right
				quad = { p2, p1, p5, p6 };
				gNormal = QVector3D(1.0f, 0.0f, 0.0f);
				break;
			case WallSide::POS_Y: // Top
				quad = { p3, p2, p6, p7 };
				gNormal = QVector3D(0.0f, 1.0f, 0.0f);
				break;
			case WallSide::NEG_X: // Left
				quad = { p0, p3, p7, p4 };
				gNormal = QVector3D(-1.0f, 0.0f, 0.0f);
				break;
			case WallSide::POS_Z: // Front
				quad = { p5, p4, p7, p6 };
				gNormal = QVector3D(0.0f, 0.0f, -1.0f);
				break;
			default:
				throw std::out_of_range("Invalid side index");
		}

		std::array<Vertex, 4> face;
		for (int i = 0; i < 4; ++i) {
			face[i] = Vertex { quad[i], gNormal };
		}

		return { face, {0, 1, 2, 0, 2, 3} };
	}

	TestChamberQuad generateTCQuadAt(QVector3D center, WallSide side, float size, unsigned int surfaceID, unsigned int parentID)
	{
		float h = size / 2.0f;

		QVector3D p0 = center + QVector3D(-h, -h, -h);
		QVector3D p1 = center + QVector3D(+h, -h, -h);
		QVector3D p2 = center + QVector3D(+h, +h, -h);
		QVector3D p3 = center + QVector3D(-h, +h, -h);
		QVector3D p4 = center + QVector3D(-h, -h, +h);
		QVector3D p5 = center + QVector3D(+h, -h, +h);
		QVector3D p6 = center + QVector3D(+h, +h, +h);
		QVector3D p7 = center + QVector3D(-h, +h, +h);

		std::array<QVector3D, 4> quad;
		QVector3D gNormal;

		switch (side) {
			case WallSide::NEG_Z: // Back
				quad = { p0, p1, p2, p3 };
				gNormal = QVector3D(0.0f, 0.0f, 1.0f);
				break;
			case WallSide::NEG_Y: // Bottom
				quad = { p1, p0, p4, p5 };
				gNormal = QVector3D(0.0f, -1.0f, 0.0f);
				break;
			case WallSide::POS_X: // Right
				quad = { p2, p1, p5, p6 };
				gNormal = QVector3D(1.0f, 0.0f, 0.0f);
				break;
			case WallSide::POS_Y: // Top
				quad = { p3, p2, p6, p7 };
				gNormal = QVector3D(0.0f, 1.0f, 0.0f);
				break;
			case WallSide::NEG_X: // Left
				quad = { p0, p3, p7, p4 };
				gNormal = QVector3D(-1.0f, 0.0f, 0.0f);
				break;
			case WallSide::POS_Z: // Front
				quad = { p5, p4, p7, p6 };
				gNormal = QVector3D(0.0f, 0.0f, -1.0f);
				break;
			default:
				throw std::out_of_range("Invalid side index");
		}

		std::array<TestChamberVertex, 4> face;
		for (int i = 0; i < 4; ++i) {
			face[i] = TestChamberVertex{ quad[i], gNormal, surfaceID, parentID, static_cast<unsigned int>(side), QVector2D(0,0) };
		}

		return { face, {0, 1, 2, 0, 2, 3}};
	}

	void CreateMesh(std::vector<Quad> quads, Mesh& out)
	{
		unsigned int vertexOffset = 0;
		for (const auto& quad : quads) {
			// Add vertices
			for(int i=0;i<4;i++)
			{
				out.vertices.push_back(quad.verts[i]);
			}
			// Add indices (adjusted)
			for (unsigned int idx : quad.indices) {
				out.indices.push_back(idx + vertexOffset);
			}
			vertexOffset += static_cast<unsigned int>(quad.verts.size());
		}
	}

	void CreateTCMesh(std::vector<TestChamberQuad> quads, TestChamberMesh& out)
	{
		unsigned int vertexOffset = 0;
		for (const auto& quad : quads) {
			// Add vertices
			for(int i=0;i<4;i++)
			{
				out.vertices.push_back(quad.verts[i]);
			}
			// Add indices (adjusted)
			for (unsigned int idx : quad.indices) {
				out.indices.push_back(idx + vertexOffset);
			}
			vertexOffset += static_cast<unsigned int>(quad.verts.size());
		}
	}
	
	void generateWireframeGrid3D(int subdivisions, const QVector3D& A, const QVector3D& B,const QVector3D& C, const QVector3D& D, std::vector<LineVertex>& out, const QVector3D& color)
	{
		for (int i = 0; i <= subdivisions; ++i) {
			float t = static_cast<float>(i) / subdivisions;

			// Horizontal line (left to right)
			QVector3D left  = Lerp(A, D, t);
			QVector3D right = Lerp(B, C, t);
			out.emplace_back(left, color);
			out.emplace_back(right, color);

			// Vertical line (top to bottom)
			QVector3D top    = Lerp(A, B, t);
			QVector3D bottom = Lerp(D, C, t);
			out.emplace_back(top, color);
			out.emplace_back(bottom, color);
		}
	}

	void QuadCornersToSubdividedLineWireframes(int subdivisions, const std::vector<QVector3D>& corners, std::vector<LineVertex>& out, const QVector3D& color, int start)
	{
		if(corners.size() != 4) throw std::runtime_error("Subdividing multiple quads is not supported yet");
		generateWireframeGrid3D(subdivisions, corners.at(0),corners.at(1), corners.at(2), corners.at(3), out, color);
	}

	void QuadCornersToLineWireframe(const std::vector<QVector3D>& corners, std::vector<LineVertex>& out, const QVector3D& color, int start)
	{
		for (int i = start; i < start+4; ++i)
		{
			out.emplace_back(corners[i], color);
			out.emplace_back(corners[start + ((i - start + 1) % 4)], color);
		}
	}

	void MultiQuadCornersToLineWireframe(const std::vector<QVector3D>& corners, std::vector<LineVertex>& out, const QVector3D& color)
	{
		for(int i=0;i < corners.size() / 4;i++)
		{
			QuadCornersToLineWireframe(corners, out, color, i*4);
		}
	}
}