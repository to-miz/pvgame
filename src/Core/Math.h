#pragma once
#ifndef _MATHS_H_INCLUDED_
#define _MATHS_H_INCLUDED_

#define USE_SSE2
#include <sse_mathfun_extension.h>

float cos( float x );
float sin( float x );
void sincos( float x, float* s, float* c );
float log( float x );
float exp( float x );
float tan( float x );
float cot( float x );
float atan( float x );
float atan2( float y, float x );
float sqrt( float x );
float rsqrt( float x );
float fast_rsqrt( float x );

constexpr const float HalfPi32 = 3.1415926535897932384626433832795f / 2.0f;
constexpr const float Pi32     = 3.1415926535897932384626433832795f;
constexpr const float TwoPi32  = 2 * 3.1415926535897932384626433832795f;

float degreesToRadians( float d );
float radiansToDegrees( float r );

float floor( float x );
float ceil( float x );
float round( float x );
float abs( float x );

#endif  // _MATHS_H_INCLUDED_
