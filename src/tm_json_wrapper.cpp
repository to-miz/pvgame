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

void serialize( const JsonValue& value, recti& out )
{
	auto attr   = value.getObject();
	out.left    = attr["left"].getInt();
	out.top     = attr["top"].getInt();
	out.right   = attr["right"].getInt();
	out.bottom  = attr["bottom"].getInt();
}
void serialize( const JsonValue& value, QuadTexCoords& out )
{
	auto attr = value.getArray();
	for( auto i = 0; i < attr.size(); ++i ) {
		auto entry        = attr[i].getObject();
		out.elements[i].x = entry["x"].getFloat();
		out.elements[i].y = entry["y"].getFloat();
	}
}
void serialize( const JsonValue& value, vec2& out )
{
	auto attr = value.getObject();
	out.x     = attr["x"].getFloat();
	out.y     = attr["y"].getFloat();
}

void serialize( const JsonValue& value, float& out ) { out = value.getFloat(); }
void serialize( const JsonValue& value, bool& out ) { out = value.getBool(); }
void serialize( const JsonValue& value, bool8& out ) { out = value.getBool(); }
void serialize( const JsonValue& value, char& out ) { out = (char)value.getInt(); }
void serialize( const JsonValue& value, int8& out ) { out = (int8)value.getInt(); }
void serialize( const JsonValue& value, uint8& out ) { out = (uint8)value.getUInt(); }
void serialize( const JsonValue& value, int16& out ) { out = (int16)value.getInt(); }
void serialize( const JsonValue& value, uint16& out ) { out = (uint16)value.getUInt(); }
void serialize( const JsonValue& value, int32& out ) { out = (int32)value.getInt(); }
void serialize( const JsonValue& value, uint32& out ) { out = (uint32)value.getUInt(); }