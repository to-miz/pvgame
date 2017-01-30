#define TM_BEZIER_IMPLEMENTATION
#define TMB_ASSERT assert
#define TMB_VECTOR vec2
#include <tm_bezier.h>

BezierForwardDifferencerData makeEasingCurve( vec2arg curve0, vec2arg curve1 )
{
	return computeBezierForwardDifferencer( 10, 0, 0, curve0.x, curve0.y, curve1.x, curve1.y, 1,
	                                        1 );
}
