struct string_logger : string_builder {
	int32 lines    = 0;
	int32 maxLines = 100;

	string_logger() = default;
	string_logger( char* buffer, int32 len, int32 maxLines )
	: string_builder( buffer, len ), maxLines( maxLines )
	{
	}

	void clear()
	{
		string_builder::clear();
		lines = 0;
	}

	void removeLinesFromTop( int32 count )
	{
		assert( count <= lines );
		lines -= count;
		auto end       = ptr;
		auto remaining = sz;
		for( auto i = 0; i < count; ++i ) {
			auto line = getLine( {end, remaining} );
			end += line.size();
			remaining -= line.size();
		}
		assert( remaining >= 0 );
		memmove( ptr, end, remaining );
		sz = remaining;
	}
	void checkLines( StringView text )
	{
		int32 lineCount = safe_truncate< int32 >( count( text.begin(), text.end(), '\n' ) );
		lines += lineCount;
		if( lines > maxLines ) {
			removeLinesFromTop( lines - maxLines );
		}
	}

	template < class... Types >
	string_builder& log( const char* format, const Types&... args )
	{
		auto start = ptr + sz;
		auto len   = snprint( start, cap - sz, format, args... );
		sz += len;
		checkLines( {start, len} );
		return *this;
	}
	template < class... Types >
	string_builder& logln( const char* format, const Types&... args )
	{
		// a little bit hacky, but this is only used for debug logging
		if( lines + 1 > maxLines ) {
			removeLinesFromTop( 1 );
		}
		if( cap > 200 ) {
			while( ( cap - sz ) < 200 ) {
				removeLinesFromTop( 1 );
			}
		}
		auto start = ptr + sz;
		auto len   = snprint( start, cap - sz, format, args... );
		sz += len;
		if( cap - sz ) {
			*( ptr + sz ) = '\n';
			++sz;
			++len;
		} else {
			*( ptr + sz - 1 ) = '\n';
		}
		checkLines( {start, len} );
		return *this;
	}
};