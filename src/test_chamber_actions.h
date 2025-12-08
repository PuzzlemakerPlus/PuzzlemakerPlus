#pragma once
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
	struct SelectionVolume
	{
		IntVector3D mins;
		IntVector3D maxs;
	};
	class TCAction : public Undoable
	{
	protected:
		TCAction(TestChamber* tc, Selector* sel) : 
			m_testChamber(tc), m_selector(sel) {}

		NodeID toNode(const UniqueSurfaceIdentifier& usi) const
		{
			return m_testChamber->searchNodeWithCoordSize(
				usi.nodeCord, static_cast<unsigned int>(usi.size));
		}
		NodeID toNode(const UniqueVolumeIdentifier& uvi) const
		{
			return m_testChamber->searchNodeWithCoordSize(
				uvi.nodeCord, static_cast<unsigned int>(uvi.size));
		}
		TestChamber* m_testChamber = nullptr;
		Selector* m_selector = nullptr;
	};
	class TCExtrudeAction : public TCAction
	{
	public:
		TCExtrudeAction(TestChamber* tc,
			Selector* sel,
			std::vector<UniqueVolumeIdentifier> extrudeUVIs,
			SelectionVolume selectionVolume)
			: TCAction(tc, sel), m_extrudeUVIs(std::move(extrudeUVIs)), m_prevSelectionVolume(std::move(selectionVolume)){}

		int undo() override;
		int redo() override;

	private:
		std::vector<UniqueVolumeIdentifier> m_extrudeUVIs;
		SelectionVolume m_prevSelectionVolume;
	};

	class TCRecedeAction : public TCAction
	{
	public:
		TCRecedeAction(TestChamber* tc,
			Selector* sel,
			std::vector<UniqueVolumeIdentifier> recedeUVIs,
			SelectionVolume selectionVolume)
			: TCAction(tc, sel), m_recedeUVIs(std::move(recedeUVIs)), m_prevSelectionVolume(std::move(selectionVolume)) {}

		int undo() override;
		int redo() override;

	private:
		std::vector<UniqueVolumeIdentifier> m_recedeUVIs;
		SelectionVolume m_prevSelectionVolume;
	};
}