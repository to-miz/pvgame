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

#endif // _RANGE_H_INCLUDED_
