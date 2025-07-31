// test_chamber.h
// Declares the class that represents the test chamber itself and the octree nodes
#ifndef PMP_TEST_CHAMBER_H
#pragma once

#include <map>
#include <optional>
#include <memory>

namespace PMP
{
	// DO NOT USE THIS DIRECTLY, this is just used to initialize the value in the test chamber class
	const float defaultRootNodeSize = 8192.0f;

	class TestChamberNode
	{
	public:
		TestChamberNode( int parentID ) :
			m_parentNode(parentID)
		{}

		TestChamberNode() {} // Constructor without parent, used for root node only

		// Array itself being optional would be better, but std::optional doesn't seem to allow that
		std::optional<int> m_childNodes[8];
		std::optional<int> m_parentNode;
	};

	class TestChamber
	{
	public:
		TestChamber();

		void CreateChildrenForNode( int parentID );
		// TODO
		//void DestroyChildrenForNode( int parentID );

	private:
		// Add node, return its ID, and increment next node ID
		// std::nullopt for the parent ID means it's the root node
		// - not making this a default value so it is a deliberate programming choice
		int CreateOctreeNode( std::optional<int> parentID );

		std::map<int, TestChamberNode> m_nodeMap;
		int m_rootNode;

		// FIXME: needs logic to reuse old IDs so this doesn't overflow and break everything
		int m_nextNodeId;

		// Size of the root node in hammer units - this is effectively the internal max map size
		float m_rootNodeWorldSize;
	};
}

#endif // !PMP_TEST_CHAMBER_H