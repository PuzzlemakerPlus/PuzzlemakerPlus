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
	QVector3D getWallFaceCenter(QVector3D center, unsigned int size, QVector3D normal)
	{
		return center + size/2 * -normal;
	}

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
	unsigned int getChildOrder(QVector3D coord, QVector3D parentCenter)
	{
		QVector3D delta = coord - parentCenter;

		// Determine sign bits: 0 for negative, 1 for positive
		unsigned int xBit = (delta.x() >= 0) ? 1 : 0;
		unsigned int yBit = (delta.y() >= 0) ? 1 : 0;
		unsigned int zBit = (delta.z() >= 0) ? 1 : 0;

		// Bit pattern: xBit (LSB), yBit, zBit (MSB)
		return (zBit << 2) | (yBit << 1) | xBit;
	}

	QVector3D getChildCenter(unsigned int order, unsigned int parentSize, QVector3D parentCenter)
	{
		//std::cout << parentSize << std::endl;
		if (parentSize % 4 != 0) throw std::runtime_error("Parent node size must be divisible by 4");
		if (order > 7) throw std::runtime_error("Order can not be greater than 7");

		int offset = parentSize / 4;
		// Branchless calculation
		int xSign = ((order & 1) << 1) - 1;               // 1 if odd, -1 if even
		int ySign = (((order >> 1) & 1) << 1) - 1;        // 1 if bit 1 is 0, -1 if 1
		int zSign = (((order >> 2) & 1) << 1) - 1;        // 1 if bit 2 is 0, -1 if 1

		return parentCenter + QVector3D(offset * xSign, offset * ySign, offset * zSign);
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
			QVector3D pos = getChildCenter(i, parentNode.m_size, parentNode.m_center);
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

	void TestChamber::createOctreeNode(NodeID newID, std::optional<NodeID> parentID, QVector3D center, unsigned int size)
	{
		if ( parentID.has_value() )
			m_nodeMap.emplace(newID, TestChamberNode( parentID.value(), center, size, m_nodeMap.at(parentID.value()).m_active));
		else
			m_nodeMap.emplace(newID, TestChamberNode(center, size));
	}

	NodeID TestChamber::positionToNodeID(QVector3D pos)
	{
		// make sure all coordinates are positive
		QVector3D displaced = pos + QVector3D(m_rootNodeWorldSize,m_rootNodeWorldSize,m_rootNodeWorldSize);
		uint64_t x = static_cast<uint64_t>(std::llround(displaced.x()));
		uint64_t y = static_cast<uint64_t>(std::llround(displaced.y()));
		uint64_t z = static_cast<uint64_t>(std::llround(displaced.z()));

		//good known magic numbers for hashing
		uint64_t h = x * 73856093ULL ^ y * 19349663ULL ^ z * 83492791ULL; 

		// simple 64-bit “scrambler”
		h ^= h >> 33;
		h *= 0xff51afd7ed558ccdULL;
		h ^= h >> 33;
		h *= 0xc4ceb9fe1a85ec53ULL;
		h ^= h >> 33;
		return h;
	}

	// TRADE OFFER:
	// I 
	// GET O(1) ACCESS TO NODES
	// YOU
	// GET O(8 * n^2) SEARCH TIME TO FINDING FREE ID
	void TestChamber::findFreeIDs(std::array<NodeID, 8>& out)
	{
		std::unordered_set<NodeID> used;

		for (const auto& [id, _] : m_nodeMap)
			used.insert(id);

		NodeID next = 0;
		for (int i = 0; i < 8; ++i) {
			while (used.count(next))
				++next;
			out[i] = next;
			used.insert(next);
			++next;
		}
	}

	std::array<QVector3D, 6> getNeighborNodeCenters(const unsigned int nodeSize, const QVector3D nodeCenter, float epsilon = 0.5f)
	{
		float s = nodeSize + epsilon;
		return {
			nodeCenter + QVector3D(s, 0.0f, 0.0f),  // +X
			nodeCenter + QVector3D(-s, 0.0f, 0.0f),  // -X
			nodeCenter + QVector3D(0.0f,  s, 0.0f),  // +Y
			nodeCenter + QVector3D(0.0f, -s, 0.0f),  // -Y
			nodeCenter + QVector3D(0.0f, 0.0f,  s),  // +Z
			nodeCenter + QVector3D(0.0f, 0.0f, -s)   // -Z
		};
	}

	std::array<QVector3D, 6> getOuterFaceCoords(const unsigned int nodeSize, const QVector3D nodeCenter, float epsilon = 0.5f)
	{
		float halfSize = nodeSize / 2.0f + epsilon;

		return {
			nodeCenter + QVector3D( halfSize, 0.0f, 0.0f),  // +X
			nodeCenter + QVector3D(-halfSize, 0.0f, 0.0f),  // -X
			nodeCenter + QVector3D(0.0f,  halfSize, 0.0f),  // +Y
			nodeCenter + QVector3D(0.0f, -halfSize, 0.0f),  // -Y
			nodeCenter + QVector3D(0.0f, 0.0f,  halfSize),  // +Z
			nodeCenter + QVector3D(0.0f, 0.0f, -halfSize)   // -Z
		};
	}

	int TestChamber::setNodeActive(NodeID node, bool active) 
	{
		if(m_nodeMap.at(node).m_active == false) return 0; // State of the node has not changed

		m_nodeMap.at(node).m_active = active;
		// Also need to make sure all adjacent nodes have matching size.
		std::array<QVector3D, 6> outerCoords = getNeighborNodeCenters(m_nodeMap.at(node).m_size, m_nodeMap.at(node).m_center);
		for(const auto& coord : outerCoords)
		{
			subdivideNodeToCoord(coord, 0, m_nodeMap.at(node).m_size);
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
			std::array<QVector3D, 6> outerCoords = getNeighborNodeCenters(m_nodeMap.at(nodes[i]).m_size, m_nodeMap.at(nodes[i]).m_center );
			for ( const auto& coord : outerCoords )
			{
				subdivideNodeToCoord( coord, m_rootNode, m_nodeMap.at(nodes[i]).m_size );
			}
		}
		return nodesChanged; // State of the node has changed
	}

	std::optional<ChamberSurface> TestChamber::getSurface(NodeID node, WallSide side)
	{
		ChamberSurface surf = m_surfaceMap.at(node)[static_cast<int>(side)];
		if(surf.side == WallSide::NONE) return std::nullopt;
		return std::optional<ChamberSurface>(surf);
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

	bool TestChamber::coordInNode(QVector3D coord, NodeID nodeID)
	{
		const TestChamberNode& node = m_nodeMap.at(nodeID);
		float halfSize = node.m_size / 2.0f;

		return (coord.x() >= node.m_center.x() - halfSize && coord.x() <= node.m_center.x() + halfSize) &&
			   (coord.y() >= node.m_center.y() - halfSize && coord.y() <= node.m_center.y() + halfSize) &&
			   (coord.z() >= node.m_center.z() - halfSize && coord.z() <= node.m_center.z() + halfSize);
	}

	void TestChamber::getNeighborNodes(NodeID node, std::array<std::optional<NodeID>, 6>& out)
	{
		const TestChamberNode& currentNode = m_nodeMap.at(node);
		std::array<QVector3D, 6> neighborCoords = getNeighborNodeCenters(currentNode.m_size, currentNode.m_center);

		for (int i = 0; i < 6; ++i)
		{
			try {
				NodeID neighborNodeID = getNodeOfSizeAtCoord(neighborCoords[i], currentNode.m_size);
				out[i] = neighborNodeID;
			} catch (const std::runtime_error&) {
				out[i] = std::nullopt; // Out of bounds
			}
		}
	}

	bool TestChamber::areNeighborsSameSize(NodeID node) {
		const TestChamberNode& currentNode = m_nodeMap.at(node);
		std::array<QVector3D, 6> neighborCoords = getNeighborNodeCenters(currentNode.m_size, currentNode.m_center);
		for (int i = 0; i < 6; ++i)
		{
			if (isPointOOB(neighborCoords[i])) continue;
			auto r = tryGetNodeAtCoord(neighborCoords[i]);
			if(r == std::nullopt) return false; // Neighbor does not exist
		}
		return true;
	}

	std::optional<NodeID> TestChamber::tryGetNodeAtCoord(QVector3D coord)
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

	NodeID TestChamber::subdivideNodeToCoord(QVector3D coord, NodeID parentID, unsigned int size)
	{
		if(isPointOOB(coord)) return parentID;
		if(m_nodeMap.at(parentID).m_size < size) throw std::runtime_error("Something went wrong trying to subdivide node");
		if(m_nodeMap.at(parentID).m_size == size) return parentID;
		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		if(!m_nodeMap.at(parentID).m_childIDs.has_value()) createChildrenOfNode(parentID);
		return subdivideNodeToCoord(coord, m_nodeMap.at(parentID).m_childIDs.value()[i], size);
	}

	std::array<QVector3D, 4> TestChamber::getChamberSurfaceCorners(const ChamberSurface& surface)
	{
		const TestChamberNode& node = m_nodeMap.at(surface.parentNode);
		float halfSize = node.m_size / 2.0f;

		QVector3D normal = getWallNormal(surface.side);
		QVector3D surfCenter = getWallFaceCenter(node.m_center, node.m_size, normal);
		QVector3D up     = getWallUp(surface.side);
		QVector3D right  = QVector3D::crossProduct(normal, up);

		// Quad corners: counter-clockwise
		QVector3D p0 = surfCenter + (-right - up) * halfSize;  // bottom-left
		QVector3D p1 = surfCenter + ( right - up) * halfSize;  // bottom-right
		QVector3D p2 = surfCenter + ( right + up) * halfSize;  // top-right
		QVector3D p3 = surfCenter + (-right + up) * halfSize;  // top-left

		return {p0, p1, p2, p3};
	}

	QVector3D TestChamber::getSurfaceCenter(ChamberSurface surface)
	{
		return m_nodeMap.at(surface.parentNode).m_size * 0.5f * getWallNormal(surface.side) * -1 + m_nodeMap.at(surface.parentNode).m_center;
	}

	//Given a wallside of a node, should this wall be drawn
	bool TestChamber::isSurface(WallSide side, NodeID node) const
	{
		const QVector3D center = m_nodeMap.at(node).m_center;
		const float halfSize = m_nodeMap.at(node).m_size / 2.0f;
		const float epsilon = 0.4f;
		QVector3D offset;
		using enum WallSide;
		switch (side) {
			case POS_X: offset = {+halfSize + epsilon, 0, 0}; break;
			case NEG_X: offset = {-halfSize - epsilon, 0, 0}; break;
			case POS_Y: offset = {0, +halfSize + epsilon, 0}; break;
			case NEG_Y: offset = {0, -halfSize - epsilon, 0}; break;
			case POS_Z: offset = {0, 0, +halfSize + epsilon}; break;
			case NEG_Z: offset = {0, 0, -halfSize - epsilon}; break;
			case NONE: throw std::runtime_error("Invalid WallSide");
		}

		QVector3D neighborCoord = center + offset;

		bool isVisible = isPointOOB(neighborCoord); //if coordinate behind this surface is OOB, this surface should be drawn
		if (!isVisible) { // the coordinate behind this surface is in bound
			NodeID neighborId = searchNodeWithCoord(neighborCoord);
			isVisible = !isNodeActive(neighborId); // check if the node behind this surface is inactive
		}
		return isVisible;
	}

	// Raycast function to find the closest hit on a surface
	std::optional<RayHitResult> TestChamber::rayCast(Ray ray) const
	{
		float closestT = std::numeric_limits<float>::infinity();
		std::optional<RayHitResult> closestHit;

		for (const auto& [_, surfaces] : m_surfaceMap)
		{
			for(const auto& surface : surfaces)
			{
				if(surface.side == WallSide::NONE) continue;
				if(!WallFacesDirection(surface.side,ray.direction)) continue;
				const TestChamberNode& node = m_nodeMap.at(surface.parentNode);

				// Wall geometry
				float halfSize = node.m_size / 2.0f;
				QVector3D up = getWallUp(surface.side);
				QVector3D normal = getWallNormal(surface.side);
				QVector3D right = QVector3D::crossProduct(normal, up);
				QVector3D center = getWallFaceCenter(node.m_center, node.m_size, normal);

				QVector3D p0 = center + (-right - up) * halfSize;
				QVector3D p1 = center + ( right - up) * halfSize;
				QVector3D p2 = center + ( right + up) * halfSize;
				QVector3D p3 = center + (-right + up) * halfSize;

				float tTemp;
				QVector3D hitPosTemp;

				if (rayIntersectsQuad(ray, p0, p1, p2, p3, tTemp, hitPosTemp) && tTemp > 0.0f && tTemp < closestT)
				{
					closestT = tTemp;
					closestHit = RayHitResult{surface.parentNode, hitPosTemp, surface};
				}
			}
		}
		return closestHit;
	}

	bool TestChamber::isPointOOB(QVector3D coord) const
	{
		int halfSize = m_rootNodeWorldSize / 2;
		return std::abs(coord.x()) > halfSize || std::abs(coord.y()) > halfSize || std::abs(coord.z()) > halfSize;
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

	const NodeID TestChamber::searchNodeWithCoordSize(QVector3D coord, unsigned int size) const
	{
		return searchNodeWithCoordSize(coord, size, m_rootNode);
	}
	const NodeID TestChamber::searchNodeWithCoordSize(QVector3D coord, unsigned int size, NodeID parentID) const
	{
		if(isPointOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if(m_nodeMap.at(parentID).m_size == size) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		return searchNodeWithCoordSize(coord, size, m_nodeMap.at(parentID).m_childIDs.valueRead().at(i));
	}

	void TestChamber::getNodeBBox(NodeID id, std::array<QVector3D, 2>& out)
	{
		const TestChamberNode& node = m_nodeMap.at(id);

		float halfSize = node.m_size * 0.5f;

		out[0] = QVector3D(node.m_center.x() - halfSize,
			node.m_center.y() - halfSize,
			node.m_center.z() - halfSize);

		out[1] = QVector3D(node.m_center.x() + halfSize,
			node.m_center.y() + halfSize,
			node.m_center.z() + halfSize);
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
				std::array<QVector3D, 2> childBBox;
				getNodeBBox( vs[i], childBBox);
				if ( boundingBoxesIntersect( childBBox[0], childBBox[1], mins, maxs ) )
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
		std::array<QVector3D, 2> nodeBBox;
		getNodeBBox(node, nodeBBox);
		// check if bbox intersect
		bool overlap = boundingBoxesIntersect(nodeBBox[0], nodeBBox[1], mins, maxs);
		if (!overlap) return;
		bool encapsulated = isBoxAInsideBoxB(nodeBBox[0], nodeBBox[1], mins, maxs);
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
		// TODO: optimize this so we're not doing the intersection check twice
		else if (overlap)
		{
			//std::cout << "second condition \n";
			out.push_back(node);
		}
	}

	void TestChamber::generateQuads(std::vector<Quad>& out) const
	{
		if(m_surfaceMap.size() == 0) throw std::runtime_error("Attempted to generate quads for test chamber with no surface. Forgot to call GenerateSurfaces()?");
		for (const auto& [_, surfaces] : m_surfaceMap)
		{
			for(const auto& surface : surfaces)
			{
				if(surface.side == WallSide::NONE) continue;
				const NodeID nodeID = surface.parentNode;
				out.push_back(generateQuadAt(m_nodeMap.at(nodeID).m_center, surface.side, m_nodeMap.at(nodeID).m_size));
			}
		}
	}

	void TestChamber::generateTCQuads(std::vector<TestChamberQuad>& out) const
	{
		if(m_surfaceMap.size() == 0) throw std::runtime_error("Attempted to generate quads for test chamber with no surface. Forgot to call GenerateSurfaces()?");
		for (const auto& [surfaceID, surfaces] : m_surfaceMap)
		{
			for(const auto& surface : surfaces)
			{
				if(surface.side == WallSide::NONE) continue;
				const NodeID nodeID = surface.parentNode;
				out.push_back(generateTCQuadAt(m_nodeMap.at(nodeID).m_center, surface.side, m_nodeMap.at(nodeID).m_size, surfaceID, nodeID));
			}
		}
	}

	void TestChamber::generateSurfaces()
	{
		m_surfaceMap.clear();
		for (const auto& [id, node] : getNodeMap()) {
			if (!isNodeActive(id) || nodeHasChild(id)) continue;
			const QVector3D& center = node.m_center;
			
			std::array<ChamberSurface, 6>& walls = m_surfaceMap.defaultConstruct(id);
			for (int i = 0; i < 6; ++i) {
				WallSide side = static_cast<WallSide>(i);
				if(isSurface(side,id))
				{
					walls[i] = {id,side};
				}
			}
		}
	}

	void TestChamber::calculateCenterPointsSurfaceDisplacement(const std::vector<ChamberSurface>& surfs, float dist, std::vector<QVector3D>& out)
	{
		for (const auto& s : surfs) {
			// Get the center of the surface
			const TestChamberNode& node = m_nodeMap.at(s.parentNode);
			QVector3D center = node.m_center;
			float size = node.m_size;

			// Get the normal of the surface
			QVector3D normal = getWallNormal(s.side);

			// Calculate the new position
			QVector3D newPos = center + normal * size * dist;
			out.push_back(newPos);
		}
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
		generateSurfaces();
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
		generateSurfaces();
		return nodesChanged;
	}

	void TestChamber::makeUSI(NodeID nodeID, UniqueSurfaceIdentifer& out, WallSide side)
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
			if(node.m_parentID.has_value()) std::cout << "NodeID: " << nodeId << ", child of " << node.m_parentID.value() << "\n";
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
			std::cout << (isLast ? "└── " : "├── ");
		}
#endif
		std::cout << "Node " << id << " at " << printVector3D(m_nodeMap.at(id).m_center) << " with size " << m_nodeMap.at(id).m_size << "\n";

		const auto& childVec = children[id];
		for (size_t i = 0; i < childVec.size(); ++i) {
			bool last = (i == childVec.size() - 1);
#ifdef WIN32
			_printNodesTree(children, childVec[i], prefix + (isLast ? "    " : "|   "), last);
#else
			_printNodesTree(children, childVec[i], prefix + (isLast ? "    " : "│   "), last);
#endif
		}
	}

	void TestChamber::printNodesTree() {
		std::map<NodeID, std::vector<NodeID>> children;
		NodeID root = 0;

		for (const auto& [id, node] : m_nodeMap) {
			if (node.m_parentID.has_value()) {
				children[*node.m_parentID].push_back(id);
			} else {
				root = id;  // fallback assumption
			}
		}

		_printNodesTree(children, root, "", true);
	}

	// TODO finish this function
	void TestChamber::unsubdivideNode(NodeID node) {
		// a node can be unsubdivided only all its children have 
		//  1. same number of children (either they all have or none)
		//  2. same active state
		//  3. its neighbor nodes have same active state
		int size = m_nodeMap.at(node).m_size;
		std::optional<NodeID> parentID = m_nodeMap.at(node).m_parentID;
		if (!parentID) return;
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
