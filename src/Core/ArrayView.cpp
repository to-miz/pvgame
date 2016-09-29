#define TMA_ASSERT assert
#define TMA_MEMCPY memcpy
#define TMA_MEMMOVE memmove
#define TMA_NO_STD_ITERATOR
#define TMA_USE_OWN_TYPES
typedef int32 tma_size_t;
typedef vec2i tma_point;
#ifdef ARCHITECTURE_X64
	#define TMA_INT64_ACCOSSORS
	typedef intmax tma_index_t;
#endif
#define TMA_EMPLACE_BACK_RETURNS_POINTER
#include <tm_arrayview.h>

template< class T > using Array = ArrayView< T >;
template< class T > using UninitializedArray = UninitializedArrayView< T >;
template< class T > using UArray = UninitializedArrayView< T >;
template< class T > using Grid = GridView< T >;

template < class T >
Array< T > makeArrayImpl( StackAllocator* allocator, int32 size )
{
	auto p = allocateArray( allocator, T, size );
	return {p, ( p ) ? ( size ) : ( 0 )};
}
template < class T >
UArray< T > makeUArrayImpl( StackAllocator* allocator, int32 size )
{
	auto p = allocateArray( allocator, T, size );
	return {p, 0, ( p ) ? ( size ) : ( 0 )};
}

#define makeArray( allocator, type, size ) makeArrayImpl< type >( ( allocator ), ( size ) )
#define makeUArray( allocator, type, size ) makeUArrayImpl< type >( ( allocator ), ( size ) )