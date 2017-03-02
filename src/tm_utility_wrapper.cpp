#define TM_UTILITY_IMPLEMENTATION
// #define TMUT_IMPLEMENT_CTYPE_FUNCTIONS
#define TMUT_STRLEN strlen
#define TMUT_MEMCPY memcpy
#define TMUT_MEMSET memset
#define TMUT_MEMCMP memcmp
#define TMUT_MEMMOVE memmove
#define TMUT_ASSERT assert
#define TMUT_ABS abs
#if defined( ARCHITECTURE_X64 )
	#define TMUT_POINTER_SIZE 8
#elif defined( ARCHITECTURE_X86 )
	#define TMUT_POINTER_SIZE 4
#else
	#error ARCHITECTURE undefined
#endif
#ifdef _MSC_VER
	#define TMUT_NO_STRICMP
	#define TMUT_NO_STRNICMP
	#define TMUT_NO_STRREV
	#define stricmp _stricmp
	#define strrev _strrev
	#define strnicmp _strnicmp
#endif
#define TMUT_OWN_TYPES
#define TMUT_SAFE_COUNTOF
typedef int32 tmut_size_t;
typedef uintptr tmut_uintptr;

#ifndef GAME_NO_STD
	#define TMUT_NO_SWAP
#else
	#define TM_USE_OWN_BEGIN_END
#endif

#include <tm_utility.h>

using utility::min;
using utility::max;
using utility::MinMaxPair;
using utility::minmax;
using utility::median;

// c++17 for as macro
#define FOR( x ) for( auto&& x )

template < class T >
int32 distance( T a, T b )
{
	return safe_truncate< int32 >( b - a );
}