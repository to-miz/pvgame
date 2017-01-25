struct Line3 {
	vec3 start;
	vec3 dir;
};
struct Ray3 {
	vec3 start;
	vec3 dir;
};

struct ShortestLineBetweenLinesResult {
	float tA, tB;
};
ShortestLineBetweenLinesResult shortestLineBetweenLines( vec3arg aStart, vec3arg aDir,
                                                         vec3arg bStart, vec3arg bDir )
{
	ShortestLineBetweenLinesResult result;
	auto diffStart = aStart - bStart;
	auto aa        = dot( aDir, aDir );
	auto ab        = dot( aDir, bDir );
	auto bb        = dot( bDir, bDir );
	auto pa        = dot( aDir, diffStart );
	auto pb        = dot( bDir, diffStart );

	auto denom = 1.0f / ( aa * bb - ab * ab );
	result.tA  = ( ab * pb - bb * pa ) * denom;
	result.tB  = ( aa * pb - ab * pa ) * denom;
	return result;
}
struct testRayVsPlaneResult {
	float t;
	inline explicit operator bool() const { return t != FLOAT_MAX; }
};
testRayVsPlaneResult testRayVsPlane( vec3arg rayOrigin, vec3arg rayDir, vec3arg planeOrigin,
                                     vec3arg planeNormal )
{
	testRayVsPlaneResult result = {FLOAT_MAX};

	auto denom = dot( rayDir, planeNormal );
	if( denom == 0 ) {
		auto projection = dot( planeOrigin - rayOrigin, planeNormal );
		if( floatEqZero( projection ) ) {
			result.t = 0;
		}
	} else {
		result.t = dot( planeOrigin - rayOrigin, planeNormal ) / denom;
	}
	return result;
}
bool testRayVsAabb( vec3arg rayOrigin, vec3arg rayDir, aabbarg box, float* t = nullptr )
{
	auto oneOverDirX = 1.0f / rayDir.x;
	auto oneOverDirY = 1.0f / rayDir.y;
	auto oneOverDirZ = 1.0f / rayDir.z;

	auto tMinX = ( box.min.x - rayOrigin.x ) * oneOverDirX;
	auto tMinY = ( box.min.y - rayOrigin.y ) * oneOverDirY;
	auto tMinZ = ( box.min.z - rayOrigin.z ) * oneOverDirZ;

	auto tMaxX = ( box.max.x - rayOrigin.x ) * oneOverDirX;
	auto tMaxY = ( box.max.y - rayOrigin.y ) * oneOverDirY;
	auto tMaxZ = ( box.max.z - rayOrigin.z ) * oneOverDirZ;

	auto mmX  = minmax( tMinX, tMaxX );
	auto mmY  = minmax( tMinY, tMaxY );
	auto mmZ  = minmax( tMinZ, tMaxZ );
	auto tMin = max( mmX.first, mmY.first, mmZ.first );
	auto tMax = min( mmX.second, mmY.second, mmZ.second );

	if( t ) {
		*t = tMin;
	}
	return tMin <= tMax;
}

struct TestRayVsAabbOption {
	float t;
	vec3i normal;
};
static bool operator<( const TestRayVsAabbOption& a, const TestRayVsAabbOption& b )
{
	return a.t < b.t;
}
struct TestRayVsAabbResult {
	TestRayVsAabbOption enter; // intersection with aabb when entering
	TestRayVsAabbOption leave; // intersection with aabb when leaving
};
bool testRayVsAabb( vec3arg rayOrigin, vec3arg rayDir, aabbarg box, TestRayVsAabbResult* out )
{
	assert( out );

	auto oneOverDirX = 1.0f / rayDir.x;
	auto oneOverDirY = 1.0f / rayDir.y;
	auto oneOverDirZ = 1.0f / rayDir.z;

	TestRayVsAabbOption tMinX = {( box.min.x - rayOrigin.x ) * oneOverDirX, {-1, 0, 0}};
	TestRayVsAabbOption tMinY = {( box.min.y - rayOrigin.y ) * oneOverDirY, {0, -1, 0}};
	TestRayVsAabbOption tMinZ = {( box.min.z - rayOrigin.z ) * oneOverDirZ, {0, 0, -1}};

	TestRayVsAabbOption tMaxX = {( box.max.x - rayOrigin.x ) * oneOverDirX, {1, 0, 0}};
	TestRayVsAabbOption tMaxY = {( box.max.y - rayOrigin.y ) * oneOverDirY, {0, 1, 0}};
	TestRayVsAabbOption tMaxZ = {( box.max.z - rayOrigin.z ) * oneOverDirZ, {0, 0, 1}};

	auto mmX  = minmax( tMinX, tMaxX );
	auto mmY  = minmax( tMinY, tMaxY );
	auto mmZ  = minmax( tMinZ, tMaxZ );
	auto tMin = max( mmX.first, mmY.first, mmZ.first );
	auto tMax = min( mmX.second, mmY.second, mmZ.second );

	out->enter.t      = tMin.t;
	out->enter.normal = tMin.normal;
	out->leave.t      = tMax.t;
	out->leave.normal = tMax.normal;
	return tMin.t <= tMax.t;
}

bool testRayVsObb( vec3arg rayOrigin, vec3arg rayDir, aabbarg box, mat4arg transform,
                   float* t = nullptr )
{
	vec3 xAxis  = {transform.m[0], transform.m[1], transform.m[2]};
	vec3 yAxis  = {transform.m[4], transform.m[5], transform.m[6]};
	vec3 zAxis  = {transform.m[8], transform.m[9], transform.m[10]};
	vec3 oobPos = {transform.m[12], transform.m[13], transform.m[14]};
	vec3 delta  = oobPos - rayOrigin;

	auto xDeltaProjected      = dot( xAxis, delta );
	auto yDeltaProjected      = dot( yAxis, delta );
	auto zDeltaProjected      = dot( zAxis, delta );
	auto xOneOverDirProjected = 1.0f / dot( xAxis, rayDir );
	auto yOneOverDirProjected = 1.0f / dot( yAxis, rayDir );
	auto zOneOverDirProjected = 1.0f / dot( zAxis, rayDir );

	auto tMinX = ( xDeltaProjected + box.min.x ) * xOneOverDirProjected;
	auto tMinY = ( yDeltaProjected + box.min.y ) * yOneOverDirProjected;
	auto tMinZ = ( zDeltaProjected + box.min.z ) * zOneOverDirProjected;

	auto tMaxX = ( xDeltaProjected + box.max.x ) * xOneOverDirProjected;
	auto tMaxY = ( yDeltaProjected + box.max.y ) * yOneOverDirProjected;
	auto tMaxZ = ( zDeltaProjected + box.max.z ) * zOneOverDirProjected;

	auto mmX  = minmax( tMinX, tMaxX );
	auto mmY  = minmax( tMinY, tMaxY );
	auto mmZ  = minmax( tMinZ, tMaxZ );
	auto tMin = max( mmX.first, mmY.first, mmZ.first );
	auto tMax = min( mmX.second, mmY.second, mmZ.second );

	if( t ) {
		*t = tMin;
	}
	return tMin <= tMax;
}