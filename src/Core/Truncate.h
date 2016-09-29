#pragma once

#ifndef _TRUNCATE_H_INCLUDED_
#define _TRUNCATE_H_INCLUDED_

template < class ReturnType, class ValueType >
inline constexpr ReturnType safe_truncate( ValueType val )
{
#if defined( _DEBUG ) || defined( ACHE_INTERNAL )
	static_assert(
		std::is_integral< ValueType >::value || std::is_floating_point< ValueType >::value,
		"ValueType needs to be integral or floating point for truncation" );
	assert( val <= static_cast< ValueType >( numeric_limits< ReturnType >::max() ) );
	if( std::is_unsigned< ReturnType >::value ) {
		assert( val >= static_cast< ValueType >( numeric_limits< ReturnType >::min() ) );
	}
#endif
	return static_cast< ReturnType >( val );
}

#endif // _TRUNCATE_H_INCLUDED_
