#include "WString.h"

WString::WString( int32 length )
: length( length )
{
	assert( length > 0 );
	data = new wchar_t[length];
}
WString::WString( WString&& other )
: data( other.data ), length( other.length )
{
	other.data = nullptr;
}
WString::WString( const WString& other )
: length( other.length )
{
	data = new wchar_t[other.length];
	memcpy( data, other.data, other.length * sizeof( wchar_t ) );
}
WString& WString::operator=( const WString& other )
{
	if( this != &other ) {
		delete[] data;
		data = new wchar_t[other.length];
		length = other.length;
		memcpy( data, other.data, other.length * sizeof( wchar_t ) );
	}
	return *this;
}
WString::~WString()
{
	if( data ) {
		delete[] data;
		data = nullptr;
	}
}

WString WString::fromUtf8( StringView str )
{
	static_assert( sizeof( wchar_t ) == 2 && alignof( wchar_t ) == alignof( uint16 ),
	               "wchar_t size is unexpected" );

	auto strMaxSize = str.size() * 2 + 1;
	WString result( strMaxSize );
	result.length =
	    utf16::convertUtf8ToUtf16( str.data(), str.size(), (uint16*)result.data, strMaxSize - 1 );
	result.data[result.length] = 0;
	return result;
}

bool equalsIgnoreCase( const wchar_t* a, const wchar_t* b )
{
	while( *a && *b ) {
		if( toupper( *a ) != toupper( *b ) ) return false;
		++a;
		++b;
	}
	return *a == *b;
}