#include "selection.h"
#include "test_chamber.h"
#include "test_chamber_types.h"
#include "undo.h"
#include "util/3d/mesh.h"
#include <optional>

//For handling undo and redo of actions done on test chambers
//DO NOT store pointers of surfaces as it will be lost

//In the future, it may be better to store the coordinates to the nodes instead of the node id as it can change 

namespace PMP
{
    class TCExtrudeAction : public Undoable
    {
        public:
        TCExtrudeAction(TestChamber* tc, Selector* sel, NodeID extrudeNode, NodeID newSelectNode, WALLSIDE extrudeSide, WALLSIDE newSelectSide) : 
            m_testChamber(tc),
            m_selector(sel),
            m_extrudeNode(extrudeNode),
            m_newSelectNode(newSelectNode),
            m_extrudeSide(extrudeSide),
            m_newSelectSide(newSelectSide)
        {

        }
        int undo() override
        {
            if(m_testChamber->SetNodeActive(m_extrudeNode, true))
            {
                m_testChamber->GenerateSurfaces();
			    m_selector->clearSurfaces();
                std::optional<ChamberSurface> surf = m_testChamber->GetSurface(m_extrudeNode, m_extrudeSide);
                if(surf.has_value()) m_selector->setSelection(surf.value());
                return 1;
            }
            return 0;
        }
        int redo() override
        {
            if(m_testChamber->SetNodeActive(m_extrudeNode, false))
            {
                m_testChamber->GenerateSurfaces();
			    m_selector->clearSurfaces();
                if(m_newSelectSide != WALLSIDE::NONE){
                    std::optional<ChamberSurface> surf = m_testChamber->GetSurface(m_newSelectNode, m_newSelectSide);
                    if(surf.has_value()) m_selector->setSelection(surf.value());
                }
                return 1;
            }
            return 0;
        }
        private:
        TestChamber* m_testChamber;
        Selector* m_selector;
        NodeID m_extrudeNode;
        WALLSIDE m_extrudeSide;
        NodeID m_newSelectNode;
        WALLSIDE m_newSelectSide;
    };

    class TCRecedeAction : Undoable
    {
        int undo();
        int redo();
    };
}