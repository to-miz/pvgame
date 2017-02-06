#pragma once

#ifndef _CORETYPES_H_INCLUDED_
#define _CORETYPES_H_INCLUDED_

struct null_t {};
const null_t null;

struct bool8 {
	int8 value;
	bool8& operator=( bool other )
	{
		value = other;
		return *this;
	}
	inline constexpr bool operator!=( bool other ) const { return value != (int8)other; }
	inline constexpr bool operator!=( bool8 other ) const
	{
		return ( value != 0 ) != ( other.value != 0 );
	}
	inline constexpr bool operator==( bool other ) const { return value == (int8)other; }
	inline constexpr bool operator==( bool8 other ) const
	{
		return ( value != 0 ) == ( other.value != 0 );
	}
	explicit operator bool() const
	{
		assert( value == 0 || value == 1 );
		return value != 0;
	}
};

#endif // _CORETYPES_H_INCLUDED_
