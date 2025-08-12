#include "selection.h"
#include "util/3d/ray.h"
#include "test_chamber.h"
#include <qvectornd.h>

namespace PMP
{
    //Call this BEFORE test chamber node gets updated
    //This is because surfaces are regenerated every time the node tree structure changes
    void Selector::clearSurfaces()
    {
        m_surfaceSelection.clear(); //Because the surfaces always changes when test 
        
        //And make appropriate changes to the selection
    }
    void Selector::getSelectionSurfaceCorners(std::vector<QVector3D>& out)
    {
        for(const auto& sel : m_surfaceSelection)
        {
            qDebug() << "Get selection corners for surface";
            std::array<QVector3D,4> corners = m_testChamber->GetChamberSurfaceCorners(sel);
            out.insert(out.end(), corners.begin(), corners.end());
        }
    }
    void Selector::setSelection(ChamberSurface surface, int index)
    {
        if(index < m_surfaceSelection.size())
        {
            m_surfaceSelection[index] = surface;
        }
        else{
            m_surfaceSelection.push_back(surface);
        }
        qDebug() << "Selected surface of node " << surface.parentNode;
    }
    void Selector::setSelectionMultiple(std::vector<ChamberSurface>& surfaces)
    {
        for(const auto& sel : surfaces)
		{
            m_surfaceSelection.push_back(sel);
		}
    }
    void Selector::addSelection(ChamberSurface surface)
    {
        m_surfaceSelection.push_back(surface);
    }
    // Select single surface with ray cast
    void Selector::clickSelectSurface(QPoint point, QMatrix4x4 projection, QMatrix4x4 view)
    {
        Ray ray = ScreenPosToWorldRay(m_viewPortWidth,m_viewPortHeight, point, projection, view);
        if(m_testChamber)
        {
            std::optional<RayHitResult> cast = m_testChamber->RayCast(ray);
            if(cast.has_value())
            {
                m_surfaceSelection.clear();
                setSelection(cast->surface, 0);
            }
        }
    }
    // Select single surface with ray cast
    void Selector::clickSelectSurfaceAdd(QPoint point, QMatrix4x4 projection, QMatrix4x4 view)
    {
        Ray ray = ScreenPosToWorldRay(m_viewPortWidth, m_viewPortHeight, point, projection, view);
        if (m_testChamber)
        {
            std::optional<RayHitResult> cast = m_testChamber->RayCast(ray);
            if (cast.has_value())
            {
                addSelection(cast->surface);
            }
        }
    }
}