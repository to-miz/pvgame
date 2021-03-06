#pragma once

#ifndef _WIN32MALLOC_H_INCLUDED_
#define _WIN32MALLOC_H_INCLUDED_

#ifndef WIN32
	#define WIN32
#endif
#define MSPACES 1
#define ONLY_MSPACES 1
#define USE_LOCKS 0
#define REALLOC_ZERO_BYTES_FREES
#define NO_MALLINFO 0

#if DLMALLOC_DEBUG_CHECKS
	#define FOOTERS 1
#endif
#define INSECURE 0

#define USE_DL_PREFIX

#include <dlmalloc.h>

#endif // _WIN32MALLOC_H_INCLUDED_
