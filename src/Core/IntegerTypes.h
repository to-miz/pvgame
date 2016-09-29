#pragma once

#ifndef _INTEGERTYPES_H_INCLUDED_
#define _INTEGERTYPES_H_INCLUDED_

#include <cstdint>

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

// type of what is the fastest way to index into arrays and for loops
typedef uint32 uintfast;

typedef size_t uintptr;
static_assert( sizeof( uintptr ) == sizeof( void* ), "pointer size mismatch" );

#if defined( ARCHITECTURE_X86 )
	typedef int32 intmax;
	#define INTMAX_MIN INT32_MIN
	#define INTMAX_MAX INT32_MAX
	typedef uint32 uintmax;
	#define UINTMAX_MIN UINT32_MIN
	#define UINTMAX_MAX UINT32_MAX
#elif defined( ARCHITECTURE_X64 )
	typedef int64 intmax;
	#define INTMAX_MIN INT64_MIN
	#define INTMAX_MAX INT64_MAX
	typedef uint64 uintmax;
	#define UINTMAX_MIN UINT64_MIN
	#define UINTMAX_MAX UINT64_MAX
#else
	#error ARCHITECTURE undefined
#endif

#endif // _INTEGERTYPES_H_INCLUDED_
