/*
 * fnv - Fowler/Noll/Vo- hash code
 *
 * Please do not copyright this code.  This code is in the public domain.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * By:
 *	chongo <Landon Curt Noll> /\oo/\
 *      http://www.isthe.com/chongo/

 Modified by Tolga Mizrak 2016
 */

#define FNV_32_PRIME 0x01000193u
#define FNV_32_INIT 0x811C9DC5u
#define FNV_64_PRIME 0X00000100000001B3ull
#define FNV_64_INIT 0XCBF29CE484222325ull

uint32 fnv1a32( const uint8* first, const uint8* last, uint32 seed = FNV_32_INIT )
{
	uint32 hval = seed;

	for( ; first < last; ++first ) {
		hval ^= (uint32)*first;
#if defined( NO_FNV_GCC_OPTIMIZATION )
		hval *= FNV_32_PRIME;
#else
		hval += ( hval << 1 ) + ( hval << 4 ) + ( hval << 7 ) + ( hval << 8 ) + ( hval << 24 );
#endif
	}

	return hval;
}
// nullterminated string version
uint32 fnv1a32( const char* s, uint32 seed = FNV_32_INIT )
{
	uint32 hval = seed;

	assert( s );
	for( const uint8* p = (const uint8*)s; *p; ++p ) {
		hval ^= (uint32)*p;
#if defined( NO_FNV_GCC_OPTIMIZATION )
		hval *= FNV_32_PRIME;
#else
		hval += ( hval << 1 ) + ( hval << 4 ) + ( hval << 7 ) + ( hval << 8 ) + ( hval << 24 );
#endif
	}

	return hval;
}
uint32 fnv1a32( const void* first, const void* last, uint32 seed = FNV_32_INIT )
{
	return fnv1a32( (const uint8*)first, (const uint8*)last, seed );
}
uint32 fnv1a32( const void* buffer, size_t size, uint32 seed = FNV_32_INIT )
{
	return fnv1a32( (const uint8*)buffer, (const uint8*)buffer + size, seed );
}
uint32 fnv1a32( const void* buffer, int32 size, uint32 seed = FNV_32_INIT )
{
	return fnv1a32( (const uint8*)buffer, (const uint8*)buffer + size, seed );
}

uint64 fnv1a64( const uint8* first, const uint8* last, uint64 seed = FNV_64_INIT )
{
	uint64 hval = seed;

	for( ; first < last; ++first ) {
		hval ^= (uint64)*first;

#if defined( NO_FNV_GCC_OPTIMIZATION )
		hval *= FNV_64_PRIME;
#else
		hval += ( hval << 1 ) + ( hval << 4 ) + ( hval << 5 ) + ( hval << 7 ) + ( hval << 8 )
		        + ( hval << 40 );
#endif
	}

	return hval;
}
// nullterminated string version
uint64 fnv1a64( const char* s, uint64 seed = FNV_64_INIT )
{
	assert( s );
	uint64 hval = seed;

	for( const uint8* p = (const uint8*)s; *p; ++p ) {
		hval ^= (uint64)*p;

#if defined( NO_FNV_GCC_OPTIMIZATION )
		hval *= FNV_64_PRIME;
#else
		hval += ( hval << 1 ) + ( hval << 4 ) + ( hval << 5 ) + ( hval << 7 ) + ( hval << 8 )
		        + ( hval << 40 );
#endif
	}

	return hval;
}
uint64 fnv1a64( const void* first, const void* last, uint64 seed = FNV_64_INIT )
{
	return fnv1a64( (const uint8*)first, (const uint8*)last, seed );
}
uint64 fnv1a64( const void* buffer, size_t size, uint64 seed = FNV_64_INIT )
{
	return fnv1a64( (const uint8*)buffer, (const uint8*)buffer + size, seed );
}
uint64 fnv1a64( const void* buffer, int32 size, uint64 seed = FNV_64_INIT )
{
	return fnv1a64( (const uint8*)buffer, (const uint8*)buffer + size, seed );
}