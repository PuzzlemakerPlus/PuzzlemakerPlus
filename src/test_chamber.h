// test_chamber.h
// Declares the class that represents the test chamber itself and the octree nodes
#pragma once

#include <map>
#include <vector>
#include <optional>
#include <qvectornd.h>
#include <unordered_map>

#include "test_chamber_types.h"

#include "util/3d/mesh.h"
#include "util/3d/ray.h"

namespace PMP
{
	// DO NOT USE THIS DIRECTLY, this is just used to initialize the value in the test chamber class
	const unsigned int defaultRootNodeSize = 8192;
	class Selector;

	//Based on the information in this struct, it is possible to identify a surface without knowing its parentID or surfaceID
	struct UniqueSurfaceIdentifer
	{
		uint size;
		QVector3D nodeCord;
		WALLSIDE side = WALLSIDE::NONE;
	};

	class TestChamber
	{
	public:
		TestChamber();

		void CreateChildrenForNode( NodeID parentID );
		// Recursive
		void DestroyChildrenForNode( NodeID parentID );

		unsigned int GetRootNodeSize() const {return m_rootNodeWorldSize;};
		int GetRootNodeSizeInt() const {return static_cast<int>(m_rootNodeWorldSize);};
		const std::unordered_map<NodeID, TestChamberNode>& GetNodeMap() const {return m_nodeMap;};
		std::optional<ChamberSurface> GetSurface(NodeID node, WALLSIDE side);
		void GetSurfaceChildren(std::vector<ChamberSurface>& out, NodeID node, WALLSIDE side);
		std::array<QVector3D, 4> GetChamberSurfaceCorners(const ChamberSurface& surface);
		
		bool IsOutOOB(QVector3D coord) const;
		bool IsSurface (WALLSIDE side, NodeID node) const;
		bool IsNodeActive(NodeID node) const {return m_nodeMap.at(node).m_active;};
		bool NodeHasChild(NodeID node) const {return m_nodeMap.at(node).m_childIDs.has_value();};

		// Find the smallest node that includes the give coordinate
		const NodeID SearchNodeWithCoord(QVector3D coord, NodeID parentID=0) const;
		// Find the node with specified size that includes the give coordinate
		const NodeID SearchNodeWithCoordSize(QVector3D coord, unsigned int size, NodeID parentID=0) const;
		std::optional<RayHitResult> RayCast(Ray ray) const;

		// Expand the tree until the coordinate is enclosed by a node of given size
		NodeID SubdivideNodeToCoord(QVector3D coord, NodeID parentID, unsigned int size);
		int SetNodeActive(NodeID node, bool active);

		void GenerateSurfaces();
		int ExtrudeSurfaceMultiple(Selector* selector);
		void GenerateQuads(std::vector<Quad>& out) const;
		
		int ExtrudeSurface(Selector* selector);
		int RecedeSurface(Selector* selector);

		void makeUSI (NodeID node, UniqueSurfaceIdentifer& out, WALLSIDE side = WALLSIDE::NONE);
		void calculateCenterPointsSurfaceDisplacement(const std::vector<ChamberSurface>& surfs, float dist, std::vector<QVector3D>& out);
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

		// Size of the root node in hammer units - this is effectively the internal max map size
		NodeID m_rootNode;
		unsigned int m_rootNodeWorldSize;
		
		std::unordered_map<NodeID, TestChamberNode> m_nodeMap;
		std::unordered_map<NodeID, std::array<ChamberSurface, 6>> m_surfaceMap;
	};

}
