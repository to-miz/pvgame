template < class T >
struct Ring {
	typedef int32 size_type;
	T* ptr;
	uint32 first;
	uint32 last;
	uint32 cap;
	bool is_full;

	// STL container stuff
	struct iterator {
		typedef T value_type;
		typedef int32 difference_type;
		typedef T* pointer;
		typedef T& reference;
#ifndef GAME_NO_STD
		typedef std::bidirectional_iterator_tag iterator_category;
#endif

		T* begin;
		uint32 cur;
		uint32 cap;

		bool operator==( const iterator& other ) const { return compare( this, &other, 1 ) == 0; }
		bool operator!=( const iterator& other ) const { return compare( this, &other, 1 ) != 0; }

		T& operator*() const { return *( begin + ( cur % cap ) ); }
		T* operator->() const { return begin + ( cur % cap ); }

		// prefix
		iterator& operator++()
		{
			++cur;
			return *this;
		}
		iterator& operator--()
		{
			--cur;
			return *this;
		}

		// postfix
		iterator operator++( int )
		{
			auto ret = *this;
			++cur;
			return ret;
		}
		iterator operator--( int )
		{
			auto ret = *this;
			--cur;
			return ret;
		}
	};

	struct const_iterator {
		typedef T value_type;
		typedef int32 difference_type;
		typedef T* pointer;
		typedef T& reference;
#ifndef GAME_NO_STD
		typedef std::bidirectional_iterator_tag iterator_category;
#endif

		const T* begin;
		uint32 cur;
		uint32 cap;

		bool operator==( const const_iterator& other ) const
		{
			return compare( this, &other, 1 ) == 0;
		}

		bool operator!=( const const_iterator& other ) const
		{
			return compare( this, &other, 1 ) != 0;
		}

		T& operator*() const { return *( begin + ( cur % cap ) ); }
		T* operator->() const { return begin + ( cur % cap ); }

		// prefix
		const_iterator& operator++()
		{
			++cur;
			return *this;
		}
		const_iterator& operator--()
		{
			--cur;
			return *this;
		}

		// postfix
		const_iterator operator++( int )
		{
			auto ret = *this;
			++cur;
			return ret;
		}
		const_iterator operator--( int )
		{
			auto ret = *this;
			--cur;
			return ret;
		}
	};

	typedef T value_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef int32 difference_type;
	typedef int32 size_type;

	iterator begin() const { return {ptr, first, cap}; }
	iterator end() const {
		if( first == last && is_full ) {
			return {ptr, last + cap, cap};
		}
		return {ptr, last, cap};
	}
	const_iterator cbegin() const { return {ptr, first, cap}; }
	const_iterator cend() const
	{
		if( first == last && is_full ) {
			return {ptr, last + cap, cap};
		}
		return {ptr, last, cap};
	}

	size_type max_size() const { return safe_truncate< size_type >( cap ); }
	size_type capacity() const { return safe_truncate< size_type >( cap ); }

	pointer data() const { return ptr; }
	size_type size() const
	{
		if( is_full ) return capacity();
		if( first > last ) {
			return safe_truncate< size_type >( ( cap - first ) + last );
		}
		return safe_truncate< size_type >( last - first );
	}
	size_type length() const { return size(); }
	bool empty() const { return first == last && !is_full; }
	bool full() const { return first == last && is_full; }
	size_type remaining() const { return capacity() - size(); }

	reference operator[]( size_type i )
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < size() );
		auto index = ( first + i ) % cap;
		return ptr[index];
	}

	const reference operator[]( size_type i ) const
	{
		assert( ptr );
		assert( i >= 0 );
		assert( i < size() );
		auto index = ( first + i ) % cap;
		return ptr[index];
	}

	reference at( size_type i ) { return operator[]( i ); }

	const reference at( size_type i ) const { return operator[]( i ); }

	reference back()
	{
		assert( ptr );
		assert( !empty() );
		return ptr[last - 1];
	}

	reference front()
	{
		assert( ptr );
		assert( !empty() );
		return ptr[first];
	}

	void push_back( const T& elem )
	{
		assert( ptr );
		assert( !full() );
		ptr[last] = elem;
		last      = ( last + 1 ) % cap;
		is_full   = ( first == last );
	}

	void pop_back()
	{
		assert( ptr );
		assert( !empty() );
		last    = ( last - 1 ) % cap;
		is_full = false;
	}

	void push_front( const T& elem )
	{
		assert( ptr );
		assert( !full() );
		first      = ( first - 1 ) % cap;
		ptr[first] = elem;
		is_full    = ( first == last );
	}

	void pop_front()
	{
		assert( ptr );
		assert( !empty() );
		first   = ( first + 1 ) % ( cap + 1 );
		is_full = false;
	}

	void clear()
	{
// TODO: decide which one is the right thing to do:
#if 0
		last = first;
#else
		first = last = 0;
#endif
		is_full = false;
	}

	bool isInitialized() const { return ptr != nullptr; }

	void assign( Ring other )
	{
		assert( cap >= (uint32)other.size() );
		assert( ptr );

		if( other.last > other.first ) {
			first = 0;
			last  = other.last - other.first;
			copy( ptr, other.ptr + other.first, last );
		} else if( other.last < other.first || other.is_full ) {
			first      = 0;
			last       = other.size();
			auto count = other.cap - other.first;
			copy( ptr, other.ptr + other.first, count );
			copy( ptr + count, other.ptr, other.last );
		} else {
			clear();
		}
	}

	operator Ring< const T >() const {
		return Ring< const T >{ptr, first, last, cap, is_full};
	}
};

template< class T >
Ring< T > makeRingView( T* ptr, int32 size, int32 first, int32 last, bool full = true )
{
	assert( size >= 0 );
	assert( first >= 0 && last >= 0 );
	if( first != last ) {
		full = false;
	}
	return {ptr, (uint32)first, (uint32)last, (uint32)size, full};
}