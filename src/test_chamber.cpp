// octree_test.cpp
// Implements the class that represents the test chamber itself and the octree nodes
#include "test_chamber.h"

#include <cassert>
#include <array>
#include <qvectornd.h>
#include <string>
#include <vector>
#include <iostream>

namespace PMP
{
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
		//posx posy posz
		//negx posy posz
		//posx negy posz
		//negx negy posz
		//posx posy negz
		//negx posy negz
		//posx negy negz
		//negx negy negz
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
			m_nodeMap.emplace(newID, TestChamberNode( parentID.value(), center, size));
		else
			m_nodeMap.emplace(newID, TestChamberNode(center, size));
	}

	void TestChamber::FindFreeIDs(std::array<NodeID,8>& out)
	{
		for ( int i = 0; i < 8; ++i )
		{
			NodeID minId = 0;
			for (const auto& [nodeId, node] : m_nodeMap) {
				if(minId == nodeId)
				{
					minId = nodeId + 1;
				}
				for(int j = 0; j <= i;++j)
				{
					if(minId == out[j])
					{
						minId = out[j] + 1;
					}
				}
			}
			out[i] = minId;
		}
		// for ( int i = 0; i < 8; ++i )
		// {
		// 	for (const auto& [nodeId, node] : m_nodeMap) {
		// 		if(out[i] == nodeId)
		// 		{
		// 			throw std::range_error("Error Finding Free ID for new nodes");
		// 		}
		// 	}
		// }
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
