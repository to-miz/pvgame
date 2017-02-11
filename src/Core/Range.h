#pragma once

#ifndef _RANGE_H_INCLUDED_
#define _RANGE_H_INCLUDED_

template < class T >
struct trange {
	T min;
	T max;

	inline explicit operator bool() const
	{
		assert( min <= max );
		return min < max;
	}

#if RANGE_ARGS_AS_CONST_REF
	typedef const trange& rangearg;
#else
	typedef trange rangearg;
#endif
	inline bool operator==( rangearg rhs ) const { return min == rhs.min && max == rhs.max; }
	inline bool operator!=( rangearg rhs ) const { return min != rhs.min || max != rhs.max; }
};

#if RANGE_ARGS_AS_CONST_REF
template < class T >
using trangearg = const trange< T >&;
#else
template < class T >
using trangearg = trange< T >;
#endif

typedef trange< int32 > rangei;
typedef trange< float > rangef;
typedef trangearg< int32 > rangeiarg;
typedef trangearg< float > rangefarg;
typedef trange< uint16 > rangeu16;

template < class T, class V > trange< T > offsetBy( trangearg< T > r, V v )
{
	static_assert( std::is_convertible< V, T >::value, "Value is not convertible" );
	return {r.min + v, r.max + v};
}
template< class T > T length( trangearg< T > r )
{
	return r.max - r.min;
}
template< class T > T width( trangearg< T > r )
{
	return r.max - r.min;
}

template< class T, class U > trange< T > Range( trangearg< U > other )
{
	return {(T)other.min, (T)other.max};
}
template < class A, class B, class C = typename std::common_type< A, B >::type >
trange< C > Range( A min, B max )
{
	return {(C)min, (C)max};
}

template < class T >
bool isValid( trangearg< T > r )
{
	return r.min <= r.max;
}
template < class T >
bool isEmpty( trangearg< T > r )
{
	return r.min == r.max;
}
template < class T, class U >
bool isInRange( trangearg< T > r, U other )
{
	return other >= r.min && other < r.max;
}

// range based index loop

struct range_iterator {
	int32 value;

	inline bool operator!=( range_iterator other ) const { return this->value < other.value; }
	inline bool operator==( range_iterator other ) const { return this->value >= other.value; }
	inline range_iterator& operator++()
	{
		++value;
		return *this;
	}
	inline range_iterator operator++( int )
	{
		auto result = *this;
		++value;
		return result;
	}

	inline int32& operator*() { return value; }
	inline const int32& operator*() const { return value; }
};

range_iterator begin( rangeiarg r ) { return {r.min}; }
range_iterator end( rangeiarg r ) { return {r.max}; }

#endif  // _RANGE_H_INCLUDED_
