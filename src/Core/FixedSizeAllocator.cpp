struct FixedSizeAllocatoreFreeEntry {
	int32 prev;
	int32 next;
};

struct FixedSizeAllocator {
	char* ptr;
	int32 size;
	int32 capacity;
	uint16 elementSize;
	uint16 alignment;
	int32 firstFree;
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

void* allocate( FixedSizeAllocator* allocator, size_t size, uint32 alignment )
{
	assert( isValid( allocator ) );
	assert( size % allocator->elementSize == 0 );
	assert( alignment == allocator->alignment );
	not_implemented();
	return nullptr;
}
void* reallocate( FixedSizeAllocator* allocator, void* ptr, size_t newSize, size_t oldSize,
                  uint32 alignment )
{
	assert( isValid( allocator ) );
	assert( newSize % allocator->elementSize == 0 );
	assert( oldSize % allocator->elementSize == 0 );
	assert( alignment == allocator->alignment );
	not_implemented();
	if( newSize == oldSize ) {
		return ptr;
	}
	return nullptr;
}
void free( FixedSizeAllocator* allocator, void* ptr, size_t size, uint32 alignment )
{
	assert( isValid( allocator ) );
	not_implemented();
}

FixedSizeAllocator makeFixedSizeAllocator( FixedSizeAllocator* allocator, int32 capacity,
                                           uint16 elementSize, uint16 alignment )
{
	return {allocateArray( allocator, char, capacity ), 0, capacity, elementSize, alignment, -1};
}
FixedSizeAllocator makeFixedSizeAllocator( void* ptr, int32 capacity, uint16 elementSize,
                                           uint16 alignment )
{
	return {(char*)ptr, 0, capacity, elementSize, alignment};
}