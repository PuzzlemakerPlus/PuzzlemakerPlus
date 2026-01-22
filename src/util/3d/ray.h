// ray.h
// Utility functions for ray casting
#pragma once

#include <qvectornd.h>

namespace PMP
{
struct Ray
{
	QVector3D origin;
	QVector3D direction;
};

Ray screenPosToWorldRay(int width, int height, QPoint mousePos, QMatrix4x4 projectionMatrix, QMatrix4x4 viewMatrix);
bool rayIntersectsTriangle(const Ray& ray, const QVector3D& v0, const QVector3D& v1, const QVector3D& v2, float& t,
                           QVector3D& hitPos, float epsilon = 1e-6f);
bool rayIntersectsQuad(const Ray& ray, const QVector3D& p0, const QVector3D& p1, const QVector3D& p2,
                       const QVector3D& p3, float& t, QVector3D& hitPos);
bool rayIntersectsAABB(Ray ray, QVector3D center, float size, float& tHit);
} // namespace PMP
