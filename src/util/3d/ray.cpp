
#include "ray.h"
namespace PMP {
    bool RayIntersectsAABB(QVector3D origin, QVector3D dir, QVector3D center, float size)
    {
        const float halfSize = size / 2.0f;
        QVector3D min = center - QVector3D(halfSize, halfSize, halfSize);
        QVector3D max = center + QVector3D(halfSize, halfSize, halfSize);

        float tMin = -std::numeric_limits<float>::infinity();
        float tMax =  std::numeric_limits<float>::infinity();

        for (int i = 0; i < 3; ++i)
        {
            float invD = 1.0f / dir[i];
            float t0 = (min[i] - origin[i]) * invD;
            float t1 = (max[i] - origin[i]) * invD;

            if (invD < 0.0f)
                std::swap(t0, t1);

            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);

            if (tMax < tMin)
                return false;
        }

        return true;
    }

}
