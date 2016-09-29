#if defined( _MSC_VER ) && ( _MSC_VER >= 1020 )
#pragma once
#endif

#ifndef _SCOPEGUARD_H_INCLUDED_
#define _SCOPEGUARD_H_INCLUDED_

// TODO: decide on namespace
// namespace core
// {

class ScopeGuardBase
{
protected:
	ScopeGuardBase() = default;
	ScopeGuardBase( ScopeGuardBase&& other ) : active( other.active ) { other.active = false; }

public:
	void dismiss() { active = false; }

protected:
	bool active = true;
};

template < class Func >
class ScopeGuard : public ScopeGuardBase
{
public:
	explicit ScopeGuard( Func&& func ) : func( std::move( func ) ) {}
	explicit ScopeGuard( const Func& func ) : func( func ) {}

	ScopeGuard( ScopeGuard&& other )
	: ScopeGuardBase( std::move( other ) ), func( std::move( other.func ) )
	{
	}

	~ScopeGuard()
	{
		if( active ) func();
	}

private:
	void* operator new( size_t ) = delete;

	ScopeGuard() = delete;
	ScopeGuard( const ScopeGuard& ) = delete;
	ScopeGuard& operator=( const ScopeGuard& ) = delete;

private:
	Func func;
};

template < class Func >
ScopeGuard< typename std::decay< Func >::type > makeScopeGuard( Func&& func )
{
	return ScopeGuard< typename std::decay< Func >::type >( std::forward< Func >( func ) );
}

namespace scope_guard_helper
{
struct DEFER_TAG {
};

template < class Func >
inline ScopeGuard< Func > operator+( DEFER_TAG, Func&& lambda )
{
	return makeScopeGuard( std::forward< Func >( lambda ) );
}
}

// } // namespace core

#define SCOPE_EXIT( ... ) \
	auto PP_JOIN( scope_guard_, __LINE__ ) = scope_guard_helper::DEFER_TAG() + [ __VA_ARGS__ ]()
#define SCOPE_GUARD( ... ) scope_guard_helper::DEFER_TAG() + [ __VA_ARGS__ ]()

#endif
