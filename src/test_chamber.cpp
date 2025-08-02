// octree_test.cpp
// Implements the class that represents the test chamber itself and the octree nodes
#include "test_chamber.h"

#include <array>
#include <string>
#include <vector>
#include <iostream>

namespace PMP
{
	// TestChamber function implementations
	// ==============================================
	// TODO: This has been modified to use PMP::OptionalArray, but since it's unused it's currently untested.

	TestChamber::TestChamber() :
		m_rootNodeWorldSize(defaultRootNodeSize),
		m_nextNodeId(0)
	{
		// Create the root node
		// Currently, this will always be node ID 0, but explicitly storing it seems like a good idea
		m_rootNode = CreateOctreeNode(0, std::nullopt );
	}

	void TestChamber::CreateChildrenForNode( NodeID parentID )
	{
		TestChamberNode& parentNode = m_nodeMap[parentID];
		std::array<NodeID, 8>& parentChildIds = parentNode.m_childIDs.value_initialize();
		std::array<NodeID, 8> freeIDs;
		FindFreeIDs(freeIDs);
		for ( int i = 0; i < 8; ++i )
		{
			parentChildIds[i] = CreateOctreeNode(freeIDs[i],parentID);
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

	NodeID TestChamber::CreateOctreeNode(NodeID newID, std::optional<NodeID> parentID )
	{

		if ( parentID.has_value() )
			m_nodeMap.emplace(newID, TestChamberNode( parentID.value() ));
		else
			m_nodeMap.emplace(newID, TestChamberNode());

		return newID;
	}
	void TestChamber::UpdateNextNodeID()
	{
		//This code will need to be updated to implement reusing old nodeIDs
		m_nextNodeId++;
		return;
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

	void _PrintNodesTree(std::map<NodeID, std::vector<NodeID>>& children, NodeID id, std::string prefix, bool isLast) 
	{
		std::cout << prefix;

		if (!prefix.empty()) {
			std::cout << (isLast ? "└── " : "├── ");
		}

		std::cout << "Node " << id << "\n";

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

}
