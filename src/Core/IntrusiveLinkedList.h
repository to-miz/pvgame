#pragma once

#ifndef _INTRUSIVELINKEDLIST_H_INCLUDED_
#define _INTRUSIVELINKEDLIST_H_INCLUDED_

// singly linked list

template < class T, T* const T::*next = &T::next >
struct IntrusiveLinkedListIterator {
	T* current;

	IntrusiveLinkedListIterator& operator++()
	{
		// prefix
		this->current = this->current->*next;
		return *this;
	}
	IntrusiveLinkedListIterator operator++( int )
	{
		// postifx
		IntrusiveLinkedListIterator result = *this;
		this->current                      = this->current->*next;
		return result;
	}
	bool operator==( const IntrusiveLinkedListIterator& other ) const
	{
		return this->current == other.current;
	}
	bool operator!=( const IntrusiveLinkedListIterator& other ) const
	{
		return this->current != other.current;
	}

	T* operator->() const { return current; }
	T& operator*() const { return *current; }
	operator T*() const { return current; }
	IntrusiveLinkedListIterator& operator=( T* other )
	{
		this->current = other;
		return *this;
	}
};

template < class T, T* T::* const next = &T::next >
struct IntrusiveLinkedList {
	T* head;
	T* tail;
	int32 count;

	void push( T* entry )
	{
		entry->*next = nullptr;
		if( !tail ) {
			head  = entry;
			tail  = entry;
			count = 1;
		} else {
			tail->*next = entry;
			tail        = entry;
			++count;
		}
	}

	T* operator[]( intmax i )
	{
		assert( i >= 0 && i < (intmax)count );
		auto result = head;
		while( result && i ) {
			result = result->*next;
			--i;
		}
		return result;
	}

	T* erase( T* entry )
	{
		assert( entry );
		assert( head );
		if( entry == head ) {
			head = entry->*next;
		} else {
			auto prev = head;
			while( prev->*next != entry ) {
				prev = prev->*next;
				assert( prev );
			}
			prev->*next = entry->*next;
			if( entry == tail ) {
				tail = prev;
				assert( !tail || tail->*next == nullptr );
			}
		}
		--count;
		return entry->*next;
	}

	IntrusiveLinkedListIterator< T, next > begin() { return {head}; }
	IntrusiveLinkedListIterator< T, next > end() { return {nullptr}; }
	// need to const cast just to make the pointed to member const
	IntrusiveLinkedListIterator< const T, next > begin() const
	{
		return {head};
	}
	IntrusiveLinkedListIterator< const T, next > end() const
	{
		return {nullptr};
	}
	IntrusiveLinkedListIterator< const T, next > cbegin() const
	{
		return {head};
	}
	IntrusiveLinkedListIterator< const T, next > cend() const
	{
		return {nullptr};
	}
};

// doubly linked list

template < class T, T* T::* const next = &T::next, T* T::* const prev = &T::prev >
struct IntrusiveDoublyLinkedList {
	T* head;
	T* tail;
	int32 count;

	void push( T* entry )
	{
		entry->*prev = nullptr;
		entry->*next = nullptr;
		if( !tail ) {
			head  = entry;
			tail  = entry;
			count = 1;
		} else {
			auto prev   = tail;
			prev->*next = entry;
			tail        = entry;
			tail->*prev = prev;
			++count;
		}
	}

	T* operator[]( intmax i )
	{
		assert( i >= 0 && i < (intmax)count );
		auto result = head;
		while( result && i ) {
			result = result->*next;
			--i;
		}
		return result;
	}

	T* erase( T* entry )
	{
		assert( entry );
		auto prev = entry->*prev;
		auto next = entry->*next;
		if( prev ) {
			prev->*next = next;
		}
		if( next ) {
			next->*prev = prev;
		}
		if( entry == head ) {
			assert( head->*prev == nullptr );
			head = next;
			assert( !head || head->*prev == nullptr );
		}
		if( entry == tail ) {
			assert( tail->*next == nullptr );
			tail = prev;
			assert( !tail || tail->*next == nullptr );
		}
		--count;
		return next;
	}

	IntrusiveLinkedListIterator< T > begin() { return {head}; }
	IntrusiveLinkedListIterator< T > end() { return {nullptr}; }
	IntrusiveLinkedListIterator< const T > begin() const { return {head}; }
	IntrusiveLinkedListIterator< const T > end() const { return {nullptr}; }
	IntrusiveLinkedListIterator< const T > cbegin() const { return {head}; }
	IntrusiveLinkedListIterator< const T > cend() const { return {nullptr}; }
};

template < class T >
IntrusiveLinkedList< T > makeLinkedList( T* node )
{
	IntrusiveLinkedList< T > result = {};
	result.head                     = node;
	while( node ) {
		result.tail = node;
		node        = node->*next;
		++result.count;
	}
	return result;
}
template < class T >
IntrusiveLinkedList< T > makeDoublyLinkedList( T* node )
{
	IntrusiveLinkedList< T > result = {};
	result.head                     = node;
	while( node ) {
		node = node->*next;
		++result.count;
		if( result.count > 10 ) {
			break;
		}
	}
	return result;
}

// based on the algorithm description on
// http://www.chiark.greenend.org.uk/~sgtatham/algorithms/listsort.html
template < class T >
struct HeadTailPair {
	T* head;
	T* tail;
};
template < class T, class Cmp, T* T::*next = &T::next >
HeadTailPair< T > merge_sort( T* list, Cmp cmp )
{
	HeadTailPair< T > result = {list};
	if( list ) {
		size_t splitSize = 1;
		for( ;; ) {
			auto a = result.head;
			assert( a );
			T* mergedList = nullptr;
			auto merges   = 0;
			T* selected   = nullptr;

			do {
				++merges;
				auto b        = a;
				size_t aCount = 0;
				for( size_t i = 0; i < splitSize; ++i ) {
					b = b->*next;
					++aCount;
					if( !b ) {
						break;
					}
				}

				size_t bCount = splitSize;
				do {
					if( !aCount || ( bCount && b && !cmp( *a, *b ) ) ) {
						selected = b;
						b        = b->*next;
						--bCount;
					} else {
						selected = a;
						a        = a->*next;
						--aCount;
					}
					if( mergedList ) {
						mergedList->*next = selected;
					} else {
						result.head = selected;
					}
					mergedList = selected;
				} while( aCount || ( bCount && b ) );

				a = b;
			} while( a );
			mergedList->*next = nullptr;
			result.tail       = mergedList;
			if( merges < 2 ) {
				break;
			}
			splitSize *= 2;
		}
	}
	return result;
}

template < class T, class Cmp, T* T::*Next >
void merge_sort( IntrusiveLinkedList< T, Next >* list, Cmp cmp )
{
	if( !list || list->count <= 0 ) {
		return;
	}

	auto result = merge_sort< Next >( list->head, cmp );
	list->head  = result.head;
	list->tail  = result.tail;
}

template < class T, class Cmp, T* T::*next = &T::next, T* T::*prev = &T::prev >
HeadTailPair< T > merge_sort_doubly( T* list, Cmp cmp )
{
	HeadTailPair< T > result = {list};
	if( list ) {
		size_t splitSize = 1;
		for( ;; ) {
			auto a = result.head;
			assert( a );
			T* mergedList = nullptr;
			auto merges   = 0;
			T* selected   = nullptr;

			do {
				++merges;
				auto b        = a;
				size_t aCount = 0;
				for( size_t i = 0; i < splitSize; ++i ) {
					b = b->*next;
					++aCount;
					if( !b ) {
						break;
					}
				}

				size_t bCount = splitSize;
				do {
					if( !aCount || ( bCount && b && !cmp( *a, *b ) ) ) {
						selected = b;
						b        = b->*next;
						--bCount;
					} else {
						selected = a;
						a        = a->*next;
						--aCount;
					}
					if( mergedList ) {
						mergedList->*next = selected;
					} else {
						result.head = selected;
					}
					selected->*prev = mergedList;
					mergedList      = selected;
				} while( aCount || ( bCount && b ) );

				a = b;
			} while( a );
			mergedList->*next = nullptr;
			result.tail       = mergedList;
			if( merges < 2 ) {
				break;
			}
			splitSize *= 2;
		}
	}
	return result;
}

template < class T, class Cmp, T* T::*Next, T* T::*Prev >
void merge_sort( IntrusiveDoublyLinkedList< T, Next, Prev >* list, Cmp cmp )
{
	if( !list || list->count <= 0 ) {
		return;
	}

	auto result = merge_sort_doubly< Next, Prev >( list->head, cmp );
	list->head  = result.head;
	list->tail  = result.tail;
}

#endif  // _INTRUSIVELINKEDLIST_H_INCLUDED_
