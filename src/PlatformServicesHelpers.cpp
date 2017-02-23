int32 getTimeStampString( char* buffer, int32 size )
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->getTimeStampString( buffer, size );
}
short_string< 50 > getTimeStampString()
{
	short_string< 50 > result;
	result.resize(::getTimeStampString( result.data(), result.capacity() ) );
	return result;
}

StringView toString( VirtualKeyEnumValues key )
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->getKeyboardKeyName( key );
}

FilenameString getOpenFilename( const char* filter, const char* initialDir, bool multiselect )
{
	FilenameString result;
	result.resize( GlobalPlatformServices->getOpenFilename( filter, initialDir, multiselect,
	                                                        result.data(), result.capacity() ) );
	return result;
}
extern global_var PlatformServices* GlobalPlatformServices;
FilenameString getSaveFilename( const char* filter, const char* initialDir )
{
	FilenameString result;
	result.resize( GlobalPlatformServices->getSaveFilename( filter, initialDir, result.data(),
	                                                        result.capacity() ) );
	return result;
}

StringView readFile( StackAllocator* allocator, StringView filename )
{
	assert( GlobalPlatformServices );
	auto buffer = beginVector( allocator, char );
	buffer.resize( (int32)GlobalPlatformServices->readFileToBuffer( filename, buffer.data(),
	                                                                buffer.capacity() ) );
	endVector( allocator, &buffer );
	return {buffer.data(), buffer.size()};
}

// memory

// global_var allocation methods
void* allocate( size_t size, uint32 alignment )
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->allocate( size, alignment );
}
void* reallocate( void* ptr, size_t newSize, size_t oldSize, uint32 alignment )
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->reallocate( ptr, newSize, oldSize, alignment );
}
void* reallocateInPlace( void* ptr, size_t newSize, size_t oldSize, uint32 alignment )
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->reallocateInPlace( ptr, newSize, oldSize, alignment );
}
void deallocate( void* ptr, size_t size, uint32 alignment )
{
	assert( GlobalPlatformServices );
	GlobalPlatformServices->deallocate( ptr, size, alignment );
}

template < class T >
T* allocate( size_t count = 1 )
{
	return (T*)::allocate( count * sizeof( T ), alignof( T ) );
}
template < class T >
T* reallocate( T* ptr, size_t newCount, size_t oldCount )
{
	return (T*)::reallocate( ptr, newCount * sizeof( T ), oldCount * sizeof( T ), alignof( T ) );
}
template < class T >
T* reallocateInPlace( T* ptr, size_t newCount, size_t oldCount )
{
	return (T*)::reallocateInPlace( ptr, newCount * sizeof( T ), oldCount * sizeof( T ),
	                                alignof( T ) );
}
template < class T >
void deallocate( T* ptr, size_t count = 1 )
{
	::deallocate( ptr, count * sizeof( T ), alignof( T ) );
}

void* operator new( std::size_t size ) /*throw( std::bad_alloc )*/
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->malloc( size );
}
void operator delete( void* ptr ) /*throw()*/
{
	assert( GlobalPlatformServices );
	GlobalPlatformServices->free( ptr );
}
void* operator new[]( std::size_t size ) /*throw(std::bad_alloc)*/
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->malloc( size );
}
void operator delete[]( void* ptr ) /*throw()*/
{
	assert( GlobalPlatformServices );
	GlobalPlatformServices->free( ptr );
}

struct DynamicStackAllocator : StackAllocator {
	DynamicStackAllocator() : StackAllocator{} {}
	DynamicStackAllocator( size_t capacity )
	: StackAllocator{(char*)GlobalPlatformServices->malloc( capacity ), 0, capacity, 0}
	{
	}
	~DynamicStackAllocator()
	{
		if( ptr ) {
			GlobalPlatformServices->free( ptr );
			ptr = nullptr;
		}
	}
	DynamicStackAllocator( DynamicStackAllocator&& other )
	: StackAllocator{other.ptr, other.size, other.capacity, other.lastPoppedAlignment}
	{
		other.releaseOwnership();
	}
	DynamicStackAllocator& operator=( DynamicStackAllocator&& other )
	{
		if( this != &other ) {
			if( ptr ) {
				GlobalPlatformServices->free( ptr );
			}
			ptr                 = other.ptr;
			size                = other.size;
			capacity            = other.capacity;
			lastPoppedAlignment = other.lastPoppedAlignment;
			other.releaseOwnership();
		}
		return *this;
	}

	DynamicStackAllocator( const DynamicStackAllocator& ) = delete;
	DynamicStackAllocator& operator=( const DynamicStackAllocator& ) = delete;

	void fitToSize()
	{
		if( GlobalPlatformServices->reallocInPlace( ptr, size ) ) {
			capacity = size;
		} else {
			assert( 0 );
		}
	}

	void releaseOwnership()
	{
		ptr                 = nullptr;
		size                = 0;
		capacity            = 0;
		lastPoppedAlignment = 0;
	}
};

template < class T >
Array< T > allocateArrayView( int32 size )
{
	return makeArrayView( allocate< T >( size ), size );
}