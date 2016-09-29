#pragma once

#ifndef _RECT_H_INCLUDED_
#define _RECT_H_INCLUDED_

template< class T >
union trect {
	struct {
		T left;
		T top;
		T right;
		T bottom;
	};
	struct {
		T x;
		T y;
		T unused0_[2];
	};
	struct {
		tvec2< T > xy;
		T unused1_[2];
	};
	struct {
		tvec2< T > leftTop;
		tvec2< T > rightBottom;
	};
	T elements[4];
};

typedef trect< float > rectf;
typedef trect< int32 > recti;

template< class T >
union tregion {
	struct {
		T left;
		T top;
		T width;
		T height;
	};
	struct {
		T x;
		T y;
		T unused0_[2];
	};
	struct {
		tvec2< T > xy;
		T unused1_[2];
	};
	struct {
		tvec2< T > leftTop;
		tvec2< T > dim;
	};
	T elements[4];
};

typedef tregion< float > regionf;
typedef tregion< int32 > regioni;

#if defined( RECT_ARGS_AS_CONST_REF ) && RECT_ARGS_AS_CONST_REF == 1
	template< class T >
	using trectarg = const trect< T >&;
	template< class T >
	using tregionarg = const tregion< T >&;
#else
	template< class T >
	using trectarg = trect< T >;
	template< class T >
	using tregionarg = tregion< T >;
#endif
typedef trectarg< float > rectfarg;
typedef trectarg< int32 > rectiarg;
typedef tregionarg< float > regionfarg;
typedef tregionarg< int32 > regioniarg;

template< class T > trect< T > Rect( T left, T top, T right, T bottom );
template< class T > trect< T > Rect( T left, T top, tvec2< T > rightBottom );
template< class T > trect< T > Rect( tvec2< T > leftTop, tvec2< T > rightBottom );
template< class T > trect< T > Rect( tvec2< T > leftTop, T right, T bottom );
template< class T > trect< T > Rect( tregionarg< T > region );
// converts region to rectf by considering possibility of negative dim
template< class T > trect< T > RectCorrected( tregionarg< T > region );
template< class T > trect< T > RectWH( T left, T top, T width, T height );
template< class T > trect< T > RectWH( tvec2< T > leftTop, T width, T height );
template< class T > trect< T > RectWH( tvec2< T > leftTop, tvec2< T > dim );
template< class T > trect< T > RectWH( T left, T top, tvec2< T > dim );
template< class T > trect< T > RectHalfSize( T centerX, T centerY, T halfWidth, T halfHeight );
template< class T > trect< T > RectHalfSize( tvec2< T > center, T halfWidth, T halfHeight );
template< class T > trect< T > RectHalfSize( tvec2< T > center, tvec2< T > halfSize );
template< class T > trect< T > RectHalfSize( T centerX, T centerY, tvec2< T > halfSize );
template< class T > trect< T > RectBounding( trectarg< T > a, trectarg< T > b );
template< class T > trect< T > RectBounding( tvec2< T >* vertices, intmax count );
template< class T > trect< T > RectClipped( trectarg< T > a, trectarg< T > b );
template< class T > trect< T > RectOverlap( trectarg< T > a, trectarg< T > b );
template< class T > trect< T > RectCentered( tvec2< T > center, T width, T height );
template< class T > trect< T > RectTranslation( T x, T y );
template< class T > trect< T > RectTranslation( tvec2< T > translation );
template< class T > trect< T > RectMin( trectarg< T > a, trectarg< T > b );
template< class T > trect< T > RectMax( trectarg< T > a, trectarg< T > b );
template< class T > trect< T > RectMirroredHorizontal( trectarg< T > a, T originX );
template< class T > trect< T > RectMirroredVertical( trectarg< T > a, T originY );
template< class T > trect< T > RectMirroredDiagonal( trectarg< T > a, T originX, T originY );
template< class T > trect< T > RectMirroredHorizontal( trectarg< T > a );
template< class T > trect< T > RectMirroredVertical( trectarg< T > a );
template< class T > trect< T > RectMirroredDiagonal( trectarg< T > a );
template< class T > trect< T > RectSetLeft( trectarg< T > a, T left );
template< class T > trect< T > RectSetTop( trectarg< T > a, T top );
template< class T > trect< T > RectSetRight( trectarg< T > a, T right );
template< class T > trect< T > RectSetBottom( trectarg< T > a, T bottom );
template< class T > trect< T > RectSetWidth( trectarg< T > a, T width );
template< class T > trect< T > RectSetHeight( trectarg< T > a, T height );
template< class T > T width( trectarg< T > r );
template< class T > T height( trectarg< T > r );
template< class T > tvec2< T > center( trectarg< T > r );
template< class T > trect< T > lerp( float t, trectarg< T > start, trectarg< T > end );
template< class T > tvec2< T > dimensions( trectarg< T > r );
template< class T > trect< T > translate( trectarg< T > r, T x, T y );
template< class T > trect< T > translate( trectarg< T > r, tvec2< T > translation );
template< class T > trect< T > grow( trectarg< T > r, T amount );
template< class T > trect< T > shrink( trectarg< T > r, T amount );
template< class T > bool isPointInside( trectarg< T > r, T x, T y );
template< class T > bool isPointInside( trectarg< T > r, tvec2< T > point );
template< class T > bool isOverlapping( trectarg< T > a, trectarg< T > b );
template< class T > bool isInside( trectarg< T > a, trectarg< T > b );
template< class T > bool isValid( trectarg< T > r );

template< class T > trect< T > ceil( trectarg< T > r );
template< class T > trect< T > floor( trectarg< T > r );
template< class T > trect< T > sweep( trectarg< T > r, tvec2< T > delta );

// conversions
template< class T, class U > trect< T > RectTiled( trectarg< U > rect, T tileWidth, T tileHeight );
template< class T, class U > trect< T > RectTiled( trectarg< U > rect, tvec2< T > tileSize );
template< class T, class U > trect< T > Rect( U left, U top, U right, U bottom );
template< class T, class U > trect< T > Rect( trectarg< U > other );

template< class T > tregion< T > Region( T left, T top, T width, T height );
template< class T > tregion< T > Region( tvec2< T > leftTop, T width, T height );
template< class T > tregion< T > Region( tvec2< T > leftTop, tvec2< T > dim );
template< class T > tregion< T > Region( T left, T top, tvec2< T > dim );
template< class T > tregion< T > Region( tregionarg< T > r );
template< class T > tregion< T > RegionRB( T left, T top, T right, T bottom );
template< class T > tregion< T > RegionRB( T left, T top, tvec2< T > rightBottom );
template< class T > tregion< T > RegionRB( tvec2< T > leftTop, tvec2< T > rightBottom );
template< class T > tregion< T > RegionRB( tvec2< T > leftTop, T right, T bottom );
template< class T > T width( tregionarg< T > r );
template< class T > T height( tregionarg< T > r );
template< class T > tvec2< T > center( tregionarg< T > r );
template< class T > tvec2< T > dimensions( tregionarg< T > r );
template< class T > tregion< T > translate( tregionarg< T > r, T x, T y );
template< class T > tregion< T > translate( tregionarg< T > r, tvec2< T > translation );
template< class T > tregion< T > grow( tregionarg< T > r, T amount );
template< class T > tregion< T > shrink( tregionarg< T > r, T amount );

recti RectTiledIndex( rectfarg rect, float tileWidth, float tileHeight )
{
	recti result;
	float invTileWidth = 1.0f / tileWidth;
	float invTileHeight = 1.0f / tileHeight;
	result.left = (int32)floor( rect.left * invTileWidth );
	result.right = (int32)ceil( rect.right * invTileWidth );
	result.top = (int32)floor( rect.top * invTileHeight );
	result.bottom = (int32)ceil( rect.bottom * invTileHeight );
	return result;
}

// template impl
template< class T > trect< T > Rect( T left, T top, T right, T bottom )
{
	return {left, top, right, bottom};
}
template< class T > trect< T > Rect( T left, T top, tvec2< T > rightBottom )
{
	return {left, top, rightBottom.x, rightBottom.y};
}
template< class T > trect< T > Rect( tvec2< T > leftTop, tvec2< T > rightBottom )
{
	return {leftTop.x, leftTop.y, rightBottom.x, rightBottom.y};
}
template< class T > trect< T > Rect( tvec2< T > leftTop, T right, T bottom )
{
	return {leftTop.x, leftTop.y, right, bottom};

}
template< class T > trect< T > Rect( tregionarg< T > region )
{
	return {region.left, region.top, region.left + region.width, region.top + region.height};
}
// converts region to rectf by considering possibility of negative dim
template< class T > trect< T > RectCorrected( tregionarg< T > region )
{
	trect< T > result;
	if( region.width < 0 ) {
		result.left = region.left + region.width;
		result.right = region.left;
	} else {
		result.left = region.left;
		result.right = region.left + region.width;
	}
	if( region.height < 0 ) {
		result.top = region.top + region.height;
		result.bottom = region.top;
	} else {
		result.top = region.top;
		result.bottom = region.top + region.height;
	}
	return result;
}
template< class T > trect< T > RectWH( T left, T top, T width, T height )
{
	return {left, top, left + width, top + height};
}
template< class T > trect< T > RectWH( tvec2< T > leftTop, T width, T height )
{
	return {leftTop.x, leftTop.y, leftTop.x + width, leftTop.y + height};
}
template< class T > trect< T > RectWH( tvec2< T > leftTop, tvec2< T > dim )
{
	return {leftTop.x, leftTop.y, leftTop.x + dim.x, leftTop.y + dim.y};
}
template< class T > trect< T > RectWH( T left, T top, tvec2< T > dim )
{
	return {left, top, left + dim.x, top + dim.y};
}
template< class T > trect< T > RectHalfSize( T centerX, T centerY, T halfWidth, T halfHeight )
{
	return {centerX - halfWidth, centerY - halfHeight, centerX + halfWidth, centerY + halfHeight};
}
template< class T > trect< T > RectHalfSize( tvec2< T > center, T halfWidth, T halfHeight )
{
	return {center.x - halfWidth, center.y - halfHeight, center.x + halfWidth,
			center.y + halfHeight};
}
template < class T >
trect< T > RectHalfSize( tvec2< T > center, tvec2< T > halfSize )
{
	return {center.x - halfSize.x, center.y - halfSize.y, center.x + halfSize.x, center.y + halfSize.y};
}
template < class T >
trect< T > RectHalfSize( T centerX, T centerY, tvec2< T > halfSize )
{
	return {centerX - halfSize.x, centerY - halfSize.y, centerX + halfSize.x, centerY + halfSize.y};
}
template< class T > trect< T > RectBounding( trectarg< T > a, trectarg< T > b )
{
	return RectMax( a, b );
}
template< class T > trect< T > RectBounding( tvec2< T >* vertices, intmax count )
{
	assert( vertices );
	trect< T > result;
	if( count ) {
		result = {vertices[0].x, vertices[0].y, vertices[0].x, vertices[0].y};
		for( intmax i = 1; i < count; ++i, ++vertices ) {
			auto v = *vertices;
			if( result.left > v.x ) result.left = v.x;
			if( result.top > v.y ) result.top = v.y;
			if( result.right < v.x ) result.right = v.x;
			if( result.bottom < v.y ) result.bottom = v.y;
		}
	} else {
		result = {};
	}
	return result;

}
template< class T > trect< T > RectClipped( trectarg< T > a, trectarg< T > b )
{
	return RectMin( a, b );
}
template< class T > trect< T > RectOverlap( trectarg< T > a, trectarg< T > b )
{
	return RectMin( a, b );
}
template< class T > trect< T > RectCentered( tvec2< T > center, T width, T height )
{
	auto h = floorCentered( center.x, width );
	auto v = floorCentered( center.y, height );
	return {h.x, v.x, h.y, v.y};
}
template< class T > trect< T > RectTranslation( T x, T y )
{
	return {x, y, x, y};
}
template< class T > trect< T > RectTranslation( tvec2< T > translation )
{
	return {translation.x, translation.y, translation.x, translation.y};
}
template< class T > trect< T > RectMin( trectarg< T > a, trectarg< T > b )
{
	trect< T > result;
	result.left = MAX( a.left, b.left );
	result.top = MAX( a.top, b.top );
	result.right = MIN( a.right, b.right );
	result.bottom = MIN( a.bottom, b.bottom );
	return result;
}
template< class T > trect< T > RectMax( trectarg< T > a, trectarg< T > b )
{
	trect< T > result;
	result.left = MIN( a.left, b.left );
	result.top = MIN( a.top, b.top );
	result.right = MAX( a.right, b.right );
	result.bottom = MAX( a.bottom, b.bottom );
	return result;
}
template< class T > trect< T > RectMirroredHorizontal( trectarg< T > a, T originX )
{
	return {2 * originX - a.right, a.top, 2 * originX - a.left, a.bottom};
}
template< class T > trect< T > RectMirroredVertical( trectarg< T > a, T originY )
{
	return {a.left, 2 * originY - a.bottom, a.right, 2 * originY - a.top};
}
template < class T > trect< T > RectMirroredDiagonal( trectarg< T > a, T originX, T originY )
{
	return {2 * originX - a.right, 2 * originY - a.bottom, 2 * originX - a.left,
			2 * originY - a.top};
}
template< class T > trect< T > RectMirroredHorizontal( trectarg< T > a )
{
	return {-a.right, a.top, -a.left, a.bottom};
}
template< class T > trect< T > RectMirroredVertical( trectarg< T > a )
{
	return {a.left, -a.bottom, a.right, -a.top};
}
template< class T > trect< T > RectMirroredDiagonal( trectarg< T > a )
{
	return {-a.right, -a.bottom, -a.left, -a.top};
}
template< class T > trect< T > RectSetLeft( trectarg< T > a, T left )
{
	return {left, a.top, a.right, a.bottom};
}
template< class T > trect< T > RectSetTop( trectarg< T > a, T top )
{
	return {a.left, top, a.right, a.bottom};
}
template< class T > trect< T > RectSetRight( trectarg< T > a, T right )
{
	return {a.left, a.top, right, a.bottom};
}
template< class T > trect< T > RectSetBottom( trectarg< T > a, T bottom )
{
	return {a.left, a.top, a.right, bottom};
}
template< class T > trect< T > RectSetWidth( trectarg< T > a, T width )
{
	return {a.left, a.top, a.left + width, a.bottom};
}
template< class T > trect< T > RectSetHeight( trectarg< T > a, T height )
{
	return {a.left, a.top, a.right, a.top + height};
}
template< class T > T width( trectarg< T > r )
{
	return r.right - r.left;
}
template< class T > T height( trectarg< T > r )
{
	return r.bottom - r.top;
}
template< class T > tvec2< T > center( trectarg< T > r )
{
	return {( r.left + r.right ) * 0.5f, ( r.top + r.bottom ) * 0.5f};
}
template< class T > trect< T > lerp( float t, trectarg< T > start, trectarg< T > end )
{
	return start + t * ( end - start );
}
template< class T > tvec2< T > dimensions( trectarg< T > r )
{
	return {r.right - r.left, r.bottom - r.top};
}
template< class T > trect< T > translate( trectarg< T > r, T x, T y )
{
	return {r.left + x, r.top + y, r.right + x, r.bottom + y};
}
template< class T > trect< T > translate( trectarg< T > r, tvec2< T > translation )
{
	return {r.left + translation.x, r.top + translation.y, r.right + translation.x,
			r.bottom + translation.y};
}
template< class T > trect< T > grow( trectarg< T > r, T amount )
{
	return {r.left - amount, r.top - amount, r.right + amount, r.bottom + amount};
}
template< class T > trect< T > shrink( trectarg< T > r, T amount )
{
	return {r.left + amount, r.top + amount, r.right - amount, r.bottom - amount};
}
template< class T > bool isPointInside( trectarg< T > r, T x, T y )
{
	return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}
template< class T > bool isPointInside( trectarg< T > r, tvec2< T > point )
{
	return point.x >= r.left && point.x < r.right && point.y >= r.top && point.y < r.bottom;
}
template< class T > bool isOverlapping( trectarg< T > a, trectarg< T > b )
{
	return ( ( a.left < b.right ) && ( a.right > b.left ) && ( a.bottom > b.top )
			 && ( a.top < b.bottom ) );
}
template< class T > bool isInside( trectarg< T > a, trectarg< T > b )
{
	return ( ( a.right <= b.right ) && ( a.left >= b.left ) && ( a.top >= b.top )
			 && ( a.bottom <= b.bottom ) );
}
template< class T > bool isValid( trectarg< T > r )
{
	assert( isValid( r.left ) );
	assert( isValid( r.top ) );
	assert( isValid( r.right ) );
	assert( isValid( r.bottom ) );
	return ( r.right >= r.left ) && ( r.bottom >= r.top );
}

template< class T > trect< T > ceil( trectarg< T > r )
{
	return {ceil( r.left ), ceil( r.top ), ceil( r.right ), ceil( r.bottom )};
}
template< class T > trect< T > floor( trectarg< T > r )
{
	return {floor( r.left ), floor( r.top ), floor( r.right ), floor( r.bottom )};
}
template< class T > trect< T > sweep( trectarg< T > r, tvec2< T > delta )
{
	auto result = r;
	if( delta.x < 0 ) {
		result.left += delta.x;
	} else {
		result.right += delta.x;
	}
	if( delta.y < 0 ) {
		result.top += delta.y;
	} else {
		result.bottom += delta.y;
	}
	return result;
}

// conversions
template< class T, class U > trect< T > RectTiled( trectarg< U > rect, T tileWidth, T tileHeight )
{
	return {rect.left * tileWidth, rect.top * tileHeight, rect.right * tileWidth,
			rect.bottom * tileHeight};
}
template< class T, class U > trect< T > RectTiled( trectarg< U > rect, tvec2< T > tileSize )
{
	return {rect.left * tileSize.x, rect.top * tileSize.y, rect.right * tileSize.x,
			rect.bottom * tileSize.y};
}
template< class T, class U > trect< T > Rect( U left, U top, U right, U bottom )
{
	return {(T)left, (T)top, (T)right, (T)bottom};
}
template< class T, class U > trect< T > Rect( trectarg< U > other )
{
	return {(T)other.left, (T)other.top, (T)other.right, (T)other.bottom};
}

// region
template< class T > tregion< T > Region( T left, T top, T width, T height )
{
	return {left, top, width, height};
}
template< class T > tregion< T > Region( tvec2< T > leftTop, T width, T height )
{
	return {leftTop.x, leftTop.y, width, height};
}
template< class T > tregion< T > Region( tvec2< T > leftTop, tvec2< T > dim )
{
	return {leftTop.x, leftTop.y, dim.x, dim.y};
}
template< class T > tregion< T > Region( T left, T top, tvec2< T > dim )
{
	return {left, top, dim.x, dim.y};
}
template< class T > tregion< T > Region( trectarg< T > r )
{
	return {r.left, r.top, r.right - r.left, r.bottom - r.top};
}
template< class T > tregion< T > RegionRB( T left, T top, T right, T bottom )
{
	return {left, top, right - left, bottom - top};
}
template< class T > tregion< T > RegionRB( T left, T top, tvec2< T > rightBottom )
{
	return {left, top, rightBottom.x - left, rightBottom.y - top};
}
template< class T > tregion< T > RegionRB( tvec2< T > leftTop, tvec2< T > rightBottom )
{
	return {leftTop.x, leftTop.y, rightBottom.x - leftTop.x, rightBottom.y - leftTop.y};
}
template< class T > tregion< T > RegionRB( tvec2< T > leftTop, T right, T bottom )
{
	return {leftTop.x, leftTop.y, right - leftTop.x, bottom - leftTop.y};
}
template< class T > T width( tregionarg< T > r )
{
	return r.width;
}
template< class T > T height( tregionarg< T > r )
{
	return r.height;
}
template< class T > tvec2< T > center( tregionarg< T > r )
{
	return {r.left + r.width * 0.5f, r.top + r.height * 0.5f};
}
template< class T > tvec2< T > dimensions( tregionarg< T > r )
{
	return {r.width, r.height};
}
template< class T > tregion< T > translate( tregionarg< T > r, T x, T y )
{
	return {r.left + x, r.top + y, r.width, r.top};
}
template< class T > tregion< T > translate( tregionarg< T > r, tvec2< T > translation )
{
	return {r.left + translation.x, r.top + translation.y, r.width, r.height};
}
template< class T > tregion< T > grow( tregionarg< T > r, T amount )
{
	return {r.left - amount, r.top - amount, r.width + amount, r.height + amount};
}
template< class T > tregion< T > shrink( tregionarg< T > r, T amount )
{
	return {r.left + amount, r.top + amount, r.width - amount, r.height - amount};
}

// operators

template< class T > trect< T > operator+( trectarg< T > lhs, trectarg< T > rhs )
{
	return {lhs.left + rhs.left, lhs.top + rhs.top, lhs.right + rhs.right, lhs.bottom + rhs.bottom};
}
template< class T > trect< T > operator-( trectarg< T > lhs, trectarg< T > rhs )
{
	return {lhs.left - rhs.left, lhs.top - rhs.top, lhs.right - rhs.right, lhs.bottom - rhs.bottom};
}
template< class T > trect< T > operator*( trectarg< T > r, T s )
{
	return {r.left * s, r.top * s, r.right * s, r.bottom * s};
}
template< class T > trect< T > operator*( T s, trectarg< T > r )
{
	return {r.left * s, r.top * s, r.right * s, r.bottom * s};
}
template< class T > trect< T > operator/( trectarg< T > r, T s )
{
	return {r.left / s, r.top / s, r.right / s, r.bottom / s};
}
template< class T > trect< T > operator/( T s, trectarg< T > r )
{
	return {r.left / s, r.top / s, r.right / s, r.bottom / s};
}

template< class T > trect< T >& operator+=( trect< T >& lhs, trectarg< T > rhs )
{
	lhs.left += rhs.left;
	lhs.top += rhs.top;
	lhs.right += rhs.right;
	lhs.bottom += rhs.bottom;
	return lhs;
}
template< class T > trect< T >& operator-=( trect< T >& lhs, trectarg< T > rhs )
{
	lhs.left -= rhs.left;
	lhs.top -= rhs.top;
	lhs.right -= rhs.right;
	lhs.bottom -= rhs.bottom;
	return lhs;
}
template< class T > trect< T >& operator*=( trect< T >& lhs, T rhs )
{
	lhs.left *= rhs;
	lhs.top *= rhs;
	lhs.right *= rhs;
	lhs.bottom *= rhs;
	return lhs;
}
template< class T > trect< T >& operator/=( trect< T >& lhs, T rhs )
{
	lhs.left /= rhs;
	lhs.top /= rhs;
	lhs.right /= rhs;
	lhs.bottom /= rhs;
	return lhs;
}

template< class T > tregion< T > operator+( tregionarg< T > lhs, tregionarg< T > rhs )
{
	return {lhs.left + rhs.left, lhs.top + rhs.top, lhs.width + rhs.width, lhs.height + rhs.height};
}
template< class T > tregion< T > operator-( tregionarg< T > lhs, tregionarg< T > rhs )
{
	return {lhs.left - rhs.left, lhs.top - rhs.top, lhs.width - rhs.width, lhs.height - rhs.height};
}
template< class T > tregion< T > operator*( tregionarg< T > r, T s )
{
	return {r.left * s, r.top * s, r.width * s, r.height * s};
}
template< class T > tregion< T > operator*( T s, tregionarg< T > r )
{
	return {r.left * s, r.top * s, r.width * s, r.height * s};
}
template< class T > tregion< T > operator/( tregionarg< T > r, T s )
{
	return {r.left / s, r.top / s, r.width / s, r.height / s};
}
template< class T > tregion< T > operator/( T s, tregionarg< T > r )
{
	return {r.left / s, r.top / s, r.width / s, r.height / s};
}

template< class T > tregion< T >& operator+=( tregion< T >& lhs, tregionarg< T > rhs )
{
	lhs.left += rhs.left;
	lhs.top += rhs.top;
	lhs.width += rhs.width;
	lhs.height += rhs.height;
	return lhs;
}
template< class T > tregion< T >& operator-=( tregion< T >& lhs, tregionarg< T > rhs )
{
	lhs.left -= rhs.left;
	lhs.top -= rhs.top;
	lhs.width -= rhs.width;
	lhs.height -= rhs.height;
	return lhs;
}
template< class T > tregion< T >& operator*=( tregion< T >& lhs, T rhs )
{
	lhs.left *= rhs;
	lhs.top *= rhs;
	lhs.width *= rhs;
	lhs.height *= rhs;
	return lhs;
}
template< class T > tregion< T >& operator/=( tregion< T >& lhs, T rhs )
{
	lhs.left /= rhs;
	lhs.top /= rhs;
	lhs.width /= rhs;
	lhs.height /= rhs;
	return lhs;
}

#endif // _RECT_H_INCLUDED_
