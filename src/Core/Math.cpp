#include "Math.h"

namespace simd
{
struct vec4 {
	__m128 data;

	static vec4 make( float a, float b, float c, float d ) { return {_mm_set_ps( a, b, c, d )}; }
};

struct vec4result {
	alignas( 16 ) float elements[4];

	float operator[]( int32 i ) const
	{
		assert( i >= 0 && i < 4 );
		return elements[i];
	}
};

vec4result toResult( const vec4& v )
{
	vec4result result;
	_mm_store_ps( result.elements, v.data );
	return result;
}

struct sincos_result4 {
	vec4result s;
	vec4result c;
};
sincos_result4 sincos( const vec4& angles )
{
	v4sf s;
	v4sf c;
	sincos_ps( angles.data, &s, &c );
	sincos_result4 result;
	_mm_store_ps( result.s.elements, s );
	_mm_store_ps( result.c.elements, c );
	return result;
}
sincos_result4 sincos( float a, float b, float c, float d )
{
	return sincos( vec4::make( a, b, c, d ) );
}

struct sincos_result {
	float s;
	float c;
};
sincos_result sincos( float angle )
{
	sincos_result result;
	::sincos( angle, &result.s, &result.c );
	return result;
}
}

float cos( float x )
{
	v4sf sse_value = _mm_set_ps1( x );
	sse_value = cos_ps( sse_value );
	return _mm_cvtss_f32( sse_value );
}
float sin( float x )
{
	v4sf sse_value = _mm_set_ps1( x );
	sse_value = sin_ps( sse_value );
	return _mm_cvtss_f32( sse_value );
}
void sincos( float x, float* s, float* c )
{
	assert( s );
	assert( c );
	v4sf sse_value = _mm_set_ps1( x );
	v4sf sse_s;
	v4sf sse_c;
	sincos_ps( sse_value, &sse_s, &sse_c );
	*s = _mm_cvtss_f32( sse_s );
	*c = _mm_cvtss_f32( sse_c );
}
float log( float x )
{
	v4sf sse_value = _mm_set_ps1( x );
	sse_value = log_ps( sse_value );
	return _mm_cvtss_f32( sse_value );
}
float exp( float x )
{
	v4sf sse_value = _mm_set_ps1( x );
	sse_value = exp_ps( sse_value );
	return _mm_cvtss_f32( sse_value );
}
float tan( float x )
{
	v4sf sse_value = _mm_set_ps1( x );
	sse_value = tan_ps( sse_value );
	return _mm_cvtss_f32( sse_value );
}
float cot( float x )
{
	v4sf sse_value = _mm_set_ps1( x );
	sse_value = cot_ps( sse_value );
	return _mm_cvtss_f32( sse_value );
}
float atan( float x )
{
	v4sf sse_value = _mm_set_ps1( x );
	sse_value = atan_ps( sse_value );
	return _mm_cvtss_f32( sse_value );
}
float atan2( float y, float x )
{
	v4sf sse_y = _mm_set_ps1( y );
	v4sf sse_x = _mm_set_ps1( x );
	v4sf sse_result = atan2_ps( sse_y, sse_x );
	return _mm_cvtss_f32( sse_result );
}

float sqrt( float x )
{
	return sqrt_ps( x );
}
float rsqrt( float x )
{
#ifdef MATH_FAST_RSQRT
	return fast_rsqrt( x );
#else
	v4sf val = _mm_div_ps( _mm_set1_ps( 1.0f ), _mm_sqrt_ps( _mm_set_ps1( x ) ) );
	return _mm_cvtss_f32( val );
#endif
}
float fast_rsqrt( float x )
{
	return rsqrt_ps( x );
}

float degreesToRadians( float d )
{
	return d * ( TwoPi32 / 360 );
}
float radiansToDegrees( float r )
{
	return r * ( 360 / TwoPi32 );
}

#if defined( _MSC_VER ) && !defined( __clang__ )
	#ifdef ARCHITECTURE_X64
		extern "C" {
			float floorf( float );
			float ceilf( float );
		}
		#pragma intrinsic( floorf, ceilf )
		float floor( float x )
		{
			return floorf( x );
		}
		float ceil( float x )
		{
			return ceilf( x );
		}
	#elif ARCHITECTURE_X86
		extern "C" {
			double floor( double );
			double ceil( double );
		}
		#pragma intrinsic( floor, ceil )
		float floor( float x )
		{
			return (float)floor( (double)x );
		}
		float ceil( float x )
		{
			return (float)ceil( (double)x );
		}
	#endif
#else
	extern "C" {
		float floorf( float );
		float ceilf( float );
	}
	float floor( float x )
	{
		return floorf( x );
	}
	float ceil( float x )
	{
		return ceilf( x );
	}
#endif

float abs( float x )
{
	auto bits     = bit_cast< uint32 >( x );
	auto abs_bits = bits & ~( FLT_SIGN_MASK );
	return bit_cast< float >( abs_bits );
}
float round( float x ) { return floor( x + 0.5f ); }