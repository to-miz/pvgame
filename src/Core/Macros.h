#pragma once

#ifndef _MACROS_H_INCLUDED_
#define _MACROS_H_INCLUDED_

#define kilobytes( x ) ( ( x ) * 1024 )
#define megabytes( x ) ( ( x ) * 1024 * 1024 )
#define gigabytes( x ) ( ( x ) * 1024 * 1024 * 1024 )

// attribute to mark mutable global_var objects
#define global_var

#define InvalidDefaultCase                   \
	default: {                               \
		assert( 0 && "InvalidDefaultCase" ); \
		break;                               \
	}

// #define offsetof( type, member ) ( (size_t)( &( (type*)0 )->member ) )

#define PP_JOIN2( a, b ) a##b
#define PP_JOIN( a, b ) PP_JOIN2( a, b )

#ifdef GAME_DEBUG
	#define assert_m( x, msg ) assert( ( x ) && ( msg ) )
	#define debug_error( x ) assert( 0 && ( x ) )
	#define debug_break() __debugbreak()

	#if GAME_OUT_OF_MEMORY_ASSERTION
		#define OutOfMemory() assert( 0 && "Out Of Memory" )
	#else
		#define OutOfMemory() ( LOG( ERROR, "Out Of Memory: {} {}", __FILE__, __LINE__ ) )
	#endif // GAME_OUT_OF_MEMORY_ASSERTION

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
	#define assert_m( x, msg ) ( (void)0 )
	#define OutOfMemory() ( (void)0 )
	#define debug_error( x ) ( (void)0 )
	#define debug_break() ( (void)0 )
	#define break_if( x ) ( (void)0 )
	#define InvalidCodePath() ( (void)0 )

	#define CONSTEXPR constexpr
	#define const_assert( x )
#endif

#ifdef GAME_DEBUG
	#define DEBUG_WRAP( x ) x
	// assert with an initializer of y
	#define assert_init( x, y ) \
		do {                    \
			x;                  \
			assert( y );        \
		} while( 0 )
#else
	#define DEBUG_WRAP( x )
	#define assert_init( x, y ) ( (void)0 )
#endif // !defined( NDEBUG )

#define SWIZZLE_3( obj, x, y, z ) {( obj ).x, ( obj ).y, ( obj ).z}
// #define SWIZZLE( obj, ... ) {( obj ).__VA_ARGS__}

#define BITFIELD( x ) ( 1u << ( x ) )

#define not_implemented() debug_error( "not implemented" )

#define FOR( x ) for( auto&& x )
// macro version of if with initializer, search & replace once C++1z has them
#define if_init( init, cond )  \
	if( bool once_ = false ) { \
	} else                     \
		for( init; !once_ && ( cond ); once_ = true )

// iterate over two collections at once
#define zip_for( collection0, collection1 )                                                      \
	if( bool once_ = false ) {                                                                   \
	} else                                                                                       \
		for( auto first = ::begin( collection0 ), firstEnd = ::end( collection0 ); !once_;       \
		     once_ = true )                                                                      \
			for( auto second = ::begin( collection1 ), secondEnd = ::end( collection1 ); !once_; \
			     once_ = true )                                                                  \
				for( ; first != firstEnd && second != secondEnd; ++first, ++second )

// while loop with max iterations
#define bounded_while( cond, max_iter ) \
	for( auto iterations_ = 0; ( cond ) && iterations_ < ( max_iter ); ++iterations_ )

#endif  // _MACROS_H_INCLUDED_
