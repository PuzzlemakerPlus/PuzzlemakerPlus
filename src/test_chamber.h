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

		void createChildrenOfNode( NodeID parentID );
		// Recursively destroys child of a node
		void destroyChildrenOfNode( NodeID parentID );

		unsigned int GetRootNodeSize() const {return m_rootNodeWorldSize;};
		int GetRootNodeSizeInt() const {return static_cast<int>(m_rootNodeWorldSize);};
		const std::unordered_map<NodeID, TestChamberNode>& getNodeMap() const {return m_nodeMap;};
		std::optional<ChamberSurface> getSurface(NodeID node, WALLSIDE side);
		void getSurfaceChildren(std::vector<ChamberSurface>& out, NodeID node, WALLSIDE side);
		std::array<QVector3D, 4> getChamberSurfaceCorners(const ChamberSurface& surface);
		QVector3D getSurfaceCenter(ChamberSurface surface);
		const std::unordered_map<NodeID, TestChamberNode>& getNodes() const {return m_nodeMap;}
		const std::unordered_map<NodeID, std::array<ChamberSurface, 6>>& getSurfaces() const {return m_surfaceMap; }
		
		// Is the given coordinate out of bounds?
		bool isOutOOB(QVector3D coord) const;
		// Is there surface on the given side of the node?
		bool isSurface (WALLSIDE side, NodeID node) const;
		bool isNodeActive(NodeID node) const {return m_nodeMap.at(node).m_active;};
		bool nodeHasChild(NodeID node) const {return m_nodeMap.at(node).m_childIDs.has_value();};

		// Find the smallest node that includes the give coordinate
		const NodeID searchNodeWithCoord(QVector3D coord, NodeID parentID=0) const;
		// Find the node with specified size that includes the give coordinate
		const NodeID searchNodeWithCoordSize(QVector3D coord, unsigned int size, NodeID parentID=0) const;
		std::optional<RayHitResult> rayCast(Ray ray) const;

		// Expand the tree until the coordinate is enclosed by a node of given size
		NodeID subdivideNodeToCoord(QVector3D coord, NodeID parentID, unsigned int size);
		int setNodeActive(NodeID node, bool active);

		// Based on the octree structure, generate surfaces
		void generateSurfaces();
		// Based on generated surfaces, generate quads for rendering. This is to separate rendering logic with octree logic
		void generateQuads(std::vector<Quad>& out) const;
		void generateTCQuads(std::vector<TestChamberQuad>& out) const;

		// Extrude all surfaces selected
		int extrudeSurfaces(Selector* selector);
		// Receve all surfaces selected
		int recedeSurfaces(Selector* selector);
		// Creates a UniqueSurfaceIdentifer for the given node and side.
		// USI should be the standard to long term description of a node to remove dependency on NodeID and SurfaceID
		void makeUSI (NodeID node, UniqueSurfaceIdentifer& out, WALLSIDE side = WALLSIDE::NONE);
		// Offsets each surface's parent node center along its normal by (node size * dist) and stores the results.
		void calculateCenterPointsSurfaceDisplacement(const std::vector<ChamberSurface>& surfs, float dist, std::vector<QVector3D>& out);

		// Debug
		void printNodes();
		void printNodesTree();
		void testChildNodeGeneration();
		
	private:
		void findFreeIDs(std::array<NodeID,8>& out);
		// std::nullopt for the parent ID means it's the root node
		// - not making this a default value so it is a deliberate programming choice
		void createOctreeNode(NodeID newID, std::optional<NodeID> parentID, QVector3D center, unsigned int size);
		void _printNodesTree(std::map<NodeID, std::vector<NodeID>>& children, NodeID id, std::string prefix, bool isLast);

		// Size of the root node in hammer units - this is effectively the internal max map size
		NodeID m_rootNode;
		unsigned int m_rootNodeWorldSize;
		
		std::unordered_map<NodeID, TestChamberNode> m_nodeMap;
		std::unordered_map<NodeID, std::array<ChamberSurface, 6>> m_surfaceMap;
	};

}
