#pragma once

#ifndef _UNICODE_H_INCLUDED_
#define _UNICODE_H_INCLUDED_

namespace utf16
{
const int32 ByteOrderMarkSize           = 1;
const int32 ByteOrderMarkSizeInBytes    = 2;
const char LittleEndianByteOrderMark[2] = {(char)( (uint8)0xFF ), (char)( (uint8)0xFE )};
const char BigEndianByteOrderMark[2]    = {(char)( (uint8)0xFE ), (char)( (uint8)0xFF )};
constexpr inline bool hasUtf16LittleEndianByteOrderMark( StringView str )
{
	return isPrefix( str,
	                 {LittleEndianByteOrderMark, countof( LittleEndianByteOrderMark )} );
}
inline bool hasUtf16LittleEndianByteOrderMark( uint16* str, int32 length )
{
	if( length >= 1 ) {
		auto p = (char*)str;
		return p[0] == LittleEndianByteOrderMark[0] && p[1] == LittleEndianByteOrderMark[1];
	} else {
		return false;
	}
}
inline bool hasUtf16LittleEndianByteOrderMark( const char* str, int32 length )
{
	if( length >= 2 ) {
		return (uint8)str[0] == LittleEndianByteOrderMark[0]
		       && (uint8)str[1] == LittleEndianByteOrderMark[1];
	} else {
		return false;
	}
}
constexpr inline bool hasUtf16BigEndianByteOrderMark( StringView str )
{
	return isPrefix( str, {BigEndianByteOrderMark, countof( BigEndianByteOrderMark )} );
}
inline bool hasUtf16BigEndianByteOrderMark( uint16* str, int32 length )
{
	if( length >= 1 ) {
		auto p = (char*)str;
		return (uint8)p[0] == BigEndianByteOrderMark[0] && (uint8)p[1] == BigEndianByteOrderMark[1];
	} else {
		return false;
	}
}
inline bool hasUtf16BigEndianByteOrderMark( const char* str, int32 length )
{
	if( length >= 2 ) {
		return (uint8)str[0] == BigEndianByteOrderMark[0]
		       && (uint8)str[1] == BigEndianByteOrderMark[1];
	} else {
		return false;
	}
}

constexpr const uint32 LEAD_SURROGATE_MIN     = 0xD800;
constexpr const uint32 LEAD_SURROGATE_MAX     = 0xDBFF;
constexpr const uint32 TRAILING_SURROGATE_MIN = 0xDC00;
constexpr const uint32 TRAILING_SURROGATE_MAX = 0xDFFF;
constexpr const uint32 LEAD_OFFSET            = 0xD800 - ( 0x10000 >> 10 );
constexpr const uint32 SURROGATE_OFFSET       = ( uint32 )( 0x10000 - ( 0xD800 << 10 ) - 0xDC00 );

constexpr inline bool isSurrogateLead( uint32 codepoint )
{
	return codepoint >= LEAD_SURROGATE_MIN && codepoint <= LEAD_SURROGATE_MAX;
}

uint32 next( const uint16*& it, int32& remaining );
void swapEndian( uint16* str, int32 length );
void swapEndian( char* str, int32 size );

struct Utf16Sequence {
	uint16 element[2];
	int8 length;
};

// TODO: get rid of ucs2 converting for windows 2000 and up

Utf16Sequence toUtf16( uint32 codepoint );
uint16 toUcs2( uint32 codepoint );
constexpr inline bool isUcs2( uint32 codepoint )
{
	return ( codepoint < 0xD7FF ) || ( codepoint >= 0xE000 && codepoint <= 0xFFFF );
}
int32 convertUtf8ToUtf16( const char* utf8Start, int32 utf8Length, uint16* out, int32 size );
int32 convertUtf8ToUcs2( const char* utf8Start, int32 utf8Length, uint16* out, int32 size );
}

namespace utf8
{
constexpr const int32 ByteOrderMarkSize        = 3;
constexpr const int32 ByteOrderMarkSizeInBytes = 3;
constexpr const char ByteOrderMark[3]          = {(char)( (uint8)0xEF ), (char)( (uint8)0xBB ),
                                         (char)( (uint8)0xBF )};

constexpr inline bool hasUtf8ByteOrderMark( StringView str )
{
	return isPrefix( str, {ByteOrderMark, countof( ByteOrderMark )} );
}

constexpr inline bool isLead( char c ) { return ( ( (uint8)c ) & 0xC0 ) != 0x80; }

uint32 next( const char*& it, int32& remaining );
inline bool next( const char*& it, int32& remaining, uint32* codepoint )
{
	assert( codepoint );
	if( !remaining ) return false;
	*codepoint = next( it, remaining );
	return true;
}
inline uint32 next( StringView* str )
{
	int32 size       = str->size();
	const char* data = str->data();
	auto ret         = next( data, size );
	str->pop_front( str->size() - size );
	return ret;
}

inline int32 advance( const char* it, int32 current, int32 size )
{
	size -= current;
	if( size ) {
		auto prev = size;
		it += current;
		next( it, size );
		return prev - size;
	}
	return 0;
}
inline int32 retreat( const char* it, int32 current )
{
	auto start = current;
	if( current <= 0 ) {
		return 0;
	}

	do {
		--current;
	} while( current > 0 && !isLead( it[current] ) );
	return start - current;
}

struct Utf8Sequence {
	char oct[4];
	int8 length;

	inline StringView asStringView() const { return {oct, length}; };
};

Utf8Sequence toUtf8( uint32 codepoint );
int32 convertUtf16ToUtf8( const uint16* utf16Start, int32 utf16Length, char* out, int32 size );
#ifndef UNICODE_NO_WCHAR_T_OVERLOAD
	inline int32 convertUtf16ToUtf8( const wchar_t* utf16Start, int32 utf16Length, char* out,
                                     int32 size )
	{
		static_assert( sizeof( wchar_t ) == sizeof( uint16 ), "wchar_t size mismatch" );
	    return convertUtf16ToUtf8( (const uint16*)utf16Start, utf16Length, out, size );
    }
#endif // !defined( UNICODE_NO_WCHAR_T_OVERLOAD )
int32 convertUcs2ToUtf8( const uint16* ucs2Start, int32 ucs2Length, char* out, int32 size );

int32 countCodepoints( const char* str, int32 size );

#ifdef GAME_USE_STD
	std::string toUtf8String( uint32 codepoint );
	// std::string convertUtf16ToUtf8( const uint16* utf16Start, int32 utf16Length );
	std::string convertUtf16ToUtf8( const uint16* utf16Start, int32 utf16Length );
	inline std::string convertUtf16ToUtf8( const wchar_t* utf16Start, int32 utf16Length )
	{
		static_assert( sizeof( wchar_t ) == sizeof( uint16 ) && ALIGNOF( wchar_t ) == ALIGNOF( uint16 ),
		               "wchar_t incompatibility" );
		return convertUtf16ToUtf8( (const uint16*)utf16Start, utf16Length );
	}
#endif
}

#endif
