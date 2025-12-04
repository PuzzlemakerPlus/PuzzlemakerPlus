// octree_test.cpp
// Implements the class that represents the test chamber itself and the octree nodes
#include "undo.h"
#include "selection.h"
#include "util/debug.h"
#include "util/3d/mesh.h" 
#include "test_chamber.h"
#include "test_chamber_types.h"
#include "test_chamber_actions.h"
#include "util/math.h"

#include <array>
#include <memory>
#include <qdebug.h>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <qvectornd.h>
#include <unordered_set>

namespace PMP
{
	TestChamber::TestChamber() :
		m_rootNodeWorldSize(defaultRootNodeSize),
		m_gridSize(gridSizeDefault)
	{
		// Create the root node
		// Root node is not 0
		m_rootNode = positionToNodeID({0,0,0}); // Warm up hash function
		createOctreeNode(m_rootNode, std::nullopt, {0,0,0}, m_rootNodeWorldSize);
	}

	// Given a coordinate, compare against the parent coordinate to find the order at which the child node is placed in the child array
	unsigned int getChildOrder(IntVector3D coord, IntVector3D parentCenter)
	{
		IntVector3D delta = coord - parentCenter;

		// Determine sign bits: 0 for negative, 1 for positive
		unsigned int xBit = (delta.x() >= 0) ? 1 : 0;
		unsigned int yBit = (delta.y() >= 0) ? 1 : 0;
		unsigned int zBit = (delta.z() >= 0) ? 1 : 0;

		// Bit pattern: xBit (LSB), yBit, zBit (MSB)
		return (zBit << 2) | (yBit << 1) | xBit;
	}

	IntVector3D getChildCenter(unsigned int order, unsigned int parentSize, IntVector3D parentCenter)
	{
		//std::cout << parentSize << std::endl;
		if (parentSize % 4 != 0) throw std::runtime_error("Parent node size must be divisible by 4");
		if (order > 7) throw std::runtime_error("Order can not be greater than 7");

		int offset = parentSize / 4;
		// Branchless calculation
		int xSign = ((order & 1) << 1) - 1;               // 1 if odd, -1 if even
		int ySign = (((order >> 1) & 1) << 1) - 1;        // 1 if bit 1 is 0, -1 if 1
		int zSign = (((order >> 2) & 1) << 1) - 1;        // 1 if bit 2 is 0, -1 if 1

		return parentCenter + IntVector3D(offset * xSign, offset * ySign, offset * zSign);
	}

	void TestChamber::createChildrenOfNode( NodeID parentID )
	{
		TestChamberNode& parentNode = m_nodeMap.at(parentID);
		std::array<NodeID, 8>& parentChildIds = parentNode.m_childIDs.value_initialize();
		
		// Child Node Generation Order
		//negx negy negz
		//posx negy negz
		//negx posy negz
		//posx posy negz
		//negx negy posz
		//posx negy posz
		//negx posy posz
		//posx posy posz
		for (unsigned int i = 0; i < 8; ++i )
		{
			IntVector3D pos = getChildCenter(i, parentNode.m_size, parentNode.m_center);
			NodeID id = positionToNodeID(pos);
			createOctreeNode(id, parentID, pos, parentNode.m_size/2);
			parentChildIds[i] = id;
		}
		parentNode.m_active = true; // Make sure the parent node is active after creating children
	}

	void TestChamber::destroyChildrenOfNode( NodeID parentID )
	{
		TestChamberNode& parentNode = m_nodeMap.at(parentID);
		if(parentNode.m_childIDs.has_value())
		{
			std::array<NodeID, 8>& parentChildIds = parentNode.m_childIDs.value();

			for ( int i = 0; i < 8; ++i )
			{
				NodeID childID = parentChildIds[i]; 
				TestChamberNode& childNode = m_nodeMap.at(childID);

				if ( childNode.m_childIDs.has_value() )
					destroyChildrenOfNode(childID);

				m_nodeMap.erase(childID);
			}

			parentNode.m_childIDs.reset();
		}
	}

	void TestChamber::createOctreeNode(NodeID newID, std::optional<NodeID> parentID, IntVector3D center, unsigned int size)
	{
		if ( parentID.has_value() )
			m_nodeMap.emplace(newID, TestChamberNode( parentID.value(), center, size, m_nodeMap.at(parentID.value()).m_active));
		else
			m_nodeMap.emplace(newID, TestChamberNode(center, size));
	}

	int TestChamber::setNodeActive(NodeID node, bool active)
	{
		if(m_nodeMap.at(node).m_active == false) return 0; // State of the node has not changed

		m_nodeMap.at(node).m_active = active;
		// Also need to make sure all adjacent nodes have matching size.
		// TODO: now that we have IntVector3D, this can be optimized further
		std::array<IntVector3D, 6> outerCoords = getNeighborNodeCenters(m_nodeMap.at(node).m_size, m_nodeMap.at(node).m_center);
		for(const auto& coord : outerCoords)
		{
			subdivideNodeToCoordOptimized(coord.toQVector3D(), 0, m_nodeMap.at(node).m_size);
		}
		return 1; // State of the node has changed
	}

	int TestChamber::setNodesActive(std::vector<NodeID>& nodes, bool active) 
	{
		int nodesChanged = 0;
		for ( int i = 0; i < nodes.size(); i++ )
		{
			m_nodeMap.at(nodes[i]).m_active = active;
			nodesChanged++;
		}
		// Match the sizes of adjacent nodes only after all nodes have had their active state set
		for ( int i = 0; i < nodes.size(); i++ )
		{
			std::array<IntVector3D, 6> outerCoords = getNeighborNodeCenters(m_nodeMap.at(nodes[i]).m_size, m_nodeMap.at(nodes[i]).m_center);
			for ( const auto& coord : outerCoords )
			{
				NodeID done = subdivideNodeToCoordOptimized( coord.toQVector3D(), m_rootNode, m_nodeMap.at(nodes[i]).m_size);
				//Debugger::instance().addDebugPoint(coord.toQVector3D(), { 1.0f, 1.0f, 0.0f } );
			}
		}
		return nodesChanged; // State of the node has changed
	}

	//Assumes out is empty during non-recursive calls
	void TestChamber::getSurfaceChildren(std::vector<ChamberSurface>& out, NodeID node, WallSide side)
	{
		// Node has children 
		if (m_nodeMap.at(node).m_childIDs.has_value())
		{
			// Iterate through all children and get their surfaces
			for (const auto& childID : m_nodeMap.at(node).m_childIDs.valueRead())
			{
				getSurfaceChildren(out, childID, side);
			}
		}
		else
		{
			// No children, so this node is the leaf node
			ChamberSurface surf = m_surfaceMap.at(node)[static_cast<int>(side)];
			if (surf.side != WallSide::NONE) out.push_back(surf);
		}
	}


	void TestChamber::getNeighborNodes(NodeID node, std::array<std::optional<NodeID>, 6>& out)
	{
		const TestChamberNode& currentNode = m_nodeMap.at(node);
		std::array<IntVector3D, 6> neighborCoords = getNeighborNodeCenters(currentNode.m_size, currentNode.m_center);

		for (int i = 0; i < 6; ++i)
		{
			try {
				NodeID neighborNodeID = getNodeOfSizeAtCoord(neighborCoords[i].toQVector3D(), currentNode.m_size);
				out[i] = neighborNodeID;
			} catch (const std::runtime_error&) {
				out[i] = std::nullopt; // Out of bounds
			}
		}
	}
	//TODO optimize this by using the fact that the coordinates are IntVector3D
	bool TestChamber::areNeighborsSameSize(NodeID node) {
		const TestChamberNode& currentNode = m_nodeMap.at(node);
		std::array<IntVector3D, 6> neighborCoords = getNeighborNodeCenters(currentNode.m_size, currentNode.m_center);
		bool success = true;
		for (int i = 0; i < 6; ++i)
		{
			if (isPointOOB(neighborCoords[i].toQVector3D())) continue;
			auto r = tryGetNodeAtCoord(neighborCoords[i].toQVector3D());
			if (r == std::nullopt) {
				success = false; // Neighbor does not exist
				//Debugger::instance().addDebugPoint(neighborCoords[i].toQVector3D(), { 1.0f,0.0f,0.0f });
			}
			//else Debugger::instance().addDebugPoint(m_nodeMap.at(r.value()).m_center.toQVector3D(), { 0.0f,1.0f,0.0f });
		}
		return success;
	}
	bool TestChamber::areNeighborsActive(NodeID node) {
		const TestChamberNode& currentNode = m_nodeMap.at(node);
		std::array<IntVector3D, 6> neighborCoords = getNeighborNodeCenters(currentNode.m_size, currentNode.m_center);
		bool success = true;
		for (int i = 0; i < 6; ++i)
		{
			if (isPointOOB(neighborCoords[i].toQVector3D())) continue;
			auto r = tryGetNodeAtCoord(neighborCoords[i]);
			if (r == std::nullopt) {
				
				continue;
			}
			else {
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

	std::optional<NodeID> TestChamber::tryGetNodeAtCoord(IntVector3D coord) const
	{
		NodeID hash = positionToNodeID(coord); 
		auto r = m_nodeMap.find(hash); // Check if node exists at this hash
		if(r == std::end(m_nodeMap)) return std::nullopt; // Node does not exist at this hash
		else {
			return r->first; // Node exists at this hash
		}
	}

	NodeID TestChamber::getLeafNodeAtCoord(QVector3D coord)
	{
		return getLeafNodeAtCoord(coord, m_rootNode);
	}
	
	NodeID TestChamber::getLeafNodeAtCoord(QVector3D coord, NodeID parentID)
	{
		if(isPointOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if(!m_nodeMap.at(parentID).m_childIDs.has_value()) return parentID; //found
		// at this point, we know the node has children and this node is still not the leaf node
		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		return getLeafNodeAtCoord(coord, m_nodeMap.at(parentID).m_childIDs.value()[i]);
	}

	NodeID TestChamber::getNodeOfSizeAtCoord(QVector3D coord, unsigned int size) {
		return getNodeOfSizeAtCoord(coord, m_rootNode, size);
	}
	
	NodeID TestChamber::getNodeOfSizeAtCoord(QVector3D coord, NodeID parentID, unsigned int size) {
		if (isPointOOB(coord)) return parentID;
		if (m_nodeMap.at(parentID).m_size == size) return parentID; //found
		if (!m_nodeMap.at(parentID).m_childIDs.has_value()) return parentID; //no more children to search. return this node

		// This should never fire because the search starts from the root node
		// The only way this can be fired is if the requested size is larger than the root node size
		if (m_nodeMap.at(parentID).m_size < size) throw std::runtime_error("Something went wrong trying to find node of specific size at the given coord. Did you try to find node larger than the root node?");
		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		return getNodeOfSizeAtCoord(coord, m_nodeMap.at(parentID).m_childIDs.value()[i], size);
	}

	NodeID TestChamber::subdivideNodeToCoordOptimized(QVector3D coord, NodeID parentID, unsigned int size)
	{
		IntVector3D p = getClosestPossibleNodeCenterAtLevel(coord, size);
		std::optional<NodeID> r = tryGetNodeAtCoord(p);
		if (r.has_value()) {
			//std::cout << "skipped\n";
			return r.value();
		}
		return subdivideNodeToCoord(coord, parentID, size);
	}

	NodeID TestChamber::subdivideNodeToCoord(QVector3D coord, NodeID parentID, unsigned int size)
	{
		// this whole function call can be avoided by checking whether the target node is already subdivided
		if(isPointOOB(coord)) return parentID;
		if(m_nodeMap.at(parentID).m_size < size) throw std::runtime_error("Something went wrong trying to subdivide node");
		if(m_nodeMap.at(parentID).m_size == size) return parentID;
		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		if(!m_nodeMap.at(parentID).m_childIDs.has_value()) createChildrenOfNode(parentID);
		return subdivideNodeToCoord(coord, m_nodeMap.at(parentID).m_childIDs.value()[i], size);
	}

	std::array<IntVector3D, 4> TestChamber::getChamberSurfaceCorners(const ChamberSurface& surface)
	{
		const TestChamberNode& node = m_nodeMap.at(surface.parentNode);
		float halfSize = node.m_size / 2.0f;

		IntVector3D normal = getWallNormal(surface.side);
		IntVector3D surfCenter = getWallFaceCenter(node.m_center, node.m_size, normal);
		IntVector3D up     = getWallUp(surface.side);
		IntVector3D right  = normal.cross(up);

		// Quad corners: counter-clockwise
		IntVector3D p0 = surfCenter + (-right - up) * halfSize;  // bottom-left
		IntVector3D p1 = surfCenter + ( right - up) * halfSize;  // bottom-right
		IntVector3D p2 = surfCenter + ( right + up) * halfSize;  // top-right
		IntVector3D p3 = surfCenter + (-right + up) * halfSize;  // top-left

		return {p0, p1, p2, p3};
	}

	IntVector3D TestChamber::getSurfaceCenter(ChamberSurface surface)
	{
		return m_nodeMap.at(surface.parentNode).m_size * 0.5f * getWallNormal(surface.side) * -1 + m_nodeMap.at(surface.parentNode).m_center;
	}


	const NodeID TestChamber::searchNodeWithCoord(QVector3D coord) const
	{
		return searchNodeWithCoord(coord, m_rootNode);
	}
	
	const NodeID TestChamber::searchNodeWithCoord(QVector3D coord, NodeID parentID) const
	{
		if(isPointOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if(!m_nodeMap.at(parentID).m_childIDs.has_value()) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		return searchNodeWithCoord(coord,  m_nodeMap.at(parentID).m_childIDs.valueRead().at(i));
	}

	const NodeID TestChamber::searchNodeWithCoordSize(const QVector3D coord, unsigned int size) const
	{
		return searchNodeWithCoordSize(coord, size, m_rootNode);
	}
	const NodeID TestChamber::searchNodeWithCoordSize(const QVector3D coord, unsigned int size, NodeID parentID) const
	{
		if(isPointOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if(m_nodeMap.at(parentID).m_size == size) return parentID; //found

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


	void TestChamber::checkChildrenTouchingBBox( NodeID parentID, const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out )
	{
		TestChamberNode& parent = m_nodeMap.at(parentID);

		if ( !parent.m_childIDs.has_value() )
		{
			// this is a leaf node and we already know it was touching if this code was reached
			out.push_back(parentID);
		}
		else
		{
			// check children
			for ( int i = 0; i < 8; i++ )
			{
				// okay, this can definitely be made more efficient, but whatever
				auto vs = parent.m_childIDs.value();
				IntVector3D nodeMin = getNodeBBoxMin(vs[i]);
				IntVector3D nodeMax = getNodeBBoxMax(vs[i]);
				if ( boundingBoxesIntersect(nodeMin, nodeMax, mins, maxs ) )
				{
					checkChildrenTouchingBBox(vs[i], mins, maxs, out);
				}
			}
		}
	}

	void TestChamber::getNodesTouchingBBox( const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out )
	{
		out.clear();
		checkChildrenTouchingBBox(m_rootNode, mins, maxs, out);
	}

	void TestChamber::subdivideNodesToBBox( const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out )
	{
		// Get a list of all the nodes touching the bbox, then subdivide the ones that overlap the edge
		std::vector<NodeID> touchingNodes;
		getNodesTouchingBBox(mins, maxs, touchingNodes); // for all nodes overlapping the bbox,
		for ( int i = 0; i < touchingNodes.size(); i++ )
		{
			//subdivide them to the bbox
			subdivideNodeToBBoxRecursive(touchingNodes[i], mins, maxs, out);
		}
	}

	void TestChamber::subdivideNodeToBBoxRecursive( NodeID node, const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out )
	{
		// check if bbox intersect
		IntVector3D nodeMin = getNodeBBoxMin(node);
		IntVector3D nodeMax = getNodeBBoxMax(node);
		bool overlap = boundingBoxesIntersect(nodeMin, nodeMax, mins, maxs);
		if (!overlap) return;
		bool encapsulated = isBoxAInsideBoxB(nodeMin, nodeMax, mins, maxs);
		if (overlap && !encapsulated)
		{
			//std::cout << "first condition \n";
			createChildrenOfNode(node);
			for ( int i = 0; i < 8; i++ )
			{
				NodeID childID = m_nodeMap.at(node).m_childIDs.value()[i];
				// keep subdividin'
				subdivideNodeToBBoxRecursive( childID, mins, maxs, out );
			}
		}
		else if (overlap)
		{
			out.push_back(node);
		}
	}

	void TestChamber::generateSurfaces()
	{
		m_surfaceMap.clear();
		for (const auto& [id, node] : getNodeMap()) {
			if (!isNodeActive(id) || nodeHasChild(id)) continue;
			
			std::array<ChamberSurface, 6>& walls = m_surfaceMap.defaultConstruct(id);
			for (int i = 0; i < 6; ++i) {
				WallSide side = static_cast<WallSide>(i); 
				// notice how WallSide indices are used for surface side
				// so you can access the respective side by walls[static_cast<int>(WallSide::POS_X)] etc.
				if(isSurface(side,id))
				{
					walls[i] = {id,side};
				}
			}
		}
	}

	int TestChamber::extrudeSurfaces(Selector* selector)
	{
		//Debugger::instance().clearDebugPoints();
		QVector3D mins = selector->getMins();
		QVector3D maxs = selector->getMaxs();
		if ( !selector->isVolumeSelection() )
		{
			// Shift selection forward
			selector->shiftSelection(selector->getSurfaceNormal() * m_gridSize);

			// Make a volume between the new and original positions
			vector3DMin(selector->getMins(), mins, mins);
			vector3DMax(selector->getMaxs(), maxs, maxs);
		}
		std::vector<NodeID> touchingNodes;
		subdivideNodesToBBox(mins, maxs, touchingNodes);
		int nodesChanged = setNodesActive(touchingNodes, false);
		//for (int i = 0; i < touchingNodes.size(); i++)
		//{
		//	Debugger::instance().addDebugPoint(m_nodeMap.at(touchingNodes[i]).m_center.toQVector3D(), {0.0f,0.0f,1.0f});
		//}

		// TODO: Hook up to undo/redo
		generateSurfaces();

		//Debugger::instance().drawDebugPoints();
		return nodesChanged;
	}

	int TestChamber::recedeSurfaces(Selector* selector)
	{
		//Debugger::instance().clearDebugPoints();
		QVector3D mins = selector->getMins();
		QVector3D maxs = selector->getMaxs();
		if ( !selector->isVolumeSelection() )
		{
			// Shift selection backward
			selector->shiftSelection(selector->getSurfaceNormal() * -m_gridSize);

			// Make a volume between the new and original positions
			vector3DMin(selector->getMins(), mins, mins);
			vector3DMax(selector->getMaxs(), maxs, maxs);
		}
		std::vector<NodeID> touchingNodes;
		subdivideNodesToBBox(mins, maxs, touchingNodes);
		int nodesChanged = setNodesActive(touchingNodes, true);
		// TODO: Hook up to undo/redo
		generateSurfaces();
		// Optimize this:
		unsubdivideNodes(touchingNodes.data(), touchingNodes.size());
		generateSurfaces();
		
		//Debugger::instance().drawDebugPoints();
		return nodesChanged;
	}
	//does destroy the given node
	void TestChamber::destroyBranch(NodeID nodeID)
	{
		auto& n = m_nodeMap.at(nodeID);
		if (!n.m_childIDs.has_value())
		{
			m_nodeMap.erase(nodeID);
			return;
		}
		for (int i = 0; i < 8; i++)
		{
			destroyBranch(n.m_childIDs.valueRead()[i]);
		}
		m_nodeMap.erase(nodeID);
	}

	void TestChamber::unsubdivideNodes(NodeID* touchingNodes, int count)
	{
		std::unordered_set<NodeID> parentsToCheck;
		
		for (int i = 0; i < count; i++)
		{
			try {
				NodeID parent = m_nodeMap.at(touchingNodes[i]).m_parentID.value();
				// Without the recursive child destruction, we can only unsubdivide parents of minimum size nodes
				if (m_nodeMap.at(parent).m_size == gridSizeMin * 2) parentsToCheck.insert(parent);

				//uncomment this if we implement fix above
				//parentsToCheck.insert(parent);
			}
			catch (const std::out_of_range&) {
				continue;
			}
		}
		
		for (const auto& parentID : parentsToCheck)
		{
			//std::cout << "Checking parent node " << parentID << " for unsubdivision\n";
			// At the moment, this needs to be called after the generateSurfaces call above which makes things a bit messy
			// TODO: Can we come up with a method to check for parents to unsubdivide before generating surfaces?
			// TODO: need to fix: if a surface was not generated in the first generateSurfaces() than it might calculate wrong
			if (canUnsubdivide(parentID)) // detecting the nodes to unsubdivide seem to work properly
			{

				for (NodeID j : m_nodeMap.at(parentID).m_childIDs.valueRead())
				{
					destroyBranch(j);
					//m_nodeMap.erase(j);
				}
				m_nodeMap.at(parentID).m_childIDs.reset();
				// we know that canUnsubdivide works properly as indicated by the green dots
				//Debugger::instance().addDebugPoint(m_nodeMap.at(parentID).m_center.toQVector3D(), { 0.0f,1.0f,0.0f });
			}
			//unsubdivideRecursive(parentID);
		}
	}

	bool TestChamber::canUnsubdivide(NodeID node)
	{
		try {
			if (!m_nodeMap.at(node).m_childIDs.has_value())
			{
				// node has no children to unsubdivide
				// maybe the parent node of this node can be unsubdivided
				//Debugger::instance().addDebugPoint(m_nodeMap.at(node).m_center.toQVector3D(), { 1.0f,1.0f,0.0f });
				return false;
			}
		}
		catch (const std::out_of_range&) {
			// node does not exist. likely already destroyed by another unsubdivide operation?
			return false;
		}
		//for its sides:
		for (int i = 0; i < 6; i++)
		{
			WallSide childSide = static_cast<WallSide>(i);
			int count = 0;
			int activeChild = 0;
			for (const auto& childID : m_nodeMap.at(node).m_childIDs.valueRead())
			{
				m_nodeMap.at(childID).m_active ? activeChild++ : activeChild;
				try {
					if (m_surfaceMap.at(childID)[i].side == childSide) //check if the side is active
					{
						//Debugger::instance().addDebugPoint(m_nodeMap.at(childID).m_center.toQVector3D(), { 0.0f,0.0f,1.0f });
						//std::cout << "Added Side " << i << "\n";
						count++;
					}
				}
				catch (const std::out_of_range&) {
					continue; // child has no surface on this side
				}
			}
			if (count % 4 != 0 || activeChild % 8 != 0) // either not all children have this side active, or not all children are active
			{
				return false;
			}
			// if count % 4 == 0, then all 4 children have the same active state on this side
			// if any one side isn't "full", we cannot unsubdivide
		}
		return true;
	}

	void TestChamber::makeUSI(NodeID nodeID, UniqueSurfaceIdentifer& out, WallSide side)
	{
		const TestChamberNode& node = m_nodeMap.at(nodeID);
		out.size = node.m_size;
		out.nodeCord = node.m_center;
		out.side = side;
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
		return IntVector3D(snap(rel.x(), scale)-scale * 0.5f, snap(rel.y(), scale) - scale * 0.5f, snap(rel.z(), scale) - scale * 0.5f);
	}

	NodeID TestChamber::getClosestNode(QVector3D coord) const
	{
		for (unsigned int level = gridSizeMin; level <= m_rootNodeWorldSize; level*=2) {
			IntVector3D center = getClosestPossibleNodeCenterAtLevel(coord, level);
			auto r = tryGetNodeAtCoord(center);
			if (r.has_value()) {
				return r.value();
			}
		}
		return m_rootNode;
	}

	void TestChamber::increaseGridSize()
	{
		m_gridSize *= 2;
		if ( m_gridSize > gridSizeMax )
			m_gridSize = gridSizeMax;
	}

	void TestChamber::decreaseGridSize()
	{
		m_gridSize /= 2;
		if ( m_gridSize < gridSizeMin )
			m_gridSize = gridSizeMin;
	}
}
