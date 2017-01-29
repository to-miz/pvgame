#pragma once

#ifndef _ALGORITHM_H_INCLUDED_
#define _ALGORITHM_H_INCLUDED_

template < class Container >
inline int32 size( const Container& container )
{
	return safe_truncate< int32 >( container.size() );
}

template< class Container >
typename Container::const_iterator max_element( const Container& container )
{
	return max_element( begin( container ), end( container ), less() );
}
template< class Container, class Comp >
typename Container::const_iterator max_element( const Container& container, Comp comp )
{
	return max_element( begin( container ), end( container ), comp );
}

template< class Container >
typename Container::const_iterator min_element( const Container& container )
{
	return min_element( begin( container ), end( container ), less() );
}
template< class Container, class Comp >
typename Container::const_iterator min_element( const Container& container, Comp comp )
{
	return min_element( begin( container ), end( container ), comp );
}

template < class RandomAccessIterator >
RandomAccessIterator rotate_right( RandomAccessIterator first, RandomAccessIterator last )
{
	assert( first < last );
	if( last - first > 1 ) {
		auto tmp = std::move( *( last - 1 ) );
		std::move_backward( first, last - 1, last );
		*first = std::move( tmp );
	}
	return first;
}
template < class RandomAccessIterator >
RandomAccessIterator rotate_left( RandomAccessIterator first, RandomAccessIterator last )
{
	assert( first <= last );
	if( last - first > 1 ) {
		auto tmp = std::move( *first );
		std::move( first + 1, last, first );
		*( last - 1 ) = std::move( tmp );
	}
	return first;
}

template < class Container, class Iterator >
Iterator unordered_erase( Container& container, Iterator it )
{
	if( it == container.end() - 1 ) {
		return container.erase( it );
	}

	*it = std::move( container.back() );
	container.pop_back();
	return it;
}

template < class Container, class Pred >
auto erase_if( Container& container, Pred pred ) -> typename Container::iterator
{
	auto last = end( container );
	return container.erase( remove_if( begin( container ), last, pred ), last );
}
template < class Container, class Pred >
auto erase_if( Container& container, typename Container::iterator first,
               typename Container::iterator last, Pred pred ) -> typename Container::iterator
{
	return container.erase( remove_if( first, last, pred ), last );
}

// removes elements from container that are equal to value, changing the size of the container
// relative order of elements is NOT preserved
template < class Container, class ValueType >
void unordered_remove( Container& container, const ValueType& value )
{
	using std::begin;
	using std::end;
	auto last = end( container );
	for( auto it = begin( container ); it != last; ) {
		// make sure value is not in container
		assert( ::std::addressof( value ) != ::std::addressof( *it ) );
		if( *it == value ) {
			it = unordered_erase( container, it );
			last = end( container );
			continue;
		}
		++it;
	}
}

template < class Container, class UnaryPredicate >
void unordered_remove_if( Container& container, UnaryPredicate pred )
{
	// TODO: implement using find_if?
	using std::begin;
	using std::end;
	auto last = end( container );
	for( auto it = begin( container ); it != last; ) {
		if( pred( *it ) ) {
			it = unordered_erase( container, it );
			last = end( container );
			continue;
		}
		++it;
	}
}

template < class Container, class Value >
bool find_and_erase( Container& container, const Value& value )
{
	auto last = end( container );
	auto it   = find( begin( container ), last, value );
	if( it != last ) {
		container.erase( it );
		return true;
	}

	return false;
}

template < class Container, class UnaryPredicate >
bool find_and_erase_if( Container& container, UnaryPredicate&& pred )
{
	using std::find;
	using std::begin;
	using std::end;
	auto end_ = end( container );
	auto it   = find_if( begin( container ), end_, std::forward< UnaryPredicate >( pred ) );
	if( it != end_ ) {
		container.erase( it );
		return true;
	}

	return false;
}

template < class Container, class RangeType >
inline typename Container::iterator erase_range( Container& container, RangeType range )
{
	using std::begin;
	return container.erase( begin( container ) + range.min, begin( container ) + range.max );
}

template < class Container >
inline typename Container::iterator erase_index( Container& container, int32 index )
{
	using std::begin;
	return container.erase( begin( container ) + index );
}

template < class T >
inline int32 erase_index( T* first, int32 count, int32 index )
{
	--count;
	move( first + index, first + index + 1, count );
	return count;
}

template < class Iterator, class ValueType >
void iota_n( Iterator first, int32 n, ValueType val )
{
	for( auto i = 0; i < n; ++i, ++val, ++first ) {
		*first = val;
	}
}

template < class Container, class T >
inline bool exists( Container& container, const T& val )
{
	using std::begin;
	using std::end;
	auto end_ = end( container );
	return find( begin( container ), end_, val ) != end_;
}

template < class Container, class UnaryPredicate >
inline bool exists_if( Container& container, UnaryPredicate&& pred )
{
	using std::begin;
	using std::end;
	auto last = end( container );
	return find_if( begin( container ), last, std::forward< UnaryPredicate >( pred ) ) != last;
}

template < class Container, class T >
inline auto find( Container& container, const T& val ) -> decltype( std::begin( container ) )
{
	using std::begin;
	using std::end;
	return find( begin( container ), end( container ), val );
}

template < class Container, class UnaryPredicate >
inline auto find_if( Container& container, UnaryPredicate&& pred )
    -> decltype( std::begin( container ) )
{
	using std::begin;
	using std::end;
	return find_if( begin( container ), end( container ), std::forward< UnaryPredicate >( pred ) );
}

template < class T >
inline T* find_or_null( T* first, T* last, const T& val )
{
	while( first != last ) {
		if( *first == val ) {
			return first;
		}
		++first;
	}
	return nullptr;
}
template < class Iterator, class UnaryPredicate >
inline auto find_if_or_null( Iterator first, Iterator last, UnaryPredicate pred )
    -> typename typeof( *first )*
{
	while( first != last ) {
		if( pred( *first ) ) {
			return &( *first );
		}
		++first;
	}
	return nullptr;
}
template < class Container, class UnaryPredicate >
inline auto find_if_or_null( Container& container, UnaryPredicate pred )
    -> typename typeof( *begin( container ) )*
{
	using std::begin;
	using std::end;
	return find_if_or_null( begin( container ), end( container ), pred );
}

#define find_first_where( container, condition )                                          \
	find_if_or_null( ( container ), [&]( const typeof( container )::value_type& entry ) { \
		return ( condition );                                                             \
	} )

template < class Iterator, class T >
NullableInt32 find_index( Iterator first, Iterator last, const T& val )
{
	auto index = 0;
	while( first != last ) {
		if( *first == val ) {
			return {index};
		}
		++first;
		++index;
	}
	return NullableInt32::makeNull();
}
template < class Container, class T >
NullableInt32 find_index( const Container& container, const T& val )
{
	using std::begin;
	using std::end;
	return find_index( begin( container ), end( container ), val );
}
template< class Iterator, class UnaryPredicate >
NullableInt32 find_index_if( Iterator first, Iterator last, UnaryPredicate pred )
{
	auto index = 0;
	while( first != last ) {
		if( pred( *first ) ) {
			return {index};
		}
		++first;
		++index;
	}
	return NullableInt32::makeNull();
}
template< class Container, class UnaryPredicate >
NullableInt32 find_index_if( const Container& container, UnaryPredicate&& pred )
{
	using std::begin;
	using std::end;
	return find_index_if( begin( container ), end( container ),
	                      std::forward< UnaryPredicate >( pred ) );
}

// appends element into container iff element isn't already in the container
// returns true if element was added
template < class Container, class Value >
bool append_unique( Container& container, Value&& value )
{
	using std::begin;
	using std::end;
	auto end_ = end( container );
	if( find( begin( container ), end_, value ) == end_ ) {
		container.push_back( std::forward< Value >( value ) );
		assert( !container.empty() );
		return true;
	}

	return false;
}

// sorts (last - 1)st element into array of [first, last)
template < class RandomAccessIterator >
RandomAccessIterator insertion_sort_last_elem( RandomAccessIterator first,
                                               RandomAccessIterator last )
{
	if( first >= last ) return first;

	auto back        = last - 1;
	const auto& elem = *back;
	auto pos         = upper_bound( first, back, elem, less() );
	if( pos != back ) {
		rotate_right( pos, last );
	}
	return pos;
}

template < class RandomAccessIterator, class Compare >
RandomAccessIterator insertion_sort_last_elem( RandomAccessIterator first,
                                               RandomAccessIterator last, Compare&& cmp )
{
	if( first >= last ) return first;

	auto back        = last - 1;
	const auto& elem = *back;
	auto pos         = upper_bound( first, back, elem, cmp );
	if( pos != back ) {
		rotate_right( pos, last );
	}
	return pos;
}

template < class Container >
typename Container::pointer queryElement( Container& container, int32 index )
{
	return ( index >= 0 && index < container.size() ) ? ( &container[index] ) : ( nullptr );
}

#ifdef ACHE_USE_STD
template < class Container >
void insertion_sort_last_elem( Container& container )
{
	using std::begin;
	using std::end;
	insertion_sort_last_elem( begin( container ), end( container ) );
}

template < class Container, class Compare >
void insertion_sort_last_elem( Container& container, Compare&& cmp )
{
	using std::begin;
	using std::end;
	insertion_sort_last_elem( begin( container ), end( container ), cmp );
}
#endif

#endif
