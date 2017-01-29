// a downwards opening parabola with its vertex at (0.5, 1.0) and its roots at
// (0, 0) and (1, 0) t in [0,1], returns values in [0,1]
inline float stationaryNegativeQuad( float t )
{
	// return 4.0f * t - 4.0f * t * t;
	return ( 4.0f - 4.0f * t ) * t;
}

// simple sineWave where you can specify amplitude and frequency
// returns t in [0,1], returns values in [-1,1]
inline float sineWave( float t, float frequency, float amplitude )
{
	return math::sin( 2 * Pi32 * frequency * t ) * amplitude;
}

// t in [0,1], returns values in [-1,1]
inline float quadSineWave( float t, float frequency, float amplitude )
{
	return stationaryNegativeQuad( t ) * sineWave( t, frequency, amplitude );
}

// converts val in [min, max] to ret in [0,1]
inline float normalizeInterval( float val, float min, float max )
{
	// val - min turns val in [min,max] to temp in [0, max - min]
	return ( val - min ) / ( max - min );
}

// converts val in [-1, 1] to ret in [0, 1]
inline float normalizeSineInterval( float val ) { return ( val + 1.0f ) * 0.5f; }

// converts val in [fromMin, fromMax] to ret in [toMin, toMax]
inline float mapInterval( float val, float fromMin, float fromMax, float toMin, float toMax )
{
	auto t = normalizeInterval( val, fromMin, fromMax );
	return lerp( t, toMin, toMax );
}

inline float smoothstep( float t )
{
	// return (-2*t*t*t) + (3*t*t);
	// simplified
	return t * t * ( 3.0f - 2.0f * t );
}

inline float smootherstep( float t )
{
	// return 6.0f*t*t*t*t*t + 15.0f*t*t*t*t + 10.0f*t*t*t;
	return t * t * t * ( t * ( t * 6.0f - 15.0f ) + 10.0f );
}

// parabola going from 0 to 1 with derivative d at 0
inline float quadratic( float t, float d = 2 ) { return ( ( ( 1 - d ) ) * t + d ) * t; }

float easeOutBounce( float t )
{
	if( t <= 0.36f ) {
		t /= 0.36f;
		return t * t;
	} else if( t < 0.72f ) {
		t = ( t - 0.36f ) * ( 1.0f / ( ( 0.72f - 0.36f ) * 0.5f ) ) - 1;
		return ( t * t - 1 ) * 0.25f + 1;
	} else if( t < 0.9f ) {
		t = ( t - 0.72f ) * ( 1.0f / ( ( 0.9f - 0.72f ) * 0.5f ) ) - 1;
		const float a = 0.25f * 0.25f;
		return ( t * t - 1 ) * a + 1;
	} else {
		t = ( t - 0.9f ) * ( 1.0f / ( ( 1.0f - 0.9f ) * 0.5f ) ) - 1;
		const float a = 0.25f * 0.25f * 0.25f;
		return ( t * t - 1 ) * a + 1;
	}
}
float easeInBounce( float t )
{
	return 1.0f - easeOutBounce( 1.0f - t );
}

float easeOutElastic( float t )
{
	if( t >= 0.95f ) return 1;

	const float a = 6.8f;
	auto denom = math::exp( t * a );
	auto nom = math::sin( t * Pi32 * a - HalfPi32 );
	return nom / denom + 1;
}
float easeInElastic( float t )
{
	return 1.0f - easeOutElastic( 1.0f - t );
}