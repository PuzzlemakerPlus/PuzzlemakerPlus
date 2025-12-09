#include "test_chamber.h"
#include "test_chamber.h"
#include "util/debug.h"
#include "util/scoped_timer.h"
#include <iostream>
#include <cassert>
// source file for simple test chamber operations with no side effects
// please keep all functions in this source file const or functions that dont change values inside the test chamber class
namespace PMP
{
	//let's try encoding directional information on an integer
	// 
	//1 bit per direction: 
	// POS_X,
	// NEG_X,
	// POS_Y,
	// NEG_Y,
	// POS_Z,
	// NEG_Z,
	// and 1 bit for indicating whether this node is "complete"
	// 
	// if a direction flag is set, that means this node has a full surface on that side
	// if a direction flag is 0, it could mean that the node has no surface on that side, or has a partial surface on that side
	// define 'complete node' as a node that has all 6 sides with each side either fully covered or fully uncovered
	//    basically, a 'complete node' can be unsubdivided
	// the 7th bit indicates whether this node is "complete" or not
	uint8_t TestChamber::nodeSurfaceFlag(NodeID node) const
	{
		uint8_t flags = 64; // start with 7th bit set to 1 (complete)
		const TestChamberNode& currentNode = m_nodeMap.at(node);
		if (currentNode.m_childIDs.has_value())
		{
			const auto& childIDs = currentNode.m_childIDs.valueRead();
			std::array<uint8_t, 8> childFlags;
			int enabledChildren = 0;
			for (int i = 0; i < 8; i++)
			{
				childFlags[i] = nodeSurfaceFlag(childIDs[i]);
				if (isNodeActive(childIDs[i])) enabledChildren++;
			}
			for (int i = 0; i < 6; i++)
			{
				WallSide side = static_cast<WallSide>(i);
				int countSide = 0;
				for(int j : childrenOnFace[i])
				{
					countSide += (int) readFlag(childFlags[j], i);
					setFlag(flags, 6, readFlag(childFlags[j], 6) && readFlag(flags, 6));
				}
				setFlag(flags, i, countSide == 4);
				// set complete flag if all 4 children have the same state on this side
				// if any side is incomplete, the node is incomplete 
				// either all enabled or all diabled
				setFlag(flags, 6, countSide % 4 == 0 && enabledChildren % 8 == 0 && readFlag(flags, 6));
				
			}
		}
		else {
			for (int i = 0; i < 6; i++)
			{
				WallSide side = static_cast<WallSide>(i);
				if (isSurface(side, node)) {
					setFlag(flags, i, true);
				}
				//Debugger::instance().addDebugPoint(m_nodeMap.at(node).m_center.toQVector3D(), { 0.0f,0.0f,1.0f });
			}
		}
		return flags;
	}
	bool TestChamber::canUnsubdivide(NodeID node) const
	{	
		return readFlag(nodeSurfaceFlag(node), 6);
	}
	bool TestChamber::areNeighborsSameSize(NodeID node) const {
		const TestChamberNode& currentNode = m_nodeMap.at(node);
		std::array<IntVector3D, 6> neighborCoords = getNeighborNodeCenters(currentNode.m_size, currentNode.m_center);
		bool success = true;
		for (int i = 0; i < 6; ++i)
		{
			if (isPointOOB(neighborCoords[i].toQVector3D())) continue;
			auto r = tryGetNodeAtCoord(neighborCoords[i].toQVector3D());
			if (r == std::nullopt) {
				success = false; // Neighbor does not exist
			}
		}
		return success;
	}
	bool TestChamber::areNeighborsActive(NodeID node) const {
		const TestChamberNode& currentNode = m_nodeMap.at(node);
		std::array<IntVector3D, 6> neighborCoords = getNeighborNodeCenters(currentNode.m_size, currentNode.m_center);
		bool success = true;
		for (int i = 0; i < 6; ++i)
		{
			if (isPointOOB(neighborCoords[i].toQVector3D())) continue;
			auto r = tryGetNodeAtCoord(neighborCoords[i]);
			if (r != std::nullopt) {
				if (m_nodeMap.at(r.value()).m_active) {
					//Debugger::instance().addDebugPoint(m_nodeMap.at(r.value()).m_center.toQVector3D(), {0.0f,1.0f,0.0f});
				}
				else {
					//Debugger::instance().addDebugPoint(m_nodeMap.at(r.value()).m_center.toQVector3D(), { 1.0f,0.0f,0.0f });
					success = false;
				}
			}
		}
		return success;
	}

	int snap(int x, int s) {
		assert(s > 0 && "s must be positive");
		int half = s / 2;
		int sign = (int)(x >= 0) * 2 - 1;
		return ((x + half * sign) / s) * s;
	}

	// level == 0 at root node
	IntVector3D TestChamber::getClosestPossibleNodeCenterAtLevel(IntVector3D coord, unsigned int scale) const
	{
		IntVector3D rel = coord + IntVector3D(scale * 0.5f, scale * 0.5f, scale * 0.5f); //shift to positive space
		return IntVector3D(snap(rel.x(), scale) - scale * 0.5f, snap(rel.y(), scale) - scale * 0.5f, snap(rel.z(), scale) - scale * 0.5f);
	}

	NodeID TestChamber::getNodeOfSizeAtCoord(QVector3D coord, unsigned int size) const {
		return getNodeOfSizeAtCoord(coord, m_rootNode, size);
	}

	NodeID TestChamber::getNodeOfSizeAtCoord(QVector3D coord, NodeID parentID, unsigned int size) const {
		if (isPointOOB(coord)) return parentID;
		if (m_nodeMap.at(parentID).m_size == size) return parentID; //found
		if (!m_nodeMap.at(parentID).m_childIDs.has_value()) return parentID; //no more children to search. return this node

		// This should never fire because the search starts from the root node
		// The only way this can be fired is if the requested size is larger than the root node size
		if (m_nodeMap.at(parentID).m_size < size) throw std::runtime_error("Something went wrong trying to find node of specific size at the given coord. Did you try to find node larger than the root node?");
		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		NodeID id = m_nodeMap.at(parentID).m_childIDs.valueRead().at(i);
		return getNodeOfSizeAtCoord(coord, id, size);
	}

	const NodeID TestChamber::searchNodeWithCoord(QVector3D coord) const
	{
		return searchNodeWithCoord(coord, m_rootNode);
	}

	const NodeID TestChamber::searchNodeWithCoord(QVector3D coord, NodeID parentID) const
	{
		if (isPointOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if (!m_nodeMap.at(parentID).m_childIDs.has_value()) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		return searchNodeWithCoord(coord, m_nodeMap.at(parentID).m_childIDs.valueRead().at(i));
	}

	const NodeID TestChamber::searchNodeWithCoordSize(const QVector3D coord, unsigned int size) const
	{
		return searchNodeWithCoordSize(coord, size, m_rootNode);
	}
	const NodeID TestChamber::searchNodeWithCoordSize(const QVector3D coord, unsigned int size, NodeID parentID) const
	{
		if (isPointOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if (m_nodeMap.at(parentID).m_size == size) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		return searchNodeWithCoordSize(coord, size, m_nodeMap.at(parentID).m_childIDs.valueRead().at(i));
	}
	const NodeID TestChamber::searchNodeWithCoordSize(const IntVector3D coord, unsigned int size) const
	{
		return searchNodeWithCoordSize(coord, size, m_rootNode);
	}
	const NodeID TestChamber::searchNodeWithCoordSize(const IntVector3D coord, unsigned int size, NodeID parentID) const
	{
		if (isPointOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if (m_nodeMap.at(parentID).m_size == size) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		return searchNodeWithCoordSize(coord, size, m_nodeMap.at(parentID).m_childIDs.valueRead().at(i));
	}

	IntVector3D TestChamber::getNodeBBoxMin(NodeID id) const
	{
		const TestChamberNode& node = m_nodeMap.at(id);
		assert(node.m_size % 2 == 0 && "How is node size not divisible by 2?");
		int half = node.m_size / 2;
		return IntVector3D(node.m_center.x() - half,
			node.m_center.y() - half,
			node.m_center.z() - half);
	}
	IntVector3D TestChamber::getNodeBBoxMax(NodeID id) const
	{
		const TestChamberNode& node = m_nodeMap.at(id);
		assert(node.m_size % 2 == 0 && "How is node size not divisible by 2?");
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
		//Debugger::instance().clearDebugPoints();
		for (const auto& [_, surfaces] : m_surfaceMap)
		{
			for (const auto& surface : surfaces)
			{
				if (surface.side == WallSide::NONE) continue;
				if (!WallFacesDirection(surface.side, ray.direction)) continue;
				const TestChamberNode& node = m_nodeMap.at(surface.parentNode);

				// Wall geometry
				float halfSize = node.m_size / 2.0f;
				QVector3D up = getWallUp(surface.side).toQVector3D();
				QVector3D gNormal = getWallNormal(surface.side).toQVector3D();
				QVector3D right = QVector3D::crossProduct(gNormal, up);
				QVector3D center = getWallFaceCenter(node.m_center, node.m_size, gNormal).toQVector3D();

				QVector3D p0 = center + (-right - up) * halfSize;
				QVector3D p1 = center + (right - up) * halfSize;
				QVector3D p2 = center + (right + up) * halfSize;
				QVector3D p3 = center + (-right + up) * halfSize;

				float tTemp;
				QVector3D hitPosTemp;

				if (rayIntersectsQuad(ray, p0, p1, p2, p3, tTemp, hitPosTemp) && tTemp < closestT && tTemp > 0.0f)
				{
					closestT = tTemp;
					closestHit = RayHitResult{ surface.parentNode, hitPosTemp, surface };
					//Debugger::instance().addDebugPoint(hitPosTemp);
				}
			}
		}
		//Debugger::instance().drawDebugPoints();
		return closestHit;
	}

	//Given a wallside of a node, should this wall be drawn
	bool TestChamber::isSurface(WallSide side, NodeID node) const
	{
		const TestChamberNode& ref = m_nodeMap.at(node);
		const IntVector3D center = ref.m_center;
		assert(ref.m_size % 2 == 0 && "node size should be an even numer");
		const int halfSize = ref.m_size / 2;
		IntVector3D norm = getWallNormal(side);
		IntVector3D offset = -(norm * halfSize + norm);
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

		// simple 64-bit scrambler
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

	IntVector3D TestChamber::getSurfaceCenter(ChamberSurface surface) const
	{
		return m_nodeMap.at(surface.parentNode).m_size * 0.5f * getWallNormal(surface.side) * -1 + m_nodeMap.at(surface.parentNode).m_center;
	}

	void TestChamber::makeUSI(NodeID nodeID, UniqueSurfaceIdentifier& out, WallSide side) const
	{
		const TestChamberNode& node = m_nodeMap.at(nodeID);
		out.size = node.m_size;
		out.nodeCord = node.m_center;
		out.side = side;
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
			std::cout << (isLast ? "������ " : "������ ");
		}
#endif
		std::cout << "Node " << id << " at " << printVector3D(m_nodeMap.at(id).m_center.toQVector3D()) << " with size " << m_nodeMap.at(id).m_size << "\n";

		const auto& childVec = children[id];
		for (size_t i = 0; i < childVec.size(); ++i) {
			bool last = (i == childVec.size() - 1);
#ifdef WIN32
			_printNodesTree(children, childVec[i], prefix + (isLast ? "    " : "|   "), last);
#else
			_printNodesTree(children, childVec[i], prefix + (isLast ? "    " : "��   "), last);
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