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
void LOG( int32 level, const char* format, const Types&... args );

#endif  // _LOG_H_INCLUDED_

#ifdef _LOG_IMPLEMENTATION_
	template < class... Types >
	void LOG( int32 level, const char* format, const Types&... args )
	{
		char timestamp[50];
		auto timestampLength = getTimeStampString( timestamp, countof( timestamp ) );

		assert( GlobalIngameLog );
		if( GlobalIngameLog->count < countof( GlobalIngameLog->entries ) ) {
			auto entry = &GlobalIngameLog->entries[GlobalIngameLog->count];
			++GlobalIngameLog->count;
			entry->alive = 10000;

			entry->messageLength = 0;
			auto remaining       = countof( entry->message );
		    entry->messageLength =
		        snprint( entry->message + entry->messageLength, remaining, "{} {}: ",
		                 StringView{timestamp, timestampLength}, getErrorLevelString( level ) );
		    remaining -= entry->messageLength;
			entry->messageLength +=
			    snprint( entry->message + entry->messageLength, remaining, format, args... );
		}
		if( GlobalDebugLogger ) {
		    debugLog( "{} {}: ", StringView{timestamp, timestampLength},
		              getErrorLevelString( level ) );
		    debugLogln( format, args... );
		}
		// TODO: also log to a file
	}
#endif // _LOG_IMPLEMENTATION_
