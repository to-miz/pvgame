#ifndef _LOG_H_INCLUDED_
#define _LOG_H_INCLUDED_

struct IngameLogEntry {
	char message[100];
	int32 messageLength;
	float alive;
};

struct IngameLog {
	IngameLogEntry entries[100];
	int32 count;
};

extern global_var IngameLog* GlobalIngameLog;

enum ErrorLevelValues {
	ERROR,
	WARNING,
	INFORMATION
};

const char* getErrorLevelString( int32 error )
{
	static const char* const ErrorLevelStrings[] = {
		"Error",
		"Warning",
		"Information"
	};
	if( error >= ERROR && error <= INFORMATION ) {
		return ErrorLevelStrings[error];
	}
	return "Unknown";
}

template < class... Types >
void LOG_IMPL( int32 level, const char* format, const char* file, size_t line,
               const Types&... args );

#define LOG( level, format, ... ) \
	LOG_IMPL( ( level ), ( format ), __FILE__, __LINE__, __VA_ARGS__ )

#endif  // _LOG_H_INCLUDED_

#ifdef _LOG_IMPLEMENTATION_
	template < class... Types >
	void LOG_IMPL( int32 level, const char* format, const char* file, size_t line,
	               const Types&... args )
	{
		char timestamp[50];
		auto timestampLength = getTimeStampString( timestamp, countof( timestamp ) );

		assert( GlobalIngameLog );
		if( GlobalIngameLog->count < countof( GlobalIngameLog->entries ) ) {
			auto entry = &GlobalIngameLog->entries[GlobalIngameLog->count];
			++GlobalIngameLog->count;
			entry->alive = 10000;

		    auto builder = string_builder{entry->message, countof( entry->message )};
		    builder.print( "{} ", StringView{timestamp, timestampLength} );
		    if( level == ERROR ) {
			    builder.print( "{}:{}: ", file, line );
		    }
		    builder.print( "{}: ", getErrorLevelString( level ) );
		    builder.print( format, args... );
		    entry->messageLength = builder.size();

			if( entry->messageLength < countof( entry->message ) ) {
				// make null terminated
				entry->message[entry->messageLength] = '\n';
				entry->message[entry->messageLength + 1] = 0;
#ifdef GAME_DLL
				if( GlobalPlatformServices ) {
					GlobalPlatformServices->outputDebugString( entry->message );
				}
#else
				OutputDebugStringA( entry->message );
#endif
			}
		}
		if( GlobalDebugLogger ) {
		    debugLog( "{} ", StringView{timestamp, timestampLength} );
		    if( level == ERROR ) {
			    debugLog( "{}:{}: ", file, line );
		    }
		    debugLog( "{}: ", getErrorLevelString( level ) );
		    debugLogln( format, args... );
		}
		// TODO: also log to a file
	}
#endif // _LOG_IMPLEMENTATION_
