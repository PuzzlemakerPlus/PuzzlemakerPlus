#pragma once
#include "util/3d/mesh.h"
#include "util/optional_array.h"

namespace PMP
{
	using NodeID = unsigned int;

	//THE PLANKS OF TOMORROW
	//Represents a physical wall of a node
	//TestChamber stores surfaces that will be used for creating quads, performing raycasts etc
	//Differentiated from rendered quads
	struct ChamberSurface {
		NodeID parentNode;
		WallSide side = WallSide::NONE;
	};

	struct RayHitResult {
		NodeID nodeID;
		QVector3D hitPoint;
		ChamberSurface surface;
	};

	class TestChamberNode{
	public:
		TestChamberNode() :
			m_size(0),
			m_active(false),
			m_parentID(std::nullopt),
			m_center(0.0f, 0.0f, 0.0f)
		{}
		
		TestChamberNode(QVector3D center, unsigned int size) :
		m_size(size),
		m_active(true),
		m_center(center)
		{} // Constructor without parent, used for root node only

		TestChamberNode( NodeID parentID, QVector3D center, unsigned int size, bool active) :
			m_size(size),
			m_active(active),
			m_center(center),
			m_parentID(parentID)
		{}
		
		bool m_active; // Node is "open" - draws surfaces on the sides, items can be placed here, etc.
		QVector3D m_center; // Position of the center of the node in world units
		unsigned int m_size; // Size of the node in world units

		std::optional<NodeID> m_parentID;
		PMP::OptionalArray<NodeID, 8> m_childIDs;
	};
}