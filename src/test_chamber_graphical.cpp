// test_chamber_graphical.cpp
// source file for graphical functions

#include "test_chamber.h"

#include <iostream>
#include <limits>
#include <vector>

#include "util/debug.h"

namespace PMP
{
void TestChamber::generateQuads(std::vector<Quad>& out) const
{
	if (m_surfaceMap.size() == 0)
		throw std::runtime_error(
		    "Attempted to generate quads for test chamber with no surface. Forgot to call GenerateSurfaces()?");
	for (const auto& [_, surfaces] : m_surfaceMap)
	{
		for (const auto& surface : surfaces)
		{
			if (surface.side == WallSide::NONE)
				continue;
			const NodeID nodeID = surface.parentNode;
			const TestChamberNode& node = m_nodeMap.at(nodeID);
			out.push_back(generateQuadAt(node.getCenter().toQVector3D(), surface.side, node.getSize()));
		}
	}
}

void TestChamber::generateTCQuads(std::vector<TestChamberQuad>& out) const
{
	if (m_surfaceMap.size() == 0)
		throw std::runtime_error("Attempted to generate quads for test chamber with no surface");
	for (const auto& [surfaceID, surfaces] : m_surfaceMap)
	{
		int sideID = 0;
		for (const auto& surface : surfaces)
		{
			if (surface.side == WallSide::NONE)
				continue;
			// TODO: surfaceID and nodeID are the exact same value. why are they being passed in?
			// std::cout << surfaceID << ", " << nodeID << "\n";
			const NodeID nodeID = surface.parentNode;
			const TestChamberNode& node = m_nodeMap.at(nodeID);
			out.push_back(generateTCQuadAt(node.getCenter().toQVector3D(), surface.side, node.getSize(), sideID, nodeID));
			sideID++;
		}
	}
}

// Same as above except only generate quads for shadow-casting surfaces
void TestChamber::findHighestActiveZ(NodeID nodeID, int& highestZ) const
{
	const TestChamberNode& node = m_nodeMap.at(nodeID);
	// nothing in this subtree can be higher than what was already found
	if (node.getCenter().z() + (int)node.getSize() / 2 <= highestZ)
		return;

	if (!node.hasChildren())
	{
		// parent nodes are always marked active, so only leaf nodes count
		if (node.getActive())
			highestZ = node.getCenter().z() + (int)node.getSize() / 2;
		return;
	}
	// visit the upper children (4-7) first so the lower ones can be skipped
	const std::array<NodeID, 8>& children = node.readChildrenID();
	for (int i : {4, 5, 6, 7, 0, 1, 2, 3})
		findHighestActiveZ(children[i], highestZ);
}

void TestChamber::generateTCShadowQuads(std::vector<TestChamberQuad>& out) const
{
	if (m_surfaceMap.size() == 0)
		throw std::runtime_error("Attempted to generate quads for test chamber with no surface");

	// find the top of the open space
	int chamberTop = std::numeric_limits<int>::min();
	findHighestActiveZ(m_rootNode, chamberTop);
	if (chamberTop == std::numeric_limits<int>::min())
		chamberTop = (int)m_rootNodeWorldSize / 2;

	for (const auto& [surfaceID, surfaces] : m_surfaceMap)
	{
		int sideID = 0;
		for (const auto& surface : surfaces)
		{
			if (surface.side == WallSide::NONE)
				continue;
			const NodeID nodeID = surface.parentNode;
			const TestChamberNode& node = m_nodeMap.at(nodeID);
			if (surface.side == WallSide::POS_Z)
			{
				if (node.getCenter().z() + (int)node.getSize() / 2 > chamberTop - gridSizeMin)
					continue;
			}
			out.push_back(generateTCQuadAt(node.getCenter().toQVector3D(), surface.side, node.getSize(), sideID, nodeID));
			sideID++;
		}
	}
	// generate extra quads to catch shadows
	std::vector<NodeID> ceilingNodes;
	QVector3D chamberMin =
	    QVector3D(-(int)m_rootNodeWorldSize / 2, -(int)m_rootNodeWorldSize / 2, chamberTop - gridSizeMin);
	QVector3D chamberMax = QVector3D(m_rootNodeWorldSize / 2, m_rootNodeWorldSize / 2, chamberTop);
	getNodesTouchingBBox(chamberMin, chamberMax, ceilingNodes);
	for (const auto& nodeID : ceilingNodes)
	{
		const TestChamberNode& node = m_nodeMap.at(nodeID);
		if (!node.getActive())
		{
			QVector3D capCenter = node.getCenter().toQVector3D();
			capCenter.setZ(chamberTop - node.getSize() / 2.0f);
			out.push_back(generateTCQuadAt(capCenter, WallSide::POS_Z, node.getSize(), nodeID, nodeID));
		}
	}
	// walls to catch light bleeding from side walls
	const QVector3D& rootCenter = m_nodeMap.at(m_rootNode).getCenter().toQVector3D();
	out.push_back(generateTCQuadAt(rootCenter, WallSide::POS_X, m_rootNodeWorldSize + 25, m_rootNode, m_rootNode, 1.1f));
	out.push_back(generateTCQuadAt(rootCenter, WallSide::POS_Y, m_rootNodeWorldSize + 25, m_rootNode, m_rootNode, 1.1f));
	out.push_back(generateTCQuadAt(rootCenter, WallSide::NEG_X, m_rootNodeWorldSize + 25, m_rootNode, m_rootNode, 1.1f));
	out.push_back(generateTCQuadAt(rootCenter, WallSide::NEG_Y, m_rootNodeWorldSize + 25, m_rootNode, m_rootNode, 1.1f));
}
} // namespace PMP