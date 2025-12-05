// test_chamber.h
// Declares the class that represents the test chamber itself and the octree nodes
#pragma once

#include <map>
#include <vector>
#include <optional>
#include <qvectornd.h>
#include <unordered_map>

#include "test_chamber_types.h"
#include "selection.h"

#include "util/math.h"
#include "util/3d/mesh.h"
#include "util/3d/ray.h"
#include "util/safemap.h"

namespace PMP
{
	// DO NOT USE THIS DIRECTLY, this is just used to initialize the value in the test chamber class
	// Actually this can totally be used directly pretty sure I wrote the comment above but idk why -asd
	// Kelsey: Lowered for now because 1024 is an easier value to work with
	const unsigned int defaultRootNodeSize = 1024; // was 8192

	const float gridSizeDefault = 32;
	const float gridSizeMin = 32;
	const float gridSizeMax = 256;

	//Based on the information in this struct, it is possible to identify a surface without knowing its parentID or surfaceID
	struct UniqueSurfaceIdentifer
	{
		uint size;
		IntVector3D nodeCord;
		WallSide side = WallSide::NONE;
	};

	class TestChamber
	{
	public:
		TestChamber();

		void createChildrenOfNode( NodeID parentID );
		// Recursively destroys child of a node
		void destroyChildrenOfNode( NodeID parentID );

		int getRootNodeSizeInt() const {return static_cast<int>(m_rootNodeWorldSize);};
		unsigned int getRootNodeSize() const {return m_rootNodeWorldSize;};
		QVector3D getNodeCenter(NodeID nodeID) const {return m_nodeMap.at(nodeID).m_center.toQVector3D();}
		const SafeUnorderedMap<NodeID, TestChamberNode>& getNodeMap() const {return m_nodeMap;};

		std::optional<ChamberSurface> getSurface(NodeID node, WallSide side) const;
		void getSurfaceChildren(std::vector<ChamberSurface>& out, NodeID node, WallSide side) const;
		bool coordInNode(QVector3D coord, NodeID nodeID) const;
		// Find the neighboring nodes of the given node in all 6 directions
		// The neighbors may be nullopt if they do not exist (if the node is at the edge of the octree)
		// The neighbors will either be the same size as the given node, or larger
		// Recursive function that guarantees to find neighbors
		void getNeighborNodes(NodeID node, std::array<std::optional<NodeID>, 6>& out) const;

		// Given a node, check if all its neighbors are the same size as itself
		// very cheap operation Try to use this instead of getNeighborNodes whenever possible
		bool areNeighborsSameSize(NodeID node) const;
		// Given a node, check if all its neighbors are active
		bool areNeighborsActive(NodeID node) const;

		// Uses the hash function to try to locate the node at the given center coordinate
		// Returns std::nullopt if there is no such node that has that exact center coordinate
		// very cheap operation
		// Will return the node even if it has children
		std::optional<NodeID> tryGetNodeAtCoord(IntVector3D coord) const;

		std::array<IntVector3D, 4> getChamberSurfaceCorners(const ChamberSurface& surface);
		IntVector3D getSurfaceCenter(ChamberSurface surface) const;
		const SafeUnorderedMap<NodeID, std::array<ChamberSurface, 6>>& getSurfaceMap() const {return m_surfaceMap; }
		
		// Is the given coordinate out of bounds?
		bool isPointOOB(QVector3D coord) const;
		bool isPointOOB(IntVector3D coord) const;
		
		// Is there surface on the given side of the node?
		bool isSurface (WallSide side, NodeID node) const;
		bool isNodeActive(NodeID node) const {return m_nodeMap.at(node).m_active;};
		bool nodeHasChild(NodeID node) const {return m_nodeMap.at(node).m_childIDs.has_value();};

		// Find the smallest node that includes the give coordinate
		// Recursive function that guarantees to find the node
		const NodeID searchNodeWithCoord(QVector3D coord) const;
		const NodeID searchNodeWithCoord(QVector3D coord, NodeID parentID) const;
		// Uses the hash function to try to locate the node that uses the given coord as the centere coordinate
		//const std::optional<NodeID> searchNodeWithCoord(IntVector3D coord) const;

		// Find the node with specified size that includes the give coordinate
		// Recursive function that guarantees to find the node
		const NodeID searchNodeWithCoordSize(const QVector3D coord, unsigned int size) const;
		const NodeID searchNodeWithCoordSize(const IntVector3D coord, unsigned int size) const;
		//Returned node may or may not have children
		//Returned node may be larger than the requested size if it does not have any child
		//Will return the smallest node that contains the coord if the node of exact size does not exist at the coordinate
		NodeID getNodeOfSizeAtCoord(QVector3D coord, unsigned int size) const;
		NodeID getNodeOfSizeAtCoord(QVector3D coord, NodeID parentID, unsigned int size) const;
		
		std::optional<RayHitResult> rayCast(Ray ray) const;

		// Returns mins and maxs of the node's bounding box: (-,-,-) and (+,+,+)
		// replaced with getNodeBBoxMin and getNodeBBoxMax
		// it's better not to initialize std::array every time we want this
		//void getNodeBBox(NodeID id, std::array<QVector3D, 2>& out);

		// Returns leaf nodes which touch the specified bbox
		void getNodesTouchingBBox(const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out) const;
		// Calculates the potential coordinate for the center of a node at the given size that is closest to the given coordinate
		// The returned coordinate may not correspond to an actual node
		IntVector3D getClosestPossibleNodeCenterAtLevel(IntVector3D coord, unsigned int nodeSize) const;

		// Based on generated surfaces, generate quads for rendering. This is to separate rendering logic with octree logic
		void generateQuads(std::vector<Quad>& out) const;
		void generateTCQuads(std::vector<TestChamberQuad>& out) const;
		// Creates a UniqueSurfaceIdentifer for the given node and side.
		// USI should be the standard to long term description of a node to remove dependency on NodeID and SurfaceID
		void makeUSI(NodeID node, UniqueSurfaceIdentifer& out, WallSide side = WallSide::NONE) const;
		// Based on generated surfaces, generate quads for rendering. This is to separate rendering logic with octree logic

		bool canUnsubdivide(NodeID node) const;
		// pass in a possible candidates of nodes that needs to be checked for unsubdividing.
		// needs a contiguous list of NodeIDs that may or may not repeat
		void unsubdivideNodes(NodeID* touchingNodes, int size);

		// Expand the tree until the coordinate is enclosed by a node of given size
		NodeID subdivideNodeToCoord(QVector3D coord, NodeID parentID, unsigned int size);
		int setNodeActive(NodeID node, bool active);
		int setNodesActive(std::vector<NodeID>& nodes, bool active);

		// Based on the octree structure, generate surfaces
		void generateSurfaces();
		

		// Extrude all surfaces selected
		int extrudeSurfaces(Selector* selector);
		// Receve all surfaces selected
		int recedeSurfaces(Selector* selector);
		// Recursively destroy a node and all its children
		void destroyBranch(NodeID nodeID);

		// Given a bounding box, subdivide all nodes within in so that their sides lie along the bounding box sides
		// - out will be set to the resulting nodes inside the bbox
		void subdivideNodesToBBox(const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out);
		void subdivideNodeToBBoxRecursive(NodeID node, const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out);

		NodeID subdivideNodeToCoordOptimized(QVector3D coord, NodeID parentID, unsigned int size);
		// Returned node will always be a leaf node
		NodeID getLeafNodeAtCoord(QVector3D coord);
		NodeID getLeafNodeAtCoord(QVector3D coord, NodeID parentID);



		// Debug
		void printNodes();
		void printNodesTree();

		void increaseGridSize();
		void decreaseGridSize();
		int getGridSize() { return m_gridSize; }
		
	private:
		// Hash function to convert position to unique node ID
		// The position must be a known center point of a node
		NodeID positionToNodeID(IntVector3D pos) const;

		// std::nullopt for the parent ID means it's the root node
		// - not making this a default value so it is a deliberate programming choice
		void createOctreeNode(NodeID newID, std::optional<NodeID> parentID, IntVector3D center, unsigned int size);
		void _printNodesTree(std::map<NodeID, std::vector<NodeID>>& children, NodeID id, std::string prefix, bool isLast);

		// TODO: shorten this name?
		void checkChildrenTouchingBBox(NodeID parentID, const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out) const;
		
		const NodeID searchNodeWithCoordSize(QVector3D coord, unsigned int size, NodeID parentID) const;
		const NodeID searchNodeWithCoordSize(IntVector3D coord, unsigned int size, NodeID parentID) const;

		IntVector3D getNodeBBoxMin(NodeID id) const;
		IntVector3D getNodeBBoxMax(NodeID id) const;
		
		NodeID m_rootNode;
		// Size of the root node in hammer units - this is effectively the internal max map size
		unsigned int m_rootNodeWorldSize;
		
		SafeUnorderedMap<NodeID, TestChamberNode> m_nodeMap;
		SafeUnorderedMap<NodeID, std::array<ChamberSurface, 6>> m_surfaceMap;

		int m_gridSize;
	};

	inline IntVector3D getWallFaceCenter(IntVector3D center, unsigned int size, IntVector3D normal)
	{
		assert(size % 2 == 0 && "Node size must be even");
		return center + (int)(size / 2) * -normal;
	}

	inline std::array<IntVector3D, 6> getNeighborNodeCenters(const unsigned int nodeSize, const IntVector3D nodeCenter)
	{
		return {
			nodeCenter + IntVector3D(nodeSize, 0.0f, 0.0f),  // +X
			nodeCenter + IntVector3D(-nodeSize, 0.0f, 0.0f),  // -X
			nodeCenter + IntVector3D(0.0f,  nodeSize, 0.0f),  // +Y
			nodeCenter + IntVector3D(0.0f, -nodeSize, 0.0f),  // -Y
			nodeCenter + IntVector3D(0.0f, 0.0f,  nodeSize),  // +Z
			nodeCenter + IntVector3D(0.0f, 0.0f, -nodeSize)   // -Z
		};
	}

	inline std::array<QVector3D, 6> getOuterFaceCoords(const unsigned int nodeSize, const QVector3D nodeCenter, float epsilon = 0.5f)
	{
		float halfSize = nodeSize / 2.0f + epsilon;

		return {
			nodeCenter + QVector3D(halfSize, 0.0f, 0.0f),  // +X
			nodeCenter + QVector3D(-halfSize, 0.0f, 0.0f),  // -X
			nodeCenter + QVector3D(0.0f,  halfSize, 0.0f),  // +Y
			nodeCenter + QVector3D(0.0f, -halfSize, 0.0f),  // -Y
			nodeCenter + QVector3D(0.0f, 0.0f,  halfSize),  // +Z
			nodeCenter + QVector3D(0.0f, 0.0f, -halfSize)   // -Z
		};
	}

	// Given a coordinate, compare against the parent coordinate to find the order at which the child node is placed in the child array
	inline unsigned int getChildOrder(IntVector3D coord, IntVector3D parentCenter)
	{
		IntVector3D delta = coord - parentCenter;

		// Determine sign bits: 0 for negative, 1 for positive
		unsigned int xBit = (delta.x() >= 0) ? 1 : 0;
		unsigned int yBit = (delta.y() >= 0) ? 1 : 0;
		unsigned int zBit = (delta.z() >= 0) ? 1 : 0;

		// Bit pattern: xBit (LSB), yBit, zBit (MSB)
		return (zBit << 2) | (yBit << 1) | xBit;
	}

	inline IntVector3D getChildCenter(unsigned int order, unsigned int parentSize, IntVector3D parentCenter)
	{
		//std::cout << parentSize << std::endl;
		if (parentSize % 4 != 0) throw std::runtime_error("Parent node size must be divisible by 4");
		if (order > 7) throw std::runtime_error("Order can not be greater than 7");

		int offset = parentSize / 4;
		// Branchless calculation
		int xSign = ((order & 1) << 1) - 1;               // 1 if odd, -1 if even
		int ySign = (((order >> 1) & 1) << 1) - 1;        // 1 if bit 1 is 0, -1 if 1
		int zSign = (((order >> 2) & 1) << 1) - 1;        // 1 if bit 2 is 0, -1 if 1

		return parentCenter + IntVector3D(offset * xSign, offset * ySign, offset * zSign);
	}

}
