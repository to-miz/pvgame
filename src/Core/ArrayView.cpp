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

// consume all available memory from stack allocator
#define beginVector( allocator, type ) beginVector_< type >( ( allocator ) )
template < class T >
UninitializedArray< T > beginVector_( StackAllocator* allocator )
{
	assert( isValid( allocator ) );
	return makeUArrayImpl< T >( allocator,
	                            safe_truncate< int32 >( getCapacityFor< T >( allocator ) ) );
}

// fit to size vector and give back unused memory to allocator, only works if v is the most recent
// allocation
template < class T >
void endVector( StackAllocator* allocator, UninitializedArray< T >* v )
{
	assert( isValid( allocator ) );
	assert( v );
	assert( isBack( allocator, v->data(), v->capacity() * sizeof( T ) ) );
	v->ptr = (T*)reallocate( allocator, v->data(), v->size() * sizeof( T ),
	                         v->capacity() * sizeof( T ), alignof( T ) );
	v->cap = v->sz;
}

// small buffer based uarray
template < class T, int32 N >
struct SmallUninitializedArray : UninitializedArray< T > {
	T buffer[N];
	SmallUninitializedArray()
	{
		ptr = buffer;
		sz  = 0;
		cap = N;
	}
};

template < class Container >
ArrayView< typename Container::value_type > makeRangeView( Container& container, rangei range )
{
	TMA_ASSERT( range.min >= 0 );
	TMA_ASSERT( range.max >= 0 );
	if( range.min >= container.size() ) {
		range.min = container.size();
	}
	if( range.max >= container.size() ) {
		range.max = container.size();
	}
	TMA_ASSERT( range.min <= range.max );
	return {container.data() + range.min, range.max - range.min};
}