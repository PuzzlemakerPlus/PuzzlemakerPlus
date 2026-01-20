// selection.h
// Handles chamber geometry selections
#pragma once

#include <optional>
#include <qmatrix4x4.h>
#include <qpoint.h>
#include <vector>

#include "test_chamber_types.h"
#include "util/3d/mesh.h"

namespace PMP
{
class TestChamber;

class Selector
{
public:
	Selector() : m_selectionActive(false), m_isVolumeSelection(false), m_mins(), m_maxs() {}

	// Returns corners of the selection
	void getSelectionGeometry(std::vector<QVector3D> &out);

	// Testchamber to make selection from
	void setTestChamber(TestChamber *tc) { m_testChamber = tc; };

	// New selection stuff
	// Might be worth changing these to not take the ray hit result directly
	void getGridTileCorners(QVector3D &mins, QVector3D &maxs, const RayHitResult &hitResult);
	void startSelection(const RayHitResult &hitResult);
	void updateSelection(const RayHitResult &hitResult);
	void endSelection();
	void clearSelection();
	// Shift selection in world space
	void shiftSelection(const QVector3D &offset);
	void shiftSelection(const IntVector3D &offset);

	QVector3D getMins() { return m_mins; }

	QVector3D getMaxs() { return m_maxs; }

	bool isSelectionActive() { return m_selectionActive; }

	bool isVolumeSelection() { return m_isVolumeSelection; }

	IntVector3D getSurfaceNormal()
	{
		Q_ASSERT(!m_isVolumeSelection);
		return m_surfaceNormal;
	}

private:
	TestChamber *m_testChamber = nullptr;

	// TODO: this needs more work to support multi selection
	bool m_selectionActive;
	QVector3D m_mins;
	QVector3D m_maxs;
	QVector3D m_startMins;
	QVector3D m_startMaxs;
	bool m_isVolumeSelection;

	// Normal of the surface the selection was started from.
	// Should ONLY be used for surface selections
	IntVector3D m_surfaceNormal;
};
} // namespace PMP
