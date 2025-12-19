#include "test_chamber.h"
#include "test_chamber.h"
#include "util/debug.h"
#include "util/scoped_timer.h"
#include <iostream>
#include <cassert>
#include <array>
#include <future>
#include <cstdint>
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
		if (currentNode.hasChildren())
		{
			const auto& childIDs = currentNode.readChildrenID();
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
	uint8_t TestChamber::nodeSurfaceFlagThreaded(NodeID node) const
	{
		uint8_t flags = 64; // start with 7th bit set to 1 (complete)
		const TestChamberNode& currentNode = m_nodeMap.at(node);
		if (currentNode.hasChildren())
		{
			const auto& childIDs = currentNode.readChildrenID();
			std::array<uint8_t, 8> childFlags;
			std::array<std::future<uint8_t>, 8> futures;
			int enabledChildren = 0;
			for (int i = 0; i < 8; i++)
			{
				const NodeID child = childIDs[i];
				futures[i] = std::async(std::launch::async, [this, child]() -> uint8_t {
					return this->nodeSurfaceFlag(child);
					});
			}
			for (int i = 0; i < 8; ++i)
			{
				childFlags[i] = futures[i].get();
				if (isNodeActive(childIDs[i])) enabledChildren++;
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
		else {
			for (int i = 0; i < 6; i++)
			{
				WallSide side = static_cast<WallSide>(i);
				if (isSurface(side, node)) {
					setFlag(flags, i, true);
				}
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
		std::array<IntVector3D, 6> neighborCoords = getNeighborNodeCenters(currentNode.getSize(), currentNode.getCenter());
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
		std::array<IntVector3D, 6> neighborCoords = getNeighborNodeCenters(currentNode.getSize(), currentNode.getCenter());
		bool success = true;
		for (int i = 0; i < 6; ++i)
		{
			if (isPointOOB(neighborCoords[i].toQVector3D())) continue;
			auto r = tryGetNodeAtCoord(neighborCoords[i]);
			if (r != std::nullopt) {
				if (!m_nodeMap.at(r.value()).getActive()) {
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
	
	void TestChamber::setNeighbors(NodeID node) {
		auto center = m_nodeMap.at(node).getCenter();
		auto size = m_nodeMap.at(node).getSize();
		auto neighborCenters = getNeighborNodeCenters(size, center);
		auto& neighborArray = m_nodeMap.at(node).getNeighbors();
		for (int i = 0; i < 6; i++)
		{
			auto n = tryGetNodeAtCoord(neighborCenters[i]);
			if (n.has_value())
			{
				neighborArray[i] = n.value(); // set the value on this node
				m_nodeMap.at(n.value()).getNeighbors()[static_cast<int>(getOpposite(static_cast<WallSide>(i)))] = node; // set the value on the neighbor
			}
		}
	}

	// level == 0 at root node
	IntVector3D TestChamber::getClosestPossibleNodeCenterAtLevel(IntVector3D coord, unsigned int scale) const
	{
		IntVector3D rel = coord + IntVector3D(scale / 2, scale / 2, scale / 2); //shift to positive space
		return IntVector3D(snap(rel.x(), scale) - scale / 2, snap(rel.y(), scale) - scale / 2, snap(rel.z(), scale) - scale / 2);
	}

	NodeID TestChamber::getNodeOfSizeAtCoord(QVector3D coord, unsigned int size) const {
		return getNodeOfSizeAtCoord(coord, m_rootNode, size);
	}

	NodeID TestChamber::getNodeOfSizeAtCoord(QVector3D coord, NodeID parentID, unsigned int size) const {
		if (isPointOOB(coord)) return parentID;
		if (m_nodeMap.at(parentID).getSize() == size) return parentID; //found
		if (!m_nodeMap.at(parentID).hasChildren()) return parentID; //no more children to search. return this node

		// This should never fire because the search starts from the root node
		// The only way this can be fired is if the requested size is larger than the root node size
		if (m_nodeMap.at(parentID).getSize() < size) throw std::runtime_error("Something went wrong trying to find node of specific size at the given coord. Did you try to find node larger than the root node?");
		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).getCenter());
		NodeID id = m_nodeMap.at(parentID).readChildrenID().at(i);
		return getNodeOfSizeAtCoord(coord, id, size);
	}

	const NodeID TestChamber::searchNodeWithCoord(QVector3D coord) const
	{
		return searchNodeWithCoord(coord, m_rootNode);
	}

	const NodeID TestChamber::searchNodeWithCoord(QVector3D coord, NodeID parentID) const
	{
		if (isPointOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if (!m_nodeMap.at(parentID).hasChildren()) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).getCenter());
		return searchNodeWithCoord(coord, m_nodeMap.at(parentID).readChildrenID().at(i));
	}

	//const NodeID TestChamber::searchNodeWithCoord

	const NodeID TestChamber::searchNodeWithCoordSize(const QVector3D coord, unsigned int size) const
	{
		return searchNodeWithCoordSize(coord, size, m_rootNode);
	}
	const NodeID TestChamber::searchNodeWithCoordSize(const QVector3D coord, unsigned int size, NodeID parentID) const
	{
		if (isPointOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if (m_nodeMap.at(parentID).getSize() == size) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).getCenter());
		return searchNodeWithCoordSize(coord, size, m_nodeMap.at(parentID).readChildrenID().at(i));
	}
	const NodeID TestChamber::searchNodeWithCoordSize(const IntVector3D coord, unsigned int size) const
	{
		return searchNodeWithCoordSize(coord, size, m_rootNode);
	}
	const NodeID TestChamber::searchNodeWithCoordSize(const IntVector3D coord, unsigned int size, NodeID parentID) const
	{
		if (isPointOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if (m_nodeMap.at(parentID).getSize() == size) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).getCenter());
		return searchNodeWithCoordSize(coord, size, m_nodeMap.at(parentID).readChildrenID().at(i));
	}

	IntVector3D TestChamber::getNodeBBoxMin(NodeID id) const
	{
		const TestChamberNode& node = m_nodeMap.at(id);
		assert(node.getSize() % 2 == 0 && "How is node size not divisible by 2?");
		int half = node.getSize() / 2;
		auto c = node.getCenter();
		return IntVector3D(c.x() - half, c.y() - half,c. z() - half);
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
		//Debugger::instance().clearDebugPoints();
		for (const auto& [_, surfaces] : m_surfaceMap)
		{
			for (const auto& surface : surfaces)
			{
				if (surface.side == WallSide::NONE) continue;
				if (!WallFacesDirection(surface.side, ray.direction)) continue;
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
		const IntVector3D center = ref.getCenter();
		assert(ref.getSize() % 2 == 0 && "node size should be an even numer");
		const int halfSize = ref.getSize() / 2;
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
		float halfSize = node.getSize() / 2.0f;
		auto c = node.getCenter();
		return (coord.x() >= c.x() - halfSize && coord.x() <= c.x() + halfSize) &&
			(coord.y() >= c.y() - halfSize && coord.y() <= c.y() + halfSize) &&
			(coord.z() >= c.z() - halfSize && coord.z() <= c.z() + halfSize);
	}

	std::optional<ChamberSurface> TestChamber::getSurface(NodeID node, WallSide side) const
	{
		ChamberSurface surf = m_surfaceMap.at(node)[static_cast<int>(side)];
		if (surf.side == WallSide::NONE) return std::nullopt;
		return std::optional<ChamberSurface>(surf);
	}

	NodeID TestChamber::positionToNodeID(IntVector3D pos) const
	{
		// is this better than bitpacking?
		int h = m_rootNodeWorldSize;
		int D = 2 * h + 1;
		return ((uint64_t)pos.x() + h) + ((uint64_t)pos.y() + h) * D + ((uint64_t)pos.z() + h) * D * D;
		//static int i = 0;
		//return i++;
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
		return m_nodeMap.at(surface.parentNode).getSize() * 0.5f * getWallNormal(surface.side) * -1 + m_nodeMap.at(surface.parentNode).getCenter();
	}

	void TestChamber::makeUSI(NodeID nodeID, UniqueSurfaceIdentifier& out, WallSide side) const
	{
		const TestChamberNode& node = m_nodeMap.at(nodeID);
		out.size = node.getSize();
		out.nodeCord = node.getCenter();
		out.side = side;
	}

	void TestChamber::printNodes()
	{
		std::cout << "TestChamber has " << m_nodeMap.size() << " Node(s)\n";
		for (const auto& [nodeId, node] : m_nodeMap) {
			if (node.hasChildren()) std::cout << "NodeID: " << nodeId << ", child of " << node.getParentID() << "\n";
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
		std::cout << "Node " << id << " at " << printVector3D(m_nodeMap.at(id).getCenter().toQVector3D()) << " with size " << m_nodeMap.at(id).getSize() << "\n";

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
			if (node.hasParentID()) {
				children[node.getParentID()].push_back(id);
			}
			else {
				root = id;  // fallback assumption
			}
		}

		_printNodesTree(children, root, "", true);
	}
}