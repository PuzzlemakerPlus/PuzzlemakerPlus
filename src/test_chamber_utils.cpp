#include "test_chamber.h"
#include <iostream>
// source file for simple test chamber operations with no side effects
// please keep all functions in this source file const or functions that dont change values inside the test chamber class
namespace PMP
{
	IntVector3D TestChamber::getNodeBBoxMin(NodeID id) const
	{
		const TestChamberNode& node = m_nodeMap.at(id);
		assert(node.m_size % 2 == 0, "How is node size not divisible by 2?");
		int half = node.m_size / 2;
		return IntVector3D(node.m_center.x() - half,
			node.m_center.y() - half,
			node.m_center.z() - half);
	}
	IntVector3D TestChamber::getNodeBBoxMax(NodeID id) const
	{
		const TestChamberNode& node = m_nodeMap.at(id);
		assert(node.m_size % 2 == 0, "How is node size not divisible by 2?");
		int half = node.m_size / 2;
		return IntVector3D(node.m_center.x() + half,
			node.m_center.y() + half,
			node.m_center.z() + half);
	}
	// Raycast function to find the closest hit on a surface
	std::optional<RayHitResult> TestChamber::rayCast(Ray ray) const
	{
		float closestT = std::numeric_limits<float>::infinity();
		std::optional<RayHitResult> closestHit;

		for (const auto& [_, surfaces] : m_surfaceMap)
		{
			for (const auto& surface : surfaces)
			{
				if (surface.side == WallSide::NONE) continue;
				if (!WallFacesDirection(surface.side, ray.direction)) continue;
				const TestChamberNode& node = m_nodeMap.at(surface.parentNode);

				// Wall geometry
				float halfSize = node.m_size / 2.0f;
				IntVector3D up = getWallUp(surface.side);
				IntVector3D normal = getWallNormal(surface.side);
				IntVector3D right = normal.cross(up);
				IntVector3D center = getWallFaceCenter(node.m_center, node.m_size, normal);

				IntVector3D p0 = center + (-right - up) * halfSize;
				IntVector3D p1 = center + (right - up) * halfSize;
				IntVector3D p2 = center + (right + up) * halfSize;
				IntVector3D p3 = center + (-right + up) * halfSize;

				float tTemp;
				QVector3D hitPosTemp;

				if (rayIntersectsQuad(ray, p0.toQVector3D(), p1.toQVector3D(), p2.toQVector3D(), p3.toQVector3D(), tTemp, hitPosTemp) && tTemp > 0.0f && tTemp < closestT)
				{
					closestT = tTemp;
					closestHit = RayHitResult{ surface.parentNode, hitPosTemp, surface };
				}
			}
		}
		return closestHit;
	}

	//Given a wallside of a node, should this wall be drawn
	//optimize this function by using IntVector3D
	bool TestChamber::isSurface(WallSide side, NodeID node) const
	{
		const IntVector3D center = m_nodeMap.at(node).m_center;
		assert(m_nodeMap.at(node).m_size % 2 == 0, "node size should be an even numer");
		const int halfSize = m_nodeMap.at(node).m_size / 2;
		IntVector3D offset;
		using enum WallSide;
		switch (side) {
		case POS_X: offset = { +halfSize + 1, 0, 0 }; break;
		case NEG_X: offset = { -halfSize - 1, 0, 0 }; break;
		case POS_Y: offset = { 0, +halfSize + 1, 0 }; break;
		case NEG_Y: offset = { 0, -halfSize - 1, 0 }; break;
		case POS_Z: offset = { 0, 0, +halfSize + 1 }; break;
		case NEG_Z: offset = { 0, 0, -halfSize - 1 }; break;
		case NONE: throw std::runtime_error("Invalid WallSide");
		}

		IntVector3D neighborCoord = center + offset;

		bool isVisible = isPointOOB(neighborCoord); //if coordinate behind this surface is OOB, this surface should be drawn
		if (!isVisible) { // the coordinate behind this surface is in bound
			NodeID neighborId = searchNodeWithCoord(neighborCoord.toQVector3D());
			isVisible = !isNodeActive(neighborId); // check if the node behind this surface is inactive
		}
		return isVisible;
	}

	bool TestChamber::coordInNode(QVector3D coord, NodeID nodeID) const
	{
		const TestChamberNode& node = m_nodeMap.at(nodeID);
		float halfSize = node.m_size / 2.0f;

		return (coord.x() >= node.m_center.x() - halfSize && coord.x() <= node.m_center.x() + halfSize) &&
			(coord.y() >= node.m_center.y() - halfSize && coord.y() <= node.m_center.y() + halfSize) &&
			(coord.z() >= node.m_center.z() - halfSize && coord.z() <= node.m_center.z() + halfSize);
	}

	std::optional<ChamberSurface> TestChamber::getSurface(NodeID node, WallSide side) const
	{
		ChamberSurface surf = m_surfaceMap.at(node)[static_cast<int>(side)];
		if (surf.side == WallSide::NONE) return std::nullopt;
		return std::optional<ChamberSurface>(surf);
	}

	NodeID TestChamber::positionToNodeID(IntVector3D pos) const
	{
		// make sure all coordinates are positive
		IntVector3D displaced = pos + IntVector3D(m_rootNodeWorldSize, m_rootNodeWorldSize, m_rootNodeWorldSize);
		uint64_t x = static_cast<uint64_t>(std::llround(displaced.x()));
		uint64_t y = static_cast<uint64_t>(std::llround(displaced.y()));
		uint64_t z = static_cast<uint64_t>(std::llround(displaced.z()));

		//good known magic numbers for hashing
		uint64_t h = x * 73856093ULL ^ y * 19349663ULL ^ z * 83492791ULL;

		// simple 64-bit ※scrambler§
		h ^= h >> 33;
		h *= 0xff51afd7ed558ccdULL;
		h ^= h >> 33;
		h *= 0xc4ceb9fe1a85ec53ULL;
		h ^= h >> 33;
		return h;
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

	void TestChamber::printNodes()
	{
		std::cout << "TestChamber has " << m_nodeMap.size() << " Node(s)\n";
		for (const auto& [nodeId, node] : m_nodeMap) {
			if (node.m_parentID.has_value()) std::cout << "NodeID: " << nodeId << ", child of " << node.m_parentID.value() << "\n";
			else std::cout << "NodeID: " << nodeId << " has no parentID" << "\n";
		}
	}

	const char* printVector3D(QVector3D in)
	{
		static char buffer[64];
		snprintf(buffer, sizeof(buffer), "(%.2f, %.2f, %.2f)", in.x(), in.y(), in.z());
		return buffer;
	}

	void TestChamber::_printNodesTree(std::map<NodeID, std::vector<NodeID>>& children, NodeID id, std::string prefix, bool isLast)
	{
		std::cout << prefix;
#ifdef WIN32
		if (!prefix.empty()) {
			std::cout << (isLast ? "`-- " : "|-- ");
		}
#else
		if (!prefix.empty()) {
			std::cout << (isLast ? "戌式式 " : "戍式式 ");
		}
#endif
		std::cout << "Node " << id << " at " << printVector3D(m_nodeMap.at(id).m_center.toQVector3D()) << " with size " << m_nodeMap.at(id).m_size << "\n";

		const auto& childVec = children[id];
		for (size_t i = 0; i < childVec.size(); ++i) {
			bool last = (i == childVec.size() - 1);
#ifdef WIN32
			_printNodesTree(children, childVec[i], prefix + (isLast ? "    " : "|   "), last);
#else
			_printNodesTree(children, childVec[i], prefix + (isLast ? "    " : "弛   "), last);
#endif
		}
	}

	void TestChamber::printNodesTree() {
		std::map<NodeID, std::vector<NodeID>> children;
		NodeID root = 0;

		for (const auto& [id, node] : m_nodeMap) {
			if (node.m_parentID.has_value()) {
				children[*node.m_parentID].push_back(id);
			}
			else {
				root = id;  // fallback assumption
			}
		}

		_printNodesTree(children, root, "", true);
	}
}