// test_chamber_utils.cpp
// simple test chamber operations with no side effects
#include "test_chamber.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <future>
#include <iostream>

#include "util/debug.h"
#include "util/scoped_timer.h"

// source file for simple test chamber operations with no side effects
// please keep all functions in this source file const or functions that dont change values inside the test chamber
// class
namespace PMP
{
// let's try encoding directional information on an integer
//
// 1 bit per direction:
//  POS_X,
//  NEG_X,
//  POS_Y,
//  NEG_Y,
//  POS_Z,
//  NEG_Z,
//  and 1 bit for indicating whether this node is "complete"
//
//  if a direction flag is set, that means this node has a full surface on that side
//  if a direction flag is 0, it could mean that the node has no surface on that side, or has a partial surface on that
//  side define 'complete node' as a node that has all 6 sides with each side either fully covered or fully uncovered
//     basically, a 'complete node' can be unsubdivided
//  the 7th bit indicates whether this node is "complete" or not
uint8_t TestChamber::nodeSurfaceFlag(NodeID node) const
{
	uint8_t flags = 64; // start with 7th bit set to 1 (complete)
	const TestChamberNode& currentNode = m_nodeMap.at(node);
	if (currentNode.hasChildren())
	{
		const auto& childIDs = currentNode.readChildrenID();
		std::array<uint8_t, 8> childFlags;
		int enabledChildren = 0;
		for (int i = 0; i < 8; i++)
		{
			childFlags[i] = nodeSurfaceFlag(childIDs[i]);
			if (isNodeActive(childIDs[i]))
				enabledChildren++;
		}
		for (int i = 0; i < 6; i++)
		{
			WallSide side = static_cast<WallSide>(i);
			int countSide = 0;
			for (int j : childrenOnFace[i])
			{
				countSide += (int)readFlag(childFlags[j], i);
				setFlag(flags, 6, readFlag(childFlags[j], 6) && readFlag(flags, 6));
			}
			setFlag(flags, i, countSide == 4);
			// set complete flag if all 4 children have the same state on this side
			// if any side is incomplete, the node is incomplete
			// either all enabled or all diabled
			setFlag(flags, 6, countSide % 4 == 0 && enabledChildren % 8 == 0 && readFlag(flags, 6));
		}
	}
	else
	{
		for (int i = 0; i < 6; i++)
		{
			WallSide side = static_cast<WallSide>(i);
			if (isSurface(side, currentNode))
			{
				setFlag(flags, i, true);
			}
			// Debugger::instance().addDebugPoint(m_nodeMap.at(node).m_center.toQVector3D(), { 0.0f,0.0f,1.0f });
		}
	}
	return flags;
}

bool TestChamber::canUnsubdivide(NodeID node) const
{
	return readFlag(nodeSurfaceFlag(node), 6);
}

int snap(int x, int s)
{
	assert(s > 0 && "s must be positive");
	int half = s / 2;
	int sign = (int)(x >= 0) * 2 - 1;
	return ((x + half * sign) / s) * s;
}

// level == 0 at root node
IntVector3D TestChamber::getClosestPossibleNodeCenterAtLevel(IntVector3D coord, unsigned int scale) const
{
	IntVector3D rel = coord + IntVector3D(scale / 2, scale / 2, scale / 2); // shift to positive space
	return IntVector3D(snap(rel.x(), scale) - scale / 2, snap(rel.y(), scale) - scale / 2,
	                   snap(rel.z(), scale) - scale / 2);
}

const NodeID TestChamber::searchNodeWithCoord(QVector3D coord) const
{
	return searchNodeWithCoord(coord, m_rootNode);
}

const NodeID TestChamber::searchNodeWithCoord(QVector3D coord, NodeID parentID) const
{
	if (isPointOOB(coord))
		throw std::runtime_error("Coordinate Out Of Bounds");
	if (!m_nodeMap.at(parentID).hasChildren())
		return parentID; // found

	unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).getCenter());
	return searchNodeWithCoord(coord, m_nodeMap.at(parentID).readChildrenID().at(i));
}

IntVector3D TestChamber::getNodeBBoxMin(NodeID id) const
{
	const TestChamberNode& node = m_nodeMap.at(id);
	assert(node.getSize() % 2 == 0 && "How is node size not divisible by 2?");
	int half = node.getSize() / 2;
	auto c = node.getCenter();
	return IntVector3D(c.x() - half, c.y() - half, c.z() - half);
}

IntVector3D TestChamber::getNodeBBoxMax(NodeID id) const
{
	const TestChamberNode& node = m_nodeMap.at(id);
	assert(node.getSize() % 2 == 0 && "How is node size not divisible by 2?");
	int half = node.getSize() / 2;
	auto c = node.getCenter();
	return IntVector3D(c.x() + half, c.y() + half, c.z() + half);
}

// Raycast function to find the closest hit on a surface
std::optional<RayHitResult> TestChamber::rayCast(Ray ray) const
{
	float closestT = std::numeric_limits<float>::infinity();
	std::optional<RayHitResult> closestHit;
	// Debugger::instance().clearDebugPoints();
	for (const auto& [_, surfaces] : m_surfaceMap)
	{
		for (const auto& surface : surfaces)
		{
			if (surface.side == WallSide::NONE)
				continue;
			if (!WallFacesDirection(surface.side, ray.direction))
				continue;
			const TestChamberNode& node = m_nodeMap.at(surface.parentNode);

			// Wall geometry
			float halfSize = node.getSize() / 2.0f;
			QVector3D up = getWallUp(surface.side).toQVector3D();
			QVector3D m_gNormal = getWallNormal(surface.side).toQVector3D();
			QVector3D right = QVector3D::crossProduct(m_gNormal, up);
			QVector3D center = getWallFaceCenter(node.getCenter(), node.getSize(), m_gNormal).toQVector3D();

			QVector3D p0 = center + (-right - up) * halfSize;
			QVector3D p1 = center + (right - up) * halfSize;
			QVector3D p2 = center + (right + up) * halfSize;
			QVector3D p3 = center + (-right + up) * halfSize;

			float tTemp;
			QVector3D hitPosTemp;

			if (rayIntersectsQuad(ray, p0, p1, p2, p3, tTemp, hitPosTemp) && tTemp < closestT && tTemp > 0.0f)
			{
				closestT = tTemp;
				closestHit = RayHitResult{surface.parentNode, hitPosTemp, surface};
				// Debugger::instance().addDebugPoint(hitPosTemp);
			}
		}
	}
	// Debugger::instance().drawDebugPoints();
	return closestHit;
}

// Given a wallside of a node, should this wall be drawn
bool TestChamber::isSurface(WallSide side, const TestChamberNode& ref) const
{
	const IntVector3D center = ref.getCenter();
	assert(ref.getSize() % 2 == 0 && "node size should be an even numer");
	const int halfSize = ref.getSize() / 2;
	IntVector3D norm = getWallNormal(side);
	IntVector3D offset = -(norm * halfSize + norm);
	IntVector3D neighborCoord = center + offset;

	if (isPointOOB(neighborCoord))
		return true;                                                     // nothing behind this surface, so it should be drawn
	return !isNodeActive(findLeafNear(neighborCoord, ref.getSize())); // drawn if the node behind it is inactive
}

//hash lookup
NodeID TestChamber::findLeafNear(IntVector3D coord, unsigned int size) const
{
	//try to find for all grid sizes
	for (unsigned int s = size; s <= m_rootNodeWorldSize; s *= 2)
	{
		std::optional<NodeID> id = tryGetNodeAtCoord(getClosestPossibleNodeCenterAtLevel(coord, s));
		if (id.has_value())
			return nodeHasChild(*id) ? searchNodeWithCoord(coord.toQVector3D(), *id) : *id;
	}
	// fallback. realistically shouldnt fire unless there is an 'offgrid' node
	// this fallback I think will catch offgrid nodes as long as it is properly part of tree
	// can a node be offgrid and also be part of the tree?
	return searchNodeWithCoord(coord.toQVector3D());
}

std::optional<ChamberSurface> TestChamber::getSurface(NodeID node, WallSide side) const
{
	ChamberSurface surf = m_surfaceMap.at(node)[static_cast<int>(side)];
	if (surf.side == WallSide::NONE)
		return std::nullopt;
	return std::optional<ChamberSurface>(surf);
}

NodeID TestChamber::positionToNodeID(IntVector3D pos) const
{
	// is this better than bitpacking?
	int h = m_rootNodeWorldSize;
	int D = 2 * h + 1;
	return ((uint64_t)pos.x() + h) + ((uint64_t)pos.y() + h) * D + ((uint64_t)pos.z() + h) * D * D;
	// static int i = 0;
	// return i++;
}

bool TestChamber::isPointOOB(QVector3D coord) const
{
	assert(m_rootNodeWorldSize % 2 == 0);
	int halfSize = m_rootNodeWorldSize / 2;
	return std::abs(coord.x()) > halfSize || std::abs(coord.y()) > halfSize || std::abs(coord.z()) > halfSize;
}

bool TestChamber::isPointOOB(IntVector3D coord) const
{
	assert(m_rootNodeWorldSize % 2 == 0);
	int halfSize = m_rootNodeWorldSize / 2;
	return std::abs(coord.x()) > halfSize || std::abs(coord.y()) > halfSize || std::abs(coord.z()) > halfSize;
}

IntVector3D TestChamber::getSurfaceCenter(ChamberSurface surface) const
{
	return m_nodeMap.at(surface.parentNode).getSize() * 0.5f * getWallNormal(surface.side) * -1 +
	       m_nodeMap.at(surface.parentNode).getCenter();
}

void TestChamber::printNodes()
{
	std::cout << "TestChamber has " << m_nodeMap.size() << " Node(s)\n";
	for (const auto& [nodeId, node] : m_nodeMap)
	{
		if (node.hasChildren())
			std::cout << "NodeID: " << nodeId << ", child of " << node.getParentID() << "\n";
		else
			std::cout << "NodeID: " << nodeId << " has no parentID" << "\n";
	}
}

const char* printVector3D(QVector3D in)
{
	static char buffer[64];
	snprintf(buffer, sizeof(buffer), "(%.2f, %.2f, %.2f)", in.x(), in.y(), in.z());
	return buffer;
}

void TestChamber::_printNodesTree(std::map<NodeID, std::vector<NodeID>>& children, NodeID id, std::string prefix,
                                  bool isLast)
{
	std::cout << prefix;
	if (!prefix.empty())
	{
		std::cout << (isLast ? "`-- " : "|-- ");
	}
	std::cout << "Node " << id << " at " << printVector3D(m_nodeMap.at(id).getCenter().toQVector3D()) << " with size "
	          << m_nodeMap.at(id).getSize() << "\n";

	const auto& childVec = children[id];
	for (size_t i = 0; i < childVec.size(); ++i)
	{
		bool last = (i == childVec.size() - 1);
		_printNodesTree(children, childVec[i], prefix + (isLast ? "    " : "|   "), last);
	}
}

void TestChamber::printNodesTree()
{
	std::map<NodeID, std::vector<NodeID>> children;
	NodeID root = 0;

	for (const auto& [id, node] : m_nodeMap)
	{
		if (node.hasParentID())
		{
			children[node.getParentID()].push_back(id);
		}
		else
		{
			root = id; // fallback assumption
		}
	}

	_printNodesTree(children, root, "", true);
}
} // namespace PMP
