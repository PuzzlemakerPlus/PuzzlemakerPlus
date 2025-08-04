
#include "ray.h"
namespace PMP {
    bool RayIntersectsTriangle(const Ray& ray,
                           const QVector3D& v0,
                           const QVector3D& v1,
                           const QVector3D& v2,
                           float& t,
                           QVector3D& hitPos,
                           float epsilon)
    {
        QVector3D edge1 = v1 - v0;
        QVector3D edge2 = v2 - v0;

        QVector3D h = QVector3D::crossProduct(ray.direction, edge2);
        float a = QVector3D::dotProduct(edge1, h);

        if (std::abs(a) < epsilon)
            return false; // Parallel

        float f = 1.0f / a;
        QVector3D s = ray.origin - v0;
        float u = f * QVector3D::dotProduct(s, h);
        if (u < 0.0f || u > 1.0f)
            return false;

        QVector3D q = QVector3D::crossProduct(s, edge1);
        float v = f * QVector3D::dotProduct(ray.direction, q);
        if (v < 0.0f || u + v > 1.0f)
            return false;

        t = f * QVector3D::dotProduct(edge2, q);
        if (t > epsilon) {
            hitPos = ray.origin + ray.direction * t;
            return true;
        }

        return false;
    }

    bool RayIntersectsQuad(const Ray& ray, const QVector3D& p0, const QVector3D& p1, const QVector3D& p2, const QVector3D& p3, float& t, QVector3D& hitPos)
    {
        // Two triangle tests
        if (RayIntersectsTriangle(ray, p0, p1, p2, t, hitPos)) return true;
        if (RayIntersectsTriangle(ray, p0, p2, p3, t, hitPos)) return true;
        return false;
    }

    bool RayIntersectsAABB(Ray ray, QVector3D center, float size, float& tHit)
    {
        const float halfSize = size / 2.0f;
        QVector3D min = center - QVector3D(halfSize, halfSize, halfSize);
        QVector3D max = center + QVector3D(halfSize, halfSize, halfSize);

        // Ray origin is inside AABB
        if ((ray.origin.x() >= min.x() && ray.origin.x() <= max.x()) &&
            (ray.origin.y() >= min.y() && ray.origin.y() <= max.y()) &&
            (ray.origin.z() >= min.z() && ray.origin.z() <= max.z()))
        {
            tHit = 0.0f;
            return true;
        }

        float tMin = -std::numeric_limits<float>::infinity();
        float tMax =  std::numeric_limits<float>::infinity();

        for (int i = 0; i < 3; ++i)
        {
            float invD = 1.0f / ray.direction[i];
            float t0 = (min[i] - ray.origin[i]) * invD;
            float t1 = (max[i] - ray.origin[i]) * invD;

            if (invD < 0.0f)
                std::swap(t0, t1);

            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);

            if (tMax < tMin)
                return false;
        }

        tHit = tMin;
        return true;
    }

}
