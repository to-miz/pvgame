struct FixedSizeAllocatoreFreeEntry {
	int32 next;
};

struct FixedSizeAllocator {
	char* ptr;
	int32 size;
	int32 capacity;
	uint16 elementSize;
	uint16 alignment;
	int32 firstFree;
	int32 elementCount;
};

char* back( FixedSizeAllocator* allocator )
{
	return allocator->ptr + allocator->size;
}
char* end( FixedSizeAllocator* allocator )
{
	return allocator->ptr + allocator->capacity;
}
char* begin( FixedSizeAllocator* allocator )
{
	return allocator->ptr;
}

bool isValid( FixedSizeAllocator* allocator )
{
	return allocator && ( allocator->ptr || ( !allocator->capacity && !allocator->size ) )
	       && allocator->size <= allocator->capacity;
}

void clear( FixedSizeAllocator* allocator )
{
	allocator->size = 0;
	allocator->firstFree = -1;
}
size_t remaining( FixedSizeAllocator* allocator )
{
	assert( isValid( allocator ) );
	return allocator->capacity - allocator->size;
}

char* getElement( FixedSizeAllocator* allocator, int32 index )
{
	assert( index >= 0 );
	assert( index < allocator->capacity );
	assert( isAligned( allocator->ptr + index, allocator->alignment ) );
	return allocator->ptr + index;
}

void* allocate( FixedSizeAllocator* allocator, size_t size, uint32 alignment )
{
	assert( isValid( allocator ) );
	assert_m( size == allocator->elementSize, "array allocation not supported" );
	assert( alignment == allocator->alignment );

	void* result = nullptr;
	if( allocator->firstFree < 0 ) {
		if( size > remaining( allocator ) ) {
			OutOfMemory();
		} else {
			result = back( allocator );
			allocator->size += allocator->elementSize;
			++allocator->elementCount;
		}
	} else {
		result = getElement( allocator, allocator->firstFree );
		FixedSizeAllocatoreFreeEntry entry;
		memcpy( &entry, result, sizeof( FixedSizeAllocatoreFreeEntry ) );
		allocator->firstFree = entry.next;
		++allocator->elementCount;
	}
	assert( !result
	        || ( isAligned( result, alignment ) && isAligned( result, allocator->alignment ) ) );
	return result;
}
void* reallocate( FixedSizeAllocator* allocator, void* ptr, size_t newSize, size_t oldSize,
                  uint32 alignment )
{
	assert( isValid( allocator ) );
	assert_m( oldSize == allocator->elementSize, "array allocation not supported" );
	assert_m( newSize == allocator->elementSize, "array allocation not supported" );
	assert( alignment == allocator->alignment );
	if( newSize == oldSize ) {
		return ptr;
	}
	return nullptr;
}
void free( FixedSizeAllocator* allocator, void* ptr, size_t size, uint32 alignment )
{
	assert( isValid( allocator ) );
	assert_m( size == allocator->elementSize, "array allocation not supported" );
	assert( alignment == allocator->alignment );
	assert( size >= sizeof( FixedSizeAllocatoreFreeEntry ) );

	if( ptr ) {
		assert( (char*)ptr >= begin( allocator ) && (char*)ptr < end( allocator ) );
		assert( isAligned( ptr, allocator->alignment ) );

		if( (char*)ptr + size == back( allocator ) ) {
			allocator->size -= safe_truncate< int32 >( size );
		} else {
			FixedSizeAllocatoreFreeEntry newFreeElem = {allocator->firstFree};
			memcpy( ptr, &newFreeElem, sizeof( FixedSizeAllocatoreFreeEntry ) );
			allocator->firstFree = distance( allocator->ptr, (char*)ptr );
		}
		--allocator->elementCount;
		if( allocator->elementCount <= 0 ) {
			allocator->size = 0;
			allocator->elementCount = 0;
			allocator->firstFree = -1;
		}
	}
}

FixedSizeAllocator makeFixedSizeAllocator( StackAllocator* allocator, int32 count,
                                           uint16 elementSize, uint16 alignment )
{
	assert( elementSize >= sizeof( FixedSizeAllocatoreFreeEntry ) );
	auto size = count * elementSize;
	return {allocateArray( allocator, char, size ), 0, size, elementSize, alignment, -1};
}
FixedSizeAllocator makeFixedSizeAllocator( void* ptr, int32 count, uint16 elementSize,
                                           uint16 alignment )
{
	assert( elementSize >= sizeof( FixedSizeAllocatoreFreeEntry ) );
	return {(char*)ptr, 0, count * elementSize, elementSize, alignment};
}