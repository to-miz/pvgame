// TODO: file system endianness

struct MemoryWriter {
	void* ptr;
	int32 sz;
	int32 cap;

	void* data() const { return ptr; }
	int32 size() const { return sz; }
	int32 capacity() const { return cap; }
	int32 remaining() const;
};

bool isValid( const MemoryWriter* writer )
{
	return writer && ( ( writer->ptr && writer->sz >= 0 && writer->cap >= 0
	                     && writer->sz <= writer->cap )
	                   || ( !writer->ptr && writer->sz == 0 && writer->cap == 0 ) );
}

int32 remaining( const MemoryWriter* writer )
{
	assert( isValid( writer ) );
	return writer->cap - writer->sz;
}
int32 MemoryWriter::remaining() const
{
	return ::remaining( this );
}

template < class T >
void write( MemoryWriter* writer, const T* src, int32 count )
{
	assert( isValid( writer ) );
	auto sz = min( count * (int32)sizeof( T ), remaining( writer ) );
	memcpy( (char*)writer->ptr + writer->sz, src, sz );
	writer->sz += sz;
}

template < class T >
void write( MemoryWriter* writer, Array< T > array )
{
	write( writer, array.data(), array.size() );
}
void write( MemoryWriter* writer, char c ) { write( writer, &c, 1 ); }
void write( MemoryWriter* writer, int32 value ) { write( writer, &value, 1 ); }
void write( MemoryWriter* writer, StringView str )
{
	write( writer, str.data(), str.size() );
}
void writePascalString( MemoryWriter* writer, StringView str )
{
	write( writer, str.size() );
	write( writer, str );
}

MemoryWriter makeMemoryWriter( StackAllocator* allocator, int32 sz )
{
	auto array = allocateArray( allocator, char, sz );
	return {array, 0, ( array ) ? ( sz ) : ( 0 )};
}
MemoryWriter makeMemoryWriter( StackAllocator* allocator )
{
	return makeMemoryWriter( allocator, safe_truncate< int32 >( remaining( allocator ) ) );
}

// reader

struct MemoryReader {
	const void* ptr;
	int32 sz;
	int32 cap;

	const void* data() const { return ptr; }
	int32 size() const { return sz; }
	int32 capacity() const { return cap; }
	int32 remaining() const;
};

bool isValid( const MemoryReader* reader )
{
	return reader && ( ( reader->ptr && reader->sz >= 0 && reader->cap >= 0
	                     && reader->sz <= reader->cap )
	                   || ( !reader->ptr && reader->sz == 0 && reader->cap == 0 ) );
}

int32 remaining( const MemoryReader* reader )
{
	assert( isValid( reader ) );
	return reader->cap - reader->sz;
}
int32 MemoryReader::remaining() const
{
	return ::remaining( this );
}

template < class T >
bool read( MemoryReader* reader, T* src, int32 count )
{
	assert( isValid( reader ) );
	auto sz        = count * (int32)sizeof( T );
	auto tooBig    = sz > remaining( reader );
	int32 copySize = ( tooBig ) ? ( remaining( reader ) ) : ( sz );
	memcpy( src, (const char*)reader->ptr + reader->sz, copySize );
	reader->sz += copySize;
	if( tooBig ) {
		// set remaining bytes to 0, so there are no uninitialized values
		memset( (char*)src + copySize, 0, sz - copySize );
		return false;
	}
	return true;
}

template < class T >
bool read( MemoryReader* reader, Array< T > array )
{
	return read( reader, array.data(), array.size() );
}
template < class T >
T read( MemoryReader* reader )
{
	T value;
	read( reader, &value, 1 );
	return value;
}
template < class T >
T peek( MemoryReader* reader )
{
	auto sz     = reader->sz;
	auto result = read< T >( reader );
	reader->sz  = sz;
	return result;
}
bool read( MemoryReader* reader, char c ) { return read< char >( reader ) == c; }
bool read( MemoryReader* reader, int32 value ) { return read< int32 >( reader ) == value; }
bool read( MemoryReader* reader, StringView str )
{
	if( remaining( reader ) < str.size() ) {
		return false;
	}
	if( str == StringView( (const char*)reader->data(), str.size() ) ) {
		reader->sz += str.size();
		return true;
	}
	return false;
}
int32 readPascalString( MemoryReader* reader, char* dest, int32 destSize )
{
	auto stringSize = read< int32 >( reader );
	auto size       = min( stringSize, destSize );
	if( !read( reader, dest, size ) ) {
		return 0;
	}
	reader->sz += stringSize - size;
	return size;
}
bool readPascalString( MemoryReader* reader, string dest )
{
	auto strSize = peek< int32 >( reader );
	dest.resize( readPascalString( reader, dest.data(), dest.capacity() ) );
	return dest.size() == strSize;
}

MemoryReader makeMemoryReader( const void* data, int32 size )
{
	return {(const char*)data, 0, ( data ) ? ( size ) : ( 0 )};
}
MemoryReader makeMemoryReader( StringView data )
{
	return makeMemoryReader( data.data(), data.size() );
}