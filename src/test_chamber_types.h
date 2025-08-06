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
		WALLSIDE side = WALLSIDE::NONE;
		NodeID parentNode;
	};

    struct RayHitResult {
        NodeID nodeID;
		ChamberSurface surface;
        QVector3D hitPoint;
    };

    class TestChamberNode{
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
}