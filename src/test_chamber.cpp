// octree_test.cpp
// Implements the class that represents the test chamber itself and the octree nodes
#include "test_chamber.h"

#include <array>

namespace PMP
{
	// TestChamber function implementations
	// ==============================================

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
		for ( int i = 0; i < 8; ++i )
		{
			parentNode.m_childIDs[i] = CreateOctreeNode(parentID);
		}
	}

	void TestChamber::DestroyChildrenForNode( NodeID parentID )
	{
		TestChamberNode& parentNode = m_nodeMap[parentID];

		for ( int i = 0; i < 8; ++i )
		{
			NodeID childID = parentNode.m_childIDs[i].value();
			TestChamberNode& childNode = m_nodeMap[childID];

			// if any child node is present than all others should be and vice versa
			if ( childNode.m_childIDs[0].has_value() )
				DestroyChildrenForNode(childID);

			m_nodeMap.erase(childID);
			parentNode.m_childIDs[i].reset();
		}
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