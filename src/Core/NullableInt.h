#pragma once

#ifndef _NULLABLEINT_H_INCLUDED_
#define _NULLABLEINT_H_INCLUDED_

template < class T >
struct NullableInt {
	T value;

	static_assert( std::is_integral< T >::value, "T must be integral" );

	static NullableInt make( T value ) { return {value}; }
	static NullableInt make( T value, bool condition )
	{
		NullableInt ret;
		if( condition ) {
			ret = {value};
		} else {
			ret = {-1};
		}
		return ret;
	}
	static NullableInt makeNull() { return {-1}; }

	constexpr explicit operator bool() const { return value >= 0; }

	NullableInt& operator=( T other )
	{
		value = other;
		return *this;
	}
	NullableInt& operator=( null_t )
	{
		value = -1;
		return *this;
	}
	T& get()
	{
		assert( value >= 0 );
		return value;
	}
	void clear() { value = -1; }
	void set( T value ) { this->value = value; }
	void set( T value, bool condition )
	{
		if( condition ) {
			this->value = value;
		} else {
			this->value = -1;
		}
	}

	bool operator==( NullableInt< T > rhs ) const { return value == rhs.value; }
	bool operator!=( NullableInt< T > rhs ) const { return value != rhs.value; }
	bool operator==( null_t ) const { return value < 0; }
	friend bool operator==( null_t, NullableInt< T > a ) { return a.value < 0; }
	bool operator!=( null_t ) const { return value >= 0; }
	friend bool operator!=( null_t, NullableInt< T > a ) { return a.value >= 0; }
};

typedef NullableInt< int32 > NullableInt32;
typedef NullableInt< int8 > NullableInt8;

inline NullableInt8 toNullableInt8( NullableInt32 val )
{
	return {safe_truncate< int8 >( val.value )};
}

static_assert( std::is_pod< NullableInt32 >::value == true, "NullableInt is not pod" );

#endif
