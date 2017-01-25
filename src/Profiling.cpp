#include <intrin.h>

struct ProfilingEvent {
	uint64 timestamp;
	uint16 infoIndex;
	enum : int8 { Begin, End } type;
};
struct ProfilingInfo {
	const char* file;
	const char* block;
	int32 line;
};

#define MAX_PROFILING_EVENTS ( 4 * 20480 )
#define MAX_PROFILING_INFOS ( 2048 )

struct ProfilingTable {
	uint16 infosCount;
	uint16 eventsCount;
	ProfilingEvent events[MAX_PROFILING_EVENTS];
	ProfilingInfo infos[MAX_PROFILING_INFOS];
};

#ifndef NO_PROFILING

	extern global_var ProfilingTable* GlobalProfilingTable;

	uint16 getProfilingInfo( const char* file, const char* block, int32 line )
	{
		for( int32 i = 0, count = GlobalProfilingTable->infosCount; i < count; ++i ) {
			auto info = &GlobalProfilingTable->infos[i];
			if( ( info->file == file || strcmp( info->file, file ) == 0 )
			    && ( info->block == block || strcmp( info->block, block ) == 0 ) ) {

				return safe_truncate< uint16 >( i );
			}
		}
		assert( GlobalProfilingTable->infosCount < MAX_PROFILING_INFOS );
		uint16 index                       = GlobalProfilingTable->infosCount++;
		GlobalProfilingTable->infos[index] = {file, block, line};
		return index;
	}

	#define PROFILING_EVENT_( type )                                                     \
		assert( GlobalProfilingTable->eventsCount < MAX_PROFILING_EVENTS );              \
		auto event = &GlobalProfilingTable->events[GlobalProfilingTable->eventsCount++]; \
		*event     = {__rdtsc(), infoIndex, ProfilingEvent::type};

	#define PROFILING_BLOCK_( file, name, line, type )                      \
		{                                                                   \
			static uint16 infoIndex = getProfilingInfo( file, name, line ); \
			PROFILING_EVENT_( type );                                       \
		}
	#define BEGIN_PROFILING_BLOCK( name ) PROFILING_BLOCK_( __FILE__, name, __LINE__, Begin )
	#define END_PROFILING_BLOCK( name ) PROFILING_BLOCK_( __FILE__, name, __LINE__, End )

	struct ScopedProfilingBlock {
		uint16 infoIndex;
		ScopedProfilingBlock( uint16 infoIndex ) : infoIndex( infoIndex )
		{
			PROFILING_EVENT_( Begin );
		}
		~ScopedProfilingBlock() { PROFILING_EVENT_( End ); }
	};

	#define PROFILE_FUNCTION2( counter, file, name, line )                                    \
		static uint16 PP_JOIN( info_index_, counter ) = getProfilingInfo( file, name, line ); \
		ScopedProfilingBlock PP_JOIN( scoped_profiling_block_, counter ) =                    \
		    ScopedProfilingBlock( PP_JOIN( info_index_, counter ) );
	#define PROFILE_FUNCTION() PROFILE_FUNCTION2( __LINE__, __FILE__, __FUNCTION__, __LINE__ )

#else // !defined( NO_PROFILING )
	#define BEGIN_PROFILING_BLOCK( name ) ( (void)0 )
	#define END_PROFILING_BLOCK( name ) ( (void)0 )
	#define PROFILE_FUNCTION() ( (void)0 )
#endif // !defined( NO_PROFILING )

struct ProfilingBlock {
	uint64 duration;
	float relRatio; // ratio of cpu usage relative to parent
	float absRatio; // ratio of cpu usage relative to absolute cpu usage
	uint16 infoIndex;
	ProfilingBlock* child;
	ProfilingBlock* next;
};
struct ProfilingState {
	IntrusiveLinkedList< ProfilingBlock > blocks;
	Array< ProfilingInfo > infos;
};

ProfilingState processProfilingEvents( StackAllocator* allocator, ProfilingTable* table )
{
	assert( isValid( allocator ) );
	assert( table );

	ProfilingState result = {};
	auto partition        = StackAllocatorPartition::ratio( allocator, 2 );
	auto primary          = partition.primary();
	auto scrap            = partition.scrap();

	struct StackEntry {
		uint64 timestamp;
		ProfilingBlock* block;
		ProfilingBlock* childrenTail;
	};
	auto stack                  = beginVector( scrap, StackEntry );
	double oneOverTotalDuration = 1;
	if( table->eventsCount ) {
		auto totalDuration =
		    table->events[table->eventsCount - 1].timestamp - table->events[0].timestamp;
		if( totalDuration ) {
			oneOverTotalDuration = 1.0 / totalDuration;
		}
	}

	for( int32 i = 0, count = table->eventsCount; i < count; ++i ) {
		auto event = &table->events[i];
		switch( event->type ) {
			case ProfilingEvent::Begin: {
				auto current       = allocateStruct( primary, ProfilingBlock );
				*current           = {};
				current->infoIndex = event->infoIndex;
				current->relRatio  = 1;
				if( stack.size() ) {
					auto parent = &stack.back();
					if( parent->childrenTail ) {
						parent->childrenTail->next = current;
					}
					parent->childrenTail = current;
					if( !parent->block->child ) {
						parent->block->child = current;
					}
				} else {
					result.blocks.push( current );
				}
				stack.push_back( {event->timestamp, current} );
				break;
			}
			case ProfilingEvent::End: {
				assert( stack.size() );
				auto current    = &stack.back();
				auto block      = current->block;
				block->duration = event->timestamp - current->timestamp;
				block->absRatio = (float)( block->duration * oneOverTotalDuration );
				assert( block->duration );
				double oneOverDuration = 1.0 / block->duration;
				stack.pop_back();
				// calculate duration relRatio of children
				for( auto child = block->child; child; child = child->next ) {
					child->relRatio = (float)( child->duration * oneOverDuration );
				}
				break;
			}
				InvalidDefaultCase;
		}
	}
	assert( stack.size() == 0 );

	result.infos = makeArray( primary, ProfilingInfo, table->infosCount );
	result.infos.assign( table->infos, table->infos + table->infosCount );

	partition.commit();
	return result;
}