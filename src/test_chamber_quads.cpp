#include "test_chamber_quads.h"
#include "test_chamber.h"
#include <iostream>
namespace PMP
{
    void PrintVector3D(const QVector3D& v) {
        std::cout << "(" << v.x() << ", " << v.y() << ", " << v.z() << ")" << std::endl;
    }
    void TestChamberQuads::generateQuads()
    {
        for (const auto& [id, node] : testChamber.GetNodeMap()) {
            if (!testChamber.IsNodeActive(id) || testChamber.NodeHasChild(id)) continue;
            std::cout << "Generating Quad for Node " << id << std::endl;
            const QVector3D& center = node.m_center;
            const float halfSize = node.m_size / 2.0f;
            const float epsilon = 0.4f;

            for (int i = 0; i < 6; ++i) {
                WALLSIDE side = static_cast<WALLSIDE>(i);
                QVector3D offset;

                switch (side) {
                    case POS_X: offset = {+halfSize + epsilon, 0, 0}; break;
                    case NEG_X: offset = {-halfSize - epsilon, 0, 0}; break;
                    case POS_Y: offset = {0, +halfSize + epsilon, 0}; break;
                    case NEG_Y: offset = {0, -halfSize - epsilon, 0}; break;
                    case POS_Z: offset = {0, 0, +halfSize + epsilon}; break;
                    case NEG_Z: offset = {0, 0, -halfSize - epsilon}; break;
                }

                QVector3D neighborCoord = center + offset;

                bool isVisible = testChamber.IsOutOOB(neighborCoord); //if coordinate behind this surface is OOB, this surface should be drawn
                if (!isVisible) { // the coordinate behind this surface is in bound
                    NodeID neighborId = testChamber.SearchNodeWithCoord(neighborCoord);
                    isVisible = !testChamber.IsNodeActive(neighborId); // check if the node behind this surface is inactive
                }

                if (isVisible) quads.push_back(generateQuadAt(center, side, node.m_size));
            }
        }
        std::cout << "Generated " << quads.size() << " Quads" << std::endl;
    }
}
