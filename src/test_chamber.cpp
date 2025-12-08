// octree_test.cpp
// Implements the class that represents the test chamber itself and the octree nodes
#include "undo.h"
#include "selection.h"
#include "util/math.h"
#include "util/debug.h"
#include "util/3d/mesh.h" 
#include "test_chamber.h"
#include "test_chamber_types.h"
#include "test_chamber_actions.h"

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

	void TestChamber::createChildrenOfNode( NodeID parentID )
	{
		TestChamberNode& parentNode = m_nodeMap.at(parentID);
		std::array<NodeID, 8>& parentChildIds = parentNode.m_childIDs.value_initialize();
		
		// Child Node Generation Order
		//negx negy negz: 0
		//posx negy negz: 1
		//negx posy negz: 2
		//posx posy negz: 3
		//negx negy posz: 4
		//posx negy posz: 5
		//negx posy posz: 6
		//posx posy posz: 7
		
		// posx -> 1, 3, 5, 7
		// posy -> 2, 3, 6, 7
		// posz -> 4, 5, 6, 7
		// negx -> 0, 2, 4, 6
		// negy -> 0, 1, 4, 5
		// negz -> 0, 1, 2, 3

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
	void TestChamber::getSurfaceChildren(std::vector<ChamberSurface>& out, NodeID node, WallSide side) const
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
	void TestChamber::getNeighborNodes(NodeID node, std::array<std::optional<NodeID>, 6>& out) const
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
	void TestChamber::checkChildrenTouchingBBox( NodeID parentID, const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out ) const
	{
		const TestChamberNode& parent = m_nodeMap.at(parentID);

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
				auto vs = parent.m_childIDs.valueRead();
				IntVector3D nodeMin = getNodeBBoxMin(vs[i]);
				IntVector3D nodeMax = getNodeBBoxMax(vs[i]);
				if ( boundingBoxesIntersect(nodeMin, nodeMax, mins, maxs ) )
				{
					checkChildrenTouchingBBox(vs[i], mins, maxs, out);
				}
			}
		}
	}
	void TestChamber::getNodesTouchingBBox( const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out ) const
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
		// TODO: unsubdivideNodes in extrudeSurfaces completely breaks. why?
		//unsubdivideNodes(touchingNodes.data(), touchingNodes.size());
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
				if (m_nodeMap.at(parent).m_size >= gridSizeMin * 2) parentsToCheck.insert(parent);
			}
			catch (const std::out_of_range&) {
				continue;
			}
		}
		for (const auto& parentID : parentsToCheck)
		{
			try {
				if (canUnsubdivide(parentID)) // detecting the nodes to unsubdivide seem to work properly
				{
					const auto& childIDs = m_nodeMap.at(parentID).m_childIDs.valueRead();
					for (NodeID j : childIDs)
					{
						destroyBranch(j);
					}
					m_nodeMap.at(parentID).m_childIDs.reset();
				}
			}
			catch (const std::out_of_range&) {
				continue;
			}			
		}
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