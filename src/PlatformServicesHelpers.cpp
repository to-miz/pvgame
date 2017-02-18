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