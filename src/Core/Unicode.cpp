#include "Unicode.h"

uint32 utf16::next( const uint16*& it, int32& remaining )
{
	uint32 codepoint = *it;
	if( isSurrogateLead( codepoint ) ) {
		if( remaining >= 2 ) {
			uint32 trail = *( ++it );
			codepoint = ( codepoint << 10 ) + trail + SURROGATE_OFFSET;
			remaining -= 2;
		} else {
			codepoint = 0xFFFFFFFF;
			remaining = 0;
			return codepoint;
		}
	} else if( codepoint > 0x10FFFF ) {
		codepoint = 0xFFFFFFFF;
	}
	++it;
	--remaining;
	return codepoint;
}

void utf16::swapEndian( uint16* str, int32 length )
{
	for( auto i = 0; i < length; ++i ) {
		str[i] = ::swapEndian( str[i] );
	}
}
void utf16::swapEndian( char* str, int32 size )
{
	assert( size % 2 == 0 );
	for( auto i = 1; i < size; i += 2 ) {
		std::swap( str[i], str[i - 1] );
	}
}

utf16::Utf16Sequence utf16::toUtf16( uint32 codepoint ) {
	Utf16Sequence result;
	if( codepoint < 0xD7FF ) {
		result.element[0] = (uint16)codepoint;
		result.length = 1;
	} else if( codepoint >= 0xE000 && codepoint <= 0xFFFF ) {
		result.element[0] = (uint16)codepoint;
		result.length = 1;
	} else if( codepoint >= 0x10000 && codepoint <= 0x10FFFF ) {
		codepoint -= 0x10000;
		result.element[0] = 0xD800 + (uint16)( codepoint >> 10 );
		result.element[1] = 0xDC00 + (uint16)( codepoint & 0x3FF );
		result.length = 2;
	} else {
		result = {};
	}
	return result;
}

uint16 utf16::toUcs2( uint32 codepoint ) {
	if( isUcs2( codepoint ) ) {
		return (uint16)codepoint;
	}
	return 0;
}

int32 utf16::convertUtf8ToUtf16( const char* utf8Start, int32 utf8Length, uint16* out, int32 size )
{
	int32 result = 0;
	while( utf8Length ) {
		auto codepoint = utf8::next( utf8Start, utf8Length );
		if( !utf8Length && codepoint == 0xFFFFFFFF ) break;
		if( codepoint == 0xFFFFFFFF ) codepoint = 0;

		auto sequence = toUtf16( codepoint );
		if( sequence.length > size ) break;
		memcpy( out, sequence.element, sequence.length * sizeof( uint16 ) );
		out += sequence.length;
		size -= sequence.length;
		result += sequence.length;
	}
	return result;
}
int32 utf16::convertUtf8ToUcs2( const char* utf8Start, int32 utf8Length, uint16* out, int32 size )
{
	int32 result = 0;
	while( utf8Length ) {
		auto codepoint = utf8::next( utf8Start, utf8Length );
		if( !utf8Length && codepoint == 0xFFFFFFFF ) break;
		if( !isUcs2( codepoint ) ) continue;

		*out = (uint16)codepoint;
		++out;
		--size;
		++result;
	}
	return result;
}

// utf8

uint32 utf8::next( const char*& it, int32& remaining )
{
	uint32 codepoint = (uint32)( (uint8)*it );

	if( codepoint < 0x80 ) { // 0xxxxxxx
		// 1 byte sequence
	} else if( ( codepoint >> 5 ) == 0x6 ) { // 110xxxxx 10xxxxxx
		// 2 byte sequence
		if( remaining >= 2 ) {
			uint32 second = (uint8)*( ++it );
			codepoint = ( ( codepoint & 0x1F ) << 6 ) | ( second & 0x3F );
			--remaining;
		} else {
			remaining = 0;
			return 0xFFFFFFFF;
		}
	} else if( ( codepoint >> 4 ) == 0xE ) { // 1110xxxx 10xxxxxx 10xxxxxx
		// 3 byte sequence
		if( remaining >= 3 ) {
			uint32 second = (uint8)*( ++it );
			uint32 third = (uint8)*( ++it );
			codepoint =
				( ( codepoint & 0xF ) << 12 ) | ( ( second & 0x3F ) << 6 ) | ( third & 0x3F );
			remaining -= 2;
		} else {
			remaining = 0;
			return 0xFFFFFFFF;
		}
	} else if( ( codepoint >> 3 ) == 0x1E ) { // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		// 4 byte sequence
		if( remaining >= 4 ) {
			uint32 second = (uint8)*( ++it );
			uint32 third = (uint8)*( ++it );
			uint32 fourth = (uint8)*( ++it );
			codepoint = ( ( codepoint & 0x7 ) << 18 ) | ( ( second & 0x3F ) << 12 )
						| ( ( third & 0x3F ) << 6 ) | ( fourth & 0x3f );
			remaining -= 3;
		} else {
			remaining = 0;
			return 0xFFFFFFFF;
		}
	} else {
		// malformed
		/*remaining = 0;
		return 0;*/
		codepoint = 0xFFFFFFFF;
	}

	++it;
	--remaining;
	return codepoint;
}

int32 utf8::countCodepoints( const char* str, int32 size )
{
	assert( size >= 0 );
	int32 ret = 0;
	while( size ) {
		next( str, size );
		++ret;
	}
	return ret;
}

utf8::Utf8Sequence utf8::toUtf8( uint32 codepoint )
{
	Utf8Sequence result;
	if( codepoint < 0x80 ) {
		// 1 byte sequence
		result.oct[0] = (char)( codepoint );
		result.length = 1;
	} else if( codepoint < 0x800 ) {
		// 2 byte sequence 110xxxxx 10xxxxxx
		result.oct[0] = (char)( 0xC0 | (uint8)( codepoint >> 6 ) );
		result.oct[1] = (char)( 0x80 | (uint8)( codepoint & 0x3F ) );
		result.length = 2;
	} else if( codepoint < 0x10000 ) {
		// 3 byte sequence 1110xxxx 10xxxxxx 10xxxxxx
		result.oct[0] = (char)( 0xE0 | (uint8)( codepoint >> 12 ) );
		result.oct[1] = (char)( 0x80 | ( (uint8)( codepoint >> 6 ) & 0x3F ) );
		result.oct[2] = (char)( 0x80 | ( (uint8)( codepoint & 0x3F ) ) );
		result.length = 3;
	} else {
		// 4 byte sequence 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		result.oct[0] = (char)( 0xF0 | ( (uint8)( codepoint >> 18 ) & 0x7 ) );
		result.oct[1] = (char)( 0x80 | ( (uint8)( codepoint >> 12 ) & 0x3F ) );
		result.oct[2] = (char)( 0x80 | ( (uint8)( codepoint >> 6 ) & 0x3F ) );
		result.oct[3] = (char)( 0x80 | ( (uint8)( codepoint & 0x3F ) ) );
		result.length = 4;
	}

	return result;
}

int32 utf8::convertUtf16ToUtf8( const uint16* utf16Start, int32 utf16Length, char* out, int32 size )
{
	int32 result = 0;
	while( utf16Length ) {
		auto codepoint = utf16::next( utf16Start, utf16Length );
		if( !utf16Length && codepoint == 0xFFFFFFFF ) break;
		if( codepoint == 0xFFFFFFFF ) codepoint = 0;

		auto sequence = toUtf8( codepoint );
		if( sequence.length > size ) break;
		memcpy( out, sequence.oct, sequence.length );
		out += sequence.length;
		size -= sequence.length;
		result += sequence.length;
	}
	return result;
}

int32 utf8::convertUcs2ToUtf8( const uint16* ucs2Start, int32 ucs2Length, char* out, int32 size )
{
	int32 result = 0;
	for( auto i = 0; i < ucs2Length; ++i ) {
		if( !utf16::isUcs2( ucs2Start[i] ) ) continue;
		auto codepoint = (uint32)ucs2Start[i];

		auto sequence = toUtf8( codepoint );
		if( sequence.length > size ) break;
		memcpy( out, sequence.oct, sequence.length );
		out += sequence.length;
		size -= sequence.length;
		result += sequence.length;
	}
	return result;
}

#ifdef GAME_USE_STD
	std::string utf8::toUtf8String( uint32 codepoint )
	{
		auto sequence = toUtf8( codepoint );
		return std::string( sequence.oct, sequence.length );
	}
	std::string utf8::convertUtf16ToUtf8( const uint16* utf16Start, int32 utf16Length )
	{
		std::string ret;
		ret.reserve( utf16Length );
		while( utf16Length ) {
			auto codepoint = utf16::next( utf16Start, utf16Length );
			auto sequence = toUtf8( codepoint );
			ret.insert( ret.end(), sequence.oct, sequence.oct + sequence.length );
		}
		return ret;
	}

#endif
