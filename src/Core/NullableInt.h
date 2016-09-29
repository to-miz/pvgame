#pragma once

#ifndef _NULLABLEINT_H_INCLUDED_
#define _NULLABLEINT_H_INCLUDED_

template < class T >
struct NullableInt {
	static_assert( std::is_integral< T >::value, "T Must be integral" );

	inline static NullableInt make( T value ) { return {value}; }
	inline static NullableInt make( T value, bool condition )
	{
		NullableInt ret;
		if( condition ) {
			ret = {value};
		} else {
			ret = {-1};
		}
		return ret;
	}
	inline static NullableInt makeNull() { return {-1}; }

	constexpr inline explicit operator bool() const { return value >= 0; }

	inline NullableInt& operator=( T other )
	{
		value = other;
		return *this;
	}
	inline NullableInt& operator=( null_t )
	{
		value = -1;
		return *this;
	}
	inline T& get()
	{
		assert( value >= 0 );
		return value;
	}
	inline void clear() { value = -1; }
	inline void set( T value ) { this->value = value; }
	inline void set( T value, bool condition )
	{
		if( condition ) {
			this->value = value;
		} else {
			this->value = -1;
		}
	}

	// TODO: private?
	T value;
};

template < class T >
inline bool operator==( NullableInt< T > a, NullableInt< T > b )
{
	return a.value == b.value;
}
template < class T >
inline bool operator!=( NullableInt< T > a, NullableInt< T > b )
{
	return a.value != b.value;
}
template < class T >
inline bool operator==( NullableInt< T > a, null_t )
{
	return a.value < 0;
}
template < class T >
inline bool operator==( null_t, NullableInt< T > a )
{
	return a.value < 0;
}
template < class T >
inline bool operator!=( NullableInt< T > a, null_t )
{
	return a.value >= 0;
}
template < class T >
inline bool operator!=( null_t, NullableInt< T > a )
{
	return a.value >= 0;
}

typedef NullableInt< int32 > NullableInt32;
typedef NullableInt< int8 > NullableInt8;

inline NullableInt8 toNullableInt8( NullableInt32 val )
{
	return {safe_truncate< int8 >( val.value )};
}

static_assert( std::is_pod< NullableInt32 >::value == true, "NullableInt is not pod" );

#endif
