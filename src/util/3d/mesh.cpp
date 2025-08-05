
#include "mesh.h"
#include <qvectornd.h>


namespace PMP
{
    QVector3D GetWallNormal(WALLSIDE side)
    {
        switch (side) {
            case POS_X:   return QVector3D(-1,  0,  0);
            case NEG_X:  return QVector3D( 1,  0,  0);
            case POS_Y: return QVector3D( 0, -1,  0);
            case NEG_Y:    return QVector3D( 0,  1,  0);
            case POS_Z:   return QVector3D( 0,  0, -1);
            case NEG_Z:  return QVector3D( 0,  0,  1);
            default: return QVector3D(0, 0, 0); // default fallback
        }
    }
    QVector3D GetWallUp(WALLSIDE side) {
        switch (side) {
            case POS_X: return QVector3D(0, 1, 0); // +Y is up on +X face
            case NEG_X: return QVector3D(0, 1, 0); // same for -X face
            case POS_Y: return QVector3D(0, 0, 1); // +Z is out, so up is -Z
            case NEG_Y: return QVector3D(0, 0, 1);  // -Z is out, up is +Z
            case POS_Z: return QVector3D(0, 1, 0); // +Y is up on +Z face
            case NEG_Z: return QVector3D(0, 1, 0); // +Y is up on -Z face
            default: return QVector3D(0, 0, 0); // default fallback
        }
    }
    bool WallFacesDirection(WALLSIDE side, const QVector3D& dir) 
    {
        QVector3D normal = GetWallNormal(side);
        return QVector3D::dotProduct(normal, dir) < 0.0f;
    }
	Quad generateCubeFace(WALLSIDE side, float size) {
        const QVector3D p0(0.0f, 0.0f, 0.0f);
        const QVector3D p1(size, 0.0f, 0.0f);
        const QVector3D p2(size, size, 0.0f);
        const QVector3D p3(0.0f, size, 0.0f);
        const QVector3D p4(0.0f, 0.0f, size);
        const QVector3D p5(size, 0.0f, size);
        const QVector3D p6(size, size, size);
        const QVector3D p7(0.0f, size, size);

        std::array<QVector3D, 4> quad;
        QVector3D normal = GetWallNormal(side);

        switch (side) {
            case WALLSIDE::NEG_Z: // Bottom
                quad = { p0, p1, p2, p3 };
                break;
            case WALLSIDE::NEG_Y: // South
                quad = { p1, p0, p4, p5 };
                break;
            case WALLSIDE::POS_X: // East
                quad = { p2, p1, p5, p6 };
                break;
            case WALLSIDE::POS_Y: // North
                quad = { p3, p2, p6, p7 };
                break;
            case WALLSIDE::NEG_X: // West
                quad = { p0, p3, p7, p4 };
                break;
            case WALLSIDE::POS_Z: // Top
                quad = { p5, p4, p7, p6 };
                break;
            default:
                throw std::out_of_range("Invalid side index");
        }

        std::array<Vertex, 4> face;
        for (int i = 0; i < 4; ++i) {
            face[i] = Vertex { quad[i], normal };
        }
        return {face, {0,1,2,0,2,3}};
    }

    Quad generateQuadAt(QVector3D center, WALLSIDE side, float size)
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
        QVector3D normal;

        switch (side) {
            case WALLSIDE::NEG_Z: // Back
                quad = { p0, p1, p2, p3 };
                normal = QVector3D(0.0f, 0.0f, 1.0f);
                break;
            case WALLSIDE::NEG_Y: // Bottom
                quad = { p1, p0, p4, p5 };
                normal = QVector3D(0.0f, -1.0f, 0.0f);
                break;
            case WALLSIDE::POS_X: // Right
                quad = { p2, p1, p5, p6 };
                normal = QVector3D(1.0f, 0.0f, 0.0f);
                break;
            case WALLSIDE::POS_Y: // Top
                quad = { p3, p2, p6, p7 };
                normal = QVector3D(0.0f, 1.0f, 0.0f);
                break;
            case WALLSIDE::NEG_X: // Left
                quad = { p0, p3, p7, p4 };
                normal = QVector3D(-1.0f, 0.0f, 0.0f);
                break;
            case WALLSIDE::POS_Z: // Front
                quad = { p5, p4, p7, p6 };
                normal = QVector3D(0.0f, 0.0f, -1.0f);
                break;
            default:
                throw std::out_of_range("Invalid side index");
        }

        std::array<Vertex, 4> face;
        for (int i = 0; i < 4; ++i) {
            face[i] = Vertex { quad[i], normal };
        }

        return { face, {0, 1, 2, 0, 2, 3} };
    }

    Mesh CreateMesh(std::vector<Quad> quads)
    {
        unsigned int vertexOffset = 0;
        std::vector<GLfloat> vertexBufferData;
        std::vector<GLint> indexBufferData;
        for (const auto& quad : quads) {
			// Add vertices
			//vertexBufferData.insert(vertexBufferData.end(), quad.verts.begin(), quad.verts.end());
			for (const auto& v : quad.verts) {
				vertexBufferData.push_back(v.Position.x());
				vertexBufferData.push_back(v.Position.y());
				vertexBufferData.push_back(v.Position.z());

				vertexBufferData.push_back(v.Normal.x());
				vertexBufferData.push_back(v.Normal.y());
				vertexBufferData.push_back(v.Normal.z());
			}

			// Add indices (adjusted)
			for (unsigned int idx : quad.indices) {
				indexBufferData.push_back(idx + vertexOffset);
			}

			vertexOffset += static_cast<unsigned int>(quad.verts.size());
		}
        return {vertexBufferData, indexBufferData};
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