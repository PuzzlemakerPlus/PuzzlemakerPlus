// octree_test.cpp
// Implements the class that represents the test chamber itself and the octree nodes
#include "test_chamber.h"

#include <array>

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
		m_rootNode = CreateOctreeNode( std::nullopt );
	}

	void TestChamber::CreateChildrenForNode( NodeID parentID )
	{
		TestChamberNode& parentNode = m_nodeMap[parentID];
		std::array<NodeID, 8>& parentChildIds = parentNode.m_childIDs.value();
		for ( int i = 0; i < 8; ++i )
		{
			parentChildIds[i] = CreateOctreeNode(parentID);
		}
	}

	void TestChamber::DestroyChildrenForNode( NodeID parentID )
	{
		TestChamberNode& parentNode = m_nodeMap[parentID];
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

	NodeID TestChamber::CreateOctreeNode( std::optional<NodeID> parentID )
	{
		NodeID newID = m_nextNodeId;
		m_nextNodeId++;

		if ( parentID.has_value() )
			m_nodeMap.emplace(newID, TestChamberNode( parentID.value() ));
		else
			m_nodeMap.emplace(newID, TestChamberNode());

		return newID;
	}
}
