// octree_test.cpp
// Implements the class that represents the test chamber itself and the octree nodes
#include "undo.h"
#include "selection.h"
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
	QVector3D getWallFaceCenter(QVector3D center, unsigned int size, QVector3D normal)
	{
		return center + size/2 * -normal;
	}

	TestChamber::TestChamber() :
		m_rootNodeWorldSize(defaultRootNodeSize)
	{
		// Create the root node
		// Currently, this will always be node ID 0, but explicitly storing it seems like a good idea
		createOctreeNode(0, std::nullopt, {0,0,0}, m_rootNodeWorldSize);
		m_rootNode = 0;
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
		TestChamberNode& parentNode = m_nodeMap[parentID];
		std::array<NodeID, 8>& parentChildIds = parentNode.m_childIDs.value_initialize();
		
		std::array<NodeID, 8> freeIDs;
		findFreeIDs(freeIDs);
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
			createOctreeNode(freeIDs[i],parentID, getChildCenter(i,parentNode.m_size, parentNode.m_center), parentNode.m_size/2); //NOTE: this code currently does not support forming a separate room.
			parentChildIds[i] = freeIDs[i];
		}
		parentNode.m_active = true; // Make sure the parent node is active after creating children
	}

	void TestChamber::destroyChildrenOfNode( NodeID parentID )
	{
		TestChamberNode& parentNode = m_nodeMap[parentID];
		if(parentNode.m_childIDs.has_value())
		{
			std::array<NodeID, 8>& parentChildIds = parentNode.m_childIDs.value();

			for ( int i = 0; i < 8; ++i )
			{
				NodeID childID = parentChildIds[i];
				TestChamberNode& childNode = m_nodeMap[childID];

				// if any child node is present than all others should be and vice versa
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
			m_nodeMap.emplace(newID, TestChamberNode( parentID.value(), center, size, m_nodeMap[parentID.value()].m_active));
		else
			m_nodeMap.emplace(newID, TestChamberNode(center, size));
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
		float halfSize = nodeSize + epsilon;
		return {
			nodeCenter + QVector3D(halfSize, 0.0f, 0.0f),  // +X
			nodeCenter + QVector3D(-halfSize, 0.0f, 0.0f),  // -X
			nodeCenter + QVector3D(0.0f,  halfSize, 0.0f),  // +Y
			nodeCenter + QVector3D(0.0f, -halfSize, 0.0f),  // -Y
			nodeCenter + QVector3D(0.0f, 0.0f,  halfSize),  // +Z
			nodeCenter + QVector3D(0.0f, 0.0f, -halfSize)   // -Z
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
		if(m_nodeMap[node].m_active == active) return 0; // State of the node has not changed

		m_nodeMap[node].m_active = active;
		// Also need to make sure all adjacent nodes have matching size.
		std::array<QVector3D, 6> outerCoords = getNeighborNodeCenters(m_nodeMap[node].m_size, m_nodeMap[node].m_center);
		for(const auto& coord : outerCoords)
		{
			subdivideNodeToCoord(coord, 0, m_nodeMap[node].m_size);
		}
		return 1; // State of the node has changed
	}

	std::optional<ChamberSurface> TestChamber::getSurface(NodeID node, WALLSIDE side)
	{
		ChamberSurface surf = m_surfaceMap[node][static_cast<int>(side)];
		if(surf.side == WALLSIDE::NONE) return std::nullopt;
		return std::optional<ChamberSurface>(surf);
	}

	//Assumes out is empty during non-recursive calls
	void TestChamber::getSurfaceChildren(std::vector<ChamberSurface>& out, NodeID node, WALLSIDE side)
	{
		// Node has children
		if (m_nodeMap[node].m_childIDs.has_value())
		{
			// Iterate through all children and get their surfaces
			for (const auto& childID : m_nodeMap[node].m_childIDs.valueRead())
			{
				getSurfaceChildren(out, childID, side);
			}
		}
		else
		{
			// No children, so this node is the leaf node
			ChamberSurface surf = m_surfaceMap[node][static_cast<int>(side)];
			if (surf.side != WALLSIDE::NONE) out.push_back(surf);
		}
	}

	NodeID TestChamber::subdivideNodeToCoord(QVector3D coord, NodeID parentID, unsigned int size)
	{
		if(isOutOOB(coord)) return parentID;
		if(m_nodeMap[parentID].m_size < size) throw std::runtime_error("Something went wrong trying to subdivide node");
		if(m_nodeMap[parentID].m_size == size) return parentID;
		unsigned int i = getChildOrder(coord, m_nodeMap[parentID].m_center);
		if(!m_nodeMap[parentID].m_childIDs.has_value()) createChildrenOfNode(parentID);
		return subdivideNodeToCoord(coord,  m_nodeMap[parentID].m_childIDs.value()[i], size);
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
		return m_nodeMap[surface.parentNode].m_size * 0.5f * getWallNormal(surface.side) * -1 + m_nodeMap[surface.parentNode].m_center;
		
	}
	//Given a wallside of a node, should this wall be drawn
	bool TestChamber::isSurface(WALLSIDE side, NodeID node) const
    {
		const QVector3D center = m_nodeMap.at(node).m_center;
        const float halfSize = m_nodeMap.at(node).m_size / 2.0f;
        const float epsilon = 0.4f;
        QVector3D offset;
		using enum WALLSIDE;
        switch (side) {
            case POS_X: offset = {+halfSize + epsilon, 0, 0}; break;
            case NEG_X: offset = {-halfSize - epsilon, 0, 0}; break;
            case POS_Y: offset = {0, +halfSize + epsilon, 0}; break;
            case NEG_Y: offset = {0, -halfSize - epsilon, 0}; break;
            case POS_Z: offset = {0, 0, +halfSize + epsilon}; break;
            case NEG_Z: offset = {0, 0, -halfSize - epsilon}; break;
            case NONE: throw std::runtime_error("Invalid WALLSIDE");
        }

        QVector3D neighborCoord = center + offset;

        bool isVisible = isOutOOB(neighborCoord); //if coordinate behind this surface is OOB, this surface should be drawn
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
				if(surface.side == WALLSIDE::NONE) continue;
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
	bool TestChamber::isOutOOB(QVector3D coord) const
	{
		int halfSize = m_rootNodeWorldSize / 2;
		return std::abs(coord.x()) > halfSize || std::abs(coord.y()) > halfSize || std::abs(coord.z()) > halfSize;
	}
	const NodeID TestChamber::searchNodeWithCoord(QVector3D coord, NodeID parentID) const
	{
		if(isOutOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if(!m_nodeMap.at(parentID).m_childIDs.has_value()) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		return searchNodeWithCoord(coord,  m_nodeMap.at(parentID).m_childIDs.valueRead().at(i));
	}
	const NodeID TestChamber::searchNodeWithCoordSize(QVector3D coord, unsigned int size, NodeID parentID) const
	{
		if(isOutOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if(m_nodeMap.at(parentID).m_size == size) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_center);
		return searchNodeWithCoordSize(coord, size, m_nodeMap.at(parentID).m_childIDs.valueRead().at(i));
	}
	void TestChamber::generateQuads(std::vector<Quad>& out) const
	{
		if(m_surfaceMap.size() == 0) throw std::runtime_error("Attempted to generate quads for test chamber with no surface. Forgot to call GenerateSurfaces()?");
		for (const auto& [_, surfaces] : m_surfaceMap)
		{
			for(const auto& surface : surfaces)
			{
				if(surface.side == WALLSIDE::NONE) continue;
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
				if(surface.side == WALLSIDE::NONE) continue;
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
            
			std::array<ChamberSurface, 6>& walls = m_surfaceMap[id];
            for (int i = 0; i < 6; ++i) {
                WALLSIDE side = static_cast<WALLSIDE>(i);
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
		std::vector<ChamberSurface> surfSel;
		selector->getSurfaceSelectionMultiple(surfSel);
		if (!surfSel.size()) return 0; // No selection

		// For building undoable action
		std::vector<UniqueSurfaceIdentifer> extrudeUSIs;
		std::vector<UniqueSurfaceIdentifer> newSelectUSIs;

		std::vector<WALLSIDE> extrudeSides;
		for (const auto& s : surfSel) {
			qDebug() << "Extruding surface with parent node ID: " << s.parentNode << " and side: " << static_cast<int>(s.side);
			if (s.side != WALLSIDE::NONE) {
				UniqueSurfaceIdentifer extrudeUSI;
				makeUSI(s.parentNode, extrudeUSI, s.side);
				extrudeUSIs.push_back(extrudeUSI);
				extrudeSides.push_back(s.side);
			}
			else throw std::runtime_error("HOW DID YOU SELECT AIR");
		}
		std::vector<QVector3D> centerPoints;
		calculateCenterPointsSurfaceDisplacement(surfSel, 1.0f, centerPoints);
		int selectionCount = 0;
		for (int i = 0; i < surfSel.size(); i++) {
			if(setNodeActive(surfSel[i].parentNode, false)) selectionCount++;
		}
		generateSurfaces();
		selector->clearSurfaces();
		// Extruded all surfaces.
		// Now we need to find new surfaces to select
		for (int i = 0; i < centerPoints.size(); i++)
		{
			if(isOutOOB(centerPoints[i])) continue; // Skip if out of bounds
			// Look for a node of the same size (to find all its child nodes as well)
			const NodeID node = searchNodeWithCoordSize(centerPoints[i], m_nodeMap[surfSel[i].parentNode].m_size);
			std::vector<ChamberSurface> surfs;
			getSurfaceChildren(surfs, node, extrudeSides[i]);

			if (surfs.size() == 0) continue;

			selector->addSelectionMultiple(surfs);
			// For each newly selectable surface, build a USI
			for (const auto& s : surfs) {
				UniqueSurfaceIdentifer usi;
				makeUSI(s.parentNode, usi, s.side);
				newSelectUSIs.push_back(usi);
			}
		}
		if (newSelectUSIs.size() > 0) UndoStack::instance().addAction(std::make_unique<TCExtrudeAction>(this, selector, extrudeUSIs, newSelectUSIs));
		else UndoStack::instance().addAction(std::make_unique<TCExtrudeAction>(this, selector, extrudeUSIs));

		// If any node was able to extrude, we consider success
		return selectionCount > 0;
	}
	int TestChamber::recedeSurfaces(Selector* selector)
	{
		std::vector<ChamberSurface> surfSel;
		selector->getSurfaceSelectionMultiple(surfSel);
		if (!surfSel.size()) return 0; // No selection

		// For building undoable action
		std::vector<UniqueSurfaceIdentifer> recedeUSIs;
		std::vector<UniqueSurfaceIdentifer> newSelectUSIs;

		std::vector<WALLSIDE> extrudeSides;
		for (const auto& s : surfSel) {
			if (s.side == WALLSIDE::NONE) throw std::runtime_error("HOW DID YOU SELECT AIR");
			//Subdivide neighbors
			std::array<QVector3D, 6> outerCoords = getNeighborNodeCenters(m_nodeMap.at(s.parentNode).m_size, m_nodeMap.at(s.parentNode).m_center);
			for (const auto& coord : outerCoords)
			{
				subdivideNodeToCoord(coord, 0, m_nodeMap.at(s.parentNode).m_size);
			}
			extrudeSides.push_back(s.side);
			UniqueSurfaceIdentifer recedeUSI;
			makeUSI(s.parentNode, recedeUSI, s.side);
			recedeUSIs.push_back(recedeUSI);
		}
		std::vector<QVector3D> centerPoints;
		calculateCenterPointsSurfaceDisplacement(surfSel, -1.0f, centerPoints);
		int selectionCount = 0;
		for (int i = 0; i < centerPoints.size(); i++)
		{
			if(isOutOOB(centerPoints[i])) continue; // Skip if out of bounds
			// Look for a node of the same size (to find all its child nodes as well)
			const unsigned int parentSize = m_nodeMap[surfSel[i].parentNode].m_size;
			NodeID node = searchNodeWithCoord(centerPoints[i]);
			if (getNodeMap().at(node).m_size != parentSize)
			{
				// Subdivide this node to match the size of the neighboring node
				node = subdivideNodeToCoord(centerPoints[i], 0, parentSize); // Retarget the node to the newly subdivided node
			}
			if (setNodeActive(node, true)) // node state was changed (without this check, the node can recede already disabled node and fail to find selection face)
			{
				selectionCount++;
			}
		}
		
		generateSurfaces();
		selector->clearSurfaces();
		for (int i = 0; i < centerPoints.size(); i++)
		{
			NodeID node = searchNodeWithCoord(centerPoints[i]);
			std::optional<ChamberSurface> surf = getSurface(node, extrudeSides[i]);
			if (surf.has_value())
			{
				// For each newly selectable surface, build a USI
				const auto& s = surf.value();
				selector->addSelection(s);
				
			}
			// Recede can completely remove surfaces. Therefore we always record the newSelectUSI 
			UniqueSurfaceIdentifer usi;
			makeUSI(node, usi, extrudeSides[i]);
			newSelectUSIs.push_back(usi);
		}
		
		if (newSelectUSIs.size() > 0) UndoStack::instance().addAction(std::make_unique<TCRecedeAction>(this, selector, recedeUSIs, newSelectUSIs));
		else UndoStack::instance().addAction(std::make_unique<TCRecedeAction>(this, selector, recedeUSIs));
		return selectionCount > 0;
	}
	void TestChamber::makeUSI(NodeID nodeID, UniqueSurfaceIdentifer& out, WALLSIDE side)
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

		std::cout << "Node " << id << " at " << printVector3D(m_nodeMap[id].m_center) << " with size " << m_nodeMap[id].m_size << "\n";

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
	void TestChamber::testChildNodeGeneration()
	{
		TestChamberNode& root = m_nodeMap[0];
		const auto& children = root.m_childIDs.value();

		for (int i = 0; i < 8; ++i)
		{
			NodeID childID = children[i];
			if (!m_nodeMap.contains(childID))
			{
				std::cerr << "Child " << i << " was not created.\n";
				continue;
			}

			TestChamberNode& child = m_nodeMap[childID];

			if (child.m_size != m_rootNodeWorldSize / 2)
			{
				std::cerr << "Child " << i << " size incorrect: " << child.m_size << " (expected " << m_rootNodeWorldSize / 2 << ")\n";
			}

			// Compute expected center
			QVector3D expectedCenter = getChildCenter(i, m_rootNodeWorldSize, {0,0,0});

			if ((child.m_center - expectedCenter).length() > 0.001f)
			{
				std::cerr << "Child " << i << " center incorrect: "
						<< child.m_center.x() << "," << child.m_center.y() << "," << child.m_center.z()
						<< " (expected " << expectedCenter.x() << "," << expectedCenter.y() << "," << expectedCenter.z() << ")\n";
			}
		}
	}
}
