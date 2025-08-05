#pragma once
#include "test_chamber.h"
#include <optional>
#include <qmatrix4x4.h>
#include <qpoint.h>
#include <vector>

namespace PMP
{
    class Selector
    {
        public:
        Selector(){}
        // returns 4 corners of all selected surfaces
        void getSelectionSurfaceCorners(std::vector<QVector3D>& out);
        // returns 8 corners of all selected nodes
        void getSelectionNodeVerts(std::vector<Vertex>& out);
        std::optional<NodeID> getNodeSelection(int index=0) const {return m_nodeSelection.size() <= index ? std::nullopt : std::optional<NodeID>{m_nodeSelection.at(index)};}
        std::optional<ChamberSurface> getSurfaceSelection(int index=0) const {return m_surfaceSelection.size() <= index ? std::nullopt : std::optional<ChamberSurface>{m_surfaceSelection.at(index)};}

        // raycast from point to test chamber to select a single surface
        void clickSelectSurface(QPoint point, QMatrix4x4 projection, QMatrix4x4 view);
        // raycast from point to test chamber to select a single node
        void clickSelectNode(QPoint point, QMatrix4x4 projection, QMatrix4x4 view);
        void dragSelectStart(QPoint start, QMatrix4x4 projection, QMatrix4x4 view);
        void dragSelectEnd(QPoint end, QMatrix4x4 projection, QMatrix4x4 view);

        void moveSelection(const ChamberSurface& selection, QVector3D direction);

        // Testchamber to make selection from
        void setTestChamber(TestChamber* tc) {m_testChamber = tc;};
        void setViewPortDim(int width, int height) {m_viewPortWidth = width; m_viewPortHeight = height;};
        void clearSurfaces();
        private:
        TestChamber* m_testChamber;
        int m_viewPortWidth;
        int m_viewPortHeight;
        //m_selection
        std::vector<ChamberSurface> m_surfaceSelection;
        std::vector<NodeID> m_nodeSelection;
    };
}