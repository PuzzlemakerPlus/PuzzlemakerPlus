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
#include <vector>

    class TCExtrudeAction : public Undoable
    {
    public:
        // Multi-extrude Multi-select
        TCExtrudeAction(TestChamber* tc,
            Selector* sel,
            std::vector<UniqueSurfaceIdentifer> extrudeUSIs,
            std::vector<UniqueSurfaceIdentifer> newSelectUSIs)
            : m_testChamber(tc)
            , m_selector(sel)
            , m_extrudeUSIs(std::move(extrudeUSIs))
            , m_newSelectUSIs(std::move(newSelectUSIs))
        {}

        // Multi-extrude No-select
        TCExtrudeAction(TestChamber* tc,
            Selector* sel,
            std::vector<UniqueSurfaceIdentifer> extrudeUSIs)
            : m_testChamber(tc)
            , m_selector(sel)
            , m_extrudeUSIs(std::move(extrudeUSIs))
        {}

        // Back-compat single-variant
        TCExtrudeAction(TestChamber* tc,
            Selector* sel,
            UniqueSurfaceIdentifer extrudeUSI,
            UniqueSurfaceIdentifer newSelectUSI)
            : TCExtrudeAction(tc, sel,
                std::vector<UniqueSurfaceIdentifer>{std::move(extrudeUSI)},
                std::vector<UniqueSurfaceIdentifer>{std::move(newSelectUSI)})
        {}

        int undo() override
        {
            bool anyChanged = false;
            for (const auto& usi : m_extrudeUSIs) {
                const NodeID node = toNode(usi);
                anyChanged |= m_testChamber->setNodeActive(node, true);
            }

            if (!anyChanged) return 0;

            m_testChamber->generateSurfaces();
            m_selector->clearSurfaces();

            for (auto it = m_extrudeUSIs.rbegin(); it != m_extrudeUSIs.rend(); ++it) {
                if (auto surf = m_testChamber->getSurface(toNode(*it), it->side); surf.has_value()) {
                    m_selector->addSelection(surf.value());
                }
            }
            return 1;
        }

        int redo() override
        {
            bool anyChanged = false;
            for (const auto& usi : m_extrudeUSIs) {
                const NodeID node = toNode(usi);
                anyChanged |= m_testChamber->setNodeActive(node, false);
            }

            if (!anyChanged) return 0;

            m_testChamber->generateSurfaces();
            m_selector->clearSurfaces();

            for (auto it = m_newSelectUSIs.rbegin(); it != m_newSelectUSIs.rend(); ++it) {
                if (it->side == WALLSIDE::NONE) continue;
                if (auto surf = m_testChamber->getSurface(toNode(*it), it->side); surf.has_value()) {
                    m_selector->addSelection(surf.value());
                }
            }
            return 1;
        }

    private:
        NodeID toNode(const UniqueSurfaceIdentifer& usi) const
        {
            return m_testChamber->searchNodeWithCoordSize(
                usi.nodeCord, static_cast<unsigned int>(usi.size));
        }

        TestChamber* m_testChamber;
        Selector* m_selector;
        std::vector<UniqueSurfaceIdentifer> m_extrudeUSIs;
        std::vector<UniqueSurfaceIdentifer> m_newSelectUSIs;
    };


    class TCRecedeAction : Undoable
    {
        int undo();
        int redo();
    };
}