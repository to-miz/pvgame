// TODO: decide which version to use, one using bitfields
// the other manually constructing 10 bit twos complement numbers
#if 0

#define NORMAL_X_SHIFT 0u
#define NORMAL_Y_SHIFT 10u
#define NORMAL_Z_SHIFT 20u
#define NORMAL_MASK 0x000003FFu
#define NORMAL_SIGN_MASK 0x00000200u
#define NORMAL_SIGN_LEFT_SHIFT 10u
#define NORMAL_X_MASK 0x000003FFu
#define NORMAL_Y_MASK 0x000FFC00u
#define NORMAL_Z_MASK 0x3FF00000u
#define NORMAL_W_MASK 0xC0000000u

#define NORMAL_N2B( x ) ( (uint32)( x ) & NORMAL_MASK )
#define NORMAL( x, y, z )                                                           \
	( ( NORMAL_N2B( x ) << NORMAL_X_SHIFT ) | ( NORMAL_N2B( y ) << NORMAL_Y_SHIFT ) \
	  | ( NORMAL_N2B( z ) << NORMAL_Z_SHIFT ) )

struct Normal {
	uint32 bits;

	inline static Normal xyz( float x, float y, float z )
	{
		// assume components are clamped within [-1, 1]
		// TODO: which version?
#if 0
		int32 xi = (int32)( x * ( ( signbit( x ) ) ? ( 512 ) : ( 511 ) ) );
		int32 yi = (int32)( y * ( ( signbit( y ) ) ? ( 512 ) : ( 511 ) ) );
		int32 zi = (int32)( z * ( ( signbit( z ) ) ? ( 512 ) : ( 511 ) ) );
#else
		// negative numbers are one off
		int32 xi = (int32)( x * 511 );
		int32 yi = (int32)( y * 511 );
		int32 zi = (int32)( z * 511 );	
#endif
		uint32 xs = ( (uint32)xi | ( ( (uint32)( xi < 0 ) ) << NORMAL_SIGN_LEFT_SHIFT ) );
		uint32 ys = ( (uint32)yi | ( ( (uint32)( yi < 0 ) ) << NORMAL_SIGN_LEFT_SHIFT ) );
		uint32 zs = ( (uint32)zi | ( ( (uint32)( zi < 0 ) ) << NORMAL_SIGN_LEFT_SHIFT ) );

		return {NORMAL( xs, ys, zs )};
	}
	inline static Normal xyz( vec3arg v )
	{
		return xyz( v.x, v.y, v.z );
	}
};

constexpr const Normal normal_pos_x_axis = {0x000001ffu};
constexpr const Normal normal_neg_x_axis = {0x00000200u};
constexpr const Normal normal_pos_y_axis = {0x0007fc00u};
constexpr const Normal normal_neg_y_axis = {0x00080000u};
constexpr const Normal normal_pos_z_axis = {0x1ff00000u};
constexpr const Normal normal_neg_z_axis = {0x20000000u};
#else
// this is only correct if the target platform happens to use twos complement
struct Normal {
	int x : 10;
	int y : 10;
	int z : 10;
	int unused : 2;
};

#define INT10_MAX ( 0x1ff )
#define INT10_MIN ( -0x200 )
constexpr const Normal normal_pos_x_axis = {INT10_MAX, 0, 0};
constexpr const Normal normal_neg_x_axis = {INT10_MIN, 0, 0};
constexpr const Normal normal_pos_y_axis = {0, INT10_MAX, 0};
constexpr const Normal normal_neg_y_axis = {0, INT10_MIN, 0};
constexpr const Normal normal_pos_z_axis = {0, 0, INT10_MAX};
constexpr const Normal normal_neg_z_axis = {0, 0, INT10_MIN};
#endif

static_assert( sizeof( Normal ) == 4, "Normal must be a 32bit packed int" );

vec3 unpackNormal( Normal normal )
{
	auto remap = []( int32 x ) {
		const float oneOverDiff = 1.0f / ( INT10_MAX - INT10_MIN );
		return ( ( x - INT10_MIN ) * oneOverDiff ) * 2 - 1;
	};
	return {remap( normal.x ), remap( normal.y ), remap( normal.z )};
}
Normal packNormal( vec3arg normal )
{
	return {remap( normal.x, -1, 1, INT10_MIN, INT10_MAX ),
	        remap( normal.y, -1, 1, INT10_MIN, INT10_MAX ),
	        remap( normal.z, -1, 1, INT10_MIN, INT10_MAX )};
}