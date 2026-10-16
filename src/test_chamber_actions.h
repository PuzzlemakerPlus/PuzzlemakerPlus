// test_chamber_actions.h
// Defines undo/redo actions for the test chamber
#pragma once

#include <memory>
#include <vector>

#include "selection.h"
#include "test_chamber.h"
#include "undo.h"

// For handling undo and redo of actions done on test chambers
// DO NOT store NodeIDs or pointers to nodes/surfaces, the octree may split or merge them
// Instead, use UniqueVolumeIdentifier for NodeID-agnostic design

namespace PMP
{
// A geometry edit: a set of volumes that all switched to one state
// Extrude records the volumes that went open -> solid, recede the ones that went solid -> open
class TCEditAction : public Undoable
{
public:
	TCEditAction(TestChamber* tc, Selector* sel, std::vector<UniqueVolumeIdentifier> volumes, bool newState,
	             Selector selBefore, Selector selAfter)
	    : m_testChamber(tc), m_selector(sel), m_volumes(std::move(volumes)), m_newState(newState),
	      m_selBefore(selBefore), m_selAfter(selAfter)
	{
	}

	//helper function for creating extrude action
	static std::unique_ptr<TCEditAction> extrude(TestChamber* tc, Selector* sel,
	                                             std::vector<UniqueVolumeIdentifier> volumes, Selector selBefore,
	                                             Selector selAfter)
	{
		return std::make_unique<TCEditAction>(tc, sel, std::move(volumes), false, selBefore, selAfter);
	}

	//helper function for creating recede action
	static std::unique_ptr<TCEditAction> recede(TestChamber* tc, Selector* sel,
	                                            std::vector<UniqueVolumeIdentifier> volumes, Selector selBefore,
	                                            Selector selAfter)
	{
		return std::make_unique<TCEditAction>(tc, sel, std::move(volumes), true, selBefore, selAfter);
	}

	int undo() override { return apply(!m_newState, m_selBefore); }

	int redo() override { return apply(m_newState, m_selAfter); }

private:
	int apply(bool state, const Selector& sel)
	{
		m_testChamber->setVolumesActive(m_volumes, state);
		*m_selector = sel;
		return 1;
	}

	TestChamber* m_testChamber;
	Selector* m_selector;
	std::vector<UniqueVolumeIdentifier> m_volumes;
	bool m_newState;
	Selector m_selBefore;
	Selector m_selAfter;
};
} // namespace PMP
