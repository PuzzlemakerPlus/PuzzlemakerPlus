
#include <qvectornd.h>

#include "selection.h"
#include "test_chamber_types.h"
#include "util/3d/mesh.h"
#include "util/3d/ray.h"
#include "test_chamber.h"
#include "util/math.h"

namespace PMP
{
	void Selector::getSelectionGeometry(std::vector<QVector3D>& out)
	{
		// TODO: this is a dumb hack, but it's late and I'm tired - Kelsey

		// Bottom
		out.push_back(m_mins);
		out.push_back(QVector3D(m_maxs.x(), m_mins.y(), m_mins.z()));
		out.push_back(QVector3D(m_maxs.x(), m_maxs.y(), m_mins.z()));
		out.push_back(QVector3D(m_mins.x(), m_maxs.y(), m_mins.z()));
		// -X
		out.push_back(m_mins);
		out.push_back(QVector3D(m_mins.x(), m_mins.y(), m_maxs.z()));
		out.push_back(QVector3D(m_maxs.x(), m_mins.y(), m_maxs.z()));
		out.push_back(QVector3D(m_maxs.x(), m_mins.y(), m_mins.z()));
		// -Y
		out.push_back(m_mins);
		out.push_back(QVector3D(m_mins.x(), m_maxs.y(), m_mins.z()));
		out.push_back(QVector3D(m_mins.x(), m_maxs.y(), m_maxs.z()));
		out.push_back(QVector3D(m_mins.x(), m_mins.y(), m_maxs.z()));
		// +X
		out.push_back(m_maxs);
		out.push_back(QVector3D(m_mins.x(), m_maxs.y(), m_maxs.z()));
		out.push_back(QVector3D(m_mins.x(), m_maxs.y(), m_mins.z()));
		out.push_back(QVector3D(m_maxs.x(), m_maxs.y(), m_mins.z()));
		// +Y
		out.push_back(m_maxs);
		out.push_back(QVector3D(m_maxs.x(), m_maxs.y(), m_mins.z()));
		out.push_back(QVector3D(m_maxs.x(), m_mins.y(), m_mins.z()));
		out.push_back(QVector3D(m_maxs.x(), m_mins.y(), m_maxs.z()));
		// Top
		out.push_back(m_maxs);
		out.push_back(QVector3D(m_maxs.x(), m_mins.y(), m_maxs.z()));
		out.push_back(QVector3D(m_mins.x(), m_mins.y(), m_maxs.z()));
		out.push_back(QVector3D(m_mins.x(), m_maxs.y(), m_maxs.z()));
	}

	void Selector::getGridTileCorners( QVector3D& mins, QVector3D& maxs, const RayHitResult& hitResult )
	{
		int gridSize = m_testChamber->getGridSize(); // for convenience
		Q_ASSERT(gridSize >= 1 && "Grid size < 1 not supported");

		IntVector3D forward = getWallNormal(hitResult.surface.side);
		IntVector3D up = getWallUp(hitResult.surface.side);
		IntVector3D right = forward.cross(up);

		IntVector3D forwardMask( abs(forward.x()), 
							   abs(forward.y()), 
							   abs(forward.z()) );
		IntVector3D sideMask( abs(up.x() + right.x()), 
							abs(up.y() + right.y()), 
							abs(up.z() + right.z()) );

		mins.setX( floor( hitResult.hitPoint.x() / gridSize ) * gridSize );
		mins.setY( floor( hitResult.hitPoint.y() / gridSize ) * gridSize );
		mins.setZ( floor( hitResult.hitPoint.z() / gridSize ) * gridSize );

		mins *= sideMask.toQVector3D();
		mins += m_testChamber->getSurfaceCenter(hitResult.surface).toQVector3D() * forwardMask.toQVector3D();

		maxs = mins + QVector3D(gridSize, gridSize, gridSize) * sideMask.toQVector3D();
	}

	void Selector::startSelection( const RayHitResult& hitResult )
	{
		getGridTileCorners(m_mins, m_maxs, hitResult);
		m_startMaxs = m_maxs;
		m_startMins = m_mins;

		m_surfaceNormal = getWallNormal(hitResult.surface.side);
		// Selection never starts as a volume
		m_isVolumeSelection = false;
		m_selectionActive = true;
	}

	void Selector::updateSelection( const RayHitResult& hitResult )
	{
		QVector3D mins, maxs;
		getGridTileCorners(mins, maxs, hitResult);

		vector3DMin(mins, m_startMins, m_mins);
		vector3DMin(maxs, m_mins, m_mins);

		vector3DMax(mins, m_startMaxs, m_maxs);
		vector3DMax(maxs, m_maxs, m_maxs);

		// If the corners are aligned on any axis, it's a flat surface selection
		m_isVolumeSelection = !( closeEnough(m_mins.x(), m_maxs.x()) ||
								 closeEnough(m_mins.y(), m_maxs.y()) ||
								 closeEnough(m_mins.z(), m_maxs.z()) );
	}

	void Selector::endSelection()
	{
		// Don't need to do anything here currently, but we will later
	}

	void Selector::clearSelection()
	{
		m_selectionActive = false;
		m_isVolumeSelection = false;
		m_maxs = m_mins = m_startMaxs = m_startMins = QVector3D(0, 0, 0);
		m_surfaceNormal = IntVector3D(0, 0, 0);
	}

	void Selector::shiftSelection( const QVector3D& offset )
	{
		m_mins += offset;
		m_maxs += offset;
		m_startMins += offset;
		m_startMaxs += offset;
	}
	void Selector::shiftSelection(const IntVector3D& offset)
	{
		m_mins += offset.toQVector3D();
		m_maxs += offset.toQVector3D();
		m_startMins += offset.toQVector3D();
		m_startMaxs += offset.toQVector3D();
	}
}