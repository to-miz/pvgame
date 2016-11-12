typedef UArray< char > string;

struct StringView {
	const char* ptr = nullptr;
	int32 sz = 0;

	typedef int32 size_type;
	static const size_type npos = INT32_MAX;

	// STL container stuff
	typedef char value_type;
	typedef const char& reference;
	typedef const char& const_reference;
	typedef const char* pointer;
	typedef const char* const_pointer;
	typedef const char* iterator;
	typedef const char* const_iterator;
	typedef int32 difference_type;

	iterator begin() const { return iterator( ptr ); }
	iterator end() const { return iterator( ptr + sz ); }
	size_type max_size() const { return sz; }
	size_type capacity() const { return sz; }

	constexpr StringView() = default;
	~StringView() = default;
	constexpr StringView( const StringView& other ) = default;
	constexpr StringView( const char* str, size_type len ) : ptr( str ), sz( len ) {}

	template < size_t N >
	constexpr StringView( const char (&str)[N] )
	: ptr( str ), sz( safe_truncate< size_type >( N ) )
	{
	}

	StringView( const char* str ) : ptr( str ), sz( 0 )
	{
		if( str ) {
			sz = safe_truncate< size_type >( strlen( str ) );
		}
	}

	constexpr StringView( const string& str ) : ptr( str.ptr ), sz( str.sz ) {}

	// substr of a StringView [pos, end)
	StringView( StringView other, size_type pos );
	// substr of a StringView [pos, len)
	StringView( StringView other, size_type pos, size_type len );


	const char* data() const { return ptr; }
	size_type size() const { return sz; }
	size_type length() const { return sz; }
	bool empty() const { return sz == 0; }
	void clear() { ptr = nullptr; sz = 0; }

	void set( const char* str )
	{
		ptr = str;
		if( str ) {
			sz = safe_truncate< size_type >( strlen( str ) );
		} else {
			sz = 0;
		}
	}

	void set( const char* str, size_type len )
	{
		assert( len >= 0 );
		ptr = str;
		sz = len;
	}

	template < size_t N >
	void set( const char (&str)[N] )
	{
		ptr = str;
		sz = safe_truncate< size_type >( N );
	}

	char operator[]( intmax i ) const
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < (intmax)sz );
		return ptr[i];
	}

	char at( intmax i ) const
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < (intmax)sz );
		return ptr[i];
	}

	char back() const
	{
		assert( ptr );
		assert( sz );
		return ptr[unsignedof( sz ) - 1];
	}

	char front() const
	{
		assert( ptr );
		assert( sz );
		return ptr[0];
	}

	void pop_front( size_type n = 1 )
	{
		assert( sz >= n );
		ptr += n;
		sz -= n;
	}

	void pop_back( size_type n = 1 )
	{
		assert( sz >= n );
		sz -= n;
	}
};

int32 compare( StringView a, StringView b )
{
	auto minSize = MIN( a.sz, b.sz );
	auto cmp = memcmp( a.ptr, b.ptr, minSize );
	if( cmp < 0 ) return -1;
	if( cmp > 0 ) return 1;
	if( a.sz < b.sz ) return -1;
	if( a.sz > b.sz ) return 1;
	return 0;
}

StringView substr( StringView str, int32 pos, int32 len = StringView::npos );

inline constexpr bool isPrefix( StringView str, StringView prefix )
{
	return ( str.sz >= prefix.sz ) && ( memcmp( str.ptr, prefix.ptr, prefix.sz ) == 0 );
}
inline constexpr bool isPostfix( StringView str, StringView postfix )
{
	return ( ( str.sz >= postfix.sz )
			 && ( memcmp( str.ptr + ( str.sz - postfix.sz ), postfix.ptr, postfix.sz ) == 0 ) );
}

/*inline bool contains( StringView str, StringView other )
{
	return find( str, other ) != StringView::npos;
}*/

inline bool operator==( StringView a, StringView b )
{
	auto sz = a.sz;
	if( sz != b.sz ) return false;

	return a.ptr == b.ptr || sz <= 0 || memcmp( a.ptr, b.ptr, sz ) == 0;
}
inline bool operator!=( StringView a, StringView b ) { return !( a == b ); }

StringView trim( StringView str, StringView whitespace = utility::whitespace );
StringView trimLeft( StringView str, StringView whitespace = utility::whitespace );
StringView trimRight( StringView str, StringView whitespace = utility::whitespace );
StringView getWord( StringView str, StringView delimeter = utility::whitespace );
StringView getLine( StringView str, int32 pos = 0 );
// trims away any non numeric character starting from left until a numeric char is encountered
StringView trimLeftNonNumeric( StringView str );
int32 copyTrimmedString( StringView str, char* buffer, int32 len );
StringView nextToken( StringView* string, StringView delimeters );

int32 copyToString( StringView str, char* dest, int32 destSize );
// copies str into buffer with escaped sequences like \n replaced with newline etc
int32 copyUnescapedString( StringView str, char* buffer, int32 size );

// impl
StringView::StringView( StringView other, size_type pos )
{
	pos = clamp( pos, 0, other.sz );
	ptr = other.ptr + pos;
	sz = other.sz - pos;
}

StringView::StringView( StringView other, size_type pos, size_type len )
{
	pos = clamp( pos, 0, other.sz );
	sz = clamp( len, 0, other.sz - pos );
	ptr = other.ptr + pos;
}
StringView substr( StringView str, int32 pos, int32 len )
{
	pos = clamp( pos, 0, str.sz );
	return StringView( str.ptr + pos, clamp( len, 0, str.sz - pos ) );
}
int32 copyToString( StringView str, char* dest, int32 destSize )
{
	assert( dest || destSize == 0 );
	auto minSize = MIN( destSize, str.sz );
	if( dest && str.ptr && minSize ) {
		memcpy( dest, str.ptr, minSize );
	}
	return minSize;
}

struct StringViewPos {
	int32 pos;
	inline operator int32 const() { return pos; }
};
StringViewPos operator+( StringViewPos pos, int32 offset )
{
	return {( pos.pos != StringView::npos ) ? ( pos.pos + offset ) : ( StringView::npos )};
}
StringViewPos operator-( StringViewPos pos, int32 offset )
{
	return {( pos.pos != StringView::npos ) ? ( pos.pos - offset ) : ( StringView::npos )};
}
StringViewPos find( StringView str, StringView findStr, int32 start = 0 )
{
	if( !findStr.size() ) {
		return {};
	}

	// check whether there is room for a match
	if( start < str.size() && findStr.size() <= str.size() - start ) {
		const char* current = str.data() + start;
		const char* prev    = current;
		size_t findStrCount = (size_t)findStr.size();
		// we don't need to check the last findStr.size() characters, since there isn't enough room
		// for a match
		size_t len = ( size_t )( str.size() - start - findStr.size() + 1 );
		// start looking for findStr at occurences of its first character
		while( ( current = (const char*)memchr( prev, (unsigned char)findStr[0], len ) )
		       != nullptr ) {
			if( memcmp( current, findStr.data(), findStrCount ) == 0 ) {
				return {( int32 )( current - str.data() )};
			}
			len -= ( int32 )( current - prev + 1 );
			prev = current + 1;
		}
	}

	return {StringView::npos};
}
StringViewPos find( StringView str, char c, int32 start = 0 )
{
	return find( str, {&c, 1}, start );
}

StringViewPos findLast( StringView str, StringView findStr, int32 start = StringView::npos )
{
	if( !findStr.size() ) {
		return {};
	}

	// check whether there is room for a match
	if( start > 0 && str.size() >= findStr.size() ) {
		size_t strSize = (size_t)str.size();
		if( (size_t)start < strSize ) {
			strSize = (size_t)start;
		}
		const char* current;
		size_t findStrCount = (size_t)findStr.size();
		// we don't need to check the last findStr.size() characters, since there isn't enough room
		// for a match
		size_t len       = ( size_t )( strSize - findStr.size() + 1 );
		const char* prev = str.data() + len;
		// start looking for findStr at occurences of its first character
		while( ( current = (const char*)memrchr( str.data(), (unsigned char)findStr[0], len ) )
		       != nullptr ) {
			if( memcmp( current, findStr.data(), findStrCount ) == 0 ) {
				return {( int32 )( current - str.data() )};
			}
			len -= ( int32 )( prev - current );
			prev = current;
		}
	}

	return {StringView::npos};
}

/*StringView trim( StringView str, StringView whitespace )
{
	return trimLeft( trimRight( str, whitespace ), whitespace );
}

StringView trimLeft( StringView str, StringView whitespace )
{
	using std::min;
	auto pos = min( str.size(), str.find_first_not_of( whitespace ) );
	return str.substr( pos );
}

StringView trimRight( StringView str, StringView whitespace )
{
	using std::min;
	auto pos = str.find_last_not_of( whitespace );
	if( pos != StringView::npos ) {
		++pos;
	} else {
		pos = str.size();
	}
	str.pop_back( str.size() - pos );
	return str;
}

StringView getWord( StringView str, StringView delimeter )
{
	auto pos = str.find_first_of( delimeter );
	return str.substr( 0, pos );
}

StringView getLine( StringView str, int32 pos )
{
	auto newline = str.find( '\n', pos );
	return str.substr( pos, (newline != StringView::npos) ? ( newline + 1 ) : ( StringView::npos ) );
}

StringView trimLeftNonNumeric( StringView str )
{
	const StringView numericSign = "0123456789-+";

	int32 pos;
	auto len = str.size();
	for( ;; ) {
		pos = str.find_first_of( numericSign );
		if( pos != StringView::npos )		 {
			if( str[pos] == '-' || str[pos] == '+' ) {
				if( pos + 1 >= len ) {
					++pos;
					break;
				}
				auto c = char_to_uint( str[pos + 1] );
				if( c < '0' || c > '9' ) {
					str = str.substr( pos + 1 );
					continue;
				} else {
					break;
				}
			} else {
				break;
			}
		} else {
			break;
		}
	}
	return str.substr( pos );
}

int32 copyTrimmedString( StringView str, char* buffer, int32 len )
{
	assert( buffer && len > 0 );
	static const StringView some_space = " \t\v";
	static const StringView new_lines = "\n\r";
	auto cur = buffer;
	str = trim( str, some_space );
	while( str.size() ) {
		auto word = getWord( str, some_space );
		if( word.end() != str.end() ) {
			// include one whitespace if last char word is not newline
			if( !word.size() || new_lines.find( word.back() ) == StringView::npos ) {
				word = str.substr( 0, word.size() + 1 );
			}
		}
		len -= word.copy( cur, len );
		if( len <= 0 ) break;
		cur += word.size();
		str.pop_front( word.size() );
		str = trimLeft( str, some_space );
	}
	return static_cast< int32 >( cur - buffer );
}

StringView nextToken( StringView* string, StringView delimeters )
{
	assert( string );
	StringView ret;
	auto pos = string->find_first_of( delimeters );
	if( pos != StringView::npos ) {
		ret = string->substr( 0, pos );
		string->pop_front( pos );
	} else {
		ret = *string;
		string->pop_front( string->size() );
	}
	return ret;
}

int32 copyUnescapedString( StringView str, char* buffer, int32 size )
{
	// TODO: implement using find or memchr instead of going byte by byte
	if( str.empty() || size <= 0 ) {
		return 0;
	}
	assert( buffer );
	size = MIN( size, str.size() );
	auto p = str.data();
	auto end = p + size;
	auto len = 0;
	while( p != end ) {
		if( *p == '\\' ) {
			++p;
			if( p == end ) {
				break;
			}
			switch( *p ) {
				case '"': {
					*buffer = '"';
					break;
				}
				case '\\': {
					*buffer = '\\';
					break;
				}
				case '/': {
					*buffer = '/';
					break;
				}
				case 'b': {
					*buffer = '\b';
					break;
				}
				case 'f': {
					*buffer = '\f';
					break;
				}
				case 'n': {
					*buffer = '\n';
					break;
				}
				case 'r': {
					*buffer = '\r';
					break;
				}
				case 't': {
					*buffer = '\t';
					break;
				}
				case 'u': {
					// TODO: use utf16 to convert to utf8
					assert( 0 && "Not implemented" );
					p += 4;
					break;
				}
				default: {
					*buffer = *p;
					break;
				}
			}
		} else {
			*buffer = *p;
		}
		++p;
		++buffer;
		++len;
	}
	return len;
}*/