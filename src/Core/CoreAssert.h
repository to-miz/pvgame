#pragma once

#ifndef _COREASSERT_H_INCLUDED_
#define _COREASSERT_H_INCLUDED_
#if 0

#include <windows.h>
#undef min
#undef max
#undef ERROR
#undef near
#undef far

// TODO: other platforms
namespace assert_impl
{
bool showMessageBox( const char* text, const char* file, size_t line )
{
	struct Ignore {
		const char* file;
		size_t line;
	};
	const int MaxIgnore                 = 16;
	static Ignore ingoreList[MaxIgnore] = {};
	static int ignoreListSize           = 0;

	for( auto i = 0, count = ignoreListSize; i < count; ++i ) {
		auto entry = ingoreList[i];
		if( line == entry.line && ( entry.file == file || strcmp( entry.file, file ) == 0 ) ) {
			return false;
		}
	}

	auto result = MessageBoxA(
	    nullptr, text, "Assertion Failed",
	    MB_CANCELTRYCONTINUE | MB_ICONERROR | MB_DEFBUTTON1 | MB_TASKMODAL | MB_SETFOREGROUND );
	switch( result ) {
		case IDCANCEL: {
			return true;
		}
		case IDTRYAGAIN: {
			return false;
		}
		case IDCONTINUE: {
			if( ignoreListSize < MaxIgnore ) {
				ingoreList[ignoreListSize++] = {file, line};
			}
			return false;
		}
		default: {
			return false;
		}
	}
}

bool assert_( const char* text, const char* file, size_t line )
{
	if( showMessageBox( text, file, line ) ) {
		return false;
	}
	return true;
}
};

#define assert( x ) \
	(void)( ( x ) || assert_impl::assert_( #x, __FILE__, __LINE__ ) || ( __debugbreak(), 0 ) )
#endif

#endif  // _COREASSERT_H_INCLUDED_
