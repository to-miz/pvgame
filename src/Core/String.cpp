typedef UArray< char > string;

template< uint8 N >
struct short_string {
	char ptr[N];
	uint8 sz;

	char* begin() { return ptr; }
	char* end() { return ptr + sz; }
	const char* begin() const { return ptr; }
	const char* end() const { return ptr + sz; }
	const char* cbegin() const { return ptr; }
	const char* cend() const { return ptr + sz; }

	int32 capacity() const { return N; }
	int32 size() const { return sz; }
	int32 length() const { return sz; }
	char* data() { return ptr; }
	bool empty() { return sz == 0; }
	int32 remaining() { return N - sz; }

	void resize( int32 newSize )
	{
		assert( newSize <= (int32)N );
		sz = (uint8)newSize;
	}

	inline void assign( StringView other ) { sz = (uint8)copyToString( other, ptr, N ); }

	inline operator StringView() const { return {ptr, sz}; }
};

void copyToString( const char* in, string* out )
{
	auto len = (int32)strlen( in );
	if( len > out->capacity() ) {
		len = out->capacity();
	}
	out->assign( in, in + len );
}

template < class... Types >
string snprint( StackAllocator* allocator, StringView format, const Types&... args )
{
	auto result = beginVector( allocator, char );
	result.resize( snprint( result.data(), result.capacity(), format, args... ) );
	endVector( allocator, &result );
	return result;
}

// create aliasing substr from str
string substr( string other, int32 pos, int32 len )
{
	pos         = clamp( pos, 0, other.sz );
	auto maxLen = other.sz - pos;
	len         = clamp( len, 0, maxLen );
	return {other.ptr + pos, len, ( len == maxLen ) ? ( other.cap - pos ) : ( len )};
}
string substr( string other, int32 pos )
{
	pos = clamp( pos, 0, other.sz );
	return {other.ptr + pos, other.sz - pos, other.cap - pos};
}

string makeString( StackAllocator* allocator, int32 len )
{
	return makeUArray( allocator, char, len );
}
string makeString( string other, int32 pos ) { return substr( other, pos ); }
string makeString( string other, int32 pos, int32 len ) { return substr( other, pos, len ); }

string makeString( char* start, int32 len ) { return {start, len, len}; }
string makeString( char* start, int32 len, int32 cap ) { return {start, len, cap}; }

// make string by copying str into {p, size}
string makeString( StringView str, char* p, int32 size )
{
	auto ret = makeString( p, size );
	ret.sz   = copyToString( str, p, size );
	return ret;
}

string makeString( StackAllocator* allocator, StringView str )
{
	auto len = min( (int32)remaining( allocator, alignof( char ) ), str.size() );
	string result = makeString( allocator, len );
	result.resize( copyToString( str, result.data(), result.capacity() ) );
	return result;
}

struct StringPool {
	UArray< char > memory;
};
StringPool makeStringPool( StackAllocator* allocator, int32 size )
{
	return {makeUArray( allocator, char, size )};
}
StringPool makeStringPool( char* buffer, int32 size )
{
	return {makeUninitializedArrayView( buffer, size )};
}
string pushString( StringPool* pool, StringView str )
{
	int32 start = pool->memory.size();
	pool->memory.append( str.begin(), str.end() );
	return makeString( pool->memory.data() + start, pool->memory.size() - start );
}
string pushString( StringPool* pool, int32 len )
{
	int32 start = pool->memory.size();
	pool->memory.resize( start + len );
	return makeString( pool->memory.data() + start, 0, pool->memory.size() - start );
}
bool hasSpace( StringPool* pool, int32 len )
{
	assert( pool );
	return pool->memory.remaining() >= len;
}