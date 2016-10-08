#pragma once

#ifndef _ALGORITHM_H_INCLUDED_
#define _ALGORITHM_H_INCLUDED_

template< class Iterator, class ValueType >
size_t count( Iterator first, Iterator last, ValueType value )
{
	size_t result = 0;
	while( last != first ) {
		if( *first == value ) {
			++result;
		}
		++first;
	}
	return result;
}

template< class Container, class Iterator >
Iterator unordered_erase( Container& container, Iterator it )
{
	if( it == container.end() - 1 ) {
		return container.erase( it );
	}

	*it = std::move( container.back() );
	container.pop_back();
	return it;
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
		assert( ::std::addressof( value ) != ::std::addressof( *i ) );
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
bool find_and_erase( Container& container, Value&& value )
{
	using std::find;
	using std::begin;
	using std::end;
	auto end_ = end( container );
	auto it = find( begin( container ), end_, value );
	if( it != end_ ) {
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
	auto it = find_if( begin( container ), end_, std::forward< UnaryPredicate >( pred ) );
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
	using std::find;
	using std::begin;
	using std::end;
	auto end_ = end( container );
	return find( begin( container ), end_, val ) != end_;
}

template < class Container, class UnaryPredicate >
inline bool exists_if( Container& container, UnaryPredicate&& pred )
{
	using std::find_if;
	using std::begin;
	using std::end;
	auto end_ = end( container );
	return find_if( begin( container ), end_, std::forward< UnaryPredicate >( pred ) ) != end_;
}

template < class Container, class T >
inline auto find( Container& container, const T& val ) -> decltype( std::begin( container ) )
{
	using std::find;
	using std::begin;
	using std::end;
	return find( begin( container ), end( container ), val );
}

template < class Container, class UnaryPredicate >
inline auto find_if( Container& container, UnaryPredicate&& pred )
	-> decltype( std::begin( container ) )
{
	using std::find_if;
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
template < class T, class UnaryPredicate >
inline T* find_if_or_null( T* first, T* last, UnaryPredicate pred )
{
	while( first != last ) {
		if( pred( *first ) ) {
			return first;
		}
		++first;
	}
	return nullptr;
}
template < class Container, class UnaryPredicate >
inline auto find_if_or_null( Container& container, UnaryPredicate pred )
	-> decltype( begin( container ) )
{
	using std::begin;
	using std::end;
	return find_if_or_null( begin( container ), end( container ), pred );
}

#define find_first_where( container, condition ) \
	find_if_or_null( ( container ),              \
					 [&]( const typeof( container )::value_type& it ) { return ( condition ); } )

template< class T >
inline NullableInt32 find_index( T* first, T* last, const T& val )
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
template< class T, class UnaryPredicate >
inline NullableInt32 find_index_if( T* first, T* last, UnaryPredicate pred )
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

// appends element into container iff element isn't already in the container
// returns true if element was added
template < class Container, class Value >
bool append_unique( Container& container, Value&& value )
{
	using std::find;
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
void insertion_sort_last_elem( RandomAccessIterator first, RandomAccessIterator last )
{
	if( first >= last ) return;

	--last;
	auto elem = *last;
	std::rotate( std::upper_bound( first, last, elem ), last, last + 1 );
}

template < class RandomAccessIterator, class Compare >
void insertion_sort_last_elem( RandomAccessIterator first, RandomAccessIterator last,
							   Compare&& cmp )
{
	if( first >= last ) return;

	--last;
	auto elem = *last;
	std::rotate( std::upper_bound( first, last, elem, cmp ), last, last + 1 );
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
