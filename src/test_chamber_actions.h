#include "selection.h"
#include "test_chamber.h"
#include "test_chamber_types.h"
#include "undo.h"
#include "util/3d/mesh.h"
#include <optional>
#include <vector>

//For handling undo and redo of actions done on test chambers
//DO NOT store pointers of surfaces as it will be lost
//Instead, use UniqueSurfaceIdentifier for NodeID/SurfaceID-agnostic design

namespace PMP
{
    class TCSurfaceAction : public Undoable
    {
    protected:
        TCSurfaceAction(TestChamber* tc, Selector* sel) : 
            m_testChamber(tc), m_selector(sel) {}

        NodeID toNode(const UniqueSurfaceIdentifer& usi) const
        {
            return m_testChamber->searchNodeWithCoordSize(
                usi.nodeCord, static_cast<unsigned int>(usi.size));
        }
        TestChamber* m_testChamber = nullptr;
        Selector* m_selector = nullptr;
    };

    class TCExtrudeAction : public TCSurfaceAction
    {
    public:
        // Multi-extrude Multi-select
        TCExtrudeAction(TestChamber* tc,
            Selector* sel,
            std::vector<UniqueSurfaceIdentifer> extrudeUSIs,
            std::vector<UniqueSurfaceIdentifer> newSelectUSIs)
            : TCSurfaceAction(tc, sel), m_extrudeUSIs(std::move(extrudeUSIs)), m_newSelectUSIs(std::move(newSelectUSIs)){}

        // Multi-extrude No-select
        TCExtrudeAction(TestChamber* tc,
            Selector* sel,
            std::vector<UniqueSurfaceIdentifer> extrudeUSIs)
            : TCSurfaceAction(tc, sel), m_extrudeUSIs(std::move(extrudeUSIs)){}

        int undo() override;
        int redo() override;

    private:
        std::vector<UniqueSurfaceIdentifer> m_extrudeUSIs;
        std::vector<UniqueSurfaceIdentifer> m_newSelectUSIs;
    };


    class TCRecedeAction : public TCSurfaceAction
    {
    public:
        // Multi-extrude Multi-select
        TCRecedeAction(TestChamber* tc,
            Selector* sel,
            std::vector<UniqueSurfaceIdentifer> extrudeUSIs,
            std::vector<UniqueSurfaceIdentifer> newSelectUSIs)
            : TCSurfaceAction(tc, sel), m_recedeUSIs(std::move(extrudeUSIs)), m_newSelectUSIs(std::move(newSelectUSIs)){}

        // Multi-extrude No-select
        TCRecedeAction(TestChamber* tc,
            Selector* sel,
            std::vector<UniqueSurfaceIdentifer> extrudeUSIs)
            : TCSurfaceAction(tc, sel), m_recedeUSIs(std::move(extrudeUSIs)){}

        int undo() override;
        int redo() override;

    private:
        std::vector<UniqueSurfaceIdentifer> m_recedeUSIs;
        std::vector<UniqueSurfaceIdentifer> m_newSelectUSIs;
    };
}