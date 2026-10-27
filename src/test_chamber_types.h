// test_chamber_types.h
// Defines types used internally by the test chamber octree structure
#pragma once

#include <unordered_set>

#include "util/3d/mesh.h"
#include "util/math.h"
#include "util/optional_array.h"

namespace PMP
{
// spatial hash may collide if this is uint32_t
// for example, in a world of size 8192, there are 512 nodes of size 16 per axis
// 512^3 = 134,217,728 possible nodes
using NodeID = uint64_t;

// THE PLANKS OF TOMORROW
// Represents a physical wall of a node
// TestChamber stores surfaces that will be used for creating quads, performing raycasts etc
// Differentiated from rendered quads
struct ChamberSurface
{
	NodeID parentNode;
	WallSide side = WallSide::NONE;
};

struct RayHitResult
{
	NodeID nodeID;
	QVector3D hitPoint;
	ChamberSurface surface;
};

class TestChamberNode
{
public:
	TestChamberNode(NodeID id, IntVector3D center, unsigned int size)
	    : m_id(id), m_size(size), m_active(true), m_center(center)
	{
	} // Constructor without parent, used for root node only

	TestChamberNode(NodeID id, NodeID parentID, IntVector3D center,
	                unsigned int size, bool active)
	    : m_id(id), m_size(size), m_active(active), m_center(center), m_parentID(parentID)
	{
	}
	NodeID getID() const { return m_id; }
	// only the root node doesnt have a parent
	bool hasParentID() const { return m_parentID.has_value(); }

	NodeID getParentID() const { return m_parentID.value(); }

	const bool hasChildren() const { return m_childIDs.has_value(); }

	std::array<NodeID, 8>& getChildrenID() { return m_childIDs.value(); }

	const std::array<NodeID, 8>& readChildrenID() const { return m_childIDs.valueRead(); }

	void resetChildrenID()
	{
		m_childIDs.reset();
	}

	bool getActive() const { return m_active; }

	void setActive(bool active)
	{
		if (m_active != active)
		{
			m_active = active;
		}
	}

	IntVector3D getCenter() const { return m_center; }

	unsigned int getSize() const { return m_size; }

	std::array<NodeID, 8>& initializeChildren()
	{
		return m_childIDs.value_initialize();
	}

	NodeID getNeighbor(WallSide direction) const { return m_neighbors[static_cast<int>(direction)]; }

	std::array<NodeID, 6>& getNeighbors() { return m_neighbors; }

private:
	const NodeID m_id;
	const IntVector3D m_center; // Position of the center of the node in world units
	const unsigned int m_size;  // Size of the node in world units
	bool m_active;              // Node is "open" - draws surfaces on the sides, items can be placed here, etc.

	std::optional<NodeID> m_parentID;
	PMP::OptionalArray<NodeID, 8> m_childIDs;
	// these neighbors are nodes with same size as this node
	std::array<NodeID, 6> m_neighbors = {0, 0, 0, 0, 0, 0};
};
} // namespace PMP
