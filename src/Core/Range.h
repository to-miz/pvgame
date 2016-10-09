#pragma once

#ifndef _RANGE_H_INCLUDED_
#define _RANGE_H_INCLUDED_

template< class T >
struct trange {
	T min;
	T max;
};

#if RANGE_ARGS_AS_CONST_REF
	template< class T >
	using trangearg = const trange< T >&;
#else
	template< class T >
	using trangearg = trange< T >;
#endif

typedef trange< int32 > rangei;
typedef trange< float > rangef;
typedef trangearg< int32 > rangeiarg;
typedef trangearg< float > rangefarg;

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


#endif // _RANGE_H_INCLUDED_
