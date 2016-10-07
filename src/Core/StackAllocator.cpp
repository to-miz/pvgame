struct StackAllocator {
	char* ptr;
	size_t size;
	size_t capacity;
	uint32 lastPoppedAlignment;
};

char* back( StackAllocator* allocator )
{
	return allocator->ptr + allocator->size;
}
char* end( StackAllocator* allocator )
{
	return allocator->ptr + allocator->capacity;
}
char* begin( StackAllocator* allocator )
{
	return allocator->ptr;
}
bool isAllocatedWithAllocator( StackAllocator* allocator, void* ptr, size_t size )
{
	return ( (char*)ptr >= allocator->ptr
	         && (char*)ptr + size <= back( allocator ) );
}
bool isBack( StackAllocator* allocator, void* ptr, size_t size )
{
	auto end = (char*)ptr + size;
	return end + getAlignmentOffset( end, allocator->lastPoppedAlignment ) == back( allocator );
}

uint32 getAlignmentOffset( StackAllocator* allocator, uint32 alignment )
{
	return getAlignmentOffset( back( allocator ), alignment );
}
bool isValid( StackAllocator* allocator )
{
	return allocator && ( allocator->ptr || ( !allocator->capacity && !allocator->size ) )
		   && allocator->size <= allocator->capacity;
}

void clear( StackAllocator* allocator )
{
	allocator->size = 0;
	allocator->lastPoppedAlignment = 1;
}
size_t remaining( StackAllocator* allocator )
{
	assert( isValid( allocator ) );
	return allocator->capacity - allocator->size;
}
// return remaining size when taking alignment of next allocation into account
size_t remaining( StackAllocator* allocator, uint32 alignment )
{
	assert( isValid( allocator ) );
	auto ret       = allocator->capacity - allocator->size;
	auto offset = getAlignmentOffset( allocator, alignment );
	if( ret <= offset ) {
		ret -= offset;
	}
	return offset;
}

void* allocate( StackAllocator* allocator, size_t size, uint32 alignment )
{
	assert( allocator );
	assert( allocator->ptr );
	assert( allocator->capacity );

	auto offset = getAlignmentOffset( back( allocator ), alignment );
	if( allocator->size + offset + size > allocator->capacity ) {
		OutOfMemory();
		return nullptr;
	}

	auto result = back( allocator ) + offset;
	allocator->size += offset + size;
	assert_alignment( result, alignment );
	allocator->lastPoppedAlignment = 1;
	return result;
}
void* reallocate( StackAllocator* allocator, void* ptr, size_t newSize, size_t oldSize,
                  uint32 alignment )
{
	assert( isValid( allocator ) );
	assert( ptr );
	if( isBack( allocator, ptr, oldSize ) ) {
		allocator->size += newSize - oldSize;
		return ptr;
	} else if( newSize < oldSize ) {
		// no reallocation needed
		return ptr;
	} else {
		LOG( ERROR, "reallocate with StackAllocator: ptr wasn't at back" );
		assert( 0 && "reallocate with StackAllocator: ptr wasn't at back" );
		auto result = allocate( allocator, newSize, alignment );
		memcpy( result, ptr, min( oldSize, newSize ) );
		return result;
	}
}
void free( StackAllocator* allocator, void* ptr, size_t size, uint32 alignment )
{
	assert( isValid( allocator ) );
	assert( !ptr || isBack( allocator, ptr, size ) );
	if( ptr && isBack( allocator, ptr, size ) )	 {
		allocator->size -= size;
		allocator->lastPoppedAlignment = alignment;
	}
}

#define allocateStruct( allocator, type ) \
	( type* ) allocate( ( allocator ), sizeof( type ), alignof( type ) )
#define allocateArray( allocator, type, count ) \
	( type* ) allocate( ( allocator ), sizeof( type ) * ( count ), alignof( type ) )
#define maxAllocateArray( allocator, type ) \
	( type* )allocate( ( allocator ), remaining( allocator, alignof( type ) ) / sizeof( type ) );

StackAllocator makeStackAllocator( StackAllocator* allocator, size_t capacity )
{
	return {allocateArray( allocator, char, capacity ), 0, capacity, 1};
}
StackAllocator makeStackAllocator( void* ptr, size_t capacity )
{
	return {(char*)ptr, 0, capacity, 1};
}

struct TemporaryMemoryGuard {
	StackAllocator* allocator;
	size_t size;
	uint32 lastPoppedAlignment;

	TemporaryMemoryGuard( StackAllocator* allocator )
	: allocator( allocator ),
	  size( allocator->size ),
	  lastPoppedAlignment( allocator->lastPoppedAlignment )
	{
	}
	~TemporaryMemoryGuard()
	{
		allocator->size                = size;
		allocator->lastPoppedAlignment = lastPoppedAlignment;
	}
};

#define TEMPORARY_MEMORY_BLOCK( allocator ) \
	if( auto _once = false ) {              \
	} else                                  \
		for( auto _scope = TemporaryMemoryGuard( allocator ); !_once; _once = true )

void* fitToSizeArraysImpl( StackAllocator* allocator, void* first, size_t firstSize,
                           size_t oldFirstSize, void* second, size_t secondSize,
                           size_t oldSecondSize, uint32 secondAlignment )
{
	/*
	turn this:
	FFFFFFFFFFFFFFFFFFF*********ASSSSSSSSSSSSSSSSSSSSS********
	into:
	FFFFFFFFFFFFFFFFFFFASSSSSSSSSSSSSSSSSSSSS*****************
	*/
	assert( isValid( allocator ) );
	assert( isAllocatedWithAllocator( allocator, first, firstSize ) );
	assert( isAllocatedWithAllocator( allocator, second, secondSize ) );
	assert( isBack( allocator, second, oldSecondSize ) );
	assert_init(
	    auto firstOldEnd = (char*)first + oldFirstSize,
	    firstOldEnd + getAlignmentOffset( firstOldEnd, secondAlignment ) == (char*)second );

	auto firstEnd             = (char*)first + firstSize;
	auto alignmentOffset      = getAlignmentOffset( firstEnd, secondAlignment );
	auto newSecondDestination = firstEnd + alignmentOffset;

	move( newSecondDestination, second, secondSize );
	auto memoryEnd = newSecondDestination + secondSize;

	allocator->size = memoryEnd - allocator->ptr;
	return newSecondDestination;
}

#if 1
template < class T, class U >
T* fitToSizeArrays( StackAllocator* allocator, U* first, size_t firstCount, size_t oldFirstCount,
                    T* second, size_t secondCount, size_t oldSecondCount )
{
	return (T*)fitToSizeArraysImpl( allocator, first, firstCount * sizeof( U ),
	                                oldFirstCount * sizeof( U ), second, secondCount * sizeof( T ),
	                                oldSecondCount * sizeof( T ), alignof( T ) );
}
#else
#define fitToSizeArrays( allocator, first, firstCount, second, secondCount )       \
	( typeof( second ) ) fitToSizeArraysImpl(                                      \
	    ( allocator ), ( first ), ( firstCount ) * sizeof( first[0] ), ( second ), \
	    ( secondCount ) * sizeof( second[0] ), alignof( typeof( second[0] ) ) )
#endif

struct StackAllocatorPartition {
	StackAllocator* allocator = nullptr;
	StackAllocator prim = {};
	size_t oldSize = 0;
	uint32 lastPoppedAlignment = 1;

	StackAllocator* primary() { return &prim; }
	StackAllocator* scrap() { return allocator; }

	StackAllocatorPartition() = default;
	StackAllocatorPartition( StackAllocatorPartition&& other )
	: allocator( other.allocator ),
	  prim( other.prim ),
	  oldSize( other.oldSize ),
	  lastPoppedAlignment( other.lastPoppedAlignment )
	{
		other.allocator = nullptr;
	}

	~StackAllocatorPartition()
	{
		if( allocator ) {
			assert( end( &prim ) <= back( allocator ) );
			allocator->size = oldSize;
			allocator->lastPoppedAlignment = lastPoppedAlignment;
		}
	}

	static StackAllocatorPartition ratio( StackAllocator* allocator, size_t primaryRatio )
	{
		StackAllocatorPartition result;
		result.oldSize                 = allocator->size;
		result.allocator               = allocator;
		auto remainingSize             = remaining( allocator );
		auto primSize                  = remainingSize - ( remainingSize / ( primaryRatio + 1 ) );
		result.prim                    = makeStackAllocator( allocator, primSize );
		result.lastPoppedAlignment     = allocator->lastPoppedAlignment;
		return result;
	}

	void commit()
	{
		assert( end( &prim ) <= back( allocator ) );
		allocator->size = back( &prim ) - begin( allocator );
		allocator->lastPoppedAlignment = prim.lastPoppedAlignment;
		allocator       = nullptr;
	}
};