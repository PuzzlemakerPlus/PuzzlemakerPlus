#include "selection.h"
#include "test_chamber.h"
#include "test_chamber_types.h"
#include "undo.h"
#include "util/3d/mesh.h"
#include <optional>

//For handling undo and redo of actions done on test chambers
//DO NOT store pointers of surfaces as it will be lost
//Instead, use UniqueSurfaceIdentifier for NodeID/SurfaceID-agnostic design

namespace PMP
{
    class TCExtrudeAction : public Undoable
    {
    public:
        TCExtrudeAction(TestChamber* tc,
                        Selector* sel,
                        UniqueSurfaceIdentifer extrudeUSI,
                        UniqueSurfaceIdentifer newSelectUSI)
            : m_testChamber(tc)
            , m_selector(sel)
            , m_extrudeUSI(std::move(extrudeUSI))
            , m_newSelectUSI(std::move(newSelectUSI))
        {}

        int undo() override
        {
            const NodeID extrudeNode = toNode(m_extrudeUSI);
            if (m_testChamber->SetNodeActive(extrudeNode, true)) {
                m_testChamber->GenerateSurfaces();
                m_selector->clearSurfaces();

                if (auto surf = m_testChamber->GetSurface(extrudeNode, m_extrudeUSI.side); surf.has_value())
                    m_selector->setSelection(*surf);

                return 1;
            }
            return 0;
        }

        int redo() override
        {
            const NodeID extrudeNode = toNode(m_extrudeUSI);
            if (m_testChamber->SetNodeActive(extrudeNode, false)) {
                m_testChamber->GenerateSurfaces();
                m_selector->clearSurfaces();

                if (m_newSelectUSI.side != WALLSIDE::NONE) {
                    const NodeID newSelNode = toNode(m_newSelectUSI);
                    if (auto surf = m_testChamber->GetSurface(newSelNode, m_newSelectUSI.side); surf.has_value())
                        m_selector->setSelection(*surf);
                }
                return 1;
            }
            return 0;
        }

    private:
        NodeID toNode(const UniqueSurfaceIdentifer& usi) const
        {
            return m_testChamber->SearchNodeWithCoordSize(usi.nodeCord, static_cast<unsigned int>(usi.size));
        }

        TestChamber* m_testChamber;
        Selector* m_selector;
        UniqueSurfaceIdentifer m_extrudeUSI;
        UniqueSurfaceIdentifer m_newSelectUSI;
    };

    class TCRecedeAction : Undoable
    {
        int undo();
        int redo();
    };
}