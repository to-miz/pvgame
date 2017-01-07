#pragma once

#ifndef _VECTOR_H_INCLUDED_
#define _VECTOR_H_INCLUDED_

enum VectorComponentValues : int32 {
	VectorComponent_X,
	VectorComponent_Y,
	VectorComponent_Z,
	VectorComponent_W,
};

template < class T >
union tvec2 {
	struct {
		T x;
		T y;
	};
	struct {
		T u;
		T v;
	};
	T elements[2];

	inline constexpr T operator[]( intmax index ) const
	{
		assert( index >= 0 && index < 2 );
		return elements[index];
	}
};

// vec2
typedef tvec2< float > vec2;
typedef tvec2< int32 > vec2i;

#if defined( VEC2_ARGS_AS_CONST_REF ) && VEC2_ARGS_AS_CONST_REF == 1
	template< class T >
	using tvec2arg = const tvec2< T >&;
#else
	template< class T >
	using tvec2arg = tvec2< T >;
#endif
typedef tvec2arg< float > vec2arg;
typedef tvec2arg< int > vec2iarg;

float cross( vec2arg a, vec2arg b );
float dot( vec2arg a, vec2arg b );
float length( vec2arg v );
bool hasMagnitude( vec2arg v );
vec2 normalize( vec2arg v );
vec2 normalize( vec2arg v, float* outLength );
vec2 safeNormalize( vec2arg v, vec2arg def = {} );
vec2 safeNormalize( vec2arg v, float* outLength, vec2arg def = {} );

float angle( vec2arg v );
float angle( vec2arg a, vec2arg b );
vec2 rotate( vec2arg v, float angle );
vec2 rotate( vec2arg v, float sin, float cos );
vec2 rotateAround( vec2arg v, vec2arg origin, float angle );
vec2 rotateAround( vec2arg v, vec2arg origin, float sin, float cos );

vec2 floor( vec2arg v );
vec2 ceil( vec2arg v );
vec2 round( vec2arg v );
vec2 floorCentered( float x, float width );

template< class T > tvec2< T > swizzle( tvec2arg< T > v, intmax x, intmax y );
template< class T > tvec2< T > turnLeft( tvec2arg< T > v ) { return {-v.y, v.x}; }
template< class T > tvec2< T > turnRight( tvec2arg< T > v ) { return {v.y, -v.x}; }
template < class T >
tvec2< T > multiplyComponents( tvec2arg< T > a, tvec2arg< T > b )
{
	return {a.x * b.x, a.y * b.y};
}

template< class A, class B, class C > tvec2< C > Vec2( A a, B b );
template < class T >
tvec2< T > clamp( tvec2arg< T > v, tvec2arg< T > min, tvec2arg< T > max )
{
	return {clamp( v.x, min.x, max.x ), clamp( v.y, min.y, max.y )};
}

// vec3
template < class T >
union tvec3 {
	struct {
		T x;
		T y;
		T z;
	};
	struct {
		tvec2< T > xy;
		T _unused0;
	};
	struct {
		T _unused1;
		tvec2< T > yz;
	};
	struct {
		T r;
		T g;
		T b;
	};
	T elements[3];

	inline constexpr T operator[]( intmax index ) const
	{
		assert( index >= 0 && index < 3 );
		return elements[index];
	}
};

typedef tvec3< float > vec3;
typedef tvec3< int32 > vec3i;
typedef tvec3< intmax > vec3im;

#if defined( VEC3_ARGS_AS_CONST_REF ) && VEC3_ARGS_AS_CONST_REF == 1
	template< class T >
	using tvec3arg = const tvec3< T >&;
#else
	template< class T >
	using tvec3arg = tvec3< T >;
#endif
typedef tvec3arg< float > vec3arg;
typedef tvec3arg< int > vec3iarg;
typedef tvec3arg< intmax > vec3imarg;

template< class T > tvec3< T > swizzle( tvec3arg< T > v, intmax x, intmax y, intmax z );

template< class A, class B, class C, class D > tvec3< D > Vec3( A a, B b, C c );
template< class T, class A > tvec3< T > Vec3( tvec2arg< T > v, A a );
template< class T, class A > tvec3< T > Vec3( A a, tvec2arg< T > v );
template< class T, class U > tvec3< T > Vec3( tvec3arg< U > v );

vec3 cross( vec3arg a, vec3arg b );
float dot( vec3arg a, vec3arg b );
float length( vec3arg v );
vec3 normalize( vec3arg v );
vec3 normalize( vec3arg v, float* length );
vec3 safeNormalize( vec3arg v, vec3arg def = {} );
vec3 safeNormalize( vec3arg v, float* length, vec3arg def = {} );

vec3 rotate( vec3arg v, float angle, vec3arg normal );
vec3 rotate( vec3arg v, float sin, float cos, vec3arg normal );
vec3 rotateAround( vec3arg v, vec3arg origin, float angle, vec3arg normal );
vec3 rotateAround( vec3arg v, vec3arg origin, float sin, float cos, vec3arg normal );
template < class T >
tvec3< T > multiplyComponents( tvec3arg< T > a, tvec3arg< T > b )
{
	return {a.x * b.x, a.y * b.y, a.z * b.z};
}

vec3 floor( vec3arg v );
vec3 ceil( vec3arg v );
vec3 round( vec3arg v );

// vec4
template < class T >
union tvec4 {
	struct {
		T x;
		T y;
		T z;
		T w;
	};
	struct {
		tvec3< T > xyz;
		T _unused0;
	};
	struct {
		T _unused1;
		tvec3< T > yzw;
	};
	struct {
		tvec2< T > xy;
		T _unused2[2];
	};
	struct {
		T _unused3;
		tvec2< T > yz;
		T _unused4;
	};
	struct {
		T _unused5[2];
		tvec2< T > zw;
	};
	struct {
		T a;
		T r;
		T g;
		T b;
	};
	T elements[4];

	inline constexpr T operator[]( intmax index ) const
	{
		assert( index >= 0 && index < 4 );
		return elements[index];
	}
};

typedef tvec4< float > vec4;

#if defined( VEC4_ARGS_AS_CONST_REF ) && VEC4_ARGS_AS_CONST_REF == 1
	template< class T >
	using tvec4arg = const tvec4< T >&;
#else
	template< class T >
	using tvec4arg = tvec4< T >;
#endif
typedef tvec4arg< float > vec4arg;

template< class T > tvec4< T > swizzle( tvec4arg< T > v, intmax x, intmax y, intmax z, intmax w );

float dot( vec4arg a, vec4arg b );
vec4 normalize( vec4arg v );
vec4 safeNormalize( vec4arg v, vec4arg def = {} );

vec4 multiplyQuaternions( vec4arg a, vec4arg b );
template < class T >
tvec4< T > multiplyComponents( tvec4arg< T > a, tvec4arg< T > b )
{
	return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

// template implementations

template < class A, class B, class C, class D = typename std::common_type< A, B, C >::type >
tvec3< D > Vec3( A a, B b, C c )
{
	return {(D)a, (D)b, (D)c};
}
template< class T, class A > tvec3< T > Vec3( tvec2arg< T > v, A a )
{
	static_assert( std::is_convertible< A, T >::value, "Values not implicitly convertible" );
	return {v.x, v.y, (T)a};
}
template < class T, class A > tvec3< T > Vec3( A a, tvec2arg< T > v )
{
	static_assert( std::is_convertible< A, T >::value, "Values not implicitly convertible" );
	return {(T)a, v.x, v.y};
}
template< class T, class U > tvec3< T > Vec3( tvec3arg< U > v )
{
	return {(T)v.x, (T)v.y, (T)v.z};
}

template< class T > tvec2< T > swizzle( tvec2arg< T > v, intmax x, intmax y )
{
	assert( x >= 0 && x < 2 );
	assert( y >= 0 && y < 2 );
	return {v.elements[x], v.elements[y]};
}
template< class T > tvec3< T > swizzle( tvec3arg< T > v, intmax x, intmax y, intmax z )
{
	assert( x >= 0 && x < 3 );
	assert( y >= 0 && y < 3 );
	assert( z >= 0 && z < 3 );
	return {v.elements[x], v.elements[y], v.elements[z]};
}
template< class T > tvec4< T > swizzle( tvec4arg< T > v, intmax x, intmax y, intmax z, intmax w )
{
	assert( x >= 0 && x < 4 );
	assert( y >= 0 && y < 4 );
	assert( z >= 0 && z < 4 );
	assert( w >= 0 && w < 4 );
	return {v.elements[x], v.elements[y], v.elements[z], v.elements[w]};
}

template < class A, class B, class C = typename std::common_type< A, B >::type >
tvec2< C > Vec2( A a, B b )
{
	return {(C)a, (C)b};
}

// operators

template < class T >
tvec2< T > operator+( tvec2arg< T > lhs, tvec2arg< T > rhs )
{
	return {lhs.x + rhs.x, lhs.y + rhs.y};
}
template < class T >
tvec2< T > operator-( tvec2arg< T > lhs, tvec2arg< T > rhs )
{
	return {lhs.x - rhs.x, lhs.y - rhs.y};
}
template < class T >
tvec2< T > operator*( tvec2arg< T > v, T s )
{
	return {v.x * s, v.y * s};
}
template < class T >
tvec2< T > operator*( T s, tvec2arg< T > v )
{
	return {v.x * s, v.y * s};
}
template < class T >
tvec2< T > operator/( tvec2arg< T > v, T s )
{
	return {v.x / s, v.y / s};
}
template < class T >
tvec2< T > operator/( T s, tvec2arg< T > v )
{
	return {v.x / s, v.y / s};
}
// vec2 version
vec2 operator/( vec2arg v, float s )
{
	float denom = 1.0f / s;
	return {v.x * denom, v.y * denom};
}
vec2 operator/( float s, vec2arg v )
{
	float denom = 1.0f / s;
	return {v.x * denom, v.y * denom};
}

template < class T >
tvec2< T > operator-( tvec2arg< T > v )
{
	return {-v.x, -v.y};
}

template < class T >
tvec2< T >& operator+=( tvec2< T >& lhs, tvec2arg< T > rhs )
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	return lhs;
}
template < class T >
tvec2< T >& operator-=( tvec2< T >& lhs, tvec2arg< T > rhs )
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	return lhs;
}
template < class T >
tvec2< T >& operator*=( tvec2< T >& v, T s )
{
	v.x *= s;
	v.y *= s;
	return v;
}
template < class T >
tvec2< T >& operator/=( tvec2< T >& v, T s )
{
	v.x /= s;
	v.y /= s;
	return v;
}
// vec2 version
vec2& operator/=( vec2& v, float s )
{
	auto denom = 1.0f / s;
	v.x *= denom;
	v.y *= denom;
	return v;
}

// vec3

template < class T >
tvec3< T > operator+( tvec3arg< T > lhs, tvec3arg< T > rhs )
{
	return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}
template < class T >
tvec3< T > operator-( tvec3arg< T > lhs, tvec3arg< T > rhs )
{
	return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}
template < class T >
tvec3< T > operator*( tvec3arg< T > v, T s )
{
	return {v.x * s, v.y * s, v.z * s};
}
template < class T >
tvec3< T > operator*( T s, tvec3arg< T > v )
{
	return {v.x * s, v.y * s, v.z * s};
}
template < class T >
tvec3< T > operator/( tvec3arg< T > v, T s )
{
	return {v.x / s, v.y / s, v.z / s};
}
template < class T >
tvec3< T > operator/( T s, tvec3arg< T > v )
{
	return {v.x / s, v.y / s, v.z / s};
}
// vec3
vec3 operator/( vec3arg v, float s )
{
	auto denom = 1.0f / s;
	return {v.x * denom, v.y * denom, v.z * denom};
}
vec3 operator/( float s, vec3arg v )
{
	auto denom = 1.0f / s;
	return {v.x * denom, v.y * denom, v.z * denom};
}

template < class T >
tvec3< T > operator-( tvec3arg< T > v )
{
	return {-v.x, -v.y, -v.z};
}

template < class T >
tvec3< T >& operator+=( tvec3< T >& lhs, tvec3arg< T > rhs )
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	return lhs;
}
template < class T >
tvec3< T >& operator-=( tvec3< T >& lhs, tvec3arg< T > rhs )
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	return lhs;
}
template < class T >
tvec3< T >& operator*=( tvec3< T >& v, T s )
{
	v.x *= s;
	v.y *= s;
	v.z *= s;
	return v;
}
template < class T >
tvec3< T >& operator/=( tvec3< T >& v, T s )
{
	v.x /= s;
	v.y /= s;
	v.z /= s;
	return v;
}
// vec3
vec3& operator/=( vec3& v, float s )
{
	auto denom = 1.0f / s;
	v.x *= denom;
	v.y *= denom;
	v.z *= denom;
	return v;
}

// vec4
template < class T >
tvec4< T > operator+( tvec4arg< T > lhs, tvec4arg< T > rhs )
{
	return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}
template < class T >
tvec4< T > operator-( tvec4arg< T > lhs, tvec4arg< T > rhs )
{
	return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}
template < class T >
tvec4< T > operator*( tvec4arg< T > v, T s )
{
	return {v.x * s, v.y * s, v.z * s, v.w * s};
}
template < class T >
tvec4< T > operator*( T s, tvec4arg< T > v )
{
	return {v.x * s, v.y * s, v.z * s, v.w * s};
}
template < class T >
tvec4< T > operator/( tvec4arg< T > v, T s )
{
	return {v.x / s, v.y / s, v.z / s, v.w / s};
}
template < class T >
tvec4< T > operator/( T s, tvec4arg< T > v )
{
	return {v.x / s, v.y / s, v.z / s, v.w / s};
}
// vec4
vec4 operator/( vec4arg v, float s )
{
	auto denom = 1.0f / s;
	return {v.x * denom, v.y * denom, v.z * denom, v.w * denom};
}
vec4 operator/( float s, vec4arg v )
{
	auto denom = 1.0f / s;
	return {v.x * denom, v.y * denom, v.z * denom, v.w * denom};
}

template < class T >
tvec4< T > operator-( tvec4arg< T > v )
{
	return {-v.x, -v.y, -v.z, -v.w};
}

template < class T >
tvec4< T >& operator+=( tvec4< T >& lhs, tvec4arg< T > rhs )
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	lhs.w += rhs.w;
	return lhs;
}
template < class T >
tvec4< T >& operator-=( tvec4< T >& lhs, tvec4arg< T > rhs )
{
	lhs.x -= rhs.x;
	lhs.y -= rhs.y;
	lhs.z -= rhs.z;
	lhs.w -= rhs.w;
	return lhs;
}
template < class T >
tvec4< T >& operator*=( tvec4< T >& v, T s )
{
	v.x *= s;
	v.y *= s;
	v.z *= s;
	v.w *= s;
	return v;
}
template < class T >
tvec4< T >& operator/=( tvec4< T >& v, T s )
{
	v.x /= s;
	v.y /= s;
	v.z /= s;
	v.w /= s;
	return v;
}
// vec4
vec4& operator/=( vec4& v, float s )
{
	auto denom = 1.0f / s;
	v.x *= denom;
	v.y *= denom;
	v.z *= denom;
	v.w *= denom;
	return v;
}

#endif  // _VECTOR_H_INCLUDED_
