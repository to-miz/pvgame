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
bool testRayVsPlane( vec3arg rayOrigin, vec3arg rayDir, vec3arg planeOrigin, vec3arg planeNormal,
                     float* t = nullptr )
{
	auto denom = dot( rayDir, planeNormal );
	if( denom == 0 ) {
		auto projection = dot( planeOrigin - rayOrigin, planeNormal );
		if( projection > -0.0001f && projection < 0.0001f ) {
			if( t ) {
				*t = 0;
			}
			return true;
		}
		return false;
	}

	auto relative = planeOrigin - rayOrigin;
	auto t_       = dot( relative, planeNormal ) / denom;
	if( t_ >= 0 ) {
		if( t ) {
			*t = t_;
		}
		return true;
	}
	return false;
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
	auto tMin = max( mmX.min, mmY.min, mmZ.min );
	auto tMax = min( mmX.max, mmY.max, mmZ.max );

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
	auto tMin = max( mmX.min, mmY.min, mmZ.min );
	auto tMax = min( mmX.max, mmY.max, mmZ.max );

	out->enter.t      = tMin.t;
	out->enter.normal = tMin.normal;
	out->leave.t      = tMax.t;
	out->leave.normal = tMax.normal;
	return tMin.t <= tMax.t;
}