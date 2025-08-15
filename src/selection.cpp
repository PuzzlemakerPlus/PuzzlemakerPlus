#include "selection.h"
#include "test_chamber_types.h"
#include "util/3d/mesh.h"
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
        std::optional<RayHitResult> cast = m_testChamber->rayCast(ray);
        if (!cast.has_value()) return;
        setSelectionSingle(cast->surface);
    }
    void Selector::clickSelectSurfaceAdd(QPoint point, QMatrix4x4 projection, QMatrix4x4 view)
    {
        Q_ASSERT(m_testChamber != nullptr && "Test chamber must be set before selecting surfaces");
        Q_ASSERT((m_viewPortWidth != 0 && m_viewPortHeight != 0) && "Viewport width and height must be set before selecting surfaces");
        Ray ray = screenPosToWorldRay(m_viewPortWidth, m_viewPortHeight, point, projection, view);
        std::optional<RayHitResult> cast = m_testChamber->rayCast(ray);
        if (!cast.has_value()) return;
        if (!hasSelection(cast->surface)) addSelection(cast->surface);
        else {
			// If already selected, remove it from selection
			m_surfaceSelection.erase(std::remove_if(m_surfaceSelection.begin(), m_surfaceSelection.end(),
				[&cast](const ChamberSurface& sel) {
					return sel.parentNode == cast->surface.parentNode && sel.side == cast->surface.side;
				}), m_surfaceSelection.end());
		}
    }
    bool Selector::hasSelection(ChamberSurface surface) const
    {
        for(auto const& sel : m_surfaceSelection)
		{
			if (sel.parentNode == surface.parentNode && sel.side == surface.side)
				return true;
		}
        return false;
    }
    void Selector::getCameraFacingSurfaces(QMatrix4x4 view, std::vector<ChamberSurface>& surfaces, std::vector<ChamberSurface>& out)
    {
        Q_ASSERT(m_testChamber != nullptr && "Test chamber must be set before selecting surfaces");
        // Camera position in world space: inverse(view) * (0,0,0,1)
        const QMatrix4x4 invView = view.inverted();           // QMatrix4x4::inverted()
        const QVector3D camPos = (invView * QVector4D(0,0,0,1)).toVector3D();

        for (const auto& surface : surfaces)
        {
            const QVector3D center = m_testChamber->getSurfaceCenter(surface);
            const QVector3D normal = getWallNormal(surface.side).normalized();
            const QVector3D toCam = camPos - center;
            // Facing camera if normal points toward camera
            if (QVector3D::dotProduct(normal, toCam) > 0.0f) {
                out.push_back(surface);
            }
        }
    }
    bool Selector::isCameraFacingSurface(QMatrix4x4 view, const ChamberSurface& surface)
    {
        Q_ASSERT(m_testChamber != nullptr && "Test chamber must be set before selecting surfaces");
        // Camera position in world space: inverse(view) * (0,0,0,1)
        const QMatrix4x4 invView = view.inverted();           // QMatrix4x4::inverted()
        const QVector3D camPos = (invView * QVector4D(0,0,0,1)).toVector3D();

        const QVector3D center = m_testChamber->getSurfaceCenter(surface);
        const QVector3D normal = getWallNormal(surface.side).normalized();
        const QVector3D toCam = camPos - center;
        // Facing camera if normal points toward camera
        return QVector3D::dotProduct(normal, toCam) > 0.0f;
    }

    void Selector::dragSelectSurfaces(QPoint boxOrigin, QPoint boxEnd, QMatrix4x4 view)
    {
        for(auto& [nodeId, surfaces] : m_testChamber->getSurfaces())
        {
            for (const auto& surface : surfaces)
            {
                if(isCameraFacingSurface(view, surface))
                {
                    
                }
            }
            
        }
    }
}