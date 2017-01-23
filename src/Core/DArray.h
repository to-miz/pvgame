#pragma once

#ifndef _DARRAY_H_INCLUDED_
#define _DARRAY_H_INCLUDED_

#define DA_NO_STD_ITERATOR
typedef int32 da_size_t;

template < class T >
struct DynamicArray {
	typedef da_size_t size_type;

	T* ptr;
	size_type sz;
	size_type cap;

	// STL container stuff
	typedef T value_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T* iterator;
	typedef const T* const_iterator;
#ifndef DA_NO_STD_ITERATOR
	typedef std::reverse_iterator< iterator > reverse_iterator;
	typedef std::reverse_iterator< const_iterator > const_reverse_iterator;
#endif
	typedef da_size_t difference_type;

	inline iterator begin() const { return iterator( ptr ); }
	inline iterator end() const { return iterator( ptr + sz ); }
	inline const_iterator cbegin() const { return const_iterator( ptr ); }
	inline const_iterator cend() const { return const_iterator( ptr + sz ); }

#ifndef DA_NO_STD_ITERATOR
	inline reverse_iterator rbegin() const { return reverse_iterator( ptr + sz ); }
	inline reverse_iterator rend() const { return reverse_iterator( ptr ); }
	inline const_reverse_iterator crbegin() const { return const_reverse_iterator( ptr + sz ); }
	inline const_reverse_iterator crend() const { return const_reverse_iterator( ptr ); }
#endif

	inline size_type max_size() const { return cap; }
	inline size_type capacity() const { return cap; }

	inline pointer data() const { return ptr; }
	inline size_type size() const { return sz; }
	inline size_type length() const { return sz; }
	inline bool empty() const { return sz == 0; }
	inline bool full() const { return sz == cap; }
	inline size_type remaining() const { return cap - sz; }

	inline reference operator[]( size_type i )
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < sz );
		return ptr[i];
	}

	inline const_reference operator[]( size_type i ) const
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < sz );
		return ptr[i];
	}

	inline reference at( size_type i )
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < sz );
		return ptr[i];
	}

	inline const_reference at( size_type i ) const
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < sz );
		return ptr[i];
	}

#ifdef TMDA_INT64_ACCOSSORS
	inline reference operator[]( int64 i )
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < (int64)sz );
		return ptr[i];
	}

	inline const_reference operator[]( int64 i ) const
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < (int64)sz );
		return ptr[i];
	}

	inline reference at( int64 i )
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < (int64)sz );
		return ptr[i];
	}

	inline const_reference at( int64 i ) const
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < (int64)sz );
		return ptr[i];
	}
#endif  // TMDA_INT64_ACCOSSORS

	inline reference back()
	{
		assert( ptr );
		assert( sz );
		return ptr[sz - 1];
	}
	inline const_reference back() const
	{
		assert( ptr );
		assert( sz );
		return ptr[sz - 1];
	}
	inline reference front()
	{
		assert( ptr );
		assert( sz );
		return ptr[0];
	}
	inline const_reference front() const
	{
		assert( ptr );
		assert( sz );
		return ptr[0];
	}

	inline void push_back( const T& elem )
	{
		assert( ptr );
		assert( sz + 1 <= cap );
		ptr[sz] = elem;
		++sz;
	}
	inline void pop_back()
	{
		--sz;
		assert( sz >= 0 );
	}

// define TMDA_EMPLACE_BACK_RETURNS_POINTER if you want emplace_back to return a pointer instead
// of reference. The reference version is how std containers in C++1z implement emplace_back.
#ifdef TMDA_EMPLACE_BACK_RETURNS_POINTER
	inline pointer emplace_back()
	{
		assert( ptr );
		assert( sz + 1 <= cap );
		++sz;
		return &ptr[sz - 1];
	}
#else
	inline reference emplace_back()
	{
		assert( ptr );
		assert( sz + 1 <= cap );
		++sz;
		return ptr[sz - 1];
	}
#endif

	inline void clear() { sz = 0; }
	inline void resize( size_type sz )
	{
		assert( sz >= 0 && sz <= cap );
		this->sz = sz;
	}
	inline void grow( size_type by )
	{
		sz += by;
		assert( sz >= 0 && sz <= cap );
	}

	inline void assign( UninitializedArrayView other ) { assign( other.begin(), other.end() ); }
	void assign( const_iterator first, const_iterator last )
	{
		assert( ( first < begin() || first >= end() )
					&& ( last < begin() || last >= end() ) );
		sz = static_cast< size_type >( last - first );
		assert( sz <= cap );
		memcpy( ptr, first, sz * sizeof( value_type ) );
	}
	void assign( const_iterator first, size_type length )
	{
		assert( length <= cap );
		assert( ( first < begin() || first >= end() )
					&& ( first + length < begin() || first + length >= end() ) );

		sz = length;
		memcpy( ptr, first, sz * sizeof( value_type ) );
	}
	void assign( size_type n, const value_type& val )
	{
		assert( n >= 0 );
		assert( ptr );
		n          = ( n < cap ) ? ( n ) : ( cap );
		sz         = n;
		auto count = n;
		for( da_size_t i = 0; i < count; ++i ) {
			ptr[i] = val;
		}
	}
	inline void assign( const std::initializer_list< T >& list )
	{
		assert( list.size() <= (size_t)capacity() );
		sz = (size_type)list.size();
		memcpy( ptr, list.begin(), sz * sizeof( value_type ) );
	}

	iterator insert( iterator position, size_type n, const value_type& val )
	{
		assert( ptr );
		assert( position >= begin() && position <= end() );

		size_type rem   = remaining();
		size_type count = ( n < rem ) ? n : ( rem );
		auto suffix     = end() - position;
		if( count > 0 ) {
			auto tmp = val;  // in case val is inside sequence
			// make room for insertion by moving suffix
			memmove( position + count, position, suffix * sizeof( value_type ) );

			sz += static_cast< size_type >( count );
			for( int i = 0; i < count; ++i ) {
				position[i] = tmp;
			}
		}
		return position;
	}
	iterator insert( iterator position, const_iterator first, const_iterator last )
	{
		assert( first <= last );
		assert( ptr || first == last );
		assert( position >= begin() && position <= end() );

		auto rem        = remaining();
		da_size_t count = static_cast< da_size_t >( last - first );
		assert( rem >= count );
		if( count > 0 && count <= rem ) {
			// range fits move entries to make room and copy
			memmove( position + count, position, ( end() - position ) * sizeof( value_type ) );
			memcpy( position, first, count * sizeof( value_type ) );
			sz += static_cast< size_type >( count );
		}
		return position;
	}

	inline iterator append( const_iterator first, const_iterator last )
	{
		return insert( end(), first, last );
	}
	inline iterator append( size_type n, const value_type& val ) { return insert( end(), n, val ); }

	iterator erase( iterator position )
	{
		assert( ptr );
		assert( position >= begin() && position <= end() );
		memmove( position, position + 1, ( end() - position - 1 ) * sizeof( value_type ) );
		--sz;
		return position;
	}
	iterator erase( iterator first, iterator last )
	{
		assert( first <= last );
		assert( ptr || first == last );
		if( first == begin() && last == end() ) {
			clear();
		} else if( first < last ) {
			assert( first >= begin() && last <= end() );
			// move suffix to where the erased range used to be
			memmove( first, last, ( end() - last ) * sizeof( value_type ) );
			sz -= static_cast< size_type >( last - first );
		}
		return first;
	}
};

#endif // _DARRAY_H_INCLUDED_
