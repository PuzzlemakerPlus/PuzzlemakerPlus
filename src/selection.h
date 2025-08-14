#pragma once
#include <vector>
#include <qpoint.h>
#include <optional>
#include <qmatrix4x4.h>

#include "util/3d/mesh.h"
#include "test_chamber_types.h"

namespace PMP
{
    class TestChamber;

    class Selector
    {
        public:
        Selector(){}
        // Returns 4 corners of all selected surfaces
        void getSelectionSurfaceCorners(std::vector<QVector3D>& out);

        

        std::optional<NodeID> getNodeSelection(int index=0) const {return m_surfaceSelection.size() <= index ? std::nullopt : std::optional<NodeID>{m_surfaceSelection.at(index).parentNode};}
        std::optional<ChamberSurface> getSurfaceSelection(int index=0) const {return m_surfaceSelection.size() <= index ? std::nullopt : std::optional<ChamberSurface>{m_surfaceSelection.at(index)};}
        void getSurfaceSelectionMultiple(std::vector<ChamberSurface>& out) const {
			out.clear();
			out.insert(out.end(), m_surfaceSelection.begin(), m_surfaceSelection.end());
		}

        // Select single surface with ray cast
        void clickSelectSurface(QPoint point, QMatrix4x4 projection, QMatrix4x4 view);
        // Add single surface to selection with ray cast
        void clickSelectSurfaceAdd(QPoint point, QMatrix4x4 projection, QMatrix4x4 view);
        // For selecting multiple surfaces. Preserves previous selection
        void addSelection(ChamberSurface surface) { m_surfaceSelection.push_back(surface); }
        // For selecting multiple surfaces. Preserves previous selection
        void addSelectionMultiple(std::vector<ChamberSurface>& surfaces);
        // Select single surface
        void setSelectionSingle(ChamberSurface surface);
        // Select multiple surfaces, clears previous selection
        void setSelectionMultiple(std::vector<ChamberSurface>& surface);


        // Testchamber to make selection from
        void setTestChamber(TestChamber* tc) {m_testChamber = tc;};
        void setViewPortDim(int width, int height) {m_viewPortWidth = width; m_viewPortHeight = height;};
        void clearSurfaces() { m_surfaceSelection.clear(); };
        private:
        TestChamber* m_testChamber = nullptr;
        int m_viewPortWidth = 0;
        int m_viewPortHeight = 0;
        //m_selection
        std::vector<ChamberSurface> m_surfaceSelection;
    };
}