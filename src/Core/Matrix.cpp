#include "Matrix.h"

mat4 matrixIdentity()
{
	mat4 result;
	result.m[0] = result.m[5] = result.m[10] = result.m[15] = 1.0f;
	result.m[1] = result.m[2] = result.m[3] = result.m[4] = result.m[6] = result.m[7] =
		result.m[8] = result.m[9] = result.m[11] = result.m[12] = result.m[13] = result.m[14] =
			0.0f;
	return result;
}
mat4 matrixTranslation( float x, float y, float z )
{
	mat4 result = matrixIdentity();
	result.m[12] = x;
	result.m[13] = y;
	result.m[14] = z;
	return result;
}
mat4 matrixScale( float x, float y, float z )
{
	mat4 result = {x, 0, 0, 0,
				   0, y, 0, 0,
				   0, 0, z, 0,
				   0, 0, 0, 1};
	return result;
}
mat4 matrixRotation( float x, float y, float z )
{
	mat4 result;
	auto sc = simd::sincos( x, y, z, 0 );
	float A = sc.c[0];
	float B = sc.s[0];
	float C = sc.c[1];
	float D = sc.s[1];
	float E = sc.c[2];
	float F = sc.s[2];

	float AD = A * D;
	float BD = B * D;

	result.m[0] = C * E;
	result.m[1] = C * F;
	result.m[2] = -D;
	result.m[4] = BD * E - A * F;
	result.m[5] = BD * F + A * E;
	result.m[6] = B * C;
	result.m[8] = AD * E + B * F;
	result.m[9] = AD * F - B * E;
	result.m[10] = A * C;

	result.m[3] = result.m[7] = result.m[11] = result.m[12] = result.m[13] = result.m[14] = 0;
	result.m[15] = 1;
	return result;
}
mat4 matrixRotation( vec3arg r )
{
	return matrixRotation( r.x, r.y, r.z );
}
mat4 matrixRotationX( float angle )
{
	float s, c;
	sincos( angle, &s, &c );

	mat4 result;
	result.m[0] = result.m[15] = 1;
	result.m[1] = result.m[2] = result.m[3] = result.m[4] = result.m[7] = result.m[8] =
		result.m[11] = result.m[12] = result.m[13] = result.m[14] = 0;

	result.m[5] = c;
	result.m[6] = s;
	result.m[9] = -s;
	result.m[10] = c;
	return result;
}
mat4 matrixRotationY( float angle )
{
	float s, c;
	sincos( angle, &s, &c );

	mat4 result;
	result.m[1] = result.m[3] = result.m[4] = result.m[6] = result.m[7] = result.m[9] =
		result.m[11] = result.m[12] = result.m[13] = result.m[14] = 0;
	result.m[5] = result.m[15] = 1;

	result.m[0] = c;
	result.m[2] = -s;
	result.m[8] = s;
	result.m[10] = c;
	return result;
}
mat4 matrixRotationZ( float angle )
{
	float s, c;
	sincos( angle, &s, &c );

	mat4 result;
	result.m[2] = result.m[3] = result.m[6] = result.m[7] = result.m[8] = result.m[9] =
		result.m[11] = result.m[12] = result.m[13] = result.m[14] = 0;

	result.m[10] = result.m[15] = 1;

	result.m[0] = c;
	result.m[1] = s;
	result.m[4] = -s;
	result.m[5] = c;
	return result;
}
mat4 matrixRotationZOrigin( float angle, float originx, float originy )
{
	mat4 result;
	float s, c;
	sincos( angle, &s, &c );
	auto a = -originx * c + originy * s + originx;
	auto b = -originx * s - originy * c + originy;

	result.m[0]  = c;
	result.m[1]  = s;
	result.m[2]  = 0;
	result.m[3]  = 0;
	result.m[4]  = -s;
	result.m[5]  = c;
	result.m[6]  = 0;
	result.m[7]  = 0;
	result.m[8]  = 0;
	result.m[9]  = 0;
	result.m[10] = 1;
	result.m[11] = 0;
	result.m[12] = a;
	result.m[13] = b;
	result.m[14] = 0;
	result.m[15] = 1;

	return result;
}

mat4 matrixOrthogonalProjection( float left, float top, float right, float bottom, float nearPlane,
                                 float farPlane )
{
	// TODO: do we need to flip z here?
	mat4 result;
	auto width  = right - left;
	auto height = bottom - top;

	result.m[1] = result.m[2] = result.m[3] = result.m[4] = result.m[6] = result.m[7] =
	    result.m[8] = result.m[9] = result.m[11] = 0;
	result.m[15]                                 = 1;

	result.m[0]      = 2 / width;
	result.m[5]      = 2 / -height;
	auto oneOverDiff = 1 / ( farPlane - nearPlane );
	result.m[10]     = oneOverDiff;
	result.m[12]     = ( left + right ) / -width;
	result.m[13]     = ( top + bottom ) / height;
	result.m[14]     = nearPlane * -oneOverDiff;

	return result;
}
mat4 matrixOrthogonalProjection( float width, float height, float nearPlane, float farPlane )
{
	/*2/w  0    0           0
	  0    2/h  0           0
	  0    0    1/(zf-zn)   0
	  0    0   -zn/(zf-zn)  1*/
	// TODO: do we need to flip z here?
	mat4 result;
	result.m[1] = result.m[2] = result.m[3] = result.m[4] = result.m[6] = result.m[7] =
		result.m[8] = result.m[9] = result.m[11] = result.m[12] = result.m[13] = 0;
	result.m[15] = 1;

	result.m[0] = 2 / width;
	result.m[5] = 2 / height;
	auto oneOverDiff = 1 / ( farPlane - nearPlane );
	result.m[10] = oneOverDiff;
	result.m[14] = -nearPlane * oneOverDiff;

	return result;
}
mat4 matrixPerspectiveProjection( float width, float height, float nearPlane, float farPlane )
{
	/*2*nearPlane/w    0                0                                        0
	0                  2*nearPlane/h    0                                        0
	0                  0                farPlane/(farPlane-nearPlane)            1
	0                  0                nearPlane*farPlane/(nearPlane-farPlane)  0*/
	mat4 result;
	result.m[1] = result.m[2] = result.m[3] = result.m[4] = result.m[6] = result.m[7] =
		result.m[8] = result.m[9] = result.m[12] = result.m[13] = result.m[15] = 0;
	result.m[11] = 1;

#ifdef GAME_FLIP_Z
	nearPlane = -nearPlane;
	farPlane = -farPlane;
#endif
	result.m[0] = 2 * nearPlane / width;
	result.m[5] = 2 * nearPlane / height;
	result.m[10] = farPlane / ( farPlane - nearPlane );
	result.m[14] = nearPlane * farPlane / ( nearPlane - farPlane ) ;
#ifdef GAME_FLIP_Z
	result.m[10] = -result.m[10];
	result.m[14] = -result.m[14];
#endif
	return result;
}
mat4 matrixPerspectiveFovProjection( float fovY, float aspect, float nearPlane, float farPlane )
{
	/*xScale     0          0                                         0
	  0          yScale     0                                         0
	  0          0          farPlane/(farPlane-nearPlane)             1
	  0          0          -nearPlane*farPlane/(farPlane-nearPlane)  0
	  where:
	  yScale = cot(fovY/2)
	  xScale = yScale / aspect ratio*/

	mat4 result;
#ifdef GAME_FLIP_Z
	nearPlane = -nearPlane;
	farPlane = -farPlane;
#endif
	float yScale = cot( fovY * 0.5f );
	float xScale = yScale / aspect;

	result.m[1] = result.m[2] = result.m[3] = result.m[4] = result.m[6] = result.m[7] =
		result.m[8] = result.m[9] = result.m[12] = result.m[13] = result.m[15] = 0;
	result.m[11] = 1;

	result.m[0] = xScale;
	result.m[5] = yScale;
	auto plane = farPlane / ( farPlane - nearPlane );
	result.m[10] = plane;
	result.m[14] = -nearPlane * plane;

#ifdef GAME_FLIP_Z
	result.m[10] = -result.m[10];
	result.m[14] = -result.m[14];
#endif

	/*float f = cot( fovY );

	result.m[1] = result.m[2] = result.m[3] = result.m[4] = result.m[6] = result.m[7] =
		result.m[8] = result.m[9] = result.m[12] = result.m[13] = result.m[15] = 0;
	result.m[11] = -1;

	result.m[0] = f / aspect;
	result.m[1 * 4 + 1] = f;
	result.m[2 * 4 + 2] = (farPlane + nearPlane) / (nearPlane - farPlane);
	result.m[3 * 4 + 2] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
	result.m[3 * 4 + 3] = 0.0f;*/
	return result;
}
mat4 matrixLookAt( vec3arg position, vec3arg lookAt, vec3 up /* = {0, 1, 0}*/ )
{
	/*
	zaxis = normal(lookAt - position)
	xaxis = normal(cross(up, zaxis))
	yaxis = cross(zaxis, xaxis)

	 xaxis.x           yaxis.x           zaxis.x          0
	 xaxis.y           yaxis.y           zaxis.y          0
	 xaxis.z           yaxis.z           zaxis.z          0
	-dot(xaxis, position)  -dot(yaxis, position)  -dot(zaxis, position)  1
	*/
	mat4 result;
	auto dir = safeNormalize( lookAt - position );
	auto right = safeNormalize( cross( up, dir ) );

	up = cross( dir, right );
	result.m[0] = right.x;
	result.m[4] = right.y;
	result.m[8] = right.z;

	result.m[1] = up.x;
	result.m[5] = up.y;
	result.m[9] = up.z;

	result.m[2] = dir.x;
	result.m[6] = dir.y;
	result.m[10] = dir.z;

	result.m[3] = 0.0f;
	result.m[7] = 0.0f;
	result.m[11] = 0.0f;

	result.m[12] = -dot( right, position );
	result.m[13] = -dot( up, position );
	result.m[14] = -dot( dir, position );

	result.m[15] = 1.0f;
	return result;
}

mat4 operator*( mat4arg lhs, mat4arg rhs )
{
	mat4 result;
	for( auto i = 0; i < 16; i += 4 ) {
	    for( auto j = 0; j < 4; ++j ) {
	        result.m[i + j] = ( lhs.m[i + 0] * rhs.m[0 + j] ) + ( lhs.m[i + 1] * rhs.m[4 + j] )
	                          + ( lhs.m[i + 2] * rhs.m[8 + j] ) + ( lhs.m[i + 3] * rhs.m[12 + j] );
	    }
	}
	return result;
}

// all transformVector functions return homogeneous coords

vec4 transformVector4( mat4arg matrix, vec4arg v )
{
	vec4 result;
	result.x = matrix.m[0] * v.x + matrix.m[4] * v.y + matrix.m[8] * v.z + matrix.m[12] * v.w;
	result.y = matrix.m[1] * v.x + matrix.m[5] * v.y + matrix.m[9] * v.z + matrix.m[13] * v.w;
	result.z = matrix.m[2] * v.x + matrix.m[6] * v.y + matrix.m[10] * v.z + matrix.m[14] * v.w;
	result.w = matrix.m[3] * v.x + matrix.m[7] * v.y + matrix.m[11] * v.z + matrix.m[15] * v.w;
	return result;
}
vec4 transformVector4( mat4arg matrix, vec3arg v )
{
	vec4 result;
	result.x = matrix.m[0] * v.x + matrix.m[4] * v.y + matrix.m[8] * v.z + matrix.m[12];
	result.y = matrix.m[1] * v.x + matrix.m[5] * v.y + matrix.m[9] * v.z + matrix.m[13];
	result.z = matrix.m[2] * v.x + matrix.m[6] * v.y + matrix.m[10] * v.z + matrix.m[14];
	result.w = matrix.m[3] * v.x + matrix.m[7] * v.y + matrix.m[11] * v.z + matrix.m[15];
	return result;
}
vec3 transformVector3( mat4arg matrix, vec3arg v )
{
	vec3 result;
	result.x = matrix.m[0] * v.x + matrix.m[4] * v.y + matrix.m[8] * v.z + matrix.m[12];
	result.y = matrix.m[1] * v.x + matrix.m[5] * v.y + matrix.m[9] * v.z + matrix.m[13];
	result.z = matrix.m[2] * v.x + matrix.m[6] * v.y + matrix.m[10] * v.z + matrix.m[14];
	return result;
}
vec2 transformVector2( mat4arg matrix, vec2arg v )
{
	vec2 result;
	result.x = matrix.m[0] * v.x + matrix.m[4] * v.y + matrix.m[12];
	result.y = matrix.m[1] * v.x + matrix.m[5] * v.y + matrix.m[13];
	return result;
}
vec3 transformVector( mat4arg matrix, vec3arg v ) { return transformVector3( matrix, v ); }
vec2 transformVector( mat4arg matrix, vec2arg v ) { return transformVector2( matrix, v ); }

vec3 toScreenSpace3( mat4arg worldViewProj, vec3arg v, float width, float height )
{
	// transform to homogeneous space
	auto t = transformVector4( worldViewProj, v );
	// to clip space
	t.xyz /= t.w;
	// to screen space
	t.x = ( t.x + 1 ) * ( width * 0.5f );
	// invert y because screen space origin is top left instead of bottom left
	t.y = ( 1 - t.y ) * ( height * 0.5f );
	t.z = ( t.z + 1 ) * 0.5f;
	return t.xyz;
}
vec2 toScreenSpace( mat4arg worldViewProj, vec3arg v, float width, float height )
{
	return toScreenSpace3( worldViewProj, v, width, height ).xy;
}

vec3 toWorldSpace( mat4arg invertedWorldViewProj, vec3arg v, float width, float height )
{
	vec4 p = { ( v.x / width ) * 2 - 1, ( 1 - ( v.y / height ) * 2 ), v.z * 2 - 1, 1};
	p = transformVector4( invertedWorldViewProj, p );
	p.xyz *= 1.0f / p.w;
	return p.xyz;
}

struct MatrixInverseResult {
	bool valid;
	mat4 matrix;
	inline explicit operator bool() const { return valid; };
};
MatrixInverseResult inverse( mat4arg mat )
{
	// formula from http://www.cg.info.hiroshima-cu.ac.jp/~miyazaki/knowledge/teche23.html
	MatrixInverseResult result;
	result.valid = false;

	auto m11 = mat.m[0];
	auto m12 = mat.m[1];
	auto m13 = mat.m[2];
	auto m14 = mat.m[3];
	auto m21 = mat.m[4];
	auto m22 = mat.m[5];
	auto m23 = mat.m[6];
	auto m24 = mat.m[7];
	auto m31 = mat.m[8];
	auto m32 = mat.m[9];
	auto m33 = mat.m[10];
	auto m34 = mat.m[11];
	auto m41 = mat.m[12];
	auto m42 = mat.m[13];
	auto m43 = mat.m[14];
	auto m44 = mat.m[15];
	// clang-format off
	float* out = result.matrix.m;
	out[ 0] = m22*m33*m44 + m23*m34*m42 + m24*m32*m43 - m22*m34*m43 - m23*m32*m44 - m24*m33*m42;
	out[ 1] = m12*m34*m43 + m13*m32*m44 + m14*m33*m42 - m12*m33*m44 - m13*m34*m42 - m14*m32*m43;
	out[ 2] = m12*m23*m44 + m13*m24*m42 + m14*m22*m43 - m12*m24*m43 - m13*m22*m44 - m14*m23*m42;
	out[ 3] = m12*m24*m33 + m13*m22*m34 + m14*m23*m32 - m12*m23*m34 - m13*m24*m32 - m14*m22*m33;
	out[ 4] = m21*m34*m43 + m23*m31*m44 + m24*m33*m41 - m21*m33*m44 - m23*m34*m41 - m24*m31*m43;
	out[ 5] = m11*m33*m44 + m13*m34*m41 + m14*m31*m43 - m11*m34*m43 - m13*m31*m44 - m14*m33*m41;
	out[ 6] = m11*m24*m43 + m13*m21*m44 + m14*m23*m41 - m11*m23*m44 - m13*m24*m41 - m14*m21*m43;
	out[ 7] = m11*m23*m34 + m13*m24*m31 + m14*m21*m33 - m11*m24*m33 - m13*m21*m34 - m14*m23*m31;
	out[ 8] = m21*m32*m44 + m22*m34*m41 + m24*m31*m42 - m21*m34*m42 - m22*m31*m44 - m24*m32*m41;
	out[ 9] = m11*m34*m42 + m12*m31*m44 + m14*m32*m41 - m11*m32*m44 - m12*m34*m41 - m14*m31*m42;
	out[10] = m11*m22*m44 + m12*m24*m41 + m14*m21*m42 - m11*m24*m42 - m12*m21*m44 - m14*m22*m41;
	out[11] = m11*m24*m32 + m12*m21*m34 + m14*m22*m31 - m11*m22*m34 - m12*m24*m31 - m14*m21*m32;
	out[12] = m21*m33*m42 + m22*m31*m43 + m23*m32*m41 - m21*m32*m43 - m22*m33*m41 - m23*m31*m42;
	out[13] = m11*m32*m43 + m12*m33*m41 + m13*m31*m42 - m11*m33*m42 - m12*m31*m43 - m13*m32*m41;
	out[14] = m11*m23*m42 + m12*m21*m43 + m13*m22*m41 - m11*m22*m43 - m12*m23*m41 - m13*m21*m42;
	out[15] = m11*m22*m33 + m12*m23*m31 + m13*m21*m32 - m11*m23*m32 - m12*m21*m33 - m13*m22*m31;
	// clang-format on

	auto det = m11 * out[0] + m12 * out[4] + m13 * out[8] + m14 * out[12];
	if( det != 0 ) {
		auto inv = 1.0f / det;
		for( auto i = 0; i < 16; ++i ) {
			out[i] *= inv;
		}
		result.valid = true;
	}

	return result;
}