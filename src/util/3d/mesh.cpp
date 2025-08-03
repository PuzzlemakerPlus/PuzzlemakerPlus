
#include "mesh.h"


namespace PMP
{
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
        QVector3D normal;

        switch (side) {
            case WALLSIDE::NEG_Z: // Bottom
                quad = { p0, p1, p2, p3 };
                normal = QVector3D(0.0f, 0.0f, 1.0f);
                break;
            case WALLSIDE::NEG_Y: // South
                quad = { p1, p0, p4, p5 };
                normal = QVector3D(0.0f, 1.0f, 0.0f);
                break;
            case WALLSIDE::POS_X: // East
                quad = { p2, p1, p5, p6 };
                normal = QVector3D(-1.0f, 0.0f, 0.0f);
                break;
            case WALLSIDE::POS_Y: // North
                quad = { p3, p2, p6, p7 };
                normal = QVector3D(0.0f, -1.0f, 0.0f);
                break;
            case WALLSIDE::NEG_X: // West
                quad = { p0, p3, p7, p4 };
                normal = QVector3D(1.0f, 0.0f, 0.0f);
                break;
            case WALLSIDE::POS_Z: // Top
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
        return {face, {0,1,2,0,2,3}};
    }
}