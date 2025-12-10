// math.h
// Math related utility functions that aren't already covered by Qt or the standard library

#include <random>
#include <cmath>
#include "math.h"

constexpr double PI = 3.14159265358979323846;

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
	bool boundingBoxesIntersect(const IntVector3D& mins1, const IntVector3D& maxs1, const IntVector3D& mins2, const IntVector3D& maxs2)
	{
		return mins1.x() < maxs2.x() && maxs1.x() > mins2.x() &&
			mins1.y() < maxs2.y() && maxs1.y() > mins2.y() &&
			mins1.z() < maxs2.z() && maxs1.z() > mins2.z();
	}
	// Returns true if box A is completely inside box B
	bool isBoxAInsideBoxB(const QVector3D& minsA, const QVector3D& maxsA, const QVector3D& minsB, const QVector3D& maxsB)
	{
		// A is inside B if A's min point is >= B's min
		// AND A's max point is <= B's max, on all axes.
		return  minsA.x() >= minsB.x() &&
			minsA.y() >= minsB.y() &&
			minsA.z() >= minsB.z() &&
			maxsA.x() <= maxsB.x() &&
			maxsA.y() <= maxsB.y() &&
			maxsA.z() <= maxsB.z();
	}
	bool isBoxAInsideBoxB(const IntVector3D& minsA, const IntVector3D& maxsA, const IntVector3D& minsB, const IntVector3D& maxsB)
	{
		// A is inside B if A's min point is >= B's min
		// AND A's max point is <= B's max, on all axes.
		return  minsA.x() >= minsB.x() &&
			minsA.y() >= minsB.y() &&
			minsA.z() >= minsB.z() &&
			maxsA.x() <= maxsB.x() &&
			maxsA.y() <= maxsB.y() &&
			maxsA.z() <= maxsB.z();
	}
	// Returns true if the bounding boxes intersect, excluding cases where one box is completely inside the other
	bool boundingBoxEdgesIntersect( const QVector3D& mins1, const QVector3D& maxs1, const QVector3D& mins2, const QVector3D& maxs2 )
	{
		return boundingBoxesIntersect(mins1, maxs1, mins2, maxs2) && (
		       mins1.x() < mins2.x() || maxs1.x() > maxs2.x() ||
		       mins1.y() < mins2.y() || maxs1.y() > maxs2.y() ||
		       mins1.z() < mins2.z() || maxs1.z() > maxs2.z() );
	}
	//random float between 0 and 1
	float randf()
	{
		std::mt19937 rng(std::random_device{}());
		static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		return dist(rng);
	}
	QVector4D uniformQuaternion()
	{
		float u1 = randf();
		float u2 = randf();
		float u3 = randf();

		return QVector4D(
			sqrt(1 - u1)*std::sin(2 * PI * u2),
			sqrt(1 - u1) * std::cos(2 * PI * u2),
			sqrt(u1) * std::sin(2 * PI * u3),
			sqrt(u1) * std::cos(2 * PI * u3));
	}
	QVector3D quaternionToUnitVector(QVector4D quat)
	{
		float x = quat.x();
		float y = quat.y();
		float z = quat.z();
		float w = quat.w();
		return QVector3D(
			2*(x*z+w*y),
			2*(y*z-w*z),
			1-2*(x*x+y*y));
	}
}