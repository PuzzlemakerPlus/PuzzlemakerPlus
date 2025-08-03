// test_chamber.h
// Declares the class that represents the test chamber itself and the octree nodes
#ifndef PMP_TEST_CHAMBER_H
#pragma once

#include <map>
#include <optional>
#include <qvectornd.h>
#include "util/optional_array.h"

namespace PMP
{
	// In case we need to change this data type later
	using NodeID = unsigned int;

	// DO NOT USE THIS DIRECTLY, this is just used to initialize the value in the test chamber class
	const unsigned int defaultRootNodeSize = 8192;

	class TestChamberNode
	{
	public:
		TestChamberNode( NodeID parentID, QVector3D center, unsigned int size, bool active) :
			m_parentID(parentID),
			m_center(center),
			m_size(size),
			m_active(active)
		{}

		TestChamberNode(QVector3D center, unsigned int size) :
			m_center(center),
			m_size(size),
			m_active(true)
		{} // Constructor without parent, used for root node only
		
		TestChamberNode() :
			m_parentID(std::nullopt),
			m_center(0.0f, 0.0f, 0.0f),
			m_size(0),
			m_active(false)
		{}

		// Array itself being optional would be better, but std::optional doesn't seem to allow that
		PMP::OptionalArray<NodeID, 8> m_childIDs;
		//std::optional<NodeID> m_childIDs[8];
		std::optional<NodeID> m_parentID;
		
		QVector3D m_center;
		unsigned int m_size;
		bool m_active;
	};

	class TestChamber
	{
	public:
		TestChamber();

		void CreateChildrenForNode( NodeID parentID );
		// Recursive
		void DestroyChildrenForNode( NodeID parentID );

		// Find the smallest node that includes the give coordinate
		const NodeID SearchNodeWithCoord(QVector3D coord, NodeID parentID=0) const;
		bool IsOutOOB(QVector3D coord) const;
		bool IsNodeActive(NodeID node) const {return m_nodeMap.at(node).m_active;};
		bool NodeHasChild(NodeID node) const {return m_nodeMap.at(node).m_childIDs.has_value();};
		unsigned int GetRootNodeSize() const {return m_rootNodeWorldSize;};
		int GetRootNodeSizeInt() const {return static_cast<int>(m_rootNodeWorldSize);};
		const std::map<NodeID, TestChamberNode>& GetNodeMap() const {return m_nodeMap;};
		std::optional<NodeID> RayCast(QVector3D origin, QVector3D dir, QVector3D& out, NodeID parentID = 0) const;

		// Expand the tree until the coordinate is enclosed by a node of given size
		NodeID SubdivideNodeToCoord(QVector3D coord, NodeID parentID, unsigned int size);
		void SetNodeActive(NodeID node, bool active);

		// Debug
		void PrintNodes();
		void PrintNodesTree();
		void TestChildNodeGeneration();
		
	private:
		// Since we always assign 8 new nodes at a time, it is better to find the free ids before adding new nodes
		// Thus, CreateOctreeNode was changed to require the newID parameter
		void FindFreeIDs(std::array<NodeID,8>& out);
		// std::nullopt for the parent ID means it's the root node
		// - not making this a default value so it is a deliberate programming choice
		void CreateOctreeNode(NodeID newID, std::optional<NodeID> parentID, QVector3D center, unsigned int size);
		
		void _PrintNodesTree(std::map<NodeID, std::vector<NodeID>>& children, NodeID id, std::string prefix, bool isLast);

		std::map<NodeID, TestChamberNode> m_nodeMap;
		NodeID m_rootNode;

		// FIXME: needs logic to reuse old IDs so this doesn't overflow and break everything
		NodeID m_nextNodeId;

		// Size of the root node in hammer units - this is effectively the internal max map size
		unsigned int m_rootNodeWorldSize;
	};
}

#endif // !PMP_TEST_CHAMBER_H
