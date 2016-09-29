#pragma once

#ifndef _WSTRING_H_INCLUDED_
#define _WSTRING_H_INCLUDED_

struct WString {
	WString() = default;
	WString( int32 length );
	WString( WString&& other );
	WString( const WString& other );
	WString& operator=( const WString& other );
	~WString();

	static WString fromUtf8( StringView str );

	wchar_t* data = nullptr;
	int32 length = 0;

	inline operator wchar_t*() const { return data; }
};

#endif // _WSTRING_H_INCLUDED_
