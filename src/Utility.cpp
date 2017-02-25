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

#ifdef GAME_DEBUG

// used to get a value from an error message, use like this:
//	error_with_value< sizeof(int) > x;
// compiler will error with something like: 'x' uses undefined class 'error_with_value<4>'
template< int32 N >
class error_with_value;

#endif