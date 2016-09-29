#pragma once

#ifndef _WARNINGS_H_INCLUDED_
#define _WARNINGS_H_INCLUDED_

// MSVC warnings

// warning C4100: 'x': unreferenced formal parameter
#pragma warning( disable: 4100 )

// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning( disable: 4201 )

// warning C4456: declaration of 'x' hides previous local declaration
#pragma warning( disable: 4456 )

// warning C4458: declaration of 'x' hides class member
#pragma warning( disable: 4458 )

///////////////////////////////////
// clang warnings

#if defined( __clang__ )

// warning: anonymous structs are a GNU extension [-Wgnu-anonymous-struct]
#pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"

// warning: anonymous types declared in an anonymous union are an extension [-Wnested-anon-types]
#pragma GCC diagnostic ignored "-Wnested-anon-types"

// warning: unused function 'stbi__sse2_available' [-Wunused-function]
#pragma GCC diagnostic ignored "-Wunused-function"

// warning: token pasting of ',' and __VA_ARGS__ is a GNU extension [-Wgnu-zero-variadic-macro-arguments]
// #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#endif // defined( __clang__ )

#endif // _WARNINGS_H_INCLUDED_
