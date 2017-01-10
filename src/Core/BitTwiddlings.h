#pragma once

#ifndef _BITTWIDDLINGS_H_INCLUDED_
#define _BITTWIDDLINGS_H_INCLUDED_

#ifdef _MSC_VER
	#include <intrin.h>
	#pragma intrinsic( _BitScanForward )
	inline int32 bitScanForward( uint32 mask )
	{
		unsigned long ret = 0;
		_BitScanForward( &ret, mask );
		return (int32)ret;
	}
	inline bool bitScanForward( uint32* index, uint32 mask )
	{
	    static_assert( sizeof( uint32 ) == sizeof( unsigned long ), "size mismatch" );
	    static_assert( alignof( uint32 ) == alignof( unsigned long ), "alignment mismatch" );
	    return _BitScanForward( (unsigned long*)index, mask ) != 0;
    }
#else
	// Credits go to: http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
	int32 bitScanForward( uint32 mask )
	{
		static const int32 MultiplyDeBruijnBitPosition[32] = {
			0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
			31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9};
		return MultiplyDeBruijnBitPosition[( ( uint32 )( ( mask & -mask ) * 0x077CB531u ) ) >> 27];
	}
	inline bool bitScanForward( uint32* index, uint32 mask )
	{
		*index = bitScanForward( mask );
		return mask != 0;
	}
#endif

#endif // _BITTWIDDLINGS_H_INCLUDED_
