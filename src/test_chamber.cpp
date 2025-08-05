// octree_test.cpp
// Implements the class that represents the test chamber itself and the octree nodes
#include "test_chamber.h"
#include "util/3d/mesh.h"
#include <array>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <qvectornd.h>
#include "util/3d/mesh.h"

namespace PMP
{
	QVector3D GetWallFaceCenter(QVector3D center, unsigned int size, QVector3D normal)
	{
		return center + size/2 * -normal;
	}

	// TestChamber function implementations
	// ==============================================
	// TODO: This has been modified to use PMP::OptionalArray, but since it's unused it's currently untested. <- Added ctest for this 

	TestChamber::TestChamber() :
		m_rootNodeWorldSize(defaultRootNodeSize),
		m_nextNodeId(0)
	{
		// Create the root node
		// Currently, this will always be node ID 0, but explicitly storing it seems like a good idea
		CreateOctreeNode(0, std::nullopt, {0,0,0}, m_rootNodeWorldSize);
		m_rootNode = 0;
	}

	// Given a coordinate, compare against the parent coordinate to find the order at which the child node is placed in the child array
	unsigned int getChildOrder(QVector3D coord, unsigned int parentSize, QVector3D parentCenter)
	{
		if (parentSize % 4 != 0)
			throw std::runtime_error("Parent node size must be divisible by 4");

		int offset = parentSize / 4;
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
		if (parentSize % 4 != 0)
			throw std::runtime_error("Parent node size must be divisible by 4");
		if (order > 7)
			throw std::runtime_error("Order can not be greater than 7");

		int offset = parentSize / 4;
		// Branchless calculation
		int xSign = ((order & 1) << 1) - 1;              // 1 if odd, -1 if even
		int ySign = (((order >> 1) & 1) << 1) - 1;        // 1 if bit 1 is 0, -1 if 1
		int zSign = (((order >> 2) & 1) << 1) - 1;        // 1 if bit 2 is 0, -1 if 1

		return parentCenter + QVector3D(offset * xSign, offset * ySign, offset * zSign);
	}

	void TestChamber::CreateChildrenForNode( NodeID parentID )
	{
		TestChamberNode& parentNode = m_nodeMap[parentID];
		std::array<NodeID, 8>& parentChildIds = parentNode.m_childIDs.value_initialize();
		std::array<NodeID, 8> freeIDs;
		FindFreeIDs(freeIDs);
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
			CreateOctreeNode(freeIDs[i],parentID, getChildCenter(i,parentNode.m_size, parentNode.m_center), parentNode.m_size/2); //NOTE: this code currently does not support forming a separate room.
			parentChildIds[i] = freeIDs[i];
		}
	}

	void TestChamber::DestroyChildrenForNode( NodeID parentID )
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
					DestroyChildrenForNode(childID);

				m_nodeMap.erase(childID);
			}

			parentNode.m_childIDs.reset();
		}
	}

	void TestChamber::CreateOctreeNode(NodeID newID, std::optional<NodeID> parentID, QVector3D center, unsigned int size)
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
	// GET O(n^2) SEARCH TIME TO FINDING FREE ID
	void TestChamber::FindFreeIDs(std::array<NodeID, 8>& out)
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

	std::array<QVector3D, 6> GetOuterFaceCoords(const unsigned int nodeSize, const QVector3D nodeCenter, float epsilon = 0.5f)
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


	void TestChamber::SetNodeActive(NodeID node, bool active) 
	{
		m_nodeMap[node].m_active = active;
		// Also need to make sure all adjacent nodes have matching size.
		std::array<QVector3D, 6> outerCoords = GetOuterFaceCoords(m_nodeMap[node].m_size, m_nodeMap[node].m_center);
		for(const auto& coord : outerCoords)
		{
			SubdivideNodeToCoord(coord, 0, m_nodeMap[node].m_size);
		}
	}

	NodeID TestChamber::SubdivideNodeToCoord(QVector3D coord, NodeID parentID, unsigned int size)
	{
		if(IsOutOOB(coord)) return parentID;
		if(m_nodeMap[parentID].m_size < size) throw std::runtime_error("Something went wrong trying to subdivide node");
		if(m_nodeMap[parentID].m_size == size) return parentID;
		unsigned int i = getChildOrder(coord, m_nodeMap[parentID].m_size, m_nodeMap[parentID].m_center);
		if(!m_nodeMap[parentID].m_childIDs.has_value())
		{
			CreateChildrenForNode(parentID);
		}
		return SubdivideNodeToCoord(coord,  m_nodeMap[parentID].m_childIDs.value()[i], size);
	}

	std::array<QVector3D, 4> TestChamber::GetChamberSurfaceCorners(const ChamberSurface& surface)
	{
		const TestChamberNode& node = m_nodeMap.at(surface.parentNode);
		float halfSize = node.m_size / 2.0f;

		QVector3D normal = GetWallNormal(surface.side);
		QVector3D center = GetWallFaceCenter(node.m_center, node.m_size, normal);
		QVector3D up     = GetWallUp(surface.side);
		QVector3D right  = QVector3D::crossProduct(normal, up);

		// Quad corners: counter-clockwise
		QVector3D p0 = center + (-right - up) * halfSize;  // bottom-left
		QVector3D p1 = center + ( right - up) * halfSize;  // bottom-right
		QVector3D p2 = center + ( right + up) * halfSize;  // top-right
		QVector3D p3 = center + (-right + up) * halfSize;  // top-left

		return {p0, p1, p2, p3};
	}

	//Given a wallside of a node, should this wall be drawn
	bool TestChamber::IsSurface(WALLSIDE side, NodeID node) const
    {
		const QVector3D center = m_nodeMap.at(node).m_center;
        const float halfSize = m_nodeMap.at(node).m_size / 2.0f;
        const float epsilon = 0.4f;
        QVector3D offset;

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

        bool isVisible = IsOutOOB(neighborCoord); //if coordinate behind this surface is OOB, this surface should be drawn
        if (!isVisible) { // the coordinate behind this surface is in bound
            NodeID neighborId = SearchNodeWithCoord(neighborCoord);
            isVisible = !IsNodeActive(neighborId); // check if the node behind this surface is inactive
        }
		return isVisible;
    }

	
	std::optional<RayHitResult> TestChamber::RayCast(Ray ray) const
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
				QVector3D normal = GetWallNormal(surface.side);
				QVector3D center = GetWallFaceCenter(node.m_center, node.m_size, normal);
				QVector3D up = GetWallUp(surface.side);
				QVector3D right = QVector3D::crossProduct(normal, up);
				float halfSize = node.m_size / 2.0f;

				QVector3D p0 = center + (-right - up) * halfSize;
				QVector3D p1 = center + ( right - up) * halfSize;
				QVector3D p2 = center + ( right + up) * halfSize;
				QVector3D p3 = center + (-right + up) * halfSize;

				float tTemp;
				QVector3D hitPosTemp;

				if (RayIntersectsQuad(ray, p0, p1, p2, p3, tTemp, hitPosTemp))
				{
					if (tTemp > 0.0f && tTemp < closestT)
					{
						closestT = tTemp;
						closestHit = RayHitResult{surface.parentNode,surface, hitPosTemp};
					}
				}
			}
		}

		return closestHit;
	}
	// Is the given coordinate out of bounds?
	bool TestChamber::IsOutOOB(QVector3D coord) const
	{
		int halfSize = m_rootNodeWorldSize / 2;
		return std::abs(coord.x()) > halfSize || std::abs(coord.y()) > halfSize || std::abs(coord.z()) > halfSize;
	}

	const NodeID TestChamber::SearchNodeWithCoord(QVector3D coord, NodeID parentID) const
	{
		if(IsOutOOB(coord)) throw std::runtime_error("Coordinate Out Of Bounds");
		if(!m_nodeMap.at(parentID).m_childIDs.has_value()) return parentID; //found

		unsigned int i = getChildOrder(coord, m_nodeMap.at(parentID).m_size, m_nodeMap.at(parentID).m_center);
		return SearchNodeWithCoord(coord,  m_nodeMap.at(parentID).m_childIDs.valueRead().at(i));
	}

	// Based on generated surfaces, generate quads for rendering
	// This is to separate rendering logic with octree logic (Octree does not need to know vertices of individual tris)
	void TestChamber::GenerateQuads(std::vector<Quad>& out) const
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

	// Based on the node structure, generate surfaces
	void TestChamber::GenerateSurfaces()
    {
		m_surfaceMap.clear();
        //std::cout << "Generating Surfaces for test chamber "<< std::endl;
        for (const auto& [id, node] : GetNodeMap()) {
            if (!IsNodeActive(id) || NodeHasChild(id)) continue;
            //std::cout << "Generating Surfaces for Node " << id << std::endl;
            const QVector3D& center = node.m_center;
            
			std::array<ChamberSurface, 6>& walls = m_surfaceMap[id];
            for (int i = 0; i < 6; ++i) {
                WALLSIDE side = static_cast<WALLSIDE>(i);
                if(IsSurface(side,id))
                {
					walls[i] = {side,id};
                }
            }
        }
        std::cout << "Generated " << m_surfaceMap.size() << " Surfaces" << std::endl;
    }
	void TestChamber::PrintNodes()
	{
		std::cout << "TestChamber has " << m_nodeMap.size() << " Node(s)\n";
		for (const auto& [nodeId, node] : m_nodeMap) {
			if(node.m_parentID.has_value()){
				std::cout << "NodeID: " << nodeId << ", child of " << node.m_parentID.value() << "\n";
			}
			else {
				std::cout << "NodeID: " << nodeId << " has no parentID" << "\n";
			}
    	}
	}
	const char* printVector3D(QVector3D in)
	{
		static char buffer[64];
		snprintf(buffer, sizeof(buffer), "(%.2f, %.2f, %.2f)", in.x(), in.y(), in.z());
		return buffer;	
	}

	void TestChamber::_PrintNodesTree(std::map<NodeID, std::vector<NodeID>>& children, NodeID id, std::string prefix, bool isLast) 
	{
		std::cout << prefix;

		if (!prefix.empty()) {
			std::cout << (isLast ? "└── " : "├── ");
		}

		std::cout << "Node " << id << " at " << printVector3D(m_nodeMap[id].m_center) << " with size " << m_nodeMap[id].m_size << "\n";
		

		const auto& childVec = children[id];
		for (size_t i = 0; i < childVec.size(); ++i) {
			bool last = (i == childVec.size() - 1);
			_PrintNodesTree(children, childVec[i], prefix + (isLast ? "    " : "│   "), last);
		}
	}

	void TestChamber::PrintNodesTree() {
		std::map<NodeID, std::vector<NodeID>> children;
		NodeID root = 0;

		for (const auto& [id, node] : m_nodeMap) {
			if (node.m_parentID.has_value()) {
				children[*node.m_parentID].push_back(id);
			} else {
				root = id;  // fallback assumption
			}
		}

		_PrintNodesTree(children, root, "", true);
	}

	void TestChamber::TestChildNodeGeneration()
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
