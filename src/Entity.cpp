struct EntityHandle {
	uint32 bits;

	inline uint32 index() { return bits - 1; }
	inline explicit operator bool() const { return bits != 0; }
	inline bool operator==( EntityHandle other ) const { return bits == other.bits; }
	inline bool operator!=( EntityHandle other ) const { return bits != other.bits; }
};

struct HandleManager {
	// TODO: keep book about invalid entity handles, so that we can detect use after remove
	uint32 ids;
};

HandleManager makeHandleManager()
{
	HandleManager result = {};
	return result;
}
EntityHandle addEntityHandle( HandleManager* handles )
{
	assert( handles );
	return {++handles->ids};
}

enum class SpatialState {
	Airborne,
	Grounded,
	FallingOff,
};
const char* getSpatialStateString( SpatialState state )
{
	static const char* const SpatialStateStrings[] = {
	    "Grounded", "FallingOff", "Airborne",
	};
	assert( valueof( state ) < countof( SpatialStateStrings ) );
	return SpatialStateStrings[valueof( state )];
}

struct CollidableRef {
	enum : int8 { None, Tile, Dynamic } type;
	uint16 index;
	EntityHandle handle;
	explicit operator bool() const { return type != None; }
	void clear() { type = None; }
	void setTile( int32 index )
	{
		this->type  = Tile;
		this->index = safe_truncate< uint16 >( index );
	}
	void setDynamic( int32 index, EntityHandle handle )
	{
		this->type   = Dynamic;
		this->index  = safe_truncate< uint16 >( index );
		this->handle = handle;
	}

	bool operator==( CollidableRef other ) { return type == other.type && index == other.index; }
	bool operator!=( CollidableRef other ) { return type != other.type || index != other.index; }
};
enum class EntityMovement : int8 { Straight, Grounded };
enum class CollisionResponse : int8 { FullStop, Bounce };
enum class EntityFaceDirection : int8 { Left, Right };
enum class EntityTeam : int8 { None, Players, Robots };

enum class EntityResponsiveness : uint8 { Responsive, NoControl };
enum class EntityHorizontalMovement : uint8 { None, Left, Right };
enum class EntityVerticalAction : uint8 { None, Jump, Crouch, ConsumeInput };
enum class EntityVerticalMovement : uint8 { None, Jump, Crouch };
enum class EntityActionState : uint8 { None, Attack };
struct EntityControl {
	EntityResponsiveness responsiveness : 1;
	EntityHorizontalMovement horizontal : 2;
	EntityVerticalAction verticalAction : 2;  // whether a new action has been input
	EntityVerticalMovement vertical : 2;  // whether old action is still valid (button held down)
	EntityActionState action : 1;
};

struct EntityStats {
	int16 hp;
};

struct Skeleton;

// this used to be CollidableComponent, but it had a lot of fields that didn't have anything to do
// with collision detection, but with reporting back how/what happened. It behaved like a full
// entity class, so now it is actually clear what this structure is: everything you need to know
// about an entity is here
// there are still "components", but for things that operate on entities as a whole, like controls
struct Entity {
	vec2 position;
	vec2 velocity;
	vec2 acceleration;
	vec2 maxSpeed;
	rectf aab;
	CollidableRef grounded;             // collidable we are standing on
	CollidableRef wallslideCollidable;  // collidable we are wallsliding against
	CollidableRef lastCollision;
	vec2 positionDelta;
	vec2 prevVeloctiy;

	CountdownTimer walljumpWindow;    // time window in which we can perform a walljump
	CountdownTimer walljumpDuration;  // how long the player can't move towards the wall

	SpatialState spatialState;
	float spatialStateTimer;  // how long we have been in the current state

	float gravityModifier;
	float bounceModifier;  // value between 0 and 2 to specify bouncing behavior (0 = keep velocity
	                       // on collision, 1 = slide along edge on collision, 2 = reflect)
	float airFrictionCoeffictient;
	float wallslideFrictionCoefficient;

	CountdownTimer aliveCountdown;  // how many frames this can be alive for, used for entities that
	                                // dissipate after a certain time
	CountdownTimer hurtFlashCountdown;

	EntityHandle handle;

	struct {
		uint8 walljumpLeft : 1;  // whether we are doing a walljump to the left or right
		uint8 dynamic : 1;       // whether collidable is a dynamic collider
		uint8 deathFlag : 1;     // whether entity is dead
		uint8 hurt : 1;          // whether entity was hurt this frame
	} flags;
	EntityFaceDirection prevFaceDirection;
	EntityFaceDirection faceDirection;
	EntityTeam team;
	EntityControl control;
	vec2 hurtNormal;

	Skeleton* skeleton;

	enum Type {
		type_none,

		type_hero,
		type_wheels,

		type_count,
	} type;
	struct Hero {
		EntityStats stats;
		int8 currentAnimationIndex;
		int32 currentAnimation;
		CountdownTimer animationLockTimer;
		CountdownTimer particleEmitTimer;
		CountdownTimer shootingAnimationTimer;
	};
	struct Wheels {
		EntityStats stats;
		CountdownTimer attackTimer;
		int32 currentAnimation;
		enum : int8 { Idle, Moving, Turning, Stopping, Attacking, Hurt } state;
		bool8 shouldTurn;
	};
	union {
		Hero hero;
		Wheels wheels;
	};

	bool walljumpLeft() const { return flags.walljumpLeft; }
	bool dynamic() const { return flags.dynamic; }
	bool dead() const { return flags.deathFlag; }
};
void setSpatialState( Entity* collidable, SpatialState state )
{
	if( state != collidable->spatialState ) {
		collidable->spatialState      = state;
		collidable->spatialStateTimer = 0;
		if( state != SpatialState::Grounded ) {
			collidable->grounded.clear();
		}
	}
}
void processSpatialState( Entity* entity, float dt )
{
	constexpr float maxFallingOffTime = 5;
	entity->spatialStateTimer += dt;
	if( entity->spatialState == SpatialState::FallingOff
	    && entity->spatialStateTimer >= maxFallingOffTime ) {
		setSpatialState( entity, SpatialState::Airborne );
	}
}
bool isSpatialStateJumpable( Entity* collidable )
{
	return collidable->spatialState == SpatialState::Grounded
	       || collidable->spatialState == SpatialState::FallingOff;
}
bool isSpatialStateWalljumpable( Entity* collidable )
{
	return /*collidable->spatialState == SpatialState::FallingOff
	       ||*/ collidable->spatialState
	       == SpatialState::Airborne;
}

Entity* getDynamicFromCollidableRef( Array< Entity > dynamics, CollidableRef ref )
{
	assert( ref.type == CollidableRef::Dynamic );
	assert( ref.index >= 0 );
	if( ref.index < dynamics.size() ) {
		auto candidate = &dynamics[ref.index];
		if( candidate->handle == ref.handle ) {
			return candidate;
		}
	}
	auto handle = ref.handle;
	return find_first_where( dynamics, entry.handle == handle );
}
float getFrictionCoefficitonFromCollidableRef( Array< Entity > dynamics, TileGrid grid,
                                               Array< TileInfo > infos, CollidableRef ref )
{
	switch( ref.type ) {
		case CollidableRef::None: {
			return 0;
		}
		case CollidableRef::Tile: {
			auto tile = grid[ref.index];
			if( tile ) {
				return infos[tile.frames.min].frictionCoefficient;
			}
			break;
		}
		case CollidableRef::Dynamic: {
			return dynamics[ref.index].wallslideFrictionCoefficient;
		}
		InvalidDefaultCase;
	}
	return 0;
}
rectf getBoundsFromCollidableRef( Array< Entity > dynamics, TileGrid grid,
                                  CollidableRef ref )
{
	rectf result = {};
	switch( ref.type ) {
		case CollidableRef::None: {
			break;
		}
		case CollidableRef::Tile: {
			auto pos = grid.coordinatesFromIndex( ref.index );
			using namespace GameConstants;
			result = RectWH( pos.x * TileWidth, pos.y * TileHeight, TileWidth, TileHeight );
			break;
		}
		case CollidableRef::Dynamic: {
			auto dynamic = &dynamics[ref.index];
			result = translate( dynamic->aab, dynamic->position );
			break;
		}
		InvalidDefaultCase;
	}
	return result;
}

struct EntitySystem {
	UArray< Entity > entries;
	int32 entriesCount;  // count of entries that are not dynamic

	Array< Entity > staticEntries() const
	{
		return makeArrayView( entries.begin(), entries.begin() + entriesCount );
	}
	Array< Entity > dynamicEntries() const
	{
		return makeArrayView( entries.begin() + entriesCount, entries.end() );
	}
};
EntitySystem makeEntitySystem( StackAllocator* allocator, int32 maxCount )
{
	EntitySystem result = {};
	result.entries      = makeUArray( allocator, Entity, maxCount );
	return result;
}

namespace EntityTraitsFlags
{
enum Values {
	CanWalljump     = BITFIELD( 0 ),
	Dynamic         = BITFIELD( 1 ),
	UseForcedNormal = BITFIELD( 2 ),
	NoFaceDirection = BITFIELD( 3 ),
};
}
struct EntityTraitsData {
	struct Flags {
		uint8 canWalljump : 1;
		uint8 dynamic : 1;
		uint8 useForcedNormal : 1;
		uint8 noFaceDirection : 1;
	} flags;
	EntityMovement movement;
	CollisionResponse response;
	EntityTeam team;
	vec2 forcedNormal;

	struct {
		float gravityModifier;
		float bounceModifier;  // value between 0 and 2 to specify bouncing behavior (0 = keep
		                       // velocity on collision, 1 = slide along edge on collision, 2 =
		                       // reflect)
		float airFrictionCoeffictient;
		float wallslideFrictionCoefficient;
	} init;

	EntityStats stats;
};
EntityTraitsData::Flags makeEntityTraitsFlags( uint8 flags )
{
	EntityTraitsData::Flags result = {};
	result.canWalljump             = ( flags & EntityTraitsFlags::CanWalljump ) != 0;
	result.dynamic                 = ( flags & EntityTraitsFlags::Dynamic ) != 0;
	result.useForcedNormal         = ( flags & EntityTraitsFlags::UseForcedNormal ) != 0;
	result.noFaceDirection         = ( flags & EntityTraitsFlags::NoFaceDirection ) != 0;
	return result;
}

const EntityTraitsData* const getEntityTraits( Entity::Type type )
{
	static const EntityTraitsData EntityTraits[] = {
	    // none
	    {},

	    // hero
	    {
	        makeEntityTraitsFlags( EntityTraitsFlags::CanWalljump ),
	        EntityMovement::Grounded,
	        CollisionResponse::Bounce,
	        EntityTeam::Players,
	        {},
	        {
	            1,                                             // gravityModifier
	            1,                                             // bounceModifier
	            GameConstants::DefaultAirFrictionCoefficient,  // airFrictionCoeffictient
	            0                                              // wallslideFrictionCoefficient
	        },
	        {10},
	    },

	    // wheels
	    {
	        {},
	        EntityMovement::Grounded,
	        CollisionResponse::Bounce,
	        EntityTeam::Robots,
	        {},
	        {
	            1,                                             // gravityModifier
	            1,                                             // bounceModifier
	            GameConstants::DefaultAirFrictionCoefficient,  // airFrictionCoeffictient
	            0                                              // wallslideFrictionCoefficient
	        },
	        {3},
	    },
	};

	assert( type >= 0 && type < Entity::type_count );
	return &EntityTraits[type];
}

struct SkeletonSystem;
Entity* addEntity( EntitySystem* entitySystem, SkeletonSystem* skeletonSystem, EntityHandle handle,
                   Entity::Type type, vec2arg position = {} );
Entity* findEntity( EntitySystem* system, EntityHandle handle )
{
	return find_first_where( system->entries, entry.handle == handle );
}