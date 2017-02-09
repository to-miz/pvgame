#pragma once

#ifndef _AABB_H_INCLUDED_
#define _AABB_H_INCLUDED_

template< class T >
union taabb {
	struct {
		T left;
		T bottom;
		T near;

		T right;
		T top;
		T far;
	};
	struct {
		T x;
		T y;
		T z;

		T _unused0[3];
	};
	struct {
		T x0;
		T y0;
		T z0;

		T x1;
		T y1;
		T z1;
	};
	struct {
		tvec3< T > min;
		tvec3< T > max;
	};
	struct {
		tvec3< T > xyz;
		tvec3< T > rightBottomFar;
	};
	T elements[6];
};

typedef taabb< float > aabb;
typedef taabb< int32 > aabbi;

#if defined( AABB_ARGS_AS_CONST_REF ) && AABB_ARGS_AS_CONST_REF == 1
	template< class T >
	using taabbarg = const taabb< T >&;
#else
	template< class T >
	using taabbarg = taabb< T >;
#endif
typedef taabbarg< float > aabbarg;

template< class T > taabb< T > Aabb( T left, T bottom, T near, T right, T top, T far );
template< class T > taabb< T > AabbWHD( T left, T bottom, T near, T width, T height, T depth );
template< class T > taabb< T > AabbWHD( tvec3arg< T > xyz, T width, T height, T depth );
template< class T > taabb< T > AabbWHD( tvec3arg< T > xyz, tvec3arg< T > whd );
template < class T, class W, class H, class D >
taabb< T > AabbHalfSize( tvec3arg< T > xyz, W hw, H hh, D hd );
template < class T, class U >
auto AabbScaled( taabbarg< T > aabb, tvec3arg< U > xyz )
    -> taabb< typename typeof( aabb.left* xyz.x ) >;
template< class T > T width( taabbarg< T > box );
template< class T > T height( taabbarg< T > box );
template< class T > T depth( taabbarg< T > box );
template< class T > tvec3< T > center( taabbarg< T > box );
template< class T > tvec3< T > dimensions( taabbarg< T > box );
template< class T > taabb< T > translate( taabbarg< T > box, T x, T y, T z );
template< class T > taabb< T > translate( taabbarg< T > box, tvec3< T > translation );
template< class T > taabb< T > grow( taabbarg< T > box, T amount );
template< class T > taabb< T > shrink( taabbarg< T > box, T amount );
template< class T > bool isPointInside( taabbarg< T > box, T x, T y, T z );
template< class T > bool isPointInside( taabbarg< T > box, tvec3< T > point );
template< class T > bool isOverlapping( taabbarg< T > a, taabbarg< T > b );
template< class T > bool isInside( taabbarg< T > a, taabbarg< T > b );
template< class T > bool isValid( taabbarg< T > box );

template< class T > taabb< T > ceil( taabbarg< T > box );
template< class T > taabb< T > floor( taabbarg< T > box );
template< class T > taabb< T > sweep( taabbarg< T > box, tvec3< T > delta );

// template impl
template< class T > taabb< T > Aabb( T left, T bottom, T near, T right, T top, T far )
{
	return {left, bottom, near, right, top, far};
}
template< class T > taabb< T > AabbWHD( T left, T bottom, T near, T width, T height, T depth )
{
	return {left, bottom, near, left + width, bottom + height, near + depth};
}
template< class T > taabb< T > AabbWHD( tvec3arg< T > xyz, T width, T height, T depth )
{
	return {xyz.x, xyz.y, xyz.z, xyz.x + width, xyz.y + height, xyz.z + depth};
}
template< class T > taabb< T > AabbWHD( tvec3arg< T > xyz, tvec3arg< T > whd )
{
	return {xyz.x, xyz.y, xyz.z, xyz.x + whd.x, xyz.y + whd.y, xyz.z + whd.z};
}
template < class T, class W, class H, class D >
taabb< T > AabbHalfSize( tvec3arg< T > xyz, W hw, H hh, D hd )
{
	static_assert( std::is_convertible< W, T >::value, "Values not implicitly convertible" );
	static_assert( std::is_convertible< H, T >::value, "Values not implicitly convertible" );
	static_assert( std::is_convertible< D, T >::value, "Values not implicitly convertible" );
	return {xyz.x - (T)hw, xyz.y - (T)hh, xyz.z - (T)hd,
	        xyz.x + (T)hw, xyz.y + (T)hh, xyz.z + (T)hd};
}
template < class T, class U >
auto AabbScaled( taabbarg< T > aabb, tvec3arg< U > xyz )
    -> taabb< typename typeof( aabb.left* xyz.x ) >
{
	return {aabb.left * xyz.x,  aabb.bottom * xyz.y, aabb.near * xyz.z,
	        aabb.right * xyz.x, aabb.top * xyz.y,    aabb.far * xyz.z};
}
template< class T > T width( taabbarg< T > box )
{
	return box.right - box.left;
}
template< class T > T height( taabbarg< T > box )
{
	return box.top - box.bottom;
}
template< class T > T depth( taabbarg< T > box )
{
	return box.far - box.near;
}
template< class T > tvec3< T > center( taabbarg< T > box )
{
	return {( box.left + box.right ) * 0.5f, ( box.top + box.bottom ) * 0.5f,
			( box.near + box.far ) * 0.5f};
}
template< class T > tvec3< T > dimensions( taabbarg< T > box )
{
	return {box.right - box.left, box.top - box.bottom, box.far - box.near};
}
template< class T > taabb< T > translate( taabbarg< T > box, T x, T y, T z )
{
	return {box.left + x, box.bottom + y, box.near + z, box.right + x, box.top + y, box.far + z};
}
template< class T > taabb< T > translate( taabbarg< T > box, tvec3< T > translation )
{
	return {box.left + translation.x,  box.bottom + translation.y,	box.near + translation.z,
			box.right + translation.x, box.top + translation.y, box.far + translation.z};
}
template< class T > taabb< T > grow( taabbarg< T > box, T amount )
{
	return {box.left - amount,  box.bottom - amount, box.near - amount,
	        box.right + amount, box.top + amount,    box.far + amount};
}
template < class T > taabb< T > shrink( taabbarg< T > box, T amount )
{
	return {box.left + amount,  box.bottom + amount, box.near + amount,
	        box.right - amount, box.top - amount,    box.far - amount};
}
template< class T > bool isPointInside( taabbarg< T > box, T x, T y, T z )
{
	return ( x >= box.left && y >= box.bottom && z >= box.near && x < box.right && y < box.top
			 && z < box.far );
}
template< class T > bool isPointInside( taabbarg< T > box, tvec3< T > point )
{
	return ( point.x >= box.left && point.y >= box.bottom && point.z >= box.near && point.x < box.right
			 && point.y < box.top && point.z < box.far );
}
template< class T > bool isOverlapping( taabbarg< T > a, taabbarg< T > b )
{
	return ( ( a.left < b.right ) && ( a.right > b.left ) && ( a.top > b.bottom )
			 && ( a.bottom < b.top ) && ( a.far > b.near ) && ( a.near < b.far ) );
}
template< class T > bool isInside( taabbarg< T > a, taabbarg< T > b )
{
	return ( ( a.right >= b.right ) && ( a.left <= b.left ) && ( a.top >= b.top )
			 && ( a.bottom <= b.bottom ) && ( a.far >= b.far ) && ( a.near <= b.near ) );
}
template< class T > bool isValid( taabbarg< T > box )
{
	assert( isValid( r.left ) );
	assert( isValid( r.bottom ) );
	assert( isValid( r.near ) );
	assert( isValid( r.right ) );
	assert( isValid( r.top ) );
	assert( isValid( r.far ) );
	return ( box.right >= box.left ) && ( box.top >= box.bottom ) && ( box.far >= box.near );
}

template< class T > taabb< T > ceil( taabbarg< T > box )
{
	return {ceil( box.left ),  ceil( box.bottom ),	ceil( box.near ),
			ceil( box.right ), ceil( box.top ), ceil( box.far )};
}
template< class T > taabb< T > floor( taabbarg< T > box )
{
	return {floor( box.left ),  floor( box.bottom ),	floor( box.near ),
			floor( box.right ), floor( box.top ), floor( box.far )};
}
template< class T > taabb< T > sweep( taabbarg< T > box, tvec3< T > delta )
{
	auto result = box;
	if( delta.x < 0 ) {
		result.left += delta.x;
	} else {
		result.right += delta.x;
	}
	if( delta.y < 0 ) {
		result.bottom += delta.y;
	} else {
		result.top += delta.y;
	}
	if( delta.z < 0 ) {
		result.near += delta.z;
	} else {
		result.far += delta.z;
	}
	return result;
}

// operators
template< class T > taabb< T > operator+( taabbarg< T > lhs, taabbarg< T > rhs )
{
	return {lhs.left + rhs.left,   lhs.bottom + rhs.bottom,		lhs.near + rhs.near,
			lhs.right + rhs.right, lhs.top + rhs.top, lhs.far + rhs.far};
}
template < class T >
taabb< T > operator-( taabbarg< T > lhs, taabbarg< T > rhs )
{
	return {lhs.left - rhs.left,   lhs.bottom - rhs.bottom, lhs.near - rhs.near,
	        lhs.right - rhs.right, lhs.top - rhs.top,       lhs.far - rhs.far};
}
template < class T, class U >
auto operator*( taabbarg< T > r, U s ) -> taabb< typename typeof( r.left* s ) >
{
	return {r.left * s, r.bottom * s, r.near * s, r.right * s, r.top * s, r.far * s};
}
template < class T, class U >
auto operator*( T s, taabbarg< T > r ) -> taabb< typename typeof( r.left* s ) >
{
	return {r.left * s, r.bottom * s, r.near * s, r.right * s, r.top * s, r.far * s};
}
template < class T, class U >
auto operator/( taabbarg< T > r, U s ) -> taabb< typename typeof( r.left / s ) >
{
	return {r.left / s, r.bottom / s, r.near / s, r.right / s, r.top / s, r.far / s};
}
template < class T, class U >
auto operator/( U s, taabbarg< T > r ) -> taabb< typename typeof( r.left / s ) >
{
	return {r.left / s, r.bottom / s, r.near / s, r.right / s, r.top / s, r.far / s};
}

template < class T >
auto operator/( taabbarg< T > r, float s ) -> taabb< typename typeof( r.left* s ) >
{
	s = 1.0f / s;
	return {r.left * s, r.bottom * s, r.near * s, r.right * s, r.top * s, r.far * s};
}
template < class T >
auto operator/( float s, taabbarg< T > r ) -> taabb< typename typeof( r.left* s ) >
{
	s = 1.0f / s;
	return {r.left * s, r.bottom * s, r.near * s, r.right * s, r.top * s, r.far * s};
}

template< class T > taabb< T >& operator+=( taabb< T >& lhs, taabbarg< T > rhs )
{
	lhs.left += rhs.left;
	lhs.bottom += rhs.bottom;
	lhs.near += rhs.near;
	lhs.right += rhs.right;
	lhs.top += rhs.top;
	lhs.far += rhs.far;
	return lhs;
}
template< class T > taabb< T >& operator-=( taabb< T >& lhs, taabbarg< T > rhs )
{
	lhs.left -= rhs.left;
	lhs.bottom -= rhs.bottom;
	lhs.near -= rhs.near;
	lhs.right -= rhs.right;
	lhs.top -= rhs.top;
	lhs.far -= rhs.far;
	return lhs;
}
template< class T > taabb< T >& operator*=( taabb< T >& lhs, T rhs )
{
	lhs.left *= rhs;
	lhs.bottom *= rhs;
	lhs.near *= rhs.near;
	lhs.right *= rhs;
	lhs.top *= rhs;
	lhs.far *= rhs.far;
	return lhs;
}
template< class T > taabb< T >& operator/=( taabb< T >& lhs, T rhs )
{
	lhs.left /= rhs;
	lhs.bottom /= rhs;
	lhs.near /= rhs.near;
	lhs.right /= rhs;
	lhs.top /= rhs;
	lhs.far /= rhs.far;
	return lhs;
}

#endif // _AABB_H_INCLUDED_
