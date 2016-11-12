#define TMC_CONVENIENCE
#define TMC_CPP_OVERLOADS
#define TMC_CPP_TEMPLATED
#define TMC_STRING_VIEW StringView
#define TMC_STRING_VIEW_DATA( str ) ( str ).data()
#define TMC_STRING_VIEW_SIZE( str ) ( str ).size()
#define TMC_ASSERT assert
#define TMC_ISDIGIT isdigit
#define TMC_ISUPPER isupper
#define TMC_ISLOWER islower
#define TMC_MEMCPY memcpy
#define TMC_MEMSET memset
#define TMC_SIGNBIT signbit
#define TMC_ISNAN isnan
#define TMC_ISINF isinf
#define TMC_STRNICMP strnicmp
#define TMC_STRNREV strnrev
#define TMC_OWN_TYPES
typedef int32 tmc_size_t;
typedef uint8 tmc_uint8;
typedef int32 tmc_int32;
typedef uint32 tmc_uint32;
typedef int64 tmc_int64;
typedef uint64 tmc_uint64;
typedef bool tmc_bool;
#define TMC_TRUE true
#define TMC_FALSE false
#define TMC_INT32_MAX INT32_MAX
#define TMC_UINT32_MAX UINT32_MAX
#define TMC_INT64_MAX INT64_MAX
#define TMC_UINT64_MAX UINT64_MAX
#define TMC_INT32_MIN INT32_MIN
#define TMC_UINT32_MIN UINT32_MIN
#define TMC_INT64_MIN INT64_MIN
#define TMC_UINT64_MIN UINT64_MIN
#define TM_CONVERSION_IMPLEMENTATION
#include <tm_conversion.h>

#define TMP_NO_CSTDIO
#define TMP_ASSERT assert
#define TMP_MEMCPY memcpy
#define TMP_OWN_TYPES
typedef int32 tmp_int32;
typedef uint32 tmp_uint32;
typedef int64 tmp_int64;
typedef uint64 tmp_uint64;
typedef int32 tmp_size_t;

// custom printers
tmp_size_t snprint( char* buffer, tmp_size_t len, const PrintFormat& initialFormatting,
                    const vec2& value );
tmp_size_t snprint( char* buffer, tmp_size_t len, const PrintFormat& initialFormatting,
                    const vec3& value );
tmp_size_t snprint( char* buffer, tmp_size_t len, const PrintFormat& initialFormatting,
                    const vec4& value );
tmp_size_t snprint( char* buffer, tmp_size_t len, const PrintFormat& initialFormatting,
                    const rectf& value );

#define TMP_NO_INCLUDE_TM_CONVERSION
#define TMP_STRING_VIEW StringView
#define TMP_STRING_VIEW_DATA( x ) ( ( x ).data() )
#define TMP_STRING_VIEW_SIZE( x ) ( ( x ).size() )
#define TMP_CUSTOM_PRINTING
#define TM_PRINT_IMPLEMENTATION
#include <tm_print.h>

tmp_size_t snprint( char* buffer, tmp_size_t len, const PrintFormat& initialFormatting,
                    const vec2& value )
{
	return ::snprint( buffer, len, "{{{}, {}}", value.x, value.y );
}
tmp_size_t snprint( char* buffer, tmp_size_t len, const PrintFormat& initialFormatting,
                    const vec3& value )
{
	return ::snprint( buffer, len, "{{{}, {}, {}}", value.x, value.y, value.z );
}
tmp_size_t snprint( char* buffer, tmp_size_t len, const PrintFormat& initialFormatting,
                    const vec4& value )
{
	return ::snprint( buffer, len, "{{{}, {}, {}, {}}", value.x, value.y, value.z, value.w );
}
tmp_size_t snprint( char* buffer, tmp_size_t len, const PrintFormat& initialFormatting,
                    const rectf& value )
{
	return ::snprint( buffer, len, "{{{}, {}, {}, {}}", initialFormatting, value.left, value.top,
	                  value.right, value.bottom );
}

struct string_builder {
	char* ptr;
	int32 sz;
	int32 cap;
	PrintFormat format;

	char* data() { return ptr; }
	int32 size() { return sz; }
	char* end() { return ptr + sz; }
	int32 remaining() { return cap - sz; }
	void clear() { sz = 0; }

	string_builder( char* ptr, int32 cap )
	: ptr( ptr ), sz( 0 ), cap( cap ), format( defaultPrintFormat() )
	{
	}
	template < class T >
	string_builder& operator<<( T value )
	{
		sz += ::print( end(), remaining(), &format, value );
		return *this;
	}
	string_builder& operator<<( char c )
	{
		if( cap - sz > 0 ) {
			*( ptr + sz++ ) = c;
		}
		return *this;
	}
	string_builder& operator<<( const char* str )
	{
		auto len = safe_truncate< int32 >( strlen( str ) );
		len      = ( len < remaining() ) ? ( len ) : ( remaining() );
		memcpy( end(), str, len );
		sz += len;
		return *this;
	}
	string_builder& operator<<( StringView str )
	{
		auto len = ( str.size() < remaining() ) ? ( str.size() ) : ( remaining() );
		memcpy( end(), str.data(), len );
		sz += len;
		return *this;
	}
	string_builder& operator<<( string str )
	{
		return operator<<( StringView( str ) );
	}

	template < class... Types >
	string_builder& print( const char* format, Types... args )
	{
		sz += snprint( ptr + sz, cap - sz, format, args... );
		return *this;
	}
	template < class... Types >
	string_builder& println( const char* format, Types... args )
	{
		sz += snprint( ptr + sz, cap - sz, format, args... );
		if( cap - sz ) {
			*( ptr + sz ) = '\n';
			++sz;
		}
		return *this;
	}
};

template < size_t N >
struct static_string_builder : string_builder {
	char buffer[N];
	static_string_builder() : string_builder( buffer, (int32)N ) {}
};

StringView asStringView( const string_builder& builder ) { return {builder.ptr, builder.sz}; }

struct NumberString {
	char data[100];
	int32 count;

	inline operator StringView const() { return {data, count}; }
};
NumberString toNumberString( float value )
{
	NumberString result;
	result.count = to_string( value, result.data, countof( result.data ) );
	return result;
}
NumberString toNumberString( int32 value )
{
	NumberString result;
	result.count = to_string( value, result.data, countof( result.data ) );
	return result;
}