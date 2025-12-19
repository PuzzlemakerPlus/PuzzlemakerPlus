// octree_test.cpp
// Implements the class that represents the test chamber itself and the octree nodes
#include "undo.h"
#include "selection.h"
#include "util/math.h"
#include "util/debug.h"
#include "util/scoped_timer.h"
#include "util/3d/mesh.h" 
#include "test_chamber.h"
#include "test_chamber_types.h"
#include "test_chamber_actions.h"

#include <array>
#include <future>
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
		
		unsigned int parentSize = parentNode.getSize();
		IntVector3D parentCenter = parentNode.getCenter();
		
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
		std::array<NodeID, 8> children = {};
		for (unsigned int i = 0; i < 8; i++ )
		{
			IntVector3D pos = getChildCenter(i, parentSize, parentCenter);
			NodeID id = positionToNodeID(pos);
			createOctreeNode(id, parentID, pos, parentSize /2);
			children[i] = id;
			m_nodeDirty.insert(id);
			
		}
		TestChamberNode& pn = m_nodeMap.at(parentID);
		pn.setActive(true); // Make sure the parent node is active after creating children
		std::array<NodeID, 8>& parentChildIds = pn.initializeChildren();
		for (unsigned int i = 0; i < 8; i++)
		{
			parentChildIds[i] = children[i];
		}
		//parentNode.m_active = true; 
	}
	
	void TestChamber::createOctreeNode(NodeID newID, std::optional<NodeID> parentID, IntVector3D center, unsigned int size)
	{
		if (m_nodeMap.find(newID) != m_nodeMap.end())
		{
			auto n = *m_nodeMap.find(newID);
			auto oldcenter = n.second.getCenter();
			std::cout << "newpos: (" << center.x() << " " << center.y() << " " << center.z() << ") ";
			std::cout << "newhash: " << positionToNodeID(center) << "\n";
			std::cout << "oldpos: (" << oldcenter.x() << " " << oldcenter.y() << " " << oldcenter.z() << ") ";
			std::cout << "oldhash: " << positionToNodeID(oldcenter) << "\n";
			throw std::runtime_error("Hash collision");
		}
		if ( parentID.has_value() )
			m_nodeMap.emplace(newID, TestChamberNode(m_nodeDirty, newID, parentID.value(), center, size, m_nodeMap.at(parentID.value()).getActive()));
		else
			m_nodeMap.emplace(newID, TestChamberNode(m_nodeDirty, newID, center, size));
		setNeighbors(newID);
	}
	int TestChamber::setNodeActive(NodeID node, bool active)
	{
		auto& tcn = m_nodeMap.at(node);
		//TODO: this is different from the comment. need to check if fixing this would break something
		if(tcn.getActive() == false) return 0; // State of the node has not changed
		tcn.setActive(active);
		// Also need to make sure all adjacent nodes have matching size.
		// TODO: now that we have IntVector3D, this can be optimized further
		std::array<IntVector3D, 6> outerCoords = getNeighborNodeCenters(tcn.getSize(), tcn.getCenter());
		for(const auto& coord : outerCoords)
		{
			subdivideNodeToCoordOptimized(coord.toQVector3D(), 0, tcn.getSize());
		}
		return 1; // State of the node has changed
	}
	int TestChamber::setNodesActive(std::vector<NodeID>& nodes, bool active) 
	{
		int nodesChanged = 0;
		for ( int i = 0; i < nodes.size(); i++ )
		{
			m_nodeMap.at(nodes[i]).setActive(active);
			nodesChanged++;
		}
		// Match the sizes of adjacent nodes only after all nodes have had their active state set
		for ( int i = 0; i < nodes.size(); i++ )
		{
			auto& tcn = m_nodeMap.at(nodes[i]);
			unsigned int size = tcn.getSize();
			std::array<IntVector3D, 6> outerCoords = getNeighborNodeCenters(tcn.getSize(), tcn.getCenter());
			for ( const auto& coord : outerCoords )
			{
				NodeID done = subdivideNodeToCoordOptimized( coord.toQVector3D(), m_rootNode, size);
				//Debugger::instance().addDebugPoint(coord.toQVector3D(), { 1.0f, 1.0f, 0.0f } );
			}
		}
		return nodesChanged; // State of the node has changed
	}
	//Assumes out is empty during non-recursive calls
	void TestChamber::getSurfaceChildren(std::vector<ChamberSurface>& out, NodeID node, WallSide side) const
	{
		const auto& tcn = m_nodeMap.at(node);
		// Node has children 
		if (tcn.hasChildren())
		{
			const auto& children = tcn.readChildrenID();
			// Iterate through all children and get their surfaces
			for (const auto& childID : children)
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
	//void TestChamber::getNeighborNodes(NodeID node, std::array<std::optional<NodeID>, 6>& out) const
	//{
	//	const TestChamberNode& currentNode = m_nodeMap.at(node);
	//	std::array<IntVector3D, 6> neighborCoords = getNeighborNodeCenters(currentNode.getSize(), currentNode.getCenter());
	//	for (int i = 0; i < 6; ++i)
	//	{
	//		try {
	//			NodeID neighborNodeID = getNodeOfSizeAtCoord(neighborCoords[i].toQVector3D(), currentNode.getSize());
	//			out[i] = neighborNodeID;
	//		} catch (const std::runtime_error&) {
	//			out[i] = std::nullopt; // Out of bounds
	//		}
	//	}
	//}
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
		auto& tcp = m_nodeMap.at(parentID);
		if(!tcp.hasChildren()) return parentID; //found
		// at this point, we know the node has children and this node is still not the leaf node
		unsigned int i = getChildOrder(coord, tcp.getCenter());
		return getLeafNodeAtCoord(coord, tcp.readChildrenID()[i]);
	}
	NodeID TestChamber::subdivideNodeToCoordOptimized(QVector3D coord, NodeID parentID, unsigned int size)
	{
		IntVector3D p = getClosestPossibleNodeCenterAtLevel(coord, size);
		std::optional<NodeID> r = tryGetNodeAtCoord(p);
		if (r.has_value()) return r.value();
		return subdivideNodeToCoord(coord, parentID, size);
	}
	NodeID TestChamber::subdivideNodeToCoord(QVector3D coord, NodeID parentID, unsigned int size)
	{
		// this whole function call can be avoided by checking whether the target node is already subdivided
		if(isPointOOB(coord)) return parentID;
		unsigned int i;
		{
			auto& tcp = m_nodeMap.at(parentID);
			if (tcp.getSize() < size) throw std::runtime_error("Something went wrong trying to subdivide node");
			if (tcp.getSize() == size) return parentID;
			i = getChildOrder(coord, tcp.getCenter());
			if (!tcp.hasChildren()) createChildrenOfNode(parentID);
		}
		auto& tcp = m_nodeMap.at(parentID);
		return subdivideNodeToCoord(coord, tcp.readChildrenID()[i], size);
	}
	std::array<IntVector3D, 4> TestChamber::getChamberSurfaceCorners(const ChamberSurface& surface)
	{
		const TestChamberNode& node = m_nodeMap.at(surface.parentNode);
		float halfSize = node.getSize() / 2.0f;

		IntVector3D m_gNormal = getWallNormal(surface.side);
		IntVector3D surfCenter = getWallFaceCenter(node.getCenter(), node.getSize(), m_gNormal);
		IntVector3D up     = getWallUp(surface.side);
		IntVector3D right  = m_gNormal.cross(up);

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

		if ( !parent.hasChildren() )
		{
			// this is a leaf node and we already know it was touching if this code was reached
			out.push_back(parentID);
			return;
		}
		// check children
		for (int i = 0; i < 8; i++)
		{
			// okay, this can definitely be made more efficient, but whatever
			auto vs = parent.readChildrenID();
			IntVector3D nodeMin = getNodeBBoxMin(vs[i]);
			IntVector3D nodeMax = getNodeBBoxMax(vs[i]);
			if (boundingBoxesIntersect(nodeMin, nodeMax, mins, maxs))
			{
				checkChildrenTouchingBBox(vs[i], mins, maxs, out);
			}
		}
	}
	void TestChamber::getNodesTouchingBBox( const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out ) const
	{
		ScopedTimer("getNodesTouchingBBox");
		out.clear();
		checkChildrenTouchingBBox(m_rootNode, mins, maxs, out);
	}
	void TestChamber::subdivideNodesToBBox( const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out)
	{
		// Get a list of all the nodes touching the bbox, then subdivide the ones that overlap the edge
		std::vector<NodeID> touchingNodes;

		getNodesTouchingBBox(mins, maxs, touchingNodes); // for all nodes overlapping the bbox,
		//std::cout << touchingNodes.size() << " nodes in selection volume\n";
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
			//if a node is overlapping but not encapsulated, subdivide further
			createChildrenOfNode(node);
			for ( int i = 0; i < 8; i++ )
			{
				const auto& children = m_nodeMap.at(node).readChildrenID();
				//std::cout << "subdividing " << children[i] << "\n";
				// keep subdividin'
				subdivideNodeToBBoxRecursive(children[i], mins, maxs, out );
			}
		}
		else if (encapsulated) // no need to subdivide. 
		{
			out.push_back(node);
		}
	}
	void TestChamber::updateSurfaces()
	{
		//std::cout << "m_surfaceMap Size: " << m_surfaceMap.size() << "\n";
		//std::cout << "m_nodeDirty Size: " << m_nodeDirty.size() << "\n";
		//std::cout << "m_nodeMap Size: " << m_nodeMap.size() << "\n";
		for (const auto& id : m_nodeDirty)
		{
			if (m_nodeMap.find(id) == m_nodeMap.end() || id == 0) {
				continue;
			}
			if (!isNodeActive(id) || nodeHasChild(id))
			{
				//delete all surfaces
				m_surfaceMap.erase(id);
			}
			else
			{
				std::array<ChamberSurface, 6>& walls = m_surfaceMap.defaultConstruct(id);
				for (int i = 0; i < 6; ++i) {
					WallSide side = static_cast<WallSide>(i);
					// notice how WallSide indices are used for surface side
					// so you can access the respective side by walls[static_cast<int>(WallSide::POS_X)] etc.
					if (isSurface(side, id))
					{
						walls[i] = { id,side };
					}
				}
			}
		}
		m_nodeDirty.clear();
		std::cout << "Done updating surfaces " << "\n";
	}
	void TestChamber::generateSurfaces()
	{
		m_surfaceMap.clear();
		std::vector<NodeID> keys;
		keys.reserve(m_nodeMap.size());
		for (auto const& [k, _] : m_nodeMap) keys.push_back(k);
		for (auto const& id : keys) {
			if (!isNodeActive(id) || nodeHasChild(id)) continue; // skip if node not active or if node has child
			// node doesnt have children and node is active
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
	void TestChamber::generateSurfacesThreaded()
	{
		struct SurfaceFlags {
			NodeID id;
			uint8_t flags;
		};

		m_surfaceMap.clear();
		constexpr int numThreads = 8;
		std::array<std::future<std::vector<SurfaceFlags>>, numThreads> futs; //returns a surface flag per node

		std::vector<NodeID> keys;
		keys.reserve(getNodeMap().size());
		for (const auto& [k, v] : m_nodeMap) keys.push_back(k);
		int launched = 0;
		std::vector<NodeID>::iterator itStart;
		std::vector<NodeID>::iterator itEnd;
		
		for (unsigned t = 0; t < numThreads; ++t) {
			const size_t begin = (keys.size() * t) / numThreads;
			const size_t end = (keys.size() * (t + 1)) / numThreads;
			if (begin >= end) break; // empty chunk (happens when keys.size() < numThreads)
			launched++;
			auto itStart = keys.begin() + begin;
			auto itEnd = keys.begin() + end;
			auto workerfunc = [this, itStart, itEnd]() -> std::vector<SurfaceFlags> {
				std::vector<SurfaceFlags> out;
				out.reserve(itEnd - itStart);
				for (auto it = itStart; it != itEnd; ++it)
				{
					NodeID id = *it;
					if (!isNodeActive(id) || nodeHasChild(id)) continue;
					uint8_t flags = 0;
					for (int i = 0; i < 6; ++i)
					{
						WallSide side = static_cast<WallSide>(i);
						if (isSurface(side, id))
						{
							flags |= uint8_t(1u << i);
						}
					}
					SurfaceFlags f = { id, flags };
					out.push_back(f);
				}
				return out;
				};
			futs[t] = std::async(std::launch::async, workerfunc);
		}
		for (unsigned t = 0; t < launched; ++t)
		{
			std::vector<SurfaceFlags> chunk = futs[t].get();
			for (SurfaceFlags flag : chunk)
			{
				std::array<ChamberSurface, 6>& walls = m_surfaceMap.defaultConstruct(flag.id);
				for (int i = 0; i < 6; ++i) {
					if (flag.flags & (1u << i)) walls[i] = { flag.id,static_cast<WallSide>(i)};
				}
			}
		}
	}
	void TestChamber::eraseNode(NodeID nodeID)
	{
		m_nodeMap.erase(nodeID);
		m_surfaceMap.erase(nodeID);
	}
	int TestChamber::extrudeSurfaces(Selector* selector)
	{
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
		// TODO: Hook up to undo/redo
		/*std::vector<UniqueVolumeIdentifier> extrudeUVIs;
		SelectionVolume sv = { mins, maxs };
		for ( const auto& nodeID : touchingNodes )
		{
			extrudeUVIs.push_back(UniqueVolumeIdentifier::fromNodeID(*this, nodeID));
		}
		UndoStack::instance().addAction(std::make_unique<TCExtrudeAction>(this, selector, extrudeUVIs, sv));*/

		{
			ScopedTimer timer = ScopedTimer("generateSurfaces");
			generateSurfaces();
		}
		//{
		//	ScopedTimer("generateSurfacesThreaded");
		//	generateSurfacesThreaded();
		//}
		//{
		//	ScopedTimer timer = ScopedTimer("updateSurfaces");
		//	updateSurfaces();
		//}

		return nodesChanged;
	}
	int TestChamber::recedeSurfaces(Selector* selector)
	{
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
		{
			ScopedTimer timer = ScopedTimer("generateSurfaces");
			generateSurfaces();
		}
		//{
		//	ScopedTimer("generateSurfacesThreaded");
		//	generateSurfacesThreaded();
		//}
		//{
		//	ScopedTimer("updateSurfaces");
		//	updateSurfaces();
		//}
		return nodesChanged;
	}
	//does destroy the given node
	void TestChamber::destroyBranch(NodeID nodeID)
	{
		auto& n = m_nodeMap.at(nodeID);
		if (!n.hasChildren())
		{
			m_nodeMap.erase(nodeID);
			return;
		}
		const auto& child = n.readChildrenID();
		for (int i = 0; i < 8; i++)
		{
			destroyBranch(child[i]);
		}
		m_nodeMap.erase(nodeID);
	}
	//does not destroy the given node
	void TestChamber::destroyChildrenOfNode(NodeID parentID)
	{
		TestChamberNode& parentNode = m_nodeMap.at(parentID);
		if (parentNode.hasChildren())
		{
			std::array<NodeID, 8>& parentChildIds = parentNode.getChildrenID();
			for (int i = 0; i < 8; ++i)
			{
				NodeID childID = parentChildIds[i];
				TestChamberNode& childNode = m_nodeMap.at(childID);

				if (childNode.hasChildren())
					destroyChildrenOfNode(childID);

				m_nodeMap.erase(childID);
				m_nodeDirty.insert(childID);
			}
			m_nodeDirty.insert(parentID);
			parentNode.resetChildrenID();
		}
	}
	void TestChamber::unsubdivideNodes(NodeID* touchingNodes, int count, std::vector<NodeID>* out)
	{
		std::unordered_set<NodeID> parentsToCheck;
		for (int i = 0; i < count; i++)
		{
			NodeID parent = m_nodeMap.at(touchingNodes[i]).getParentID();
			parentsToCheck.insert(parent);
		}
		for (const auto& parentID : parentsToCheck)
		{
			auto it = m_nodeMap.find(parentID);
			if (it == m_nodeMap.end()) continue; // node was deleted
			if (canUnsubdivide(parentID)) // detecting the nodes to unsubdivide seem to work properly
			{
				destroyChildrenOfNode(parentID);
				//std::cout << "Destroyed children of " << parentID << "\n";
			}
		}
		if (out) // add the remaining nodes to out
		{
			for (int i = 0; i < count; i++)
			{
				auto it = m_nodeMap.find(touchingNodes[i]);
				if (it != m_nodeMap.end()) out->push_back(touchingNodes[i]);
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