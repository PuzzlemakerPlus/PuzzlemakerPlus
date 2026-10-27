// test_chamber.h
// Declares the class that represents the test chamber itself and the octree nodes
#pragma once

#include <map>
#include <optional>
#include <qvectornd.h>
#include <vector>

#include "selection.h"
#include "test_chamber_types.h"
#include "undo.h"
#include "util/3d/mesh.h"
#include "util/3d/ray.h"
#include "util/math.h"
#include "util/safemap.h"


namespace PMP
{
// DO NOT USE THIS DIRECTLY, this is just used to initialize the value in the test chamber class
// There are certain behaviors I would like to implement later that depend on the root node size not being hardcoded
// - Kelsey
const unsigned int defaultRootNodeSize = 8192;

const float gridSizeDefault = 32;
const float gridSizeMin = 32;
const float gridSizeMax = 256;

struct UniqueVolumeIdentifier;

class TestChamber
{
public:
	TestChamber();

	void createChildrenOfNode(NodeID parentID);
	// Recursively destroys child of a node
	void destroyChildrenOfNode(NodeID parentID);

	int getRootNodeSizeInt() const { return static_cast<int>(m_rootNodeWorldSize); };

	QVector3D getNodeCenter(NodeID nodeID) const { return m_nodeMap.at(nodeID).getCenter().toQVector3D(); }

	const SafeUnorderedMap<NodeID, TestChamberNode>& getNodeMap() const { return m_nodeMap; };

	std::optional<ChamberSurface> getSurface(NodeID node, WallSide side) const;
	void getSurfaceChildren(std::vector<ChamberSurface>& out, NodeID node, WallSide side) const;

	// Uses the hash function to try to locate the node at the given center coordinate
	// Returns std::nullopt if there is no such node that has that exact center coordinate
	// very cheap operation
	// Will return the node even if it has children
	std::optional<NodeID> tryGetNodeAtCoord(IntVector3D coord) const;

	std::array<IntVector3D, 4> getChamberSurfaceCorners(const ChamberSurface& surface);
	IntVector3D getSurfaceCenter(ChamberSurface surface) const;

	const SafeUnorderedMap<NodeID, std::array<ChamberSurface, 6>>& getSurfaceMap() const { return m_surfaceMap; }

	// Is the given coordinate out of bounds?
	bool isPointOOB(QVector3D coord) const;
	bool isPointOOB(IntVector3D coord) const;

	// Is there surface on the given side of the node?
	bool isSurface(WallSide side, NodeID node) const;
	bool isSurface(WallSide side, const TestChamberNode& p) const;

	bool isNodeActive(NodeID node) const { return m_nodeMap.at(node).getActive(); };

	bool nodeHasChild(NodeID node) const { return m_nodeMap.at(node).hasChildren(); };

	// Find the smallest node that includes the give coordinate
	// Recursive function that guarantees to find the node
	const NodeID searchNodeWithCoord(QVector3D coord) const;
	const NodeID searchNodeWithCoord(QVector3D coord, NodeID parentID) const;
	NodeID findLeafNear(IntVector3D coord, unsigned int size) const;
	std::optional<RayHitResult> rayCast(Ray ray) const;

	// Returns mins and maxs of the node's bounding box: (-,-,-) and (+,+,+)
	// replaced with getNodeBBoxMin and getNodeBBoxMax
	// it's better not to initialize std::array every time we want this
	// void getNodeBBox(NodeID id, std::array<QVector3D, 2>& out);

	// Returns leaf nodes which touch the specified bbox
	void getNodesTouchingBBox(const QVector3D& mins, const QVector3D& maxs, std::vector<NodeID>& out) const;
	// Calculates the potential coordinate for the center of a node at the given size that is closest to the given
	// coordinate The returned coordinate may not correspond to an actual node
	IntVector3D getClosestPossibleNodeCenterAtLevel(IntVector3D coord, unsigned int nodeSize) const;

	// Based on generated surfaces, generate quads for rendering. This is to separate rendering logic with octree logic
	void generateQuads(std::vector<Quad>& out) const;
	void generateTCQuads(std::vector<TestChamberQuad>& out) const;
	void generateTCShadowQuads(std::vector<TestChamberQuad>& out) const;
	// Based on generated surfaces, generate quads for rendering. This is to separate rendering logic with octree logic

	uint8_t nodeSurfaceFlag(NodeID node) const;

	bool canUnsubdivide(NodeID node) const;
	// pass in a possible candidates of nodes that needs to be checked for unsubdividing.
	// needs a contiguous list of NodeIDs that may or may not repeat
	void unsubdivideNodes(NodeID* touchingNodes, int size, std::vector<NodeID>* out = NULL);

	// Expand the tree until the coordinate is enclosed by a node of given size
	NodeID subdivideNodeToCoord(QVector3D coord, NodeID parentID, unsigned int size);

	int setNodesActive(std::vector<NodeID>& nodes, bool active);

	// Based on the octree structure, generate surfaces
	void generateSurfaces(); //faster than threaded on extra-thin trees
	void generateSurfacesThreaded();

	// the default puzzlemaker space
	void createDefaultSpace();
	// Extrude all surfaces selected
	int extrudeSurfaces(Selector* selector);
	// Receve all surfaces selected
	int recedeSurfaces(Selector* selector);

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

	UndoStack& undoStack() { return m_undoStack; }

	// Subdivides the neighbors of each node down to that node's size
	void matchNeighborSizes(const std::vector<NodeID>& nodes);
	// For undo/redo: recreates each volume at its recorded size, sets it (and anything under it) to `active`,
	// then merges and rebuilds surfaces
	void setVolumesActive(const std::vector<UniqueVolumeIdentifier>& volumes, bool active);

private:
	// Hash function to convert position to unique node ID
	// The position must be a known center point of a node
	NodeID positionToNodeID(IntVector3D pos) const;

	// std::nullopt for the parent ID means it's the root node
	// - not making this a default value so it is a deliberate programming choice
	void createOctreeNode(NodeID newID, std::optional<NodeID> parentID, IntVector3D center, unsigned int size);
	void _printNodesTree(std::map<NodeID, std::vector<NodeID>>& children, NodeID id, std::string prefix, bool isLast);

	// TODO: shorten this name?
	void checkChildrenTouchingBBox(NodeID parentID, const QVector3D& mins, const QVector3D& maxs,
	                               std::vector<NodeID>& out) const;

	IntVector3D getNodeBBoxMin(NodeID id) const;
	IntVector3D getNodeBBoxMax(NodeID id) const;

	NodeID m_rootNode;
	// Size of the root node in hammer units - this is effectively the internal max map size
	unsigned int m_rootNodeWorldSize;

	SafeUnorderedMap<NodeID, TestChamberNode> m_nodeMap;

	SafeUnorderedMap<NodeID, std::array<ChamberSurface, 6>> m_surfaceMap;

	int m_gridSize;

	// Per-chamber undostack
	UndoStack m_undoStack;
};

// Based on the information in this struct, it is possible to identify a volume without knowing its NodeID
struct UniqueVolumeIdentifier
{
	uint size;
	IntVector3D nodeCord;

	static UniqueVolumeIdentifier fromNodeID(const TestChamber& chamber, NodeID nodeID)
	{
		const TestChamberNode& node = chamber.getNodeMap().at(nodeID);
		UniqueVolumeIdentifier uvi;
		uvi.size = node.getSize();
		uvi.nodeCord = node.getCenter();
		return uvi;
	}
};

inline IntVector3D getWallFaceCenter(IntVector3D center, unsigned int size, IntVector3D m_gNormal)
{
	assert(size % 2 == 0 && "Node size must be even");
	return center + (int)(size / 2) * -m_gNormal;
}

inline std::array<IntVector3D, 6> getNeighborNodeCenters(const unsigned int nodeSize, const IntVector3D nodeCenter)
{
	return {
	    nodeCenter + IntVector3D(nodeSize, 0.0f, 0.0f),  // +X
	    nodeCenter + IntVector3D(-nodeSize, 0.0f, 0.0f), // -X
	    nodeCenter + IntVector3D(0.0f, nodeSize, 0.0f),  // +Y
	    nodeCenter + IntVector3D(0.0f, -nodeSize, 0.0f), // -Y
	    nodeCenter + IntVector3D(0.0f, 0.0f, nodeSize),  // +Z
	    nodeCenter + IntVector3D(0.0f, 0.0f, -nodeSize)  // -Z
	};
}

inline std::array<QVector3D, 6> getOuterFaceCoords(const unsigned int nodeSize, const QVector3D nodeCenter,
                                                   float epsilon = 0.5f)
{
	float halfSize = nodeSize / 2.0f + epsilon;

	return {
	    nodeCenter + QVector3D(halfSize, 0.0f, 0.0f),  // +X
	    nodeCenter + QVector3D(-halfSize, 0.0f, 0.0f), // -X
	    nodeCenter + QVector3D(0.0f, halfSize, 0.0f),  // +Y
	    nodeCenter + QVector3D(0.0f, -halfSize, 0.0f), // -Y
	    nodeCenter + QVector3D(0.0f, 0.0f, halfSize),  // +Z
	    nodeCenter + QVector3D(0.0f, 0.0f, -halfSize)  // -Z
	};
}

// Given a coordinate, compare against the parent coordinate to find the order at which the child node is placed in the
// child array
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
	// std::cout << parentSize << std::endl;
	if (parentSize % 4 != 0)
		throw std::runtime_error("Parent node size must be divisible by 4");
	if (order > 7)
		throw std::runtime_error("Order can not be greater than 7");

	int offset = parentSize / 4;
	// Branchless calculation
	int xSign = ((order & 1) << 1) - 1;        // 1 if odd, -1 if even
	int ySign = (((order >> 1) & 1) << 1) - 1; // 1 if bit 1 is 0, -1 if 1
	int zSign = (((order >> 2) & 1) << 1) - 1; // 1 if bit 2 is 0, -1 if 1

	return parentCenter + IntVector3D(offset * xSign, offset * ySign, offset * zSign);
}

} // namespace PMP
