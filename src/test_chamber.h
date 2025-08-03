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
	using SurfaceID = unsigned int;
	using SurfaceType = unsigned int; // Surface type 0 is air

	// DO NOT USE THIS DIRECTLY, this is just used to initialize the value in the test chamber class
	const float defaultRootNodeSize = 8192.0f;

	// Nodes should be aware which octrant it occupies within the parent node's space
	// This will help figuring out which surfaces are being drawn and how rooms are formed
	enum OCTRANT {
		FULL, // A node without parent. ie the root node
		P_X_POS_Y_POS_Z,
		N_X_POS_Y_POS_Z,
		P_X_NEG_Y_POS_Z,
		N_X_NEG_Y_POS_Z,
		P_X_POS_Y_NEG_Z,
		N_X_POS_Y_NEG_Z,
		P_X_NEG_Y_NEG_Z,
		N_X_NEG_Y_NEG_Z,
	};

	class TestChamberNode
	{
	public:
		TestChamberNode( NodeID parentID, OCTRANT oct) :
			m_parentID(parentID),
			m_oct(oct)
		{}

		TestChamberNode() {} // Constructor without parent, used for root node only
		
		// Array itself being optional would be better, but std::optional doesn't seem to allow that
		PMP::OptionalArray<NodeID, 8> m_childIDs;
		//std::optional<NodeID> m_childIDs[8];
		std::optional<NodeID> m_parentID;

		OCTRANT m_oct;

		// since a node represents a 3d space, we need to know how its 6 sides are defined to draw the 3d space
	};

	class TestChamber
	{
	public:
		TestChamber();

		void CreateChildrenForNode( NodeID parentID );
		// Recursive
		void DestroyChildrenForNode( NodeID parentID );

		// Debug
		void PrintNodes();
		void PrintNodesTree();

	private:
		// Since we always assign 8 new nodes at a time, it is better to find the free ids before adding new nodes
		// Thus, CreateOctreeNode was changed to require the newID parameter
		void FindFreeIDs(std::array<NodeID,8>& out);
		// std::nullopt for the parent ID means it's the root node
		// - not making this a default value so it is a deliberate programming choice
		void CreateOctreeNode(NodeID newID, std::optional<NodeID> parentID, OCTRANT oct);

		std::map<NodeID, TestChamberNode> m_nodeMap;
		NodeID m_rootNode;

		// FIXME: needs logic to reuse old IDs so this doesn't overflow and break everything
		NodeID m_nextNodeId;

		// Size of the root node in hammer units - this is effectively the internal max map size
		float m_rootNodeWorldSize;
		
	};
}

#endif // !PMP_TEST_CHAMBER_H
