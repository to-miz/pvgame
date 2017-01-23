#include "win32malloc.h"

// warning C4127: conditional expression is constant
// warning C4702: unreachable code
// warning C4505: 'has_segment_link': unreferenced local function has been removed
#pragma warning( disable: 4127 4702 4505 )

// disable mmap and MORECORE, so that malloc cannot get more system memory
#define HAVE_MMAP 0
#define HAVE_MORECORE 0

#include <dlmalloc.c>