#pragma once

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
	const char* ErrorLevelStrings[] = {
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
void LOG( int32 level, const char* format, Types... args )
{
	assert( GlobalIngameLog );
	if( GlobalIngameLog->count < countof( GlobalIngameLog->entries ) ) {
		auto entry = &GlobalIngameLog->entries[GlobalIngameLog->count];
		++GlobalIngameLog->count;
		entry->alive = 10000;

		entry->messageLength = 0;
		auto remaining       = countof( entry->message );
		entry->messageLength = snprint( entry->message + entry->messageLength, remaining, "{}: ",
		                                getErrorLevelString( level ) );
		remaining -= entry->messageLength;
		entry->messageLength +=
		    snprint( entry->message + entry->messageLength, remaining, format, args... );
	}
	// TODO: also log to a file
}

#endif  // _LOG_H_INCLUDED_
