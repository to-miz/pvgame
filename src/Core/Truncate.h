#pragma once

#ifndef _TRUNCATE_H_INCLUDED_
#define _TRUNCATE_H_INCLUDED_

#pragma warning( push )
// warning C4127: conditional expression is constant
// if constexpr not a thing yet
#pragma warning( disable: 4127 )

template < class ReturnType, class ValueType >
inline constexpr ReturnType safe_truncate( ValueType val )
{
#if defined( _DEBUG ) || defined( GAME_DEBUG )
	static_assert(
		std::is_integral< ValueType >::value || std::is_floating_point< ValueType >::value,
		"ValueType needs to be integral or floating point for truncation" );
	if( sizeof( ReturnType ) <= sizeof( ValueType ) ) {
		assert( val <= static_cast< ValueType >( numeric_limits< ReturnType >::max() ) );
		if( std::is_unsigned< ReturnType >::value ) {
			assert( val >= static_cast< ValueType >( numeric_limits< ReturnType >::min() ) );
		}
	}
#endif
	return static_cast< ReturnType >( val );
}

#pragma warning( pop )

// deduced version of safe truncate, so that the return type does not have to be repeated
// motivation of this method is to say "truncate implicitly, but make sure at runtime that
// truncation doesn't alter value"
// basically you can write this:
//	int32 x = 12;
//	int16 y = auto_truncate( x ); // alternative: safe_truncate< int16 >( x );
template < class T >
struct auto_truncate_t {
	static_assert( std::is_integral< T >::value || std::is_floating_point< T >::value,
	               "T needs to be integral or floating point for truncation" );

	T value;

	// "universal" conversion operator that truncates "safely"
	// not really universal, since we static_assert on integral types
	template < class U >
	inline operator U() const
	{
		return safe_truncate< U >( value );
	};
};

template < class T >
auto_truncate_t< T > auto_truncate( T value )
{
	return auto_truncate_t< T >{value};
}

#endif // _TRUNCATE_H_INCLUDED_
