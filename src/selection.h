#pragma once
#include <vector>
#include <qpoint.h>
#include <optional>
#include <qmatrix4x4.h>

#include "util/3d/mesh.h"
#include "test_chamber_types.h"

namespace PMP
{
	class TestChamber;

	class Selector
	{
	public:
		Selector() :
			m_selectionActive(false),
			m_isVolumeSelection(false),
			m_mins(),
			m_maxs()
		{}
		// Returns 4 corners of all selected surfaces
		void getSelectionSurfaceCorners(std::vector<QVector3D>& out);

		

		std::optional<NodeID> getNodeSelection(int index=0) const
		{
			return m_surfaceSelection.size() <= index ? std::nullopt : std::optional<NodeID>{m_surfaceSelection.at(index).parentNode};
		}
		std::optional<ChamberSurface> getSurfaceSelection(int index=0) const
		{
			return m_surfaceSelection.size() <= index ? std::nullopt : std::optional<ChamberSurface>{m_surfaceSelection.at(index)};
		}
		void getSurfaceSelectionMultiple(std::vector<ChamberSurface>& out) const
		{
			out.clear();
			out.insert(out.end(), m_surfaceSelection.begin(), m_surfaceSelection.end());
		}

		// Testchamber to make selection from
		void setTestChamber(TestChamber* tc) {m_testChamber = tc;};
		void setViewPortDim(int width, int height) {m_viewPortWidth = width; m_viewPortHeight = height;};
		void clearSurfaces() { m_surfaceSelection.clear(); };

		// New selection stuff
		// Might be worth changing these to not take the ray hit result directly
		void getGridTileCorners(QVector3D& mins, QVector3D& maxs, const RayHitResult& hitResult);
		void startSelection(const RayHitResult& hitResult);
		void updateSelection(const RayHitResult& hitResult);
		void endSelection();
		void clearSelection();

	private:
		TestChamber* m_testChamber = nullptr;
		int m_viewPortWidth = 0;
		int m_viewPortHeight = 0;
		//m_selection
		std::vector<ChamberSurface> m_surfaceSelection;

		// TODO: this needs more work to support multi selection
		bool m_selectionActive;
		QVector3D m_mins;
		QVector3D m_maxs;
		QVector3D m_startMins;
		QVector3D m_startMaxs;
		bool m_isVolumeSelection;

		// Normal of the surface the selection was started from.
		// Should ONLY be used for surface selections
		QVector3D m_surfaceNormal; 
	};
}