#include "selection.h"
#include "util/3d/ray.h"
#include "test_chamber.h"
#include <qvectornd.h>

namespace PMP
{
    void Selector::setSelectionSingle(ChamberSurface surface)
    {
		qDebug() << "Selected single surface of node " << surface.parentNode;
        m_surfaceSelection.clear();
		m_surfaceSelection.push_back(surface);
    }
    void Selector::getSelectionSurfaceCorners(std::vector<QVector3D>& out)
    {
        for(const auto& sel : m_surfaceSelection)
        {
            std::array<QVector3D,4> corners = m_testChamber->getChamberSurfaceCorners(sel);
            out.insert(out.end(), corners.begin(), corners.end());
        }
    }
    void Selector::addSelectionMultiple(std::vector<ChamberSurface>& surfaces)
    {
        for (const auto& sel : surfaces)
        {
            m_surfaceSelection.push_back(sel);
        }
    }
    void Selector::setSelectionMultiple(std::vector<ChamberSurface>& surfaces)
    {
        m_surfaceSelection.clear();
        for(const auto& sel : surfaces)
		{
            m_surfaceSelection.push_back(sel);
		}
    }
    void Selector::clickSelectSurface(QPoint point, QMatrix4x4 projection, QMatrix4x4 view)
    {
        Q_ASSERT(m_testChamber != nullptr && "Test chamber must be set before selecting surfaces");
        Q_ASSERT((m_viewPortWidth != 0 && m_viewPortHeight != 0) && "Viewport width and height must be set before selecting surfaces");
        Ray ray = screenPosToWorldRay(m_viewPortWidth,m_viewPortHeight, point, projection, view);
        if (!m_testChamber) return;
        std::optional<RayHitResult> cast = m_testChamber->rayCast(ray);
        if (!cast.has_value()) return;
        setSelectionSingle(cast->surface);
    }
    void Selector::clickSelectSurfaceAdd(QPoint point, QMatrix4x4 projection, QMatrix4x4 view)
    {
        Ray ray = screenPosToWorldRay(m_viewPortWidth, m_viewPortHeight, point, projection, view);
        if (!m_testChamber) return;
        std::optional<RayHitResult> cast = m_testChamber->rayCast(ray);
        if (!cast.has_value()) return;
        addSelection(cast->surface);
    }
}