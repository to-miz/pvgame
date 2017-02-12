#define TM_JSON_IMPLEMENTATION
#define TMJ_STRING_VIEW StringView
#define TMJ_STRING_VIEW_DATA( name ) ( name ).data()
#define TMJ_STRING_VIEW_SIZE( name ) ( name ).size()
#define TMJ_ASSERT assert
#define TMJ_MEMCHR memchr
#define TMJ_STRLEN strlen
#define TMJ_ISDIGIT isdigit
#define TMJ_ISALPHA isalpha
#define TMJ_ISXDIGIT isxdigit
#define TMJ_ISSPACE isspace
#define TMJ_TOUPPER toupper
#define TMJ_MEMCMP memcmp
#define TMJ_MEMCPY memcpy
#define TMJ_MEMSET memset
#define TMJ_TO_INT( str, len, base, def ) to_i32_n( ( str ), ( len ), Radix{( base )}, ( def ) )
#define TMJ_TO_UINT( str, len, base, def ) to_u32_n( ( str ), ( len ), Radix{( base )}, ( def ) )
#define TMJ_TO_INT64( str, len, base, def ) to_i64_n( ( str ), ( len ), Radix{( base )}, ( def ) )
#define TMJ_TO_UINT64( str, len, base, def ) to_u64_n( ( str ), ( len ), Radix{( base )}, ( def ) )
#define TMJ_TO_FLOAT( str, len, def ) to_float_n( ( str ), ( len ), ( def ) )
#define TMJ_TO_DOUBLE( str, len, def ) to_double_n( ( str ), ( len ), ( def ) )
#define TMJ_OWN_TYPES
typedef int32 tmj_size_t;
typedef int8 tmj_int8;
typedef uintptr tmj_uintptr;
#include <tm_json.h>

template< class T > T getAs( const JsonValue& value, T def = {} );
template<> int32 getAs< int32 >( const JsonValue& value, int32 def ) { return value.getInt( def ); }
template<> uint32 getAs< uint32 >( const JsonValue& value, uint32 def ) { return value.getUInt( def ); }
template<> float getAs< float >( const JsonValue& value, float def ) { return value.getFloat( def ); }
template<> double getAs< double >( const JsonValue& value, double def ) { return value.getDouble( def ); }
template<> bool getAs< bool >( const JsonValue& value, bool def ) { return value.getBool( def ); }
template<> int64 getAs< int64 >( const JsonValue& value, int64 def ) { return value.getInt64( def ); }
template<> uint64 getAs< uint64 >( const JsonValue& value, uint64 def ) { return value.getUInt64( def ); }
template<> char getAs< char >( const JsonValue& value, char def ) { return (char)value.getInt( def ); }
template<> int8 getAs< int8 >( const JsonValue& value, int8 def ) { return (int8)value.getInt( def ); }
template<> int16 getAs< int16 >( const JsonValue& value, int16 def ) { return (int16)value.getInt( def ); }
template<> uint8 getAs< uint8 >( const JsonValue& value, uint8 def ) { return (uint8)value.getUInt( def ); }
template<> uint16 getAs< uint16 >( const JsonValue& value, uint16 def ) { return (uint16)value.getUInt( def ); }

template< class T, class = std::enable_if< std::is_enum< T >::value >::type >
void deserialize( const JsonValue& value, T& out )
{
	typename std::underlying_type< T >::type temp = {};
	deserialize( value, temp );
	out = (T)temp;
}

template < class T >
void deserialize( const JsonValue& value, trect< T >& out )
{
	auto attr  = value.getObject();
	out.left   = getAs< T >( attr["left"] );
	out.top    = getAs< T >( attr["top"] );
	out.right  = getAs< T >( attr["right"] );
	out.bottom = getAs< T >( attr["bottom"] );
}
void deserialize( const JsonValue& value, QuadTexCoords& out )
{
	auto attr = value.getArray();
	for( auto i = 0; i < attr.size(); ++i ) {
		auto entry        = attr[i].getObject();
		out.elements[i].x = entry["x"].getFloat();
		out.elements[i].y = entry["y"].getFloat();
	}
}
void deserialize( const JsonValue& value, vec2& out )
{
	auto attr = value.getObject();
	out.x     = attr["x"].getFloat();
	out.y     = attr["y"].getFloat();
}
void deserialize( const JsonValue& value, vec3& out )
{
	auto attr = value.getArray();
	out.x     = attr[0].getFloat();
	out.y     = attr[1].getFloat();
	out.z     = attr[2].getFloat();
}

void deserialize( const JsonValue& value, float& out, float def = {} )
{
	out = value.getFloat( def );
}
void deserialize( const JsonValue& value, bool& out, bool def = {} ) { out = value.getBool( def ); }
void deserialize( const JsonValue& value, bool8& out, bool def = {} ) { out = value.getBool( def ); }
void deserialize( const JsonValue& value, char& out, char def = {} )
{
	out = (char)value.getInt( def );
}
void deserialize( const JsonValue& value, int8& out, int8 def = {} )
{
	out = (int8)value.getInt( def );
}
void deserialize( const JsonValue& value, uint8& out, uint8 def = {} )
{
	out = (uint8)value.getUInt( def );
}
void deserialize( const JsonValue& value, int16& out, int16 def = {} )
{
	out = (int16)value.getInt( def );
}
void deserialize( const JsonValue& value, uint16& out, uint16 def = {} )
{
	out = (uint16)value.getUInt( def );
}
void deserialize( const JsonValue& value, int32& out, int32 def = {} )
{
	out = (int32)value.getInt( def );
}
void deserialize( const JsonValue& value, uint32& out, uint32 def = {} )
{
	out = (uint32)value.getUInt( def );
}

JsonStackAllocator makeJsonAllocator( StackAllocator* allocator, int32 size )
{
	JsonStackAllocator result = {};
	assert( size >= 0 );
	if( size ) {
		auto array = allocateArray( allocator, char, size );
		result = {array, 0, (size_t)size};
	}
	return result;
}

// make new type for flags, so that if we ever write something like
//	makeJsonDocument( allocator, file.data(), file.size() );
// it doesn't silently compile, where it interprets file.size() as the flags param
struct JsonFlags {
	uint32 flags;
};
JsonDocument makeJsonDocument( StackAllocator* allocator, StringView data,
                               JsonFlags flags = {JSON_READER_STRICT} )
{
	auto jsonAlloc = makeJsonAllocator( allocator, data.size() * sizeof( JsonValue ) );
	auto doc = jsonMakeDocument( &jsonAlloc, data.data(), data.size(), flags.flags );
	reallocateInPlace( allocator, jsonAlloc.ptr, jsonAlloc.size, jsonAlloc.capacity,
	                   alignof( char ) );
	return doc;
}