struct Camera {
	vec3 position;
	vec3 look;
	vec3 right;  // how the x axis is oriented
	vec3 up;     // how the y axis is oriented
};

Camera makeCamera( vec3arg position, vec3arg look, vec3arg up )
{
	Camera result;
	result.position = position;
	result.look     = normalize( look );
	result.up       = normalize( up );
	result.right    = normalize( cross( up, look ) );
	return result;
}
void updateCamera( Camera* camera, float xAngle, float yAngle )
{
	assert( camera );

	vec3 yDir = yAngle * camera->up;
	if( ( camera->up.y < 0.1f && camera->look.y < 0.0f && yDir.y < 0.0f )
	    || ( camera->up.y < 0.1f && camera->look.y >= 0.0f && yDir.y > 0.0f ) ) {
		yDir = {};
	}

	vec3 xDir     = xAngle * camera->right;
	camera->look  = normalize( camera->look + xDir + yDir );
	camera->right = safeNormalize( vec3{camera->look.z, 0, -camera->look.x}, camera->right );
	camera->up    = normalize( cross( camera->look, camera->right ) );
}
void updateCamera( Camera* camera, vec2arg angle ) { updateCamera( camera, angle.x, angle.y ); }
Camera cameraLookAt( const Camera& camera, vec3arg position )
{
	Camera result;
	result.position = camera.position;
	result.look     = normalize( position - camera.position );
	result.right    = normalize( cross( camera.up, result.look ) );
	result.up       = cross( result.look, result.right );
	return result;
}
Camera cameraLookDirection( const Camera& camera, vec3arg dir )
{
	assert( floatEqSoft( length( dir ), 1 ) );
	Camera result;
	result.position = camera.position;
	result.look     = dir;
	result.right    = normalize( cross( camera.up, dir ) );
	result.up       = cross( dir, result.right );
	return result;
}
mat4 getViewMatrix( Camera* camera )
{
	mat4 result;
	result.m[0] = camera->right.x;
	result.m[4] = camera->right.y;
	result.m[8] = camera->right.z;

	result.m[1] = camera->up.x;
	result.m[5] = camera->up.y;
	result.m[9] = camera->up.z;

	result.m[2]  = camera->look.x;
	result.m[6]  = camera->look.y;
	result.m[10] = camera->look.z;

	result.m[3]  = 0.0f;
	result.m[7]  = 0.0f;
	result.m[11] = 0.0f;

	result.m[12] = -dot( camera->right, camera->position );
	result.m[13] = -dot( camera->up, camera->position );
	result.m[14] = -dot( camera->look, camera->position );

	result.m[15] = 1.0f;
	return result;
}
vec3 center( Camera* camera )
{
	return {camera->position.x, camera->position.y + 50.0f, camera->position.z};
}