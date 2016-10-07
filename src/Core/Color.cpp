#define ALPHA_SHIFT 24u
#define RED_SHIFT 16u
#define GREEN_SHIFT 8u
#define BLUE_SHIFT 0u
#define ALPHA_MASK 0xFF000000u
#define RED_MASK 0x00FF0000u
#define GREEN_MASK 0x0000FF00u
#define BLUE_MASK 0x000000FFu
#define F2B_RANGE 255.99

#define U2B( x ) ( ( uint32 )(x)&0xFFu )
#define F2B( x ) U2B( (x)*F2B_RANGE )
#define B2F( x ) ( ( x ) * ( 1.0f / 255.0f ) )

#define ARGB( a, r, g, b )                                            \
	( ( ( U2B( a ) ) << ALPHA_SHIFT ) | ( ( U2B( r ) ) << RED_SHIFT ) \
	  | ( ( U2B( g ) ) << GREEN_SHIFT ) | ( ( U2B( b ) ) << BLUE_SHIFT ) )

struct Color {
	uint32 bits;

	constexpr inline static Color make( uint32 color ) { return {color}; }
	constexpr inline static Color hex( uint32 color ) { return {color}; }
	constexpr inline static Color argb( uint32 a, uint32 r, uint32 g, uint32 b )
	{
		return {ARGB( a, r, g, b )};
	}
	CONSTEXPR inline static Color argb( float a, float r, float g, float b )
	{
		const_assert( a >= 0 && a <= 1 );
		const_assert( r >= 0 && r <= 1 );
		const_assert( g >= 0 && g <= 1 );
		const_assert( b >= 0 && b <= 1 );
		return {ARGB( F2B( a ), F2B( r ), F2B( g ), F2B( b ) )};
	}
	CONSTEXPR inline static Color argb( vec4 color )
	{
		const_assert( color.a >= 0 && color.a <= 1 );
		const_assert( color.r >= 0 && color.r <= 1 );
		const_assert( color.g >= 0 && color.g <= 1 );
		const_assert( color.b >= 0 && color.b <= 1 );
		return {ARGB( F2B( color.a ), F2B( color.r ), F2B( color.g ), F2B( color.b ) )};
	}

	inline Color& operator=( uint32 bits )
	{
		this->bits = bits;
		return *this;
	}
	operator uint32() const { return bits; }

	static const Color White;
	static const Color Black;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Purple;
	static const Color Cyan;
};

CONSTEXPR Color setAlpha( uint32 color, float v )
{
	const_assert( v >= 0 && v <= 1 );
	return {( F2B( v ) << ALPHA_SHIFT ) | ( color & ~ALPHA_MASK )};
}
CONSTEXPR Color setRed( uint32 color, float v )
{
	const_assert( v >= 0 && v <= 1 );
	return {( F2B( v ) << RED_SHIFT ) | ( color & ~RED_MASK )};
}
CONSTEXPR Color setGreen( uint32 color, float v )
{
	const_assert( v >= 0 && v <= 1 );
	return {( F2B( v ) << GREEN_SHIFT ) | ( color & ~GREEN_MASK )};
}
CONSTEXPR Color setBlue( uint32 color, float v )
{
	const_assert( v >= 0 && v <= 1 );
	return {( F2B( v ) << BLUE_SHIFT ) | ( color & ~BLUE_MASK )};
}

constexpr Color setAlpha( uint32 color, int32 v )
{
	return {( U2B( v ) << ALPHA_SHIFT ) | ( color & ~ALPHA_MASK )};
}
constexpr Color setRed( uint32 color, int32 v )
{
	return {( U2B( v ) << RED_SHIFT ) | ( color & ~RED_MASK )};
}
constexpr Color setGreen( uint32 color, int32 v )
{
	return {( U2B( v ) << GREEN_SHIFT ) | ( color & ~GREEN_MASK )};
}
constexpr Color setBlue( uint32 color, int32 v )
{
	return {( U2B( v ) << BLUE_SHIFT ) | ( color & ~BLUE_MASK )};
}

constexpr uint32 getAlpha( Color color ) { return ( color.bits >> ALPHA_SHIFT ) & 0xFF; }
constexpr uint32 getRed( Color color ) { return ( color.bits >> RED_SHIFT ) & 0xFF; }
constexpr uint32 getGreen( Color color ) { return ( color.bits >> GREEN_SHIFT ) & 0xFF; }
constexpr uint32 getBlue( Color color ) { return ( color.bits >> BLUE_SHIFT ) & 0xFF; }
constexpr float getAlphaF( Color color ) { return B2F( ( color.bits >> ALPHA_SHIFT ) & 0xFF ); }
constexpr float getRedF( Color color ) { return B2F( ( color.bits >> RED_SHIFT ) & 0xFF ); }
constexpr float getGreenF( Color color ) { return B2F( ( color.bits >> GREEN_SHIFT ) & 0xFF ); }
constexpr float getBlueF( Color color ) { return B2F( ( color.bits >> BLUE_SHIFT ) & 0xFF ); }

constexpr vec4 getColorF( Color color )
{
	return {B2F( ( color.bits >> ALPHA_SHIFT ) & 0xFF ), B2F( ( color.bits >> RED_SHIFT ) & 0xFF ),
	        B2F( ( color.bits >> GREEN_SHIFT ) & 0xFF ),
	        B2F( ( color.bits >> BLUE_SHIFT ) & 0xFF )};
}

Color multiply( Color a, Color b )
{
	auto af = getColorF( a );
	auto bf = getColorF( b );
	return Color::argb( multiplyComponents( af, bf ) );
}
Color multiply( Color a, uint32 b ) { return multiply( a, Color{b} ); }

const Color Color::White  = {0xFFFFFFFF};
const Color Color::Black  = {0xFF000000};
const Color Color::Red    = {0xFFFF0000};
const Color Color::Green  = {0xFF00FF00};
const Color Color::Blue = {0xFF0000FF};
const Color Color::Yellow = {0xFFFFFF00};
const Color Color::Purple = {0xFFFF00FF};
const Color Color::Cyan = {0xFF00FFFF};