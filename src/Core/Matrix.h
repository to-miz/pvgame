#pragma once

#ifndef _MATRIX_H_INCLUDED_
#define _MATRIX_H_INCLUDED_

struct mat4 {
	float m[16];

	float operator[]( int32 i )
	{
		assert( i >= 0 );
		assert( i < 16 );
		return m[i];
	};
};

mat4 matrixIdentity();
mat4 matrixTranslation( float x, float y, float z );
mat4 matrixScale( float x, float y, float z );
mat4 matrixRotation( float x, float y, float z );
mat4 matrixRotationX( float angle );
mat4 matrixRotationY( float angle );
mat4 matrixRotationZ( float angle );

inline mat4 matrixTranslation( vec3arg v ) { return matrixTranslation( v.x, v.y, v.z ); }
mat4 matrixOrthogonalProjection( float left, float top, float right, float bottom, float nearPlane,
                                 float farPlane );
mat4 matrixPerspectiveProjection( float width, float height, float nearPlane, float farPlane );
mat4 matrixPerspectiveFovProjection( float fovY, float aspect, float nearPlane, float farPlane );
mat4 matrixLookAt( vec3arg position, vec3arg lookAt, vec3 up = {0, 1, 0} );

#if defined( MATRIX_ARGS_AS_CONST_REF ) && MATRIX_ARGS_AS_CONST_REF == 1
	typedef const mat4& mat4arg;
#else
	typedef mat4 mat4arg;
#endif

mat4 operator*( mat4arg lhs, mat4arg rhs );
vec3 transformVector( mat4arg matrix, vec3arg v );

mat4 rotateZLocal( mat4arg mat, float angle )
{
	mat4 result = mat;
	float s, c;
	sincos( angle, &s, &c );

	auto m0     = result.m[0];
	auto m1     = result.m[1];
	auto m2     = result.m[2];
	auto m3     = result.m[3];
	result.m[0] = c * m0 + s * result.m[4];
	result.m[1] = c * m1 + s * result.m[5];
	result.m[2] = c * m2 + s * result.m[6];
	result.m[3] = c * m3 + s * result.m[7];
	result.m[4] = -s * m0 + c * result.m[4];
	result.m[5] = -s * m1 + c * result.m[5];
	result.m[6] = -s * m2 + c * result.m[6];
	result.m[7] = -s * m3 + c * result.m[7];
	return result;
}
mat4 rotateZOriginLocal( mat4arg mat, float angle, float originx, float originy )
{
	mat4 result = mat;
	float s, c;
	sincos( angle, &s, &c );
	auto a = -originx * c + originy * s + originx;
	auto b = -originx * s - originy * c + originy;

	auto m0 = result.m[0];
	auto m1 = result.m[1];
	auto m2 = result.m[2];
	auto m3 = result.m[3];

	result.m[0] = c * m0 + s * result.m[4];
	result.m[1] = c * m1 + s * result.m[5];
	result.m[2] = c * m2 + s * result.m[6];
	result.m[3] = c * m3 + s * result.m[7];
	result.m[12] += a * m0 + b * result.m[4];
	result.m[13] += a * m1 + b * result.m[5];
	result.m[14] += a * m2 + b * result.m[6];
	result.m[15] += a * m3 + b * result.m[7];
	result.m[4] = -s * m0 + c * result.m[4];
	result.m[5] = -s * m1 + c * result.m[5];
	result.m[6] = -s * m2 + c * result.m[6];
	result.m[7] = -s * m3 + c * result.m[7];

	return result;
}
mat4 translateLocal( mat4arg mat, float x, float y, float z )
{
	mat4 result  = mat;
	result.m[12] = x * result.m[0] + y * result.m[4] + z * result.m[8] + result.m[12];
	result.m[13] = x * result.m[1] + y * result.m[5] + z * result.m[9] + result.m[13];
	result.m[14] = x * result.m[2] + y * result.m[6] + z * result.m[10] + result.m[14];
	result.m[15] = x * result.m[3] + y * result.m[7] + z * result.m[11] + result.m[15];
	return result;
}
mat4 scaleLocal( mat4arg mat, float x, float y, float z )
{
	mat4 result = mat;
	result.m[0] *= x;
	result.m[1] *= x;
	result.m[2] *= x;
	result.m[3] *= x;
	result.m[4] *= y;
	result.m[5] *= y;
	result.m[6] *= y;
	result.m[7] *= y;
	result.m[8] *= z;
	result.m[9] *= z;
	result.m[10] *= z;
	result.m[11] *= z;
	return result;
}
mat4 scaleOriginLocal( mat4arg mat, float sx, float sy, float originx, float originy )
{
	mat4 result = mat;
	auto a      = -originx * sx + originx;
	auto b      = -originy * sy + originy;

	result.m[12] += a * result.m[0] + b * result.m[4];
	result.m[13] += a * result.m[1] + b * result.m[5];
	result.m[14] += a * result.m[2] + b * result.m[6];
	result.m[15] += a * result.m[3] + b * result.m[7];
	result.m[0] *= sx;
	result.m[1] *= sx;
	result.m[2] *= sx;
	result.m[3] *= sx;
	result.m[4] *= sy;
	result.m[5] *= sy;
	result.m[6] *= sy;
	result.m[7] *= sy;

	return result;
}

// MatrixStack
struct MatrixStack {
	int32 currentMatrix;
	int32 size;
	mat4* data;
};

MatrixStack makeMatrixStack( StackAllocator* allocator, int32 matrixCount )
{
	assert( allocator );
	MatrixStack result = {};
	result.size = matrixCount;
	result.data = allocateArray( allocator, mat4, matrixCount );
	if( matrixCount ) {
		result.data[0] = matrixIdentity();
	}
	return result;
}
mat4& currentMatrix( MatrixStack* stack )
{
	assert( stack );
	assert( stack->currentMatrix < stack->size );
	return stack->data[unsignedof( stack->currentMatrix )];
}
void pushMatrix( MatrixStack* stack )
{
	assert( stack );
	++stack->currentMatrix;
	stack->data[stack->currentMatrix] = stack->data[stack->currentMatrix - 1];
}
void popMatrix( MatrixStack* stack )
{
	assert( stack );
	--stack->currentMatrix;
	assert( stack->currentMatrix >= 0 );
}

void loadIdentity( MatrixStack* stack )
{
	currentMatrix( stack ) = matrixIdentity();
}
void rotateZ( MatrixStack* stack, float angle )
{
	currentMatrix( stack ) = rotateZLocal( currentMatrix( stack ), angle );
}
void rotateZOrigin( MatrixStack* stack, float angle, vec2 origin )
{
	currentMatrix( stack ) = rotateZOriginLocal( currentMatrix( stack ), angle, origin.x, origin.y );
}
void translate( MatrixStack* stack, float x, float y, float z = 0.0f )
{
	currentMatrix( stack ) = translateLocal( currentMatrix( stack ), x, y, z );
}
void translate( MatrixStack* stack, vec3 v )
{
	currentMatrix( stack ) = translateLocal( currentMatrix( stack ), v.x, v.y, v.z );
}
void translate( MatrixStack* stack, vec2 v, float z = 0.0f )
{
	currentMatrix( stack ) = translateLocal( currentMatrix( stack ), v.x, v.y, z );
}
void scale( MatrixStack* stack, float x, float y, float z = 1.0f )
{
	currentMatrix( stack ) = scaleLocal( currentMatrix( stack ), x, y, z );
}
void scale( MatrixStack* stack, vec2 v, float z = 1.0f )
{
	currentMatrix( stack ) = scaleLocal( currentMatrix( stack ), v.x, v.y, z );
}
void scaleOrigin( MatrixStack* stack, vec2 v, vec2 origin )
{
	currentMatrix( stack ) =
	    scaleOriginLocal( currentMatrix( stack ), v.x, v.y, origin.x, origin.y );
}
void scaleOrigin( MatrixStack* stack, float x, float y, vec2 origin )
{
	currentMatrix( stack ) =
	    scaleOriginLocal( currentMatrix( stack ), x, y, origin.x, origin.y );
}
void multMatrix( MatrixStack* stack, mat4arg other )
{
	auto& cur = currentMatrix( stack );
	cur = other * cur;
}

#endif  // _MATRIX_H_INCLUDED_
