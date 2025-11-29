// math.h
// Math related utility functions that aren't already covered by Qt or the standard library

#include "math.h"

namespace PMP
{
	void vector3DMin( const QVector3D& v1, const QVector3D& v2, QVector3D& out )
	{
		out.setX( fmin( v1.x(), v2.x() ) );
		out.setY( fmin( v1.y(), v2.y() ) );
		out.setZ( fmin( v1.z(), v2.z() ) );
	}

	void vector3DMax( const QVector3D& v1, const QVector3D& v2, QVector3D& out )
	{
		out.setX( fmax( v1.x(), v2.x() ) );
		out.setY( fmax( v1.y(), v2.y() ) );
		out.setZ( fmax( v1.z(), v2.z() ) );
	}

	bool boundingBoxesIntersect( const QVector3D& mins1, const QVector3D& maxs1, const QVector3D& mins2, const QVector3D& maxs2 )
	{
		return mins1.x() < maxs2.x() && maxs1.x() > mins2.x() &&  
		       mins1.y() < maxs2.y() && maxs1.y() > mins2.y() &&
		       mins1.z() < maxs2.z() && maxs1.z() > mins2.z();
	}
	
	bool boundingBoxEdgesIntersect( const QVector3D& mins1, const QVector3D& maxs1, const QVector3D& mins2, const QVector3D& maxs2 )
	{
		return boundingBoxesIntersect(mins1, maxs1, mins2, maxs2) && (
		       mins1.x() < mins2.x() || maxs1.x() > maxs2.x() ||
		       mins1.y() < mins2.y() || maxs1.y() > maxs2.y() ||
		       mins1.z() < mins2.z() || maxs1.z() > maxs2.z() );
	}
}