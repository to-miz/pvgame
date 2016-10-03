#pragma once

#ifndef _COREFLOAT_H_INCLUDED_
#define _COREFLOAT_H_INCLUDED_

// TODO: this assumes little endian and IEEE-754
struct DoubleBits {
	uint32 data[2];
};
#if defined( ARCHITECTURE_LITTLE_ENDIAN ) && defined( ARCHITECTURE_IEEE_754 )
	#define FLT_MAX 3.402823466e+38f         // max value
	#define FLT_MIN 1.175494351e-38f         // min normalized positive value
	#define DBL_MAX 1.7976931348623158e+308  // max value
	#define DBL_MIN 2.2250738585072014e-308  // min positive value

	#define FLT_POSITIVE_INF_BITS 0x7F800000u
	#define FLT_NEGATIVE_INF_BITS 0xFF800000u
	#define FLT_SIGN_MASK 0x80000000u

	#define DBL_POSITIVE_INF_BITS 0x7FF00000u
	#define DBL_NEGATIVE_INF_BITS 0xFFF00000u
	#define DBL_SIGN_MASK 0x80000000u

	inline bool signbit( double v )
	{
		auto p = (char*)&v;
		return ( (uint8)p[sizeof( double ) - 1] & 0x80 ) != 0;
	}
	inline bool signbit( float v )
	{
		auto p = (char*)&v;
		return ( (uint8)p[sizeof( float ) - 1] & 0x80 ) != 0;
	}

	inline bool isinf( float v )
	{
		auto bits = bit_cast< uint32 >( v );
		// mask off sign bit
		bits &= ~FLT_SIGN_MASK;
		return bits == FLT_POSITIVE_INF_BITS;
	}
	inline bool isinf( double v )
	{
		auto bits = bit_cast< DoubleBits >( v );
		// mask off sign bit
		bits.data[1] &= ~DBL_SIGN_MASK;
		return bits.data[0] == 0 && bits.data[1] == DBL_POSITIVE_INF_BITS;
	}

	// we need to check the bytes to see if our value is nan, we can't just do v != v, since that might
	// get optimized away
	inline bool isnan( float v )
	{
		auto bits = bit_cast< uint32 >( v );
		bits &= ~FLT_SIGN_MASK;
		return bits > FLT_POSITIVE_INF_BITS;
	}
	inline bool isnan( double v )
	{
		auto bits = bit_cast< DoubleBits >( v );
		// mask off sign bit
		bits.data[1] &= ~DBL_SIGN_MASK;
		return ( bits.data[1] > DBL_POSITIVE_INF_BITS )
		       || ( bits.data[0] != 0 && bits.data[1] == DBL_POSITIVE_INF_BITS );
	}

	#elif defined( ARCHITECTURE_BIG_ENDIAN ) && defined( ARCHITECTURE_IEEE_754 )
	inline bool signbit( double v )
	{
		auto p = (char*)&v;
		return ( (uint8)p[0] & 0x80 ) != 0;
	}
	inline bool signbit( float v )
	{
		auto p = (char*)&v;
		return ( (uint8)p[0] & 0x80 ) != 0;
	}
#endif // defined( ARCHITECTURE_LITTLE_ENDIAN ) && defined( ARCHITECTURE_IEEE_754 )

inline bool isfinite( float v )
{
	// TODO: is this enough or do we define it in terms of isnan & isinf?
	return v >= -FLT_MAX && v <= FLT_MAX;
}
inline bool isnormal( float v ) { return v != 0 && isfinite( v ); }

inline bool isfinite( double v )
{
	// TODO: is this enough or do we define it in terms of isnan & isinf?
	return v >= -DBL_MAX && v <= DBL_MAX;
}
inline bool isnormal( double v ) { return v != 0 && isfinite( v ); }

#define FLOAT_MAX FLT_MAX
#define FLOAT_MIN FLT_MIN
#define DOUBLE_MIN DBL_MIN
#define DOUBLE_MAX DBL_MAX
#define FLOAT32_MAX FLT_MAX
#define FLOAT32_MIN FLT_MIN
#define FLOAT64_MAX DBL_MAX
#define FLOAT64_MIN DBL_MIN

#endif
