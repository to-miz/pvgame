typedef UArray< char > string;

void copyToString( const char* in, string* out )
{
	auto len = (int32)strlen( in );
	if( len > out->capacity() ) {
		len = out->capacity();
	}
	out->assign( in, in + len );
}
int32 copyToString( const char* in, char* out, int32 outSize )
{
	auto len = (int32)strlen( in );
	if( len > outSize ) {
		len = outSize;
	}
	memcpy( out, in, len );
	return len;
}

template < class... Types >
string snprint( StackAllocator* allocator, StringView format, const Types&... args )
{
	auto result = beginVector( allocator, char );
	result.resize( snprint( result.data(), result.capacity(), format, args... ) );
	endVector( allocator, &result );
	return result;
}