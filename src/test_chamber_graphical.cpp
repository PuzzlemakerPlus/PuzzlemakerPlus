// source file for graphical functions

#include <vector>
#include "test_chamber.h"
namespace PMP {
	void TestChamber::generateQuads(std::vector<Quad>& out) const
	{
		if (m_surfaceMap.size() == 0) throw std::runtime_error("Attempted to generate quads for test chamber with no surface. Forgot to call GenerateSurfaces()?");
		for (const auto& [_, surfaces] : m_surfaceMap)
		{
			for (const auto& surface : surfaces)
			{
				if (surface.side == WallSide::NONE) continue;
				const NodeID nodeID = surface.parentNode;
				out.push_back(generateQuadAt(m_nodeMap.at(nodeID).m_center.toQVector3D(), surface.side, m_nodeMap.at(nodeID).m_size));
			}
		}
	}
	void TestChamber::generateTCQuads(std::vector<TestChamberQuad>& out) const
	{
		if (m_surfaceMap.size() == 0) throw std::runtime_error("Attempted to generate quads for test chamber with no surface. Forgot to call GenerateSurfaces()?");
		for (const auto& [surfaceID, surfaces] : m_surfaceMap)
		{
			for (const auto& surface : surfaces)
			{
				if (surface.side == WallSide::NONE) continue;
				const NodeID nodeID = surface.parentNode;

				out.push_back(generateTCQuadAt(m_nodeMap.at(nodeID).m_center.toQVector3D(), surface.side, m_nodeMap.at(nodeID).m_size, surfaceID, nodeID));
			}
		}
	}
}