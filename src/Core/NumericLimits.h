#pragma once

#ifndef _NUMERICALLIMITS_H_INCLUDED_
#define _NUMERICALLIMITS_H_INCLUDED_

template< class T > struct numeric_limits {};
template <>
struct numeric_limits< char > {
	static_assert( sizeof( char ) == 1, "char size mismatch" );
	static constexpr char min() { return INT8_MIN; }
	static constexpr char lowest() { return INT8_MIN; }
	static constexpr char max() { return INT8_MAX; }
	static const bool is_signed = true;
};
template <>
struct numeric_limits< int8 > {
	static constexpr int8 min() { return INT8_MIN; }
	static constexpr int8 lowest() { return INT8_MIN; }
	static constexpr int8 max() { return INT8_MAX; }
	static const bool is_signed = true;
};
template <>
struct numeric_limits< uint8 > {
	static constexpr uint8 min() { return 0; }
	static constexpr uint8 lowest() { return 0; }
	static constexpr uint8 max() { return UINT8_MAX; }
	static const bool is_signed = false;
};
template <>
struct numeric_limits< int16 > {
	static constexpr int16 min() { return INT16_MIN; }
	static constexpr int16 lowest() { return INT16_MIN; }
	static constexpr int16 max() { return INT16_MAX; }
	static const bool is_signed = true;
};
template <>
struct numeric_limits< uint16 > {
	static constexpr uint16 min() { return 0; }
	static constexpr uint16 lowest() { return 0; }
	static constexpr uint16 max() { return UINT16_MAX; }
	static const bool is_signed = false;
};
template <>
struct numeric_limits< int32 > {
	static constexpr int32 min() { return INT32_MIN; }
	static constexpr int32 lowest() { return INT32_MIN; }
	static constexpr int32 max() { return INT32_MAX; }
	static const bool is_signed = true;
};
template <>
struct numeric_limits< uint32 > {
	static constexpr uint32 min() { return 0; }
	static constexpr uint32 lowest() { return 0; }
	static constexpr uint32 max() { return UINT32_MAX; }
	static const bool is_signed = false;
};
template <>
struct numeric_limits< int64 > {
	static constexpr int64 min() { return INT64_MIN; }
	static constexpr int64 lowest() { return INT64_MIN; }
	static constexpr int64 max() { return INT64_MAX; }
	static const bool is_signed = true;
};
template <>
struct numeric_limits< uint64 > {
	static constexpr uint64 min() { return 0; }
	static constexpr uint64 lowest() { return 0; }
	static constexpr uint64 max() { return UINT64_MAX; }
	static const bool is_signed = false;
};
template <>
struct numeric_limits< float > {
	static constexpr float min() { return FLOAT_MIN; }
	static constexpr float lowest() { return -FLOAT_MAX; }
	static constexpr float max() { return FLOAT_MAX; }
	static const bool is_signed = true;
};
template <>
struct numeric_limits< double > {
	static constexpr double min() { return DOUBLE_MIN; }
	static constexpr double lowest() { return -DOUBLE_MAX; }
	static constexpr double max() { return DOUBLE_MAX; }
	static const bool is_signed = true;
};

#endif // _NUMERICALLIMITS_H_INCLUDED_
