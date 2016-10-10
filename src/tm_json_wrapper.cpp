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