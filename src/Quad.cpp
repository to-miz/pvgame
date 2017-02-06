template< class T >
struct tquad {
	typedef typename T::value_type value_type;
	T elements[4];
};

typedef tquad< vec3 > Quad;
typedef tquad< vec3 > Quad3;
typedef tquad< vec2 > Quad2;

template < class T >
tquad< T > makeQuad_( trectarg< typename T::value_type > rect )
{
	tquad< T > result    = {};
	result.elements[0].x = rect.left;
	result.elements[0].y = rect.top;
	result.elements[1].x = rect.right;
	result.elements[1].y = rect.top;
	result.elements[2].x = rect.left;
	result.elements[2].y = rect.bottom;
	result.elements[3].x = rect.right;
	result.elements[3].y = rect.bottom;
	return result;
}
template < class T >
tquad< T > makeQuad_( trectarg< typename T::value_type > rect, typename T::value_type z )
{
	tquad< T > result    = {};
	result.elements[0].x = rect.left;
	result.elements[0].y = rect.top;
	result.elements[0].z = z;
	result.elements[1].x = rect.right;
	result.elements[1].y = rect.top;
	result.elements[1].z = z;
	result.elements[2].x = rect.left;
	result.elements[2].y = rect.bottom;
	result.elements[2].z = z;
	result.elements[3].x = rect.right;
	result.elements[3].y = rect.bottom;
	result.elements[3].z = z;
	return result;
}

Quad makeQuad( rectfarg rect ) { return makeQuad_< vec3 >( rect ); }
Quad makeQuad( rectfarg rect, float z ) { return makeQuad_< vec3 >( rect, z ); }

template < class T >
tquad< T > transform( mat4arg mat, const tquad< T >& q )
{
	tquad< T > result;
	result.elements[0] = transformVector( mat, q.elements[0] );
	result.elements[1] = transformVector( mat, q.elements[1] );
	result.elements[2] = transformVector( mat, q.elements[2] );
	result.elements[3] = transformVector( mat, q.elements[3] );
	return result;
}