#include "Math.h"

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
	*s = ( (float*)&sse_s )[0];
	*c = ( (float*)&sse_c )[0];
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
	return 1.0f / sqrt( x );
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