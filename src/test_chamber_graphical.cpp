// source file for graphical functions

#include <vector>
#include "test_chamber.h"
#include "util/debug.h"
#include <iostream>
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
				const TestChamberNode& node = m_nodeMap.at(nodeID);
				out.push_back(generateQuadAt(node.m_center.toQVector3D(), surface.side, node.m_size));
			}
		}
	}
	void TestChamber::generateTCQuads(std::vector<TestChamberQuad>& out) const
	{
		if (m_surfaceMap.size() == 0) throw std::runtime_error("Attempted to generate quads for test chamber with no surface");
		for (const auto& [surfaceID, surfaces] : m_surfaceMap)
		{
			int sideID = 0;
			for (const auto& surface : surfaces)
			{
				if (surface.side == WallSide::NONE) continue;
				//TODO: surfaceID and nodeID are the exact same value. why are they being passed in?
				//std::cout << surfaceID << ", " << nodeID << "\n";
				const NodeID nodeID = surface.parentNode;
				const TestChamberNode& node = m_nodeMap.at(nodeID);
				out.push_back(generateTCQuadAt(node.m_center.toQVector3D(), surface.side, node.m_size, sideID, nodeID));
				sideID++;
			}
		}
	}
	// Same as above except only generate quads for shadow-casting surfaces
	void TestChamber::generateTCShadowQuads(std::vector<TestChamberQuad>& out) const
	{
		if (m_surfaceMap.size() == 0) throw std::runtime_error("Attempted to generate quads for test chamber with no surface");
		for (const auto& [surfaceID, surfaces] : m_surfaceMap)
		{
			int sideID = 0;
			for (const auto& surface : surfaces)
			{
				if (surface.side == WallSide::NONE) continue;
				const NodeID nodeID = surface.parentNode;
				const TestChamberNode& node = m_nodeMap.at(nodeID);
				if (surface.side == WallSide::POS_Z)
				{
					if (node.m_center.z() + node.m_size / 2 > defaultRootNodeSize / 2 - gridSizeMin) continue;
				}
				out.push_back(generateTCQuadAt(node.m_center.toQVector3D(), surface.side, node.m_size, sideID, nodeID));
				sideID++;
			}
		}
		// generate extra quads to catch shadows
		std::vector<NodeID> ceilingNodes;
		QVector3D chamberMin = QVector3D(-(int)defaultRootNodeSize / 2, -(int)defaultRootNodeSize / 2, defaultRootNodeSize / 2 - gridSizeMin);
		QVector3D chamberMax = QVector3D(defaultRootNodeSize / 2, defaultRootNodeSize / 2, defaultRootNodeSize / 2);
		getNodesTouchingBBox(chamberMin, chamberMax, ceilingNodes);
		for (const auto& nodeID : ceilingNodes)
		{
			const TestChamberNode& node = m_nodeMap.at(nodeID);
			if (!node.m_active)
			{
				//Debugger::instance().addDebugPoint(m_nodeMap.at(nodeID).m_center.toQVector3D(), QVector3D(1.0f, 1.0f, 0));
				out.push_back(generateTCQuadAt(node.m_center.toQVector3D(), WallSide::POS_Z, node.m_size, nodeID, nodeID));
			}
		}
		//walls to catch light bleeding from side walls
		const QVector3D& rootCenter = m_nodeMap.at(m_rootNode).m_center.toQVector3D();
		out.push_back(generateTCQuadAt(rootCenter, WallSide::POS_X, defaultRootNodeSize+25, m_rootNode, m_rootNode, 1.1f));
		out.push_back(generateTCQuadAt(rootCenter, WallSide::POS_Y, defaultRootNodeSize+25, m_rootNode, m_rootNode, 1.1f));
		out.push_back(generateTCQuadAt(rootCenter, WallSide::NEG_X, defaultRootNodeSize+25, m_rootNode, m_rootNode, 1.1f));
		out.push_back(generateTCQuadAt(rootCenter, WallSide::NEG_Y, defaultRootNodeSize+25, m_rootNode, m_rootNode, 1.1f));
		//Debugger::instance().drawDebugPoints();
	}
}