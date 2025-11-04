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
}