// math.h
// Math related utility functions that aren't already covered by Qt or the standard library

#include <qvector3d.h>

namespace PMP
{
	// Outputs a vector3D where each coordinate is the minimum between the two vectors
	void vector3DMin( const QVector3D& v1, const QVector3D& v2, QVector3D& out );
	// Outputs a vector3D where each coordinate is the maximum between the two vectors
	void vector3DMax( const QVector3D& v1, const QVector3D& v2, QVector3D& out );

	// Check if two floats are close enough to effectively be considered equal.
	// The default epsilon is designed for world coordinates, you may want to change this dependong on your use case.
	template<typename T>
	bool closeEnough( T v1, T v2, float epsilon = 0.01f )
	{
		return ( v1 < v2 + epsilon ) && ( v1 > v2 - epsilon );
	}
}
