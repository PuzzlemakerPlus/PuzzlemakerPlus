// math.h
// Math related utility functions that aren't already covered by Qt or the standard library
#pragma once
#include <qvector3d.h>
#include <qmatrix4x4.h>

namespace PMP
{
	//Integer 3D vector class
	class IntVector3D
	{
		int _x;
		int _y;
		int _z;
	public:
		IntVector3D( int x_in = 0, int y_in = 0, int z_in = 0 )
			: _x( x_in ), _y( y_in ), _z( z_in )
		{}
		/*IntVector3D( const IntVector3D& other )
			: _x( other._x ), _y( other._y ), _z( other._z )
		{}*/

		//float vector rounded to nearest int vector
		IntVector3D( const QVector3D& other )
			: _x(static_cast<int>(std::round(other.x()))), _y(static_cast<int>(std::round(other.y()))), _z(static_cast<int>(std::round(other.z())))
		{}
		int getX() const { return _x; }
		int getY() const { return _y; }
		int getZ() const { return _z; }
		int x() const { return _x; }
		int y() const { return _y; }
		int z() const { return _z; }

		IntVector3D& setX( int x_in )
		{
			_x = x_in;
			return *this;
		}
		IntVector3D& setY( int y_in )
		{
			_y = y_in;
			return *this;
		}
		IntVector3D& setZ( int z_in )
		{
			_z = z_in;
			return *this;
		}
		IntVector3D& set( int x_in, int y_in, int z_in )
		{
			_x = x_in;
			_y = y_in;
			_z = z_in;
			return *this;
		}
		static IntVector3D zero() { return IntVector3D(0, 0, 0); }
		static IntVector3D unitX() { return IntVector3D(1, 0, 0); }
		static IntVector3D unitY() { return IntVector3D(0, 1, 0); }
		static IntVector3D unitZ() { return IntVector3D(0, 0, 1); }
		IntVector3D& operator=( const IntVector3D& other )
		{
			_x = other._x;
			_y = other._y;
			_z = other._z;
			return *this;
		}
		IntVector3D operator+( const IntVector3D& other ) const
		{
			return IntVector3D( _x + other._x, _y + other._y, _z + other._z);
		}
		IntVector3D operator-( const IntVector3D& other ) const
		{
			return IntVector3D( _x - other._x, _y - other._y, _z - other._z );
		}
		IntVector3D operator*( int scalar ) const
		{
			return IntVector3D( _x * scalar, _y * scalar, _z * scalar );
		}
		//results are rounded make sure you want that behavior
		IntVector3D operator*( float scalar ) const
		{
			float xf = static_cast<float>(_x);
			float yf = static_cast<float>(_y);
			float zf = static_cast<float>(_z);
			return IntVector3D(std::round(xf * scalar), std::round(yf * scalar), std::round(zf * scalar));
		}
		//results are rounded make sure you want that behavior
		IntVector3D operator/( int scalar ) const
		{
			float xf = static_cast<float>(_x);
			float yf = static_cast<float>(_y);
			float zf = static_cast<float>(_z);
			return IntVector3D(std::round(xf / scalar), std::round(yf / scalar), std::round(zf / scalar));
		}
		IntVector3D& operator+=( const IntVector3D& other )
		{
			_x += other._x;
			_y += other._y;
			_z += other._z;
			return *this;
		}
		IntVector3D& operator-=( const IntVector3D& other )
		{
			_x -= other._x;
			_y -= other._y;
			_z -= other._z;
			return *this;
		}
		IntVector3D& operator*=( int scalar )
		{
			_x *= scalar;
			_y *= scalar;
			_z *= scalar;
			return *this;
		}
		//results are rounded make sure you want that behavior
		IntVector3D& operator*=(float scalar)
		{
			float xf = static_cast<float>(_x);
			float yf = static_cast<float>(_y);
			float zf = static_cast<float>(_z);
			_x = std::round(xf * scalar);
			_y = std::round(yf * scalar);
			_z = std::round(zf * scalar);
			return *this;
		}
		//results are rounded make sure you want that behavior
		IntVector3D& operator/=( int scalar )
		{
			float xf = static_cast<float>(_x);
			float yf = static_cast<float>(_y);
			float zf = static_cast<float>(_z);
			_x = std::round(xf / scalar);
			_y = std::round(yf / scalar);
			_z = std::round(zf / scalar);
			return *this;
		}
		//results are rounded make sure you want that behavior
		IntVector3D& operator/=( float scalar )
		{
			float xf = static_cast<float>(_x);
			float yf = static_cast<float>(_y);
			float zf = static_cast<float>(_z);
			_x = std::round(xf / scalar);
			_y = std::round(yf / scalar);
			_z = std::round(zf / scalar);
			return *this;
		}
		friend IntVector3D operator*(int scalar, const IntVector3D& v)
		{
			return v * scalar;
		}
		//results are rounded make sure you want that behavior
		friend IntVector3D operator*(float scalar, const IntVector3D& v)
		{
			return v * scalar;
		}
		bool operator==( const IntVector3D& other ) const
		{
			return _x == other._x && _y == other._y && _z == other._z;
		}
		bool operator!=( const IntVector3D& other ) const
		{
			return !(*this == other);
		}
		int& operator[](int i)
		{
			assert(i >= 0 && i < 3);
			return i == 0 ? _x : (i == 1 ? _y : _z);
		}
		int operator[](int i) const
		{
			assert(i >= 0 && i < 3);
			return i == 0 ? _x : (i == 1 ? _y : _z);
		}
		IntVector3D operator-() const
		{
			return IntVector3D( -_x, -_y, -_z );
		}
		IntVector3D abs() const
		{
			return IntVector3D( std::abs( _x ), std::abs( _y ), std::abs( _z ) );
		}
		IntVector3D min( const IntVector3D& other ) const
		{
			return IntVector3D( std::min( _x, other._x ), std::min( _y, other._y ), std::min( _z, other._z ) );
		}
		IntVector3D max( const IntVector3D& other ) const
		{
			return IntVector3D( std::max( _x, other._x ), std::max( _y, other._y ), std::max( _z, other._z ) );
		}
		int dot( const IntVector3D& other ) const
		{
			return _x * other._x + _y * other._y + _z * other._z;
		}
		float dot(const QVector3D& other) const
		{
			return QVector3D::dotProduct(toQVector3D(), other);
		}
		IntVector3D cross( const IntVector3D& other ) const
		{
			return IntVector3D(
				_y * other._z - _z * other._y,
				_z * other._x - _x * other._z,
				_x * other._y - _y * other._x );
		}
		//note that 0 is considered positive here
		//idk how this will be useful
		IntVector3D sign() const
		{
			return IntVector3D((int)(_x >= 0) * 2 - 1, (int)(_y >= 0) * 2 - 1, (int)(_z >= 0) * 2 - 1);
		}
		int magnitudeSquared() const
		{
			return _x * _x + _y * _y + _z * _z;
		}
		float magnitude() const
		{
			return std::sqrt( static_cast<float>( magnitudeSquared() ) );
		}
		int manhattanLength() const
		{
			return std::abs(_x) + std::abs(_y) + std::abs(_z);
		}
		int manhattanDistance(const IntVector3D& other) const
		{
			return (*this - other).manhattanLength();
		}
		QVector3D toQVector3D() const
		{
			return QVector3D( static_cast<float>( _x ), static_cast<float>( _y ), static_cast<float>( _z ) );
		}
		QVector3D normalized() const
		{
			float mag = magnitude();
			if ( mag == 0 ) return QVector3D( 0, 0, 0 );
			return QVector3D( static_cast<float>( _x ) / mag, static_cast<float>( _y ) / mag, static_cast<float>( _z ) / mag );
		}
		//project other onto this vector
		QVector3D projectVector3D( const QVector3D& other ) const
		{
			QVector3D thisVec = toQVector3D();
			return other * ( QVector3D::dotProduct( thisVec, other) / other.lengthSquared() );
		}
		QVector3D lerp( const QVector3D& other, float t ) const
		{
			return toQVector3D() * ( 1 - t ) + other * t;
		}
		QVector3D lerp( const IntVector3D& other, float t ) const
		{
			return toQVector3D() * (1 - t) + other.toQVector3D() * t;
		}
	};

	// Outputs a vector3D where each coordinate is the minimum between the two vectors
	void vector3DMin( const QVector3D& v1, const QVector3D& v2, QVector3D& out );
	// Outputs a vector3D where each coordinate is the maximum between the two vectors
	void vector3DMax( const QVector3D& v1, const QVector3D& v2, QVector3D& out );

	// Tests if two AABBs overlap
	bool boundingBoxesIntersect( const QVector3D& mins1, const QVector3D& maxs1, const QVector3D& mins2, const QVector3D& maxs2 );
	bool boundingBoxesIntersect(const IntVector3D& mins1, const IntVector3D& maxs1, const IntVector3D& mins2, const IntVector3D& maxs2);
	bool isBoxAInsideBoxB(const QVector3D& minsA, const QVector3D& maxsA, const QVector3D& minsB, const QVector3D& maxsB);
	bool isBoxAInsideBoxB(const IntVector3D& minsA, const IntVector3D& maxsA, const IntVector3D& minsB, const IntVector3D& maxsB);
	// Returns true if bbox1's volume touches bbox2's edges - the order of these matters!
	bool boundingBoxEdgesIntersect( const QVector3D& mins1, const QVector3D& maxs1, const QVector3D& mins2, const QVector3D& maxs2 );
	// Check if two floats are close enough to effectively be considered equal.
	// The default epsilon is designed for world coordinates, you may want to change this dependong on your use case.
	template<typename T>
	bool closeEnough( T v1, T v2, float epsilon = 0.01f )
	{
		return ( v1 < v2 + epsilon ) && ( v1 > v2 - epsilon );
	}
	template<typename T>
	void setFlag(T& byte, int flag, bool value)
	{
		if (value)
			byte |= (1u << flag);   // set bit
		else
			byte &= ~(1u << flag);  // clear bit
	}
	template<typename T>
	bool readFlag(T byte, int bitIndex) { return (byte >> bitIndex) & 1u; }

	static QMatrix4x4 lookAt(const QVector3D& eye,
		const QVector3D& center,
		const QVector3D& up)
	{
		QVector3D f = (center - eye).normalized();
		QVector3D s = QVector3D::crossProduct(f, up.normalized()).normalized();
		QVector3D u = QVector3D::crossProduct(s, f);

		QMatrix4x4 M;
		M.setToIdentity();
		M(0, 0) = s.x(); M(0, 1) = s.y(); M(0, 2) = s.z();
		M(1, 0) = u.x(); M(1, 1) = u.y(); M(1, 2) = u.z();
		M(2, 0) = -f.x(); M(2, 1) = -f.y(); M(2, 2) = -f.z();

		QMatrix4x4 T;
		T.setToIdentity();
		T.translate(-eye);

		return M * T;
	}
	static QMatrix4x4 ortho(float left, float right,
		float bottom, float top,
		float nearPlane, float farPlane)
	{
		QMatrix4x4 M;
		M.setToIdentity();

		M(0, 0) = 2.0f / (right - left);
		M(1, 1) = 2.0f / (top - bottom);
		M(2, 2) = -2.0f / (farPlane - nearPlane);

		M(0, 3) = -(right + left) / (right - left);
		M(1, 3) = -(top + bottom) / (top - bottom);
		M(2, 3) = -(farPlane + nearPlane) / (farPlane - nearPlane);

		return M;
	}
	float randf();
	// Uniform random sampling of rotation in quaternion
	// Shoemake et al. 1992
	QVector4D uniformQuaternion();

	QVector3D quaternionToUnitVector(QVector4D quat);

}
