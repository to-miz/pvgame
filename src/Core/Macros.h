#pragma once

#ifndef _MACROS_H_INCLUDED_
#define _MACROS_H_INCLUDED_

#define kilobytes( x ) ( ( x ) * 1024 )
#define megabytes( x ) ( ( x ) * 1024 * 1024 )
#define gigabytes( x ) ( ( x ) * 1024 * 1024 * 1024 )

// attribute to mark mutable global objects
#define global

#define InvalidDefaultCase                   \
	default: {                               \
		assert( 0 && "InvalidDefaultCase" ); \
		break;                               \
	}

// #define offsetof( type, member ) ( (size_t)( &( (type*)0 )->member ) )

#define PP_JOIN2( a, b ) a##b
#define PP_JOIN( a, b ) PP_JOIN2( a, b )

#ifdef GAME_DEBUG
	#define debug_error( x ) assert( 0 && x )
	#define OutOfMemory() assert( 0 && "Out Of Memory" )
	#define InvalidCodePath() assert( 0 && "InvalidCodePath" );
	// const_assert allows assertions in constexpr functions in debug builds (consexpr is only
	// activated on release builds)
	#define CONSTEXPR
	#define const_assert( x ) assert( x )
	#define break_if( x )   \
		if( x ) {           \
			__debugbreak(); \
		}
#else
	#define OutOfMemory() ( (void)0 )
	#define debug_error( x ) ( (void)0 )
	#define break_if( x ) ( (void)0 )
	#define InvalidCodePath() ( (void)0 )

	#define CONSTEXPR constexpr
	#define const_assert( x )
#endif

#define SWIZZLE_3( obj, x, y, z ) {( obj ).x, ( obj ).y, ( obj ).z}
// #define SWIZZLE( obj, ... ) {( obj ).__VA_ARGS__}

#define not_implemented() debug_error( "not implemented" )

#endif // _MACROS_H_INCLUDED_
