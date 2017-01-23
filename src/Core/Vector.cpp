#include "Vector.h"

float cross( vec2arg a, vec2arg b ) { return a.x * b.y - a.y * b.x; }
float dot( vec2arg a, vec2arg b ) { return a.x * b.x + a.y * b.y; }
float length( vec2arg v )
{
	auto lengthSquared = dot( v, v );
	return math::sqrt( lengthSquared );
}
bool hasMagnitude( vec2arg v )
{
	return dot( v, v ) > Float::Epsilon;
}
vec2 normalize( vec2arg v )
{
	auto lengthSquared = dot( v, v );
	assert( lengthSquared != 0 );
	auto oneOverLength = math::rsqrt( lengthSquared );
	return v * oneOverLength;
}
vec2 normalize( vec2arg v, float* outLength )
{
	assert( outLength );
	auto lengthSquared = dot( v, v );
	assert( lengthSquared != 0 );
	auto length = math::sqrt( lengthSquared );
	*outLength  = length;
	return v / length;
}
vec2 safeNormalize( vec2arg v, vec2arg def /* = {}*/ )
{
	auto lengthSquared = dot( v, v );
	if( lengthSquared == 0 ) {
		return def;
	}
	auto oneOverLength = math::rsqrt( lengthSquared );
	return v * oneOverLength;
}
vec2 safeNormalize( vec2arg v, float* outLength, vec2arg def /* = {}*/ )
{
	auto lengthSquared = dot( v, v );
	if( lengthSquared == 0 ) {
		*outLength = 0;
		return def;
	}
	auto length = math::sqrt( lengthSquared );
	*outLength  = length;
	return v / length;
}

float angle( vec2arg v ) { return math::atan2( v.y, v.x ); }
float angle( vec2arg a, vec2arg b ) { return math::atan2( cross( a, b ), dot( a, b ) ); }

vec2 rotate( vec2arg v, float angle )
{
	vec2 result;
	float c, s;
	math::sincos( angle, &c, &s );
	result.x = v.x * c - v.y * s;
	result.y = v.x * s + v.y * c;
	return result;
}
vec2 rotate( vec2arg v, float sin, float cos )
{
	vec2 result;
	result.x = v.x * cos - v.y * sin;
	result.y = v.x * sin + v.y * cos;
	return result;
}
vec2 rotateAround( vec2arg v, vec2arg origin, float angle )
{
	vec2 result;
	v -= origin;
	float c, s;
	math::sincos( angle, &c, &s );
	result.x = v.x * c - v.y * s;
	result.y = v.x * s + v.y * c;
	result += origin;
	return result;
}
vec2 rotateAround( vec2arg v, vec2arg origin, float sin, float cos )
{
	vec2 result;
	v -= origin;
	result.x = v.x * cos - v.y * sin;
	result.y = v.x * sin + v.y * cos;
	result += origin;
	return result;
}

vec2 floor( vec2arg v ) { return {floor( v.x ), floor( v.y )}; }
vec2 ceil( vec2arg v ) { return {ceil( v.x ), ceil( v.y )}; }
vec2 round( vec2arg v ) { return {round( v.x ), round( v.y )}; }
vec2 floorCentered( float x, float width )
{
	auto hw = floor( width * 0.5f );
	return {x - hw, x + ( width - hw )};
}

// vec3
vec3 cross( vec3arg a, vec3arg b )
{
	vec3 result;
	result.x = a.y * b.z - b.y * a.z;
	result.y = a.z * b.x - b.z * a.x;
	result.z = a.x * b.y - b.x * a.y;
	return result;
}
float dot( vec3arg a, vec3arg b ) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float length( vec3arg v )
{
	auto lengthSquared = dot( v, v );
	return math::sqrt( lengthSquared );
}
vec3 normalize( vec3arg v )
{
	auto lengthSquared = dot( v, v );
	assert( lengthSquared != 0 );
	auto oneOverLength = math::rsqrt( lengthSquared );
	return v * oneOverLength;
}
vec3 normalize( vec3arg v, float* outLength )
{
	assert( outLength );
	auto lengthSquared = dot( v, v );
	assert( lengthSquared != 0 );
	auto length = math::sqrt( lengthSquared );
	*outLength  = length;
	return v / length;
}
vec3 safeNormalize( vec3arg v, vec3arg def /* = {}*/ )
{
	auto lengthSquared = dot( v, v );
	if( lengthSquared == 0 ) {
		return def;
	}
	auto oneOverLength = math::rsqrt( lengthSquared );
	return v * oneOverLength;
}
vec3 safeNormalize( vec3arg v, float* outLength, vec3arg def /* = {}*/ )
{
	auto lengthSquared = dot( v, v );
	if( lengthSquared == 0 ) {
		*outLength = 0;
		return def;
	}
	auto length = math::sqrt( lengthSquared );
	*outLength  = length;
	return v / length;
}

// returns angle between a and b along normal n
// n should be normalized
float angle( vec3arg a, vec3arg b, vec3arg n )
{
	return math::atan2( dot( cross( a, b ), n ), dot( a, b ) );
}
float angle( vec3arg a, vec3arg b )
{
	return math::atan2( length( cross( a, b ) ), dot( a, b ) );
}

vec3 rotate( vec3arg v, float angle, vec3arg normal )
{
	auto halfAngle = angle * 0.5f;
	float c, s;
	math::sincos( halfAngle, &c, &s );
	return rotate( v, s, c, normal );
}
vec3 rotate( vec3arg v, float sin, float cos, vec3arg normal )
{
	vec3 result;
	float x = sin * normal.x;
	float y = sin * normal.y;
	float z = sin * normal.z;
	float w = cos;

	float xx = x * x;
	float yy = y * y;
	float zz = z * z;
	float ww = w * w;

	float xy = x * y;
	float xz = x * z;
	float xw = x * w;
	float yz = y * z;
	float yw = y * w;
	float zw = z * w;

	float m00 = xx - yy - zz + ww;
	float m11 = -xx + yy - zz + ww;
	float m22 = -xx - yy + zz + ww;

	float m01 = 2.0f * ( xy + zw );
	float m10 = 2.0f * ( xy - zw );

	float m02 = 2.0f * ( xz - yw );
	float m20 = 2.0f * ( xz + yw );

	float m12 = 2.0f * ( yz + xw );
	float m21 = 2.0f * ( yz - xw );

	result.x = v.x * m00 + v.y * m01 + v.z * m02;
	result.y = v.x * m10 + v.y * m11 + v.z * m12;
	result.z = v.x * m20 + v.y * m21 + v.z * m22;
	return result;
}
vec3 rotateAround( vec3arg v, vec3arg origin, float angle, vec3arg normal )
{
	auto halfAngle = angle * 0.5f;
	float c, s;
	math::sincos( halfAngle, &c, &s );
	return rotateAround( v, origin, c, s, normal );
}
vec3 rotateAround( vec3arg v, vec3arg origin, float sin, float cos, vec3arg normal )
{
	vec3 result;
	float x = sin * normal.x;
	float y = sin * normal.y;
	float z = sin * normal.z;
	float w = cos;

	float xx = x * x;
	float yy = y * y;
	float zz = z * z;
	float ww = w * w;

	float xy = x * y;
	float xz = x * z;
	float xw = x * w;
	float yz = y * z;
	float yw = y * w;
	float zw = z * w;

	float m00 = xx - yy - zz + ww;
	float m11 = -xx + yy - zz + ww;
	float m22 = -xx - yy + zz + ww;

	float m01 = 2.0f * ( xy + zw );
	float m10 = 2.0f * ( xy - zw );

	float m02 = 2.0f * ( xz - yw );
	float m20 = 2.0f * ( xz + yw );

	float m12 = 2.0f * ( yz + xw );
	float m21 = 2.0f * ( yz - xw );

	float m03 = origin.x - origin.x * m00 - origin.y * m01 - origin.z * m02;
	float m13 = origin.y - origin.x * m10 - origin.y * m11 - origin.z * m12;
	float m23 = origin.z - origin.x * m20 - origin.y * m21 - origin.z * m22;
	// float m30 = m31 = m32 = 0.0f;
	// float m33 = 1.0;

	result.x = v.x * m00 + v.y * m01 + v.z * m02 + m03;
	result.y = v.x * m10 + v.y * m11 + v.z * m12 + m13;
	result.z = v.x * m20 + v.y * m21 + v.z * m22 + m23;
	return result;
}

vec3 floor( vec3arg v ) { return {floor( v.x ), floor( v.y ), floor( v.z )}; }
vec3 ceil( vec3arg v ) { return {ceil( v.x ), ceil( v.y ), ceil( v.z )}; }
vec3 round( vec3arg v ) { return {round( v.x ), round( v.y ), round( v.z )}; }

// vec4

float dot( vec4arg a, vec4arg b ) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
float length( vec4arg v )
{
	auto lengthSquared = dot( v, v );
	return math::sqrt( lengthSquared );
}
vec4 normalize( vec4arg v )
{
	auto lengthSquared = dot( v, v );
	assert( lengthSquared != 0 );
	auto oneOverLength = math::rsqrt( lengthSquared );
	return v * oneOverLength;
}
vec4 normalize( vec4arg v, float* outLength )
{
	assert( outLength );
	auto lengthSquared = dot( v, v );
	assert( lengthSquared != 0 );
	auto length = math::sqrt( lengthSquared );
	*outLength  = length;
	return v / length;
}
vec4 safeNormalize( vec4arg v, vec4arg def /* = {}*/ )
{
	auto lengthSquared = dot( v, v );
	if( lengthSquared == 0 ) {
		return def;
	}
	auto oneOverLength = math::rsqrt( lengthSquared );
	return v * oneOverLength;
}
vec4 safeNormalize( vec4arg v, float* outLength, vec4arg def /* = {}*/ )
{
	auto lengthSquared = dot( v, v );
	if( lengthSquared == 0 ) {
		*outLength = 0;
		return def;
	}
	auto length = math::sqrt( lengthSquared );
	*outLength  = length;
	return v / length;
}

vec4 multiplyQuaternions( vec4arg a, vec4arg b )
{
	return {a.x * b.w + a.y * b.z - a.z * b.y + a.w * b.x,
	        -a.x * b.z + a.y * b.w + a.z * b.x + a.w * b.y,
	        a.x * b.y - a.y * b.x + a.z * b.w + a.w * b.z,
	        -a.x * b.x - a.y * b.y - a.z * b.z + a.w * b.w};
}