/*
Some stl algorithms are reimplemented here.
On msvc stl headers include crt headers and pollute global namespace, so as a workaround here are
some of the more useful algorithms reimplemented.

Most functions are not 100% stand-ins for stl algorithms, most assume that iterators are in fact
pointers.
*/

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
pair< typename std::decay< A >::type, typename std::decay< B >::type > make_pair( A&& a, B&& b )
{
	return {std::forward< A >( a ), std::forward< B >( b )};
}

constexpr const intmax MaxIntroSort = 32;

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

// more efficient way to erase based on a predicate than calling container.erase in a loop
template< class Iterator, class Pred >
Iterator remove_if( Iterator first, Iterator last, Pred pred )
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

template < class RandomAccessIterator, class ValueType, class Pred >
RandomAccessIterator lower_bound( RandomAccessIterator first, RandomAccessIterator last,
                                  const ValueType& value, Pred&& pred )
{
	auto count = last - first;
	assert( count >= 0 );
	while( count > 0 ) {
		auto halfCount = count / 2;
		auto mid       = first + halfCount;
		if( pred( *mid, value ) ) {
			first = mid + 1;
			count -= halfCount + 1;
		} else {
			count = halfCount;
		}
	}
	return first;
}
template < class RandomAccessIterator, class ValueType >
RandomAccessIterator lower_bound( RandomAccessIterator first, RandomAccessIterator last,
                                  const ValueType& value )
{
	return lower_bound( first, last, value, less() );
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

template < class Iterator, class T, class Compare >
pair< Iterator, Iterator > equal_range( Iterator first, Iterator last, const T& val, Compare&& cmp )
{
	auto it = lower_bound( first, last, val, cmp );
	return make_pair( it, upper_bound( it, last, val, cmp ) );
}
template < class Iterator, class T, class Compare >
pair< Iterator, Iterator > equal_range( Iterator first, Iterator last, const T& val )
{
	return equal_range( first, last, val, less() );
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