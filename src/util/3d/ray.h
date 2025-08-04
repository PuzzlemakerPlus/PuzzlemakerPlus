#pragma once
#include <qvectornd.h>
namespace PMP
{
    using NodeID = unsigned int;

    struct Ray {
        QVector3D origin;
        QVector3D direction;
    };
    struct RayHitResult {
        NodeID nodeID;
        QVector3D hitPoint;
    };
    bool RayIntersectsTriangle(const Ray& ray,const QVector3D& v0,const QVector3D& v1,const QVector3D& v2,float& t,QVector3D& hitPos,float epsilon = 1e-6f);
    bool RayIntersectsQuad(const Ray& ray, const QVector3D& p0, const QVector3D& p1, const QVector3D& p2, const QVector3D& p3, float& t, QVector3D& hitPos);
    bool RayIntersectsAABB(Ray ray, QVector3D center, float size, float& tHit);
}