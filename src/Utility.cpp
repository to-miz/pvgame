inline bool isValid( float x ) { return isfinite( x ); }

template < class T >
void* toPtr( T v )
{
	return (void*)( (uintptr)unsignedof( v ) );
}
template < class T >
T fromPtr( void* p )
{
	return ( T )( (uintptr)p );
}