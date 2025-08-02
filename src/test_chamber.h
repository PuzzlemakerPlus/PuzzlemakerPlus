// test_chamber.h
// Declares the class that represents the test chamber itself and the octree nodes
#ifndef PMP_TEST_CHAMBER_H
#pragma once

#include <map>
#include <optional>
#include "util/optional_array.h"

namespace PMP
{
	// In case we need to change this data type later
	using NodeID = unsigned int;

	// DO NOT USE THIS DIRECTLY, this is just used to initialize the value in the test chamber class
	const float defaultRootNodeSize = 8192.0f;

	class TestChamberNode
	{
	public:
		TestChamberNode( NodeID parentID ) :
			m_parentID(parentID)
		{}

		TestChamberNode() {} // Constructor without parent, used for root node only

		// Array itself being optional would be better, but std::optional doesn't seem to allow that
		PMP::OptionalArray<NodeID, 8> m_childIDs;
		//std::optional<NodeID> m_childIDs[8];
		std::optional<NodeID> m_parentID;
	};

	class TestChamber
	{
	public:
		TestChamber();

		void CreateChildrenForNode( NodeID parentID );
		// Recursive
		void DestroyChildrenForNode( NodeID parentID );

		void PrintNodes();

	private:
		// Add node, return its ID, and increment next node ID
		// std::nullopt for the parent ID means it's the root node
		// - not making this a default value so it is a deliberate programming choice
		NodeID CreateOctreeNode( std::optional<NodeID> parentID );

		std::map<NodeID, TestChamberNode> m_nodeMap;
		NodeID m_rootNode;

		// FIXME: needs logic to reuse old IDs so this doesn't overflow and break everything
		NodeID m_nextNodeId;

		// Size of the root node in hammer units - this is effectively the internal max map size
		float m_rootNodeWorldSize;
	};
}

#endif // !PMP_TEST_CHAMBER_H
