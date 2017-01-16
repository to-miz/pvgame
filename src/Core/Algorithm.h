/*
Note that some template algorithms that exist in stl header algorithm are reimplemented here.
That is because in this code base we implement cmath functions ourselves and <algorithm> might
include it, generating errors.

Most functions are not 100% stand-ins for stl algorithms, most assume that iterators are in fact
pointers.
*/

#pragma once

#ifndef _ALGORITHM_H_INCLUDED_
#define _ALGORITHM_H_INCLUDED_

constexpr const intmax MaxIntroSort = 32;

struct less {
	template < class A, class B >
	bool operator()( const A& a, const B& b )
	{
		return a < b;
	}
};

template < class A, class B >
struct pair {
	A first;
	B second;
};
template < class A, class B >
pair< A, B > make_pair( A&& a, B&& b )
{
	return {std::forward< A >( a ), std::forward< B >( b )};
}

template < class RandomAccessIterator >
RandomAccessIterator rotate_right( RandomAccessIterator first, RandomAccessIterator last )
{
	assert( first < last );
	if( last - first > 1 ) {
		auto tmp = std::move( *( last - 1 ) );
		move( first + 1, first, last - first - 1 );
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
		move( first, first + 1, last - first - 1 );
		*( last - 1 ) = std::move( tmp );
	}
	return first;
}

template < class RandomAccessIterator, class Pred >
void insertion_sort( RandomAccessIterator first, RandomAccessIterator last, Pred& pred )
{
	assert( first <= last );
	if( first != last ) {
		for( auto next = first + 1; next < last; ++next ) {
		    // find insertion pos
		    // no binary search, because insertion sort is only used on small arrays
		    auto dest = first;
		    auto value = std::move( *next );
		    while( !pred( value, *dest ) && dest < next ) {
		        ++dest;
		    }
		    // make room for insertion
		    move( dest + 1, dest, next - dest );
		    // insert
		    *dest = std::move( value );
		}
	}
}

template < class Iterator, class Pred >
void sort( Iterator first, Iterator last, Pred pred )
{
	// TODO: use better sorting algorithm
	insertion_sort( first, last, pred );
}
template < class Iterator >
void sort( Iterator first, Iterator last )
{
	sort( first, last, less() );
}

template < class T >
T exchange( T& value, const T& newValue )
{
	T oldValue = value;
	value      = newValue;
	return oldValue;
}

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

template< class Iterator, class Comp >
Iterator max_element( Iterator first, Iterator last, Comp comp )
{
	if( first == last ) {
		return last;
	}
	Iterator max = first;
	++first;
	while( first != last ) {
		if( comp( *max, *first ) ) {
			max = first;
		}
		++first;
	}
	return max;
}
template< class Iterator >
Iterator max_element( Iterator first, Iterator last )
{
	return max_element( first, last, less() );
}
template< class Container >
typename Container::iterator max_element( const Container& container )
{
	return max_element( begin( container ), end( container ), less() );
}
template< class Container, class Comp >
typename Container::iterator max_element( const Container& container, Comp comp )
{
	return max_element( begin( container ), end( container ), comp );
}
template< class Iterator, class Comp >
Iterator min_element( Iterator first, Iterator last, Comp comp )
{
	if( first == last ) {
		return last;
	}
	Iterator min = first;
	++first;
	while( first != last ) {
		if( comp( *first, *min ) ) {
			min = first;
		}
		++first;
	}
	return min;
}
template< class Iterator >
Iterator min_element( Iterator first, Iterator last )
{
	return min_element( first, last, less() );
}
template< class Container >
typename Container::iterator min_element( const Container& container )
{
	return min_element( begin( container ), end( container ), less() );
}
template< class Container, class Comp >
typename Container::iterator min_element( const Container& container, Comp comp )
{
	return min_element( begin( container ), end( container ), comp );
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

// more efficient way to erase based on a predicate than calling container.erase in a loop
template< class Iterator, class Pred >
Iterator erase_if( Iterator first, Iterator last, Pred pred )
{
	assert( first <= last );

	auto insertPos = find_if( first, last, pred );
	if( insertPos == last ) {
		return last;
	}

	auto it = insertPos + 1;
	for( ; it != last; ++it ) {
		if( !pred( *it ) ) {
			*insertPos = std::move( *it );
			++insertPos;
		}
	}
	return insertPos;
}

template < class Container, class Pred >
auto erase_if( Container& container, Pred pred ) -> typename Container::iterator
{
	auto last = end( container );
	return container.erase( erase_if( begin( container ), last, pred ), last );
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

template < class Iterator, class T >
Iterator find( Iterator first, Iterator last, const T& val )
{
	while( first != last ) {
		if( *first == val ) {
			return first;
		}
		++first;
	}
	return last;
}
template < class Iterator, class UnaryPredicate >
inline Iterator find_if( Iterator first, Iterator last, UnaryPredicate pred )
{
	while( first != last ) {
		if( pred( *first ) ) {
			return first;
		}
		++first;
	}
	return last;
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

template < class RandomAccessIterator, class ValueType, class Pred >
RandomAccessIterator upper_bound( RandomAccessIterator first, RandomAccessIterator last,
                                  const ValueType& value, Pred&& pred )
{
	auto count = last - first;
	assert( count >= 0 );
	while( count > 0 ) {
		auto halfCount = count / 2;
		auto mid       = first + halfCount;
		if( !pred( value, *mid ) ) {
			first = mid + 1;
			count -= halfCount + 1;
		} else {
			count = halfCount;
		}
	}
	return first;
}

template < class RandomAccessIterator, class ValueType >
RandomAccessIterator upper_bound( RandomAccessIterator first, RandomAccessIterator last,
                                  const ValueType& value )
{
	return upper_bound( first, last, value, less() );
}

// sorts (last - 1)st element into array of [first, last)
template < class RandomAccessIterator >
RandomAccessIterator insertion_sort_last_elem( RandomAccessIterator first,
                                               RandomAccessIterator last )
{
	if( first >= last ) return first;

	auto back = last - 1;
	auto elem = *back;
	auto pos = upper_bound( first, back, elem, less() );
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

	auto back = last - 1;
	auto elem = *back;
	auto pos = upper_bound( first, back, elem, cmp );
	if( pos != back ) {
		rotate_right( pos, last );
	}
	return pos;
}

template < class ForwardIterator, class T >
void replace( ForwardIterator first, ForwardIterator last, const T& old_value, const T& new_value )
{
	while( first != last ) {
		if( *first == old_value ) {
			*first = new_value;
		}
		++first;
	}
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
