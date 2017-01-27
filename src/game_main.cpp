#include "DebugSwitches.h"

#include <type_traits>
#include <cassert>
#include "Core/IntegerTypes.h"
#include "Core/CoreTypes.h"
#include "Warnings.h"
#include <cstdarg>

// #define GAME_NO_STD
#ifndef GAME_NO_STD
	#include "GameStl.h"
#else
	#define MATH_NO_CRT
	#define COREFLOAT_NO_CRT
	#include "Core/StlAlgorithm.h"
#endif

#include "Core/Macros.h"
#include <cstring>

#include "Core/Math.h"
#include "tm_utility_wrapper.cpp"
#include "Core/CoreFloat.h"
#include "Core/Math.cpp"
#include "Utility.cpp"
#include "Core/Log.h"

#ifdef GAME_NO_STD
	#include "Core/NumericLimits.h"
#endif
#include "Core/Truncate.h"
#include "Core/NullableInt.h"
#include "Core/Algorithm.h"

#ifdef GAME_DEBUG
	#define ARGS_AS_CONST_REF 1
#else
	#define ARGS_AS_CONST_REF 0
#endif
#include "Core/StackAllocator.cpp"
#define VEC3_ARGS_AS_CONST_REF ARGS_AS_CONST_REF
#define VEC4_ARGS_AS_CONST_REF ARGS_AS_CONST_REF
#include "Core/Vector.cpp"
#define RECT_ARGS_AS_CONST_REF ARGS_AS_CONST_REF
#include "Core/Rect.h"
#include "Core/Matrix.cpp"
#define AABB_ARGS_AS_CONST_REF ARGS_AS_CONST_REF
#include "Core/AABB.h"
#define RANGE_ARGS_AS_CONST_REF ARGS_AS_CONST_REF
#include "Core/Range.h"

#include "Core/ArrayView.cpp"
#include "Core/StringView.cpp"
#include "Core/String.cpp"
#include "Core/Unicode.cpp"
#include "tm_conversion_wrapper.cpp"
#include "tm_bin_packing_wrapper.cpp"

#include "Core/ScopeGuard.h"

#include "easing.cpp"

#include "Core/Color.cpp"
#include "Core/Normal.cpp"
#include "ImageData.h"

#include "Core/IntrusiveLinkedList.h"
#include "Profiling.cpp"

#include "QuadTexCoords.cpp"
#include "Graphics.h"
#include "Graphics/Font.h"
#include "TextureMap.cpp"

global_var string_builder* GlobalDebugPrinter = nullptr;
#if defined( GAME_DEBUG ) || ( GAME_DEBUG_PRINTING )
	#define debugPrint( ... ) GlobalDebugPrinter->print( __VA_ARGS__ );
	#define debugPrintln( ... ) GlobalDebugPrinter->println( __VA_ARGS__ );
	#define debugPrintClear() GlobalDebugPrinter->clear()
	#define debugPrintGetString() asStringView( *GlobalDebugPrinter )
#else
	#define debugPrint( ... )
	#define debugPrintln( ... )
	#define debugPrintClear()
	#define debugPrintGetString() ( StringView{} )
#endif

#include "Core/BitTwiddlings.h"

#include "VirtualKeys.h"
#include "Inputs.cpp"
#include "GameDeclarations.h"
#include "Imgui.cpp"

#include "Graphics/ImageProcessing.cpp"

#include "JsonWriter.cpp"
#include "tm_json_wrapper.cpp"

#include "Core/Hash.cpp"
#include "Core/FixedSizeAllocator.cpp"

namespace GameConstants
{
constexpr const float Gravity                = 0.1f;
constexpr const float MovementSpeed          = 1.1f;
constexpr const float JumpingSpeed           = -3.3f;
constexpr const float WalljumpingSpeed       = -3.3f;
constexpr const float WalljumpMaxDuration    = 8;
constexpr const float WalljumpFixDuration    = 4;
constexpr const float WalljumpWindowDuration = 10;
constexpr const float WalljumpMoveThreshold  = WalljumpMaxDuration - WalljumpFixDuration;
constexpr const float WallslideFrictionCoefficient = 0.05f;

constexpr const float TileWidth  = 16.0f;
constexpr const float TileHeight = 16.0f;

constexpr const float DeltaToFrameTime = 60.0f / 1000.0f;  // constant to convert elapsed
                                                           // miliseconds to number between 0 and 1,
                                                           // where 1 = 1/60 of a second
}

struct DebugValues;

// globals
global_var PlatformServices* GlobalPlatformServices = nullptr;
global_var TextureMap* GlobalTextureMap             = nullptr;
global_var IngameLog* GlobalIngameLog               = nullptr;
global_var ImmediateModeGui* ImGui                  = nullptr;
global_var ProfilingTable* GlobalProfilingTable     = nullptr;

global_var MeshStream* debug_MeshStream = nullptr;
global_var bool debug_FillMeshStream    = true;
global_var DebugValues* debug_Values    = nullptr;

struct DebugValues {
	float groundPosition;
	float jumpHeight;
	float maxJumpHeight;
	float lastJumpHeight;
	float jumpHeightError;
};

StringView toString( VirtualKeyEnumValues key )
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->getKeyboardKeyName( key );
}

// global_var allocation methods
void* allocate( size_t size, uint32 alignment )
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->allocate( size, alignment );
}
void* reallocate( void* ptr, size_t newSize, size_t oldSize, uint32 alignment )
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->reallocate( ptr, newSize, oldSize, alignment );
}
void free( void* ptr, size_t size, uint32 alignment )
{
	assert( GlobalPlatformServices );
	GlobalPlatformServices->free( ptr, size, alignment );
}

template < class T >
T* allocate( size_t count = 1 )
{
	return (T*)::allocate( count * sizeof( T ), alignof( T ) );
}
template< class T >
T* reallocate( T* ptr, size_t newCount, size_t oldCount )
{
	return (T*)::reallocate( ptr, newCount * sizeof( T ), oldCount * sizeof( T ), alignof( T ) );
}
template< class T >
void free( T* ptr, size_t count = 1 )
{
	::free( ptr, count * sizeof( T ), alignof( T ) );
}

void* operator new( std::size_t size ) /*throw( std::bad_alloc )*/
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->malloc( size );
}
void operator delete( void* ptr ) /*throw()*/
{
	assert( GlobalPlatformServices );
	GlobalPlatformServices->mfree( ptr );
}
void* operator new[]( std::size_t size ) /*throw(std::bad_alloc)*/
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->malloc( size );
}
void operator delete[]( void* ptr ) /*throw()*/
{
	assert( GlobalPlatformServices );
	GlobalPlatformServices->mfree( ptr );
}

#include <vector>
#include <new>
#include <memory>

// #include "Core/DArray.h"

void debug_Clear()
{
	debug_MeshStream->color = 0xFF00FFFF;
	if( debug_FillMeshStream ) {
		clear( debug_MeshStream );
	}
}
void debug_PushAabb( float x, float y, float z )
{
	if( debug_FillMeshStream ) {
		pushAabb( debug_MeshStream, x - 3, y - 3, z - 3, x + 3, y + 3, z + 3 );
	}
}
void debug_PushAabb( vec3arg point ) { debug_PushAabb( point.x, point.y, point.z ); }

struct Camera {
	vec3 position;
	vec3 look;
	vec3 right;  // how the x axis is oriented
	vec3 up;     // how the y axis is oriented
};

Camera makeCamera( vec3arg position, vec3arg look, vec3arg up )
{
	Camera result;
	result.position = position;
	result.look     = normalize( look );
	result.up       = normalize( up );
	result.right    = normalize( cross( up, look ) );
	return result;
}
void updateCamera( Camera* camera, float xAngle, float yAngle )
{
	assert( camera );

	vec3 yDir = yAngle * camera->up;
	if( ( camera->up.y < 0.1f && camera->look.y < 0.0f && yDir.y < 0.0f )
	    || ( camera->up.y < 0.1f && camera->look.y >= 0.0f && yDir.y > 0.0f ) ) {
		yDir = {};
	}

	vec3 xDir     = xAngle * camera->right;
	camera->look  = normalize( camera->look + xDir + yDir );
	camera->right = safeNormalize( vec3{camera->look.z, 0, -camera->look.x}, camera->right );
	camera->up    = normalize( cross( camera->look, camera->right ) );
}
void updateCamera( Camera* camera, vec2arg angle ) { updateCamera( camera, angle.x, angle.y ); }
Camera cameraLookAt( const Camera& camera, vec3arg position )
{
	Camera result;
	result.position = camera.position;
	result.look     = normalize( position - camera.position );
	result.right    = normalize( cross( camera.up, result.look ) );
	result.up       = cross( result.look, result.right );
	return result;
}
Camera cameraLookDirection( const Camera& camera, vec3arg dir )
{
	assert( floatEqSoft( length( dir ), 1 ) );
	Camera result;
	result.position = camera.position;
	result.look     = dir;
	result.right    = normalize( cross( camera.up, dir ) );
	result.up       = cross( dir, result.right );
	return result;
}
mat4 getViewMatrix( Camera* camera )
{
	mat4 result;
	result.m[0] = camera->right.x;
	result.m[4] = camera->right.y;
	result.m[8] = camera->right.z;

	result.m[1] = camera->up.x;
	result.m[5] = camera->up.y;
	result.m[9] = camera->up.z;

	result.m[2]  = camera->look.x;
	result.m[6]  = camera->look.y;
	result.m[10] = camera->look.z;

	result.m[3]  = 0.0f;
	result.m[7]  = 0.0f;
	result.m[11] = 0.0f;

	result.m[12] = -dot( camera->right, camera->position );
	result.m[13] = -dot( camera->up, camera->position );
	result.m[14] = -dot( camera->look, camera->position );

	result.m[15] = 1.0f;
	return result;
}
vec3 center( Camera* camera )
{
	return {camera->position.x, camera->position.y + 50.0f, camera->position.z};
}

struct GameSettings {
	float mouseSensitivity;
	bool mouseInvertY;

	bool cameraTurning;
};
GameSettings makeDefaultGameSettings()
{
	GameSettings result     = {};
	result.mouseSensitivity = 0.0025f;
	result.mouseInvertY     = true;
	return result;
}

#include "VoxelGrid.cpp"

struct VoxelCollection {
	struct Frame {
		MeshId mesh;
		vec2 offset;
	};

	struct FrameInfo {
		VoxelGridTextureMap textureMap;
		recti textureRegion[VF_Count];
		float frictionCoefficient;
		aabb bounds;
		// TODO: include more meta information like gun position
	};

	struct Animation {
		string name;
		rangeu16 range;
	};

	TextureId texture;
	Array< Frame > frames;
	Array< FrameInfo > frameInfos;
	Array< Animation > animations;
	string voxelsFilename;
};

struct TileInfo {
	float frictionCoefficient;
};
struct TileSet {
	VoxelCollection voxels;
	Array< TileInfo > infos;
};

struct GameTile {
	uint8 collection;
	uint8 rotation;
	trange< uint8 > frames;
	inline explicit operator bool() const { return length( frames ) != 0; }
};

enum RoomLayerValues {
	RL_Main,
	RL_Back,
	RL_Front,

	RL_Count
};
typedef Grid< GameTile > TileGrid;
struct Room {
	struct Layer {
		TileGrid grid;
	};
	Layer layers[RL_Count];
	TileSet* tileSet;
};

rangeu16 getAnimationRange( VoxelCollection* collection, StringView name )
{
	rangeu16 result;
	if( auto animation = find_first_where( collection->animations, entry.name == name ) ) {
		result = animation->range;
	} else {
		result = {};
	}
	return result;
}
Array< VoxelCollection::Frame > getAnimationFrames( VoxelCollection* collection, rangeu16 range )
{
	return makeRangeView( collection->frames, range );
}
Array< VoxelCollection::Frame > getAnimationFrames( VoxelCollection* collection, StringView name )
{
	return makeRangeView( collection->frames, getAnimationRange( collection, name ) );
}

#include "VoxelEditor.h"

struct CountdownTimer {
	float value;

	inline explicit operator bool() const { return value > 0; }
};
bool isCountdownTimerExpired( CountdownTimer timer ) { return timer.value < Float::BigEpsilon; }
bool isCountdownActive( CountdownTimer timer ) { return timer.value > 0; }
CountdownTimer processTimer( CountdownTimer timer, float dt )
{
	CountdownTimer result = timer;
	result.value -= dt;
	if( result.value < 0 ) {
		result.value = 0;
	}
	return result;
}

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
EntityHandle addEntity( HandleManager* handles )
{
	assert( handles );
	return {++handles->ids};
}

enum class SpatialState {
	Grounded,
	FallingOff,
	Airborne,
};
static const char* const SpatilStateStrings[] = {
    "Grounded", "FallingOff", "Airborne",
};

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
};
enum class CollidableMovement : int8 { Straight, Grounded };
enum class CollidableResponse : int8 { FullStop, Bounce };
enum class CollidableFaceDirection : int8 { Left, Right };
enum class RenderType : int8 { Hero, Projectile };

struct CollidableComponent {
	vec2 position;
	vec2 velocity;
	rectf aab;
	CollidableRef grounded;             // collidable we are standing on
	CollidableRef wallslideCollidable;  // collidable we are wallsliding against
	CollidableRef lastCollision;
	vec2 positionDelta;

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

	CountdownTimer animationLockTimer;
	float animationSpeed;
	float currentFrame;
	rangeu16 currentAnimation;

	CountdownTimer particleEmitTimer;
	CountdownTimer shootingTimer;

	EntityHandle entity;

	CollidableMovement movement;
	CollidableResponse response;
	vec2 forcedNormal;
	uint8 flags;
	CollidableFaceDirection prevFaceDirection;
	CollidableFaceDirection faceDirection;

	RenderType renderType;

	enum Flags : uint8 {
		WalljumpLeft    = BITFIELD( 0 ),  // whether we are doing a walljump to the left or right
		Dynamic         = BITFIELD( 1 ),  // whether collidable is a dynamic collider
		UseForcedNormal = BITFIELD( 2 ),
		DeathFlag       = BITFIELD( 3 ),  // whether entity is dead
	};

	bool walljumpLeft() const { return ( flags & WalljumpLeft ) != 0; }
	bool dynamic() const { return ( flags & Dynamic ) != 0; }
	bool useForcedNormal() const { return ( flags & UseForcedNormal ) != 0; }
	bool dead() const { return ( flags & DeathFlag ) != 0; }
	void setForcedNormal( vec2arg normal )
	{
		forcedNormal = normal;
		flags |= UseForcedNormal;
	}
};
void setSpatialState( CollidableComponent* collidable, SpatialState state )
{
	if( state != collidable->spatialState ) {
		collidable->spatialState      = state;
		collidable->spatialStateTimer = 0;
		if( state != SpatialState::Grounded ) {
			collidable->grounded.clear();
		}
	}
}
void processSpatialState( CollidableComponent* collidable, float dt )
{
	constexpr float maxFallingOffTime = 5;
	collidable->spatialStateTimer += dt;
	if( collidable->spatialState == SpatialState::FallingOff
	    && collidable->spatialStateTimer >= maxFallingOffTime ) {
		setSpatialState( collidable, SpatialState::Airborne );
	}
}
bool isSpatialStateJumpable( CollidableComponent* collidable )
{
	return collidable->spatialState == SpatialState::Grounded
	       || collidable->spatialState == SpatialState::FallingOff;
}
bool isSpatialStateWalljumpable( CollidableComponent* collidable )
{
	return /*collidable->spatialState == SpatialState::FallingOff
	       ||*/ collidable->spatialState
	       == SpatialState::Airborne;
}
const char* getSpatialStateString( SpatialState state )
{
	assert( valueof( state ) < countof( SpatilStateStrings ) );
	return SpatilStateStrings[valueof( state )];
}
bool canEntityShoot( CollidableComponent* collidable ) { return true; }

CollidableComponent* getDynamicFromCollidableRef( Array< CollidableComponent > dynamics, CollidableRef ref )
{
	assert( ref.type == CollidableRef::Dynamic );
	assert( ref.index >= 0 );
	if( ref.index < dynamics.size() ) {
		auto candidate = &dynamics[ref.index];
		if( candidate->entity == ref.handle ) {
			return candidate;
		}
	}
	auto handle = ref.handle;
	return find_first_where( dynamics, entry.entity == handle );
}
float getFrictionCoefficitonFromCollidableRef( Array< CollidableComponent > dynamics, TileGrid grid,
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
rectf getBoundsFromCollidableRef( Array< CollidableComponent > dynamics, TileGrid grid,
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

struct CollidableSystem {
	UArray< CollidableComponent > entries;
	int32 entriesCount;  // count of entries that are not dynamic

	Array< CollidableComponent > staticEntries() const
	{
		return makeArrayView( entries.begin(), entries.begin() + entriesCount );
	}
	Array< CollidableComponent > dynamicEntries() const
	{
		return makeArrayView( entries.begin() + entriesCount, entries.end() );
	}
};
CollidableSystem makeCollidableSystem( StackAllocator* allocator, int32 maxCount )
{
	CollidableSystem result = {};
	result.entries          = makeUArray( allocator, CollidableComponent, maxCount );
	return result;
}
CollidableComponent* addCollidableComponent( CollidableSystem* system, EntityHandle entity,
                                             bool dynamic = false )
{
	assert( system );
	assert( entity );
	CollidableComponent* result = nullptr;
	if( system->entries.remaining() ) {
		if( dynamic ) {
			result  = system->entries.emplace_back();
			*result = {};
		} else {
			result =
			    system->entries.insert( system->entries.begin() + system->entriesCount, 1, {} );
			++system->entriesCount;
		}
		result->grounded.clear();
		result->entity = entity;
		setFlagCond( result->flags, CollidableComponent::Dynamic, dynamic );
		result->gravityModifier = 1;
		result->bounceModifier  = 1;
		result->animationSpeed  = 0.1f;
		result->airFrictionCoeffictient = 0.0f;
	}
	return result;
}
CollidableComponent* findCollidableComponent( CollidableSystem* system, EntityHandle entity )
{
	return find_first_where( system->entries, entry.entity == entity );
}

struct ControlComponent {
	EntityHandle entity;

	CountdownTimer jumpInputBuffer;
	CountdownTimer shootInputBuffer;
};
struct ControlSystem {
	UArray< ControlComponent > entries;
};
ControlSystem makeControlSystem( StackAllocator* allocator, int32 maxCount )
{
	ControlSystem result = {};
	result.entries       = makeUArray( allocator, ControlComponent, maxCount );
	return result;
}
ControlComponent* addControlComponent( ControlSystem* system, EntityHandle entity )
{
	assert( system );
	assert( entity );
	ControlComponent* result = nullptr;
	if( system->entries.remaining() ) {
		result         = system->entries.emplace_back();
		*result        = {};
		result->entity = entity;
	}
	return result;
}

struct GameCamera : Camera {
	CountdownTimer turnTimer;
	vec3 prevLook;
	vec3 nextLook;
	bool turnedRight;

	GameCamera& operator=( const Camera& other )
	{
		static_cast< Camera& >( *this ) = other;
		return *this;
	}
};
GameCamera makeGameCamera( vec3arg position, vec3arg look, vec3arg up )
{
	GameCamera result                = {};
	static_cast< Camera& >( result ) = makeCamera( position, look, up );
	return result;
}

struct GameDebugGuiState {
	ImmediateModeGui debugGuiState;
	int32 mainDialog;
	int32 debugOutputDialog;
	bool show;
	float fadeProgress;
	bool initialized;
};

#define GAME_MAP_WIDTH 16
#define GAME_MAP_HEIGHT 16
static int8 GameDebugMapMain[] = {
    2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    3, 3, 3, 3, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0,
    0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    4, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
    4, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 4, 4, 4, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 1,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    4, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    4, 0, 1, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 1,
    4, 0, 0, 0, 0, 0, 0, 0, 7, 0, 1, 0, 0, 0, 1, 1,
    4, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
static int8 GameDebugMapFront[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
Room makeRoom( StackAllocator* allocator, int32 width, int32 height )
{
	Room result = {};
	FOR( layer : result.layers ) {
		layer.grid = makeGrid( allocator, GameTile, width, height );
		zeroMemory( layer.grid.data(), layer.grid.size() );
	}
	return result;
}
static Room debugGetRoom( StackAllocator* allocator, TileSet* tileSet )
{
	Room result = makeRoom( allocator, GAME_MAP_WIDTH, GAME_MAP_HEIGHT );
	result.tileSet = tileSet;
	auto back   = result.layers[RL_Back].grid;
	fill( back.data(), {0, 0, 2, 3}, back.size() );
	auto processLayer = []( TileGrid grid, const int8* map ) {
		for( auto y = 0; y < GAME_MAP_HEIGHT; ++y ) {
			for( auto x = 0; x < GAME_MAP_WIDTH; ++x ) {
				auto index = x + y * GAME_MAP_WIDTH;
				auto dest  = &grid[index];
				switch( map[index] ) {
					case 1: {
						*dest = {0, 0, 0, 1};
						break;
					}
					case 2: {
						*dest = {0, 1, 0, 1};
						break;
					}
					case 3: {
						*dest = {0, 2, 0, 1};
						break;
					}
					case 4: {
						*dest = {0, 3, 0, 1};
						break;
					}
					case 5: {
						*dest = {0, 0, 1, 2};
						break;
					}
					case 6: {
						*dest = {0, 0, 3, 4};
						break;
					}
					case 7: {
						*dest = {0, 0, 4, 5};
						break;
					}
					case 0:
					default: {
						*dest = {};
						break;
					}
				}
			}
		}
	};
	processLayer( result.layers[RL_Main].grid, GameDebugMapMain );
	processLayer( result.layers[RL_Front].grid, GameDebugMapFront );
	return result;
}

struct HeroVoxelCollection {
	VoxelCollection voxels;
	// cache animation name to index results
	rangeu16 idle;
	rangeu16 idleShoot;
	rangeu16 turn;
	rangeu16 walk;
	rangeu16 jumpRising;
	rangeu16 jumpFalling;
	rangeu16 wallslide;
	rangeu16 walkShoot;
	rangeu16 jumpRisingShoot;
	rangeu16 jumpFallingShoot;
	rangeu16 wallslideShoot;
	rangeu16 landing;
};

const char* HeroAnimationNames[] = {
    "Idle",
    "IdleShoot",
    "Turn",
    "Walk",
    "JumpRising",
    "JumpFalling",
    "Wallslide",
    "WalkShoot",
    "JumpRisingShoot",
    "JumpFallingShoot",
    "WallslideShoot",
    "Landing",
};

rectf gameToScreen( rectfarg rect )
{
	rectf result;
	result.left   = rect.left;
	result.top    = -rect.top;
	result.right  = rect.right;
	result.bottom = -rect.bottom;
	return result;
}

enum class ParticleEmitterId {
	Dust,
	LandingDust,
};
enum class ParticleTexture : int8 {
	Dust,
};
namespace ParticleEmitterFlags
{
enum Values : uint32 {
	AlternateSignX = BITFIELD( 0 ),
};
}
struct ParticleEmitter {
	int32 count;
	float maxAlive;
	vec2 velocity;
	ParticleTexture textureId;
	uint32 flags;
};

struct ParticleSystem {
	struct Particle {
		vec2 position;
		vec2 velocity;
		float alive;
		float invAlive;
		ParticleTexture textureId;
	};
	UArray< Particle > particles;

	TextureId texture;
};

ParticleSystem makeParticleSystem( StackAllocator* allocator, int32 maxParticles )
{
	ParticleSystem result = {};
	result.particles      = makeUArray( allocator, ParticleSystem::Particle, maxParticles );
	return result;
}
static const ParticleEmitter ParticleEmitters[] = {
    {1, 20, {0, -0.1f}, ParticleTexture::Dust},  // Dust
    {2,
     10,
     {0.5f, -0.1f},
     ParticleTexture::Dust,
     ParticleEmitterFlags::AlternateSignX},  // LandingDust
};
Array< ParticleSystem::Particle > emitParticles( ParticleSystem* system, vec2arg position,
                                                 ParticleEmitterId emitterId )
{
	Array< ParticleSystem::Particle > result = {};
	assert( system );
	assert( valueof( emitterId ) < countof( ParticleEmitters ) );
	auto emitter = &ParticleEmitters[valueof( emitterId )];
	auto count   = min( system->particles.remaining(), emitter->count );
	if( count > 0 ) {
		auto start = system->particles.size();
		system->particles.resize( start + count );
		result = makeRangeView( system->particles, start, start + count );
		FOR( entry : result ) {
			entry.position  = position;
			entry.velocity  = emitter->velocity;
			entry.alive     = emitter->maxAlive;
			entry.invAlive  = 1.0f / emitter->maxAlive;
			entry.textureId = emitter->textureId;
		}
		if( emitter->flags & ParticleEmitterFlags::AlternateSignX ) {
			bool sign = true;
			FOR( entry : result ) {
				if( sign ) {
					entry.velocity.x = -entry.velocity.x;
					sign = !sign;
				}
			}
		}
	}
	return result;
}
void processParticles( ParticleSystem* system, float dt )
{
	auto end = system->particles.end();
	for( auto it = system->particles.begin(); it != end; ) {
		it->position += it->velocity * dt;
		it->alive -= dt;
		if( it->alive <= 0 ) {
			it  = unordered_erase( system->particles, it );
			end = system->particles.end();
			continue;
		}
		++it;
	}
}
void renderParticles( RenderCommands* renderer, ParticleSystem* system )
{
	setRenderState( renderer, RenderStateType::DepthTest, false );
	setTexture( renderer, 0, system->texture );
	static const QuadTexCoords texCoords[] = {
		makeQuadTexCoords( scale( RectWH(  0.0f, 0, 9, 9 ), 1 / 39.0f, 1 / 9.0f ) ),
		makeQuadTexCoords( scale( RectWH( 10.0f, 0, 9, 9 ), 1 / 39.0f, 1 / 9.0f ) ),
		makeQuadTexCoords( scale( RectWH( 20.0f, 0, 9, 9 ), 1 / 39.0f, 1 / 9.0f ) ),
		makeQuadTexCoords( scale( RectWH( 30.0f, 0, 9, 9 ), 1 / 39.0f, 1 / 9.0f ) )
	};
	MESH_STREAM_BLOCK( stream, renderer ) {
		FOR( particle : system->particles ) {
			auto t = particle.alive * particle.invAlive;
			stream->color = Color::White;
			// alpha blend between first and last couple of frames of the particles life time
			if( t < 0.1f ) {
				stream->color = setAlpha( Color::White, remap( t, 0, 0.1f, 0.0f, 1.0f ) );
			} else if( t >= 0.9f ) {
				stream->color = setAlpha( Color::White, remap( t, 0.9f, 1.0f, 1.0f, 0.0f ) );
			}
			rectf rect = {-4, -4, 5, 5};
			rect = gameToScreen( translate( rect, particle.position ) );
			auto index = clamp( (int32)lerp( 1.0f - t, 0.0f, 4.0f ), 0, 3 );
			// TODO: use a texture atlas for particles and a lookup table
			assert( particle.textureId == ParticleTexture::Dust );
			pushQuad( stream, rect, 0, texCoords[index] );
		}
	}
	setRenderState( renderer, RenderStateType::DepthTest, true );
}

struct GameState {
	TileSet tileSet;
	HeroVoxelCollection heroVoxelCollection;
	ParticleSystem particleSystem;
	VoxelCollection projectile;

	HandleManager entityHandles;
	CollidableSystem collidableSystem;
	ControlSystem controlSystem;
	CollidableComponent* player;
	UArray< EntityHandle > entityRemovalQueue;
	bool initialized;

	GameCamera camera;
	rectf cameraFollowRegion;
	bool useGameCamera;
	bool lighting;

	Room room;

	// debug fields
	GameDebugGuiState debugGui;

	bool paused;

	bool debugCamera;
	bool debugCollisionBoxes;
};

bool emitProjectile( GameState* game, vec2arg origin, vec2arg velocity )
{
	// TODO: emit different types of projectiles based on upgrades
	auto projectileId = addEntity( &game->entityHandles );
	auto projectile   = addCollidableComponent( &game->collidableSystem, projectileId );
	if( projectile ) {
		projectile->position                = origin;
		projectile->velocity                = velocity;
		projectile->aab                     = {-3, -3, 3, 3};
		projectile->gravityModifier         = 0;
		projectile->bounceModifier          = 2;
		projectile->airFrictionCoeffictient = 0;
		projectile->movement                = CollidableMovement::Straight;
		projectile->response                = CollidableResponse::Bounce;
		projectile->renderType              = RenderType::Projectile;
		projectile->aliveCountdown          = {60};
		return true;
	}
	return false;
}

void processControlSystem( GameState* game, ControlSystem* control,
                           CollidableSystem* collidableSystem, GameInputs* inputs, float dt,
                           bool frameBoundary )
{
	assert( control );
	assert( collidableSystem );
	assert( inputs );

	using namespace GameConstants;

	for( auto& entry : control->entries ) {
		if( auto collidable = findCollidableComponent( collidableSystem, entry.entity ) ) {
			collidable->velocity.x = 0;
			// TODO: do keymapping to actions
			if( isKeyDown( inputs, KC_Left ) ) {
				if( isCountdownTimerExpired( collidable->walljumpDuration )
				    || collidable->walljumpLeft() ) {

					collidable->velocity.x = -MovementSpeed;
					if( collidable->walljumpWindow ) {
						collidable->faceDirection = CollidableFaceDirection::Right;
					} else {
						collidable->faceDirection = CollidableFaceDirection::Left;
					}
				}
			}
			if( isKeyDown( inputs, KC_Right ) ) {
				if( isCountdownTimerExpired( collidable->walljumpDuration )
				    || !collidable->walljumpLeft() ) {

					collidable->velocity.x = MovementSpeed;
					if( !collidable->walljumpWindow ) {
						collidable->faceDirection = CollidableFaceDirection::Right;
					} else {
						collidable->faceDirection = CollidableFaceDirection::Left;
					}
				}
			}
			if( isKeyPressed( inputs, KC_Up ) && !collidable->grounded ) {
				entry.jumpInputBuffer = {4};
			}
			// TODO: input buffering
			auto jumpDown = isKeyDown( inputs, KC_Up );
			if( collidable->spatialState == SpatialState::Airborne && jumpDown
			    && collidable->velocity.y < 0 ) {
			} else {
				if( collidable->velocity.y < 0 ) {
					collidable->velocity.y = 0;
				}
			}
			if( frameBoundary && jumpDown && isSpatialStateJumpable( collidable ) ) {
				collidable->velocity.y = JumpingSpeed;
				setSpatialState( collidable, SpatialState::Airborne );
			}
			// walljump
			if( frameBoundary && entry.jumpInputBuffer && isSpatialStateWalljumpable( collidable )
			    && collidable->walljumpWindow ) {

				collidable->walljumpWindow   = {};
				collidable->wallslideCollidable.clear();
				collidable->velocity.y       = WalljumpingSpeed;
				collidable->walljumpDuration = {WalljumpMaxDuration};
				// LOG( INFORMATION, "Walljump attempted" );
			}

			// shooting
			if( isKeyPressed( inputs, KC_Space ) && canEntityShoot( collidable ) ) {
				entry.shootInputBuffer = {1};
			}
			if( entry.shootInputBuffer && frameBoundary ) {
				// consume input
				entry.shootInputBuffer = {};

				collidable->shootingTimer = {20};

				// TODO: move this into a shoot projectile function
				// TODO: load gun position dynamicly together with the voxel collection
				vec2 gunOffset = {6, 14};
				if( collidable->faceDirection == CollidableFaceDirection::Left ) {
					gunOffset.x = -gunOffset.x;
				}
				auto gunPosition = collidable->position + gunOffset;
				vec2 velocity = {3, 0};
				if( collidable->faceDirection == CollidableFaceDirection::Left ) {
					velocity.x = -velocity.x;
				}
				if( !emitProjectile( game, gunPosition, velocity ) ) {
					collidable->shootingTimer = {};
				}
			}
		}
		entry.jumpInputBuffer  = processTimer( entry.jumpInputBuffer, dt );
		entry.shootInputBuffer = processTimer( entry.shootInputBuffer, dt );
	}
}

enum class AppFocus { Game, Voxel, TexturePack, Animator, Easing };

#include "PhysicsHitTest.cpp"

#include "Editor/TexturePack/TexturePack.h"
#include "Editor/Animator/Animator.h"

struct EasingState {
	bool initialized;
	ImmediateModeGui gui;
	float t;
	float lastT;

	vec2 position;
	vec2 velocity;
	float duration;

	enum { Count = 100 };
	float yPositions0[Count];
	float yPositions1[Count];
};

struct AppData {
	PlatformServices platform;
	PlatformInfo* platformInfo;

	// app storage for globals
	// globals get assigned pointers to these, so that dll hotloading doesn't break
	IngameLog log;
	TextureMap textureMap;
	ImmediateModeGui guiState;
	string_builder debugPrinter;
	ProfilingTable profilingTable;
	DebugValues debugValues;

	float frameTimeAcc;

	mat4 perspective;
	mat4 orthogonal;

	StackAllocator stackAllocator;
	MatrixStack matrixStack;
	RenderCommands renderer;
	GameSettings settings;
	Font font;
	bool resourcesLoaded;

	float width;
	float height;

	MeshStream debugMeshStream;

	// custom data
	AppFocus focus;
	VoxelState voxelState;
	GameState gameState;
	TexturePackState texturePackState;
	AnimatorState animatorState;
	EasingState easingState;

	bool mouseLocked;
	bool displayDebug;
};

bool loadVoxelCollection( StackAllocator* allocator, StringView filename, VoxelCollection* out );

#include "Editor/TexturePack/TexturePack.cpp"
#include "Editor/Animator/Animator.cpp"

void fillVoxelGridFromImage( VoxelGrid* grid, ImageData image )
{
	assert( image );
	assert( grid );

	zeroMemory( grid->data, countof( grid->data ) );

	for( auto y = 0; y < image.height; ++y ) {
		for( auto x = 0; x < image.width; ++x ) {
			auto index = ( x + y * image.width ) * 4;
			// uint32 r = image.data[index + 0];
			// uint32 g = image.data[index + 1];
			// uint32 b = image.data[index + 2];
			uint32 a = image.data[index + 3];

			// TODO: use color keying
			if( a != 0 ) {
				auto gridIndex        = x + y * grid->width /* + grid->width * grid->height*/;
				grid->data[gridIndex] = 1;
			}
		}
	}
}

VoxelGridTextureMap makeDefaultVoxelGridTextureMap( TextureId texture )
{
	VoxelGridTextureMap result   = {};
	result.texture               = texture;
	const float subdivisionWidth = 1.0f / 6.0f;
	for( int32 i = 0; i < 6; ++i ) {
		auto entry       = &result.entries[i];
		auto left        = i * subdivisionWidth;
		entry->texCoords = makeQuadTexCoords( rectf{left, 0, left + subdivisionWidth, 1} );
	}
	return result;
}
VoxelGridTextureMap makeHeroVoxelGridTextureMap( TextureId texture )
{
	VoxelGridTextureMap result = {};
	// inverse texture width
	auto itw = 1.0f / 32.0f;
	// inverse texture height
	auto ith                    = 1.0f / 24.0f;
	result.texture              = texture;
	result.entries[0].texCoords = makeQuadTexCoords( RectWH( 0.0f, 0.0f, 16 * itw, 24 * ith ) );
	result.entries[1].texCoords =
	    makeQuadTexCoords( RectWH( 17.0f * itw, 0.0f, 3 * itw, 24 * ith ) );
	result.entries[2].texCoords = makeQuadTexCoords( RectWH( 0.0f, 0.0f, 16 * itw, 24 * ith ) );
	result.entries[3].texCoords =
	    makeQuadTexCoords( RectWH( 21.0f * itw, 0.0f, 3 * itw, 24 * ith ) );
	result.entries[4].texCoords = makeQuadTexCoordsCw90( result.entries[1].texCoords );
	result.entries[5]           = result.entries[4];
	return result;
}

GAME_STORAGE PlatformRemapInfo initializeApp( void* memory, size_t size,
                                              PlatformServices platformServices,
                                              PlatformInfo* platformInfo );
INITIALIZE_APP( initializeApp )
{
	// temporarily set GlobalPlatformServices to point to platformServices, so that the constructor
	// of AppData has access to platform services
	// reloadApp will properly initialize GlobalPlatformServices later
	GlobalPlatformServices = &platformServices;

	char* p  = (char*)memory;
	auto app = (AppData*)p;
	new( app ) AppData();

	assert( isAligned( app ) );
	assert( size >= sizeof( AppData ) );
	p += sizeof( AppData );
	size -= sizeof( AppData );

	app->platform     = platformServices;
	app->platformInfo = platformInfo;
	app->guiState     = defaultImmediateModeGui();

	app->stackAllocator         = makeStackAllocator( p, size );
	auto allocator              = &app->stackAllocator;
	auto renderCommandsCapacity = megabytes( 2 );
	app->matrixStack            = makeMatrixStack( allocator, 16 );
	app->renderer = makeRenderCommands( allocator, renderCommandsCapacity, &app->matrixStack );
	app->settings = makeDefaultGameSettings();
	app->width    = 1600;
	app->height   = 900;

	app->debugMeshStream = makeMeshStream( allocator, 4000, 12000, nullptr );
	app->textureMap      = {makeUArray( allocator, TextureMapEntry, 100 )};
	app->debugPrinter    = string_builder( allocateArray( allocator, char, 2048 ), 2048 );
	auto result          = reloadApp( memory, size );

	auto aspect      = app->width / app->height;
	app->perspective = matrixPerspectiveFovProjection( degreesToRadians( 65 ), aspect, -1, 1 );
	app->orthogonal  = matrixOrthogonalProjection( 0, 0, app->width, app->height, -1, 1 );

	// custom data

	// voxel
	auto voxel    = &app->voxelState;
	voxel->camera = makeCamera( {}, {0, 0, 1}, {0, 1, 0} );

	// generate voxels from image

	voxel->meshStream = makeMeshStream( allocator, 4000, 12000, nullptr );

	result.success = result.success && isValid( &app->renderer );
	return result;
}

GAME_STORAGE PlatformRemapInfo reloadApp( void* memory, size_t size );
RELOAD_APP( reloadApp )
{
	PlatformRemapInfo result = {};
	auto app                 = (AppData*)memory;
	assert( isAligned( app ) );
	assert( size >= sizeof( AppData ) );
	assert( app->width > 1 && app->height > 1 );

	result.width                   = (int32)app->width;
	result.height                  = (int32)app->height;
	debug_MeshStream               = &app->debugMeshStream;
	GlobalPlatformServices         = &app->platform;
	GlobalIngameLog                = &app->log;
	GlobalTextureMap               = &app->textureMap;
	GlobalDebugPrinter             = &app->debugPrinter;
	GlobalProfilingTable           = &app->profilingTable;
	debug_Values                   = &app->debugValues;
	app->profilingTable.infosCount = 0;

	result.success    = true;
	result.logStorage = GlobalIngameLog;
	result.textureMap = GlobalTextureMap;
	ImGui             = &app->guiState;
	return result;
}

bool loadVoxelCollectionTextureMapping( StackAllocator* allocator, StringView filename,
                                        VoxelCollection* out )
{
	assert( out );
	auto partition = StackAllocatorPartition::ratio( allocator, 1 );

	auto primary = partition.primary();
	auto scrap   = partition.scrap();

	auto jsonDataMaxSize = kilobytes( 200 );
	auto jsonData        = allocateArray( scrap, char, jsonDataMaxSize );
	auto jsonDataSize =
	    GlobalPlatformServices->readFileToBuffer( filename, jsonData, jsonDataMaxSize );

	size_t jsonDocSize                     = kilobytes( 200 );
	JsonStackAllocatorStruct jsonAllocator = {allocateArray( scrap, char, jsonDocSize ), 0,
	                                          jsonDocSize};
	auto doc =
	    jsonMakeDocument( &jsonAllocator, jsonData, (int32)jsonDataSize, JSON_READER_STRICT );
	if( !doc || !doc.root.getObject() ) {
		return false;
	}
	auto root = doc.root.getObject();

	*out         = {};
	out->texture = GlobalPlatformServices->loadTexture( root["texture"].getString() );
	if( !out->texture ) {
		return false;
	}
	auto textureInfo = getTextureInfo( out->texture );
	auto itw         = 1.0f / textureInfo->width;
	auto ith         = 1.0f / textureInfo->height;

	auto mapping      = root["mapping"].getArray();
	int32 framesCount = 0;
	FOR( animationVal : mapping ) {
		auto animation = animationVal.getObject();
		framesCount += animation["frames"].getArray().size();
	}

	out->frames     = makeArray( primary, VoxelCollection::Frame, framesCount );
	out->frameInfos = makeArray( primary, VoxelCollection::FrameInfo, framesCount );
	out->animations = makeArray( primary, VoxelCollection::Animation, mapping.size() );

	int32 currentFrame = 0;
	for( int32 i = 0, count = mapping.size(); i < count; ++i ) {
		auto animation  = mapping[i].getObject();
		auto dest       = &out->animations[i];
		dest->name      = makeString( primary, animation["name"].getString() );
		dest->range.min = safe_truncate< uint16 >( currentFrame );

		FOR( frameVal : animation["frames"].getArray() ) {
			auto frame     = frameVal.getObject();
			auto destFrame = &out->frames[currentFrame];
			auto destInfo  = &out->frameInfos[currentFrame];
			++currentFrame;

			*destFrame                    = {};
			*destInfo                     = {};
			destInfo->frictionCoefficient = 1;

			for( auto face = 0; face < VF_Count; ++face ) {
				auto faceObject = frame[VoxelFaceStrings[face]].getObject();

				destInfo->textureMap.texture = out->texture;
				serialize( faceObject["rect"], destInfo->textureRegion[face] );
				serialize( faceObject["texCoords"], destInfo->textureMap.entries[face].texCoords );
				destInfo->frictionCoefficient = faceObject["frictionCoefficient"].getFloat( 1 );
				FOR( vert : destInfo->textureMap.entries[face].texCoords.elements ) {
					vert.x *= itw;
					vert.y *= ith;
				}
			}
			serialize( frame["offset"], destFrame->offset );
		}

		dest->range.max = safe_truncate< uint16 >( currentFrame );
	}
	out->voxelsFilename = makeString( primary, root["voxels"].getString() );

	partition.commit();
	return true;
}
bool loadVoxelGridsFromFile( PlatformServices* platform, StringView filename,
                             Array< VoxelGrid > grids )
{
	auto bytesToRead = grids.size() * sizeof( VoxelGrid );
	if( !bytesToRead ) {
		return false;
	}
	if( platform->readFileToBuffer( filename, grids.data(), bytesToRead ) != bytesToRead ) {
		return false;
	}
	return true;
}
aabb getBoundsFromVoxelGrid( const VoxelGrid* grid )
{
	int32 left   = 10000;
	int32 bottom = 10000;
	int32 near   = 10000;
	int32 right  = -10000;
	int32 top    = -10000;
	int32 far    = -10000;

	for( auto z = 0, depth = grid->depth; z < depth; ++z ) {
		for( auto y = 0, height = grid->height; y < height; ++y ) {
			for( auto x = 0, width = grid->width; x < width; ++x ) {
				auto index = x + y * width + z * width * height;
				auto cell  = grid->data[index];
				if( cell != EmptyCell ) {
					left   = min( left, x );
					bottom = min( bottom, height - y );
					near   = min( near, z );
					right  = max( right, x + 1 );
					top    = max( top, height - y + 1 );
					far    = max( far, z + 1 );
				}
			}
		}
	}
	return {left * CELL_WIDTH,  bottom * CELL_HEIGHT, near * CELL_DEPTH,
	        right * CELL_WIDTH, top * CELL_HEIGHT,    far * CELL_DEPTH};
}
bool loadVoxelCollection( StackAllocator* allocator, StringView filename, VoxelCollection* out )
{
	if( !loadVoxelCollectionTextureMapping( allocator, filename, out ) ) {
		return false;
	}
	TEMPORARY_MEMORY_BLOCK( allocator ) {
		auto grids = makeArray( allocator, VoxelGrid, out->frames.size() );
		if( !loadVoxelGridsFromFile( GlobalPlatformServices, out->voxelsFilename, grids ) ) {
			return false;
		}
		for( auto i = 0; i < grids.size(); ++i ) {
			auto grid  = &grids[i];
			auto frame = &out->frames[i];
			auto info  = &out->frameInfos[i];
			TEMPORARY_MEMORY_BLOCK( allocator ) {
				int32 vertices = (int32)getCapacityFor< Vertex >( allocator ) / 2;
				int32 indices  = ( vertices * sizeof( Vertex ) ) / sizeof( uint16 );
				auto stream    = makeMeshStream( allocator, vertices, indices, nullptr );
				generateMeshFromVoxelGrid( &stream, grid, &info->textureMap, VoxelCellSize );
				frame->mesh = GlobalPlatformServices->uploadMesh( toMesh( &stream ) );
				assert( frame->mesh );
				info->bounds = getBoundsFromVoxelGrid( grid );
			}
		}
	}
	return true;
}

bool loadAnimatedVoxelCollection( StackAllocator* allocator, StringView filename,
                                  VoxelCollection* out, Array< const char* > animationNames,
                                  Array< rangeu16 > indices )
{
	assert( animationNames.size() == indices.size() );
	if( !loadVoxelCollection( allocator, filename, out ) ) {
		return false;
	}

	for( auto i = 0; i < animationNames.size(); ++i ) {
		indices[i] = getAnimationRange( out, animationNames[i] );
	}
	return true;
}
bool loadHeroVoxelCollection( StackAllocator* allocator, StringView filename,
                              HeroVoxelCollection* out )
{
	return loadAnimatedVoxelCollection( allocator, filename, &out->voxels,
	                                    makeArrayView( HeroAnimationNames ),
	                                    makeArrayView( &out->idle, 12 ) );
}
bool loadTileSet( StackAllocator* allocator, StringView filename, TileSet* out ) {
	if( !loadVoxelCollection( allocator, filename, &out->voxels ) ) {
		return false;
	}
	out->infos = makeArray( allocator, TileInfo, out->voxels.frameInfos.size() );
	for( auto i = 0; i < out->infos.size(); ++i ) {
		auto dest = &out->infos[i];
		*dest = {};
		dest->frictionCoefficient = out->voxels.frameInfos[i].frictionCoefficient;
	}
	return true;
}

static void processCamera( GameInputs* inputs, GameSettings* settings, Camera* camera, float dt )
{
	auto speed = 20.0f * dt;
	if( isKeyDown( inputs, KC_Key_W ) ) {
		auto dir = camera->look;
		camera->position += dir * speed;
	}
	if( isKeyDown( inputs, KC_Key_S ) ) {
		auto dir = camera->look;
		camera->position += -dir * speed;
	}
	if( isKeyDown( inputs, KC_Key_D ) ) {
		auto dir = camera->right;
		camera->position += dir * speed;
	}
	if( isKeyDown( inputs, KC_Key_A ) ) {
		auto dir = camera->right;
		camera->position += -dir * speed;
	}
	if( isKeyDown( inputs, KC_Space ) ) {
		camera->position.y += speed;
	}
	if( isKeyDown( inputs, KC_Down ) ) {
		camera->position.y += -speed;
	}

	auto cameraDelta = inputs->mouse.delta * settings->mouseSensitivity;
	if( settings->mouseInvertY ) {
		cameraDelta.y = -cameraDelta.y;
	}
	updateCamera( camera, cameraDelta );
}

#include "VoxelEditor.cpp"

const float SafetyDistance = 0.01f;

bool testPointVsAxisAlignedLineSegment( float x, float y, float deltaX, float deltaY,
                                        float segmentX, float segmentStartY, float segmentEndY,
                                        float* t )
{
	*t                 = 0;
	auto relativePos   = segmentX - x;
	auto intersectionT = relativePos / deltaX;
	auto intersectionY = y + deltaY * intersectionT;
	if( intersectionT > 0 && intersectionY >= segmentStartY && intersectionY < segmentEndY ) {
		*t = intersectionT;
		return true;
	}
	return false;
}
struct CollisionInfo {
	float t;
	vec2 normal;
	vec2 push;
};
struct PushPair {
	float push;
	vec2 normal;
};
static bool operator<( const PushPair& a, const PushPair& b ) { return a.push < b.push; }
bool testAabVsAab( vec2arg aPosition, rectfarg a, vec2arg delta, rectfarg b, float maxT,
                   CollisionInfo* info )
{
	rectf sum = {b.left - a.right + SafetyDistance, b.top - a.bottom + SafetyDistance,
	             b.right - a.left - SafetyDistance, b.bottom - a.top - SafetyDistance};

	float intermediate;
	bool collided = false;
	if( isPointInside( sum, aPosition ) ) {
		PushPair leftPush       = {aPosition.x - sum.left, -1, 0};
		PushPair upPush         = {aPosition.y - sum.top, 0, -1};
		PushPair rightPush      = {sum.right - aPosition.x, 1, 0};
		PushPair downPush       = {sum.bottom - aPosition.y, 0, 1};
		const PushPair& minPush = min( min( leftPush, upPush ), min( rightPush, downPush ) );
		info->t                 = -1;
		info->normal            = minPush.normal;
		info->push              = minPush.push * minPush.normal;
		collided                = true;
		if( ( minPush.normal.x < 0 && delta.x < info->push.x )
		    || ( minPush.normal.x > 0 && delta.x > info->push.x )
		    || ( minPush.normal.y < 0 && delta.y < info->push.y )
		    || ( minPush.normal.y > 0 && delta.y > info->push.y ) ) {
			// if we are moving away from collision faster than the push, ignore it
			// the collision will be resolved by just letting the point move
			collided = false;
		}
	} else {
		if( delta.x != 0 ) {
			if( testPointVsAxisAlignedLineSegment( aPosition.x, aPosition.y, delta.x, delta.y,
			                                       sum.left, sum.top, sum.bottom,
			                                       &intermediate ) ) {
				if( intermediate < maxT ) {
					maxT         = intermediate;
					info->t      = intermediate;
					info->normal = {-1, 0};
					collided     = true;
				}
			}
			if( testPointVsAxisAlignedLineSegment( aPosition.x, aPosition.y, delta.x, delta.y,
			                                       sum.right, sum.top, sum.bottom,
			                                       &intermediate ) ) {
				if( intermediate < maxT ) {
					maxT         = intermediate;
					info->t      = intermediate;
					info->normal = {1, 0};
					collided     = true;
				}
			}
		}

		if( delta.y != 0 ) {
			if( testPointVsAxisAlignedLineSegment( aPosition.y, aPosition.x, delta.y, delta.x,
			                                       sum.top, sum.left, sum.right, &intermediate ) ) {
				if( intermediate < maxT ) {
					maxT         = intermediate;
					info->t      = intermediate;
					info->normal = {0, -1};
					collided     = true;
				}
			}
			if( testPointVsAxisAlignedLineSegment( aPosition.y, aPosition.x, delta.y, delta.x,
			                                       sum.bottom, sum.left, sum.right,
			                                       &intermediate ) ) {
				if( intermediate < maxT ) {
					maxT         = intermediate;
					info->t      = intermediate;
					info->normal = {0, 1};
					collided     = true;
				}
			}
		}
	}

	return collided;
}

struct CollisionResult {
	CollidableRef collision;
	CollisionInfo info;
	inline explicit operator bool() const { return static_cast< bool >( collision ); }
};
static CollisionResult detectCollisionVsTileGrid( CollidableComponent* collidable, vec2arg velocity,
                                                  TileGrid grid, recti tileGridRegion, float maxT )
{
	using namespace GameConstants;

	CollisionResult result = {};
	result.info.t          = maxT;
	for( auto y = tileGridRegion.top; y < tileGridRegion.bottom; ++y ) {
		for( auto x = tileGridRegion.left; x < tileGridRegion.right; ++x ) {
			auto index = grid.index( x, y );
			auto tile  = grid[index];
			if( tile ) {
				rectf tileBounds   = RectWH( x * TileWidth, y * TileHeight, TileWidth, TileHeight );
				CollisionInfo info = {};
				if( testAabVsAab( collidable->position, collidable->aab, velocity, tileBounds,
				                  result.info.t, &info ) ) {
					if( info.t < result.info.t ) {
						result.collision.setTile( index );
						result.info = info;
					}
				}
			}
		}
	}
	return result;
}
static CollisionResult detectCollisionVsDynamics( CollidableComponent* collidable, vec2arg velocity,
                                                  Array< CollidableComponent > dynamics,
                                                  float maxT )
{
	using namespace GameConstants;

	CollisionResult result = {};
	result.info.t          = maxT;
	FOR( dynamic : dynamics ) {
		CollisionInfo info = {};
		if( testAabVsAab( collidable->position, collidable->aab, velocity,
		                  translate( dynamic.aab, dynamic.position ), result.info.t, &info ) ) {
			if( info.t < result.info.t ) {
				result.collision.setDynamic( indexof( dynamics, dynamic ), dynamic.entity );
				result.info = info;
			}
		}
	}
	return result;
}

static recti getSweptTileGridRegion( const CollidableComponent* collidable, vec2arg velocity )
{
	using namespace GameConstants;
	auto currentPlayerAab = translate( collidable->aab, collidable->position );
	// sweep the bounding box along collidable velocity to get bounding box of the sweep region
	auto entrySweptAab = sweep( currentPlayerAab, velocity );
	// turn the swept bounding box to tile grid region that we need to check for collisions
	// tiles outside this region can't possibly collide with the collidable
	auto tileGridRegion = RectTiledIndex( entrySweptAab, TileWidth, TileHeight );
	tileGridRegion      = RectMin( tileGridRegion, recti{0, 0, GAME_MAP_WIDTH, GAME_MAP_HEIGHT} );
	return tileGridRegion;
}

CollisionResult findCollision( CollidableComponent* collidable, vec2arg velocity, TileGrid grid,
                               recti tileGridRegion, Array< CollidableComponent > dynamics,
                               float maxT, bool dynamic )
{
	auto collision = detectCollisionVsTileGrid( collidable, velocity, grid, tileGridRegion, maxT );
	if( !dynamic && ( !collision || collision.info.t > 0 ) ) {
		auto dynamicCollision = detectCollisionVsDynamics( collidable, velocity, dynamics, maxT );
		if( !collision || ( dynamicCollision && dynamicCollision.info.t < collision.info.t ) ) {
			if( dynamicCollision.info.t < 0 ) {
				// calculate max movement in given push direction by doing another round of
				// tile grid collision detection
				auto safety      = dynamicCollision.info.normal * SafetyDistance;
				auto adjustedVel = dynamicCollision.info.push + safety;
				auto sweptRegion = getSweptTileGridRegion( collidable, adjustedVel );
				auto maxMovementCollision =
				    detectCollisionVsTileGrid( collidable, adjustedVel, grid, sweptRegion, 1 );
				if( maxMovementCollision ) {
					if( maxMovementCollision.info.t > 0 ) {
						// TODO: we are being squished between dynamic and tile, handle?

						collision      = dynamicCollision;
						auto newSafety = maxMovementCollision.info.normal * SafetyDistance;
						// recalculate push by taking into account the old and new
						// safetyDistances
						collision.info.push =
						    adjustedVel * maxMovementCollision.info.t - safety + newSafety;
					} else {
						// TODO: we are being squished between dynamic and tile, handle?
					}
				} else {
					collision = dynamicCollision;
				}
			} else {
				collision = dynamicCollision;
			}
		}
	}
	return collision;
}

// TODO: rename function, since CollidableComponent does way more than colliding
void processCollidables( Array< CollidableComponent > entries, TileGrid grid,
                         Array< TileInfo > infos, Array< CollidableComponent > dynamics,
                         bool dynamic, float dt, bool frameBoundary )
{
	using namespace GameConstants;
	constexpr const float eps = 0.00001f;

	// TODO: air movement should be accelerated instead of instant
	FOR( entry : entries ) {
		auto oldPosition = entry.position;

		entry.walljumpWindow   = processTimer( entry.walljumpWindow, dt );
		entry.walljumpDuration = processTimer( entry.walljumpDuration, dt );
		auto wasAlive          = isCountdownActive( entry.aliveCountdown );
		entry.aliveCountdown   = processTimer( entry.aliveCountdown, dt );
		if( wasAlive && isCountdownTimerExpired( entry.aliveCountdown ) ) {
			entry.flags |= CollidableComponent::DeathFlag;
		}

		// animations
		entry.currentFrame += entry.animationSpeed * dt;
		if( !entry.grounded ) {
			entry.animationLockTimer = {};
		}
		entry.animationLockTimer = processTimer( entry.animationLockTimer, dt );

		entry.shootingTimer = processTimer( entry.shootingTimer, dt );

		// make entry move away from the wall if a walljump was executed
		if( entry.walljumpDuration ) {
			// move away only for 4 frames in total
			if( entry.walljumpDuration.value > WalljumpMoveThreshold ) {
				if( entry.walljumpLeft() ) {
					entry.velocity.x = -MovementSpeed;
				} else {
					entry.velocity.x = MovementSpeed;
				}
			}
		}

		// update
		if( frameBoundary ) {
			if( !entry.grounded || entry.movement != CollidableMovement::Grounded ) {
				entry.velocity.y += Gravity * entry.gravityModifier;
				entry.velocity.y -= entry.airFrictionCoeffictient * entry.velocity.y;
				if( entry.wallslideCollidable && entry.velocity.y > 0 ) {
					// apply wallslide friction only if falling
					auto friction = getFrictionCoefficitonFromCollidableRef(
					    dynamics, grid, infos, entry.wallslideCollidable );
					entry.velocity.y -= friction * WallslideFrictionCoefficient * entry.velocity.y;
				}
			}
		}
		auto oldVelocity = entry.velocity;

		processSpatialState( &entry, dt );

		auto vdt        = dt;
		recti mapBounds = {0, 0, GAME_MAP_WIDTH, GAME_MAP_HEIGHT};

		auto isGroundBased = entry.movement == CollidableMovement::Grounded
		                     && floatEqSoft( entry.bounceModifier, 1.0f );
		// find position of gap and whether we should squeeze into it this frame
		// entry must have ground based movement and bounceModifier of 1 (sliding behavior) for
		// gap fitting
		if( isGroundBased && oldVelocity.x != 0 && oldVelocity.y != 0 ) {
			auto entryGridX = (int32)floor( entry.position.x / TileWidth );
			auto entryGridY = (int32)floor( entry.position.y / TileHeight );
			auto entryGridYNext =
			    (int32)floor( ( entry.position.y + entry.velocity.y * vdt ) / TileHeight );
			if( entryGridY != entryGridYNext
			    && ( entryGridY >= 0 && entryGridY < GAME_MAP_HEIGHT ) ) {
				if( oldVelocity.y > 0 ) {
					++entryGridY;
					++entryGridYNext;
				}

				bool gapExists = false;
				int32 gapTileIndex;
				int32 gapTileX = entryGridX;
				int32 gapTileY = entryGridY;
				if( oldVelocity.x < 0 ) {
					gapTileX -= 1;
				} else {
					gapTileX += 1;
				}
				if( gapTileX >= 0 && gapTileX < GAME_MAP_WIDTH ) {
					int32 step = 1;
					if( oldVelocity.y < 0 ) {
						step = -1;
					}
					for( gapTileY = entryGridY; gapTileY != entryGridYNext; gapTileY += step ) {
						if( gapTileY < 0 || gapTileY >= GAME_MAP_HEIGHT ) {
							continue;
						}
						gapTileIndex = grid.index( gapTileX, gapTileY );
						auto gap     = grid[gapTileIndex];
						if( !gap ) {
							auto neighborTileX = gapTileX;
							auto neighborTileY = gapTileY;
							if( oldVelocity.y > 0 ) {
								neighborTileY -= 1;
							} else {
								neighborTileY += 1;
							}
							if( !isPointInside( mapBounds, neighborTileX, neighborTileY ) ) {
								continue;
							}
							auto neighborIndex = grid.index( neighborTileX, neighborTileY );
							auto neighbor      = grid[neighborIndex];
							// check whether neighbor is solid
							if( neighbor ) {
								// gap exists and is valid candidate for squeezing in
								gapExists = true;
								break;
							}
						}
					}
				}

				if( gapExists ) {
					rectf gapBounds = RectWH( gapTileX * TileWidth, gapTileY * TileHeight,
					                          TileWidth, TileHeight );
#if 0
						auto nextPlayerAab =
						    translate( entry.aab, entry.position + entry.velocity * vdt );
						assert( abs( velocity.x ) <= TileWidth );

						auto currentPlayerAab = translate( entry.aab, entry.position );
						auto yDelta = gapBounds.top - currentPlayerAab.top;
						entry.position.y += yDelta;
						float xDelta;
						if( velocity.x < 0 ) {
							auto xBoundary = max( gapBounds.left, nextPlayerAab.left );
							xDelta         = xBoundary - currentPlayerAab.left;
						} else {
							auto xBoundary = min( gapBounds.right, nextPlayerAab.right );
							xDelta         = xBoundary - currentPlayerAab.right;
						}
						entry.position.x += xDelta;

						auto deltaLength        = sqrt( xDelta * xDelta + yDelta * yDelta );
						float velocityMagnitude = length( entry.velocity );
						auto ratio              = deltaLength / velocityMagnitude;
						vdt -= ratio;
#else
					// instead of calculating velocity deltas, calculate t value of how
					// much to apply velocity to be inside gap
					auto yDelta = gapBounds.top - ( entry.aab.top + entry.position.y );
					auto t      = yDelta / oldVelocity.y;
					entry.position += oldVelocity * t;
					vdt -= t;
#endif
				}
			}
		}

		// collision detection begins here
		auto velocity = entry.velocity * vdt;

		// broadphase
		// get the region of tiles that we actually touch when moving along velocity
		auto tileGridRegion = getSweptTileGridRegion( &entry, velocity );

		// check grounded state of entry
		if( entry.grounded ) {
			switch( entry.grounded.type ) {
				case CollidableRef::None: {
					break;
				}
				case CollidableRef::Tile: {
					assert_init( auto tile = grid[entry.grounded.index], tile );
					auto p = grid.coordinatesFromIndex( entry.grounded.index );
					auto tileBounds =
					    RectWH( p.x * TileWidth, p.y * TileHeight, TileWidth, TileHeight );
					CollisionInfo info = {};
					if( !testAabVsAab( entry.position, entry.aab, {0, 1}, tileBounds, 1, &info )
					    || info.t > SafetyDistance + eps || info.t < 0 ) {
						entry.grounded.clear();
					}
					break;
				}
				case CollidableRef::Dynamic: {
					auto other         = getDynamicFromCollidableRef( dynamics, entry.grounded );
					CollisionInfo info = {};
					if( !other
					    || !testAabVsAab( entry.position, entry.aab, {0, 1},
					                      translate( other->aab, other->position ), 1, &info )
					    || info.t > SafetyDistance + eps || info.t < 0 ) {
						entry.grounded.clear();
					}
					break;
				}
					InvalidDefaultCase;
			}
			if( !entry.grounded ) {
				// try and find a static entry as new ground
				auto findNewStaticGround = [&]() {
					auto bottom = MIN( tileGridRegion.bottom + 1, mapBounds.bottom );
					for( int32 y = tileGridRegion.top; y < bottom; ++y ) {
						for( int32 x = tileGridRegion.left; x < tileGridRegion.right; ++x ) {
							auto index = grid.index( x, y );
							auto tile  = grid[index];
							if( tile ) {
								auto tileBounds =
								    RectWH( x * TileWidth, y * TileHeight, TileWidth, TileHeight );
								CollisionInfo info = {};
								if( testAabVsAab( entry.position, entry.aab, {0, 1}, tileBounds, 1,
								                  &info ) ) {
									if( info.normal.y < 0 && info.t >= 0
									    && info.t < SafetyDistance + eps ) {
										entry.position.y += info.t - SafetyDistance;
										entry.grounded.setTile( index );
										return;
									}
								}
							}
						}
					}
				};
				findNewStaticGround();
			}
			if( !entry.grounded ) {
				// try and find a dynamic entry as new ground
				auto findNewDynamicGround = [&]() {
					FOR( other : dynamics ) {
						if( &entry == &other ) {
							continue;
						}
						CollisionInfo info = {};
						if( testAabVsAab( entry.position, entry.aab, {0, 1},
						                  translate( other.aab, other.position ), 1, &info ) ) {
							if( info.normal.y < 0 && info.t >= 0
							    && info.t < SafetyDistance + eps ) {
								entry.position.y += info.t - SafetyDistance;
								entry.grounded.setDynamic( indexof( dynamics, other ),
								                           other.entity );
								return;
							}
						}
					}
				};
				findNewDynamicGround();
			}

			if( !entry.grounded ) {
				setSpatialState( &entry, SpatialState::FallingOff );
			}
		}

		if( entry.grounded && entry.grounded.type == CollidableRef::Dynamic ) {
			if( auto dynamicGround = getDynamicFromCollidableRef( dynamics, entry.grounded ) ) {
				velocity += dynamicGround->positionDelta;
			}
		}
		// recalculate tileGridRegion with new velocity
		tileGridRegion = getSweptTileGridRegion( &entry, velocity );

		// remaining is the amount of "frame time" we want to move this frame, 1 == full one frame
		// worth of movement. If entry is not alive for the whole frame, we want to move for the
		// amount it will be alive for
		float remaining = 1;
		if( wasAlive ) {
			remaining = min( 1.0f, entry.aliveCountdown.value );
		}

		entry.lastCollision.clear();
		constexpr const auto maxIterations = 4;
		for( auto iterations = 0; iterations < maxIterations && remaining > 0.0f; ++iterations ) {
			auto collision = findCollision( &entry, velocity, grid, tileGridRegion, dynamics,
			                                remaining, dynamic );

			auto normal = collision.info.normal;
			auto t      = collision.info.t;
			if( entry.useForcedNormal() ) {
				normal = entry.forcedNormal;
				if( collision.info.normal.x < 0 ) {
					normal.x = -normal.x;
				}
				if( collision.info.normal.y < 0 ) {
					normal.y = -normal.y;
				}
			}
			if( t > 0 ) {
				bool processCollision = true;
				if( isGroundBased ) {
					// check whether we just fell off and are trying to get back on top of previous
					// ground
					if( entry.spatialState == SpatialState::FallingOff && !floatEqZero( velocity.x )
					    && velocity.y > 0 ) {

						// TODO: implement
					}
				}

				if( processCollision ) {
					entry.position += velocity * t;
					if( collision ) {
						entry.position += normal * SafetyDistance;
					}
					remaining -= t;
				}
			} else {
				entry.position += collision.info.push + normal * SafetyDistance;
			}

			if( normal.y < 0 ) {
				entry.grounded = collision.collision;
				setSpatialState( &entry, SpatialState::Grounded );
			}

#ifdef GAME_DEBUG
			if( entry.grounded ) {
				debug_Values->groundPosition = entry.position.y;
			}
			if( !entry.grounded && velocity.y < 0 ) {
				debug_Values->jumpHeight = entry.position.y - debug_Values->groundPosition;
			}
			if( normal.y < 0 ) {
				if( debug_Values->jumpHeight < debug_Values->maxJumpHeight ) {
					debug_Values->maxJumpHeight = debug_Values->jumpHeight;
				}
				debug_Values->jumpHeightError =
				    abs( debug_Values->jumpHeight - debug_Values->lastJumpHeight );
				debug_Values->lastJumpHeight = debug_Values->jumpHeight;
			}
#endif  // defined( GAME_DEBUG )

			// response
			if( collision ) {
				entry.lastCollision = collision.collision;
			}
			if( entry.response == CollidableResponse::FullStop ) {
				if( collision ) {
					break;
				}
			} else if( entry.response == CollidableResponse::Bounce ) {
				if( collision ) {
					// reflect velocity based on the collision normal
					entry.velocity -= entry.bounceModifier * normal * dot( normal, entry.velocity );
					velocity -= entry.bounceModifier * normal * dot( normal, velocity );
				}
				if( collision && normal.x != 0 && normal.y == 0 && !entry.grounded
				    && ( ( oldVelocity.x >= 0 ) == ( normal.x < 0 ) ) ) {

					// we are wallsliding
					setFlagCond( entry.flags, CollidableComponent::WalljumpLeft, ( normal.x < 0 ) );
					entry.walljumpWindow = {WalljumpWindowDuration};
					entry.wallslideCollidable = collision.collision;
				}
				auto lengthSquared = dot( entry.velocity, entry.velocity );
				if( t >= 0 && ( t <= 0.000001f || lengthSquared < eps ) ) {
					break;
				}
			} else {
				InvalidCodePath();
			}
		}
		if( remaining > 0.0f ) {
			entry.position += velocity * remaining;
		}
		entry.positionDelta = entry.position - oldPosition;

		if( !entry.walljumpWindow ) {
			entry.wallslideCollidable.clear();
		}
		if( entry.grounded ) {
			entry.walljumpWindow = {};
			entry.wallslideCollidable.clear();
		}
		// check that we are upholding safety distance to collidables in velocity direction
		if( isGroundBased ) {
			auto wasAlreadySliding = (bool)entry.wallslideCollidable;
			if( !floatEqZero( entry.velocity.x ) || entry.wallslideCollidable ) {
				vec2 searchDir = {1, 0};
				if( entry.velocity.x < 0 ) {
					searchDir = {-1, 0};
				}
				if( entry.wallslideCollidable ) {
					if( !entry.walljumpLeft() ) {
						searchDir = {-1, 0};
					}
				}
				auto region = getSweptTileGridRegion( &entry, searchDir );
				auto collision =
				    findCollision( &entry, searchDir, grid, region, dynamics, 1, dynamic );
				if( !collision || collision.info.normal.y != 0 ) {
					// clear wallsliding
					entry.walljumpWindow = {};
					entry.wallslideCollidable.clear();
				} else {
					if( !entry.grounded && wasAlreadySliding ) {
						// reset wallsliding
						// TODO: do we want to reset the walljump duration here?
						// that means wallsliding will behave like a toggle (not requiring holding a
						// button down)
						entry.walljumpWindow = {WalljumpWindowDuration};
						entry.wallslideCollidable = collision.collision;
					}
				}
				// make sure that we are SafetyDistance away from the wall we are sliding against
				if( collision && collision.info.t < SafetyDistance ) {
					auto diff = SafetyDistance - collision.info.t;
					entry.position.x += diff * collision.info.normal.x;
					entry.velocity.x = 0;
				}
			}

			// adjust face direction if we are wallsliding
			if( entry.walljumpWindow ) {
				if( entry.walljumpLeft() ) {
					entry.faceDirection = CollidableFaceDirection::Left;
				} else {
					entry.faceDirection = CollidableFaceDirection::Right;
				}
			}
		}
	}

}

static void doCollisionDetection( Room* room, CollidableSystem* system, float dt,
                                  bool frameBoundary )
{
	using namespace GameConstants;
	assert( room );

	auto grid = room->layers[RL_Main].grid;

	processCollidables( system->dynamicEntries(), grid, room->tileSet->infos, {}, true, dt,
	                    frameBoundary );
	processCollidables( system->staticEntries(), grid, room->tileSet->infos,
	                    system->dynamicEntries(), false, dt, frameBoundary );
}

static StringView detailedDebugOutput( AppData* app, char* buffer, int32 size )
{
	auto builder  = string_builder( buffer, size );
	auto renderer = &app->renderer;

	auto info = app->platformInfo;

	builder << "FrameTime: " << info->totalFrameTime
			<< "\nGameTime: " << info->gameTime
			<< "\nRenderTime: " << info->renderTime
	        << "\nAverage FrameTime:" << info->averageFrameTime
	        << "\nAverage GameTime:" << info->averageGameTime
	        << "\nAverage RenderTime:" << info->averageRenderTime
	        << "\nMin FrameTime: " << info->minFrameTime
	        << "\nMax FrameTime: " << info->maxFrameTime;

	builder << "\n\n";
	builder.println( "Draw Calls: {}\nVertices: {}\nIndices: {}", info->drawCalls, info->vertices,
	                 info->indices );

	builder << '\n'
	        << "\nFPS: " << info->fps << "\nAverage Fps:" << info->averageFps
	        << "\nMin Fps: " << info->minFps << "\nMax Fps: " << info->maxFps;

	builder << '\n'
	        << "\nLight Position: " << renderer->lightPosition.x << ", "
	        << renderer->lightPosition.y << ", " << renderer->lightPosition.z;
	if( info->recordingInputs ) {
		builder << "\nRecording Inputs: " << info->recordingFrame;
	}
	if( info->replayingInputs ) {
		builder << "\nReplaying Inputs: " << info->recordingFrame;
	}

	builder << "\njumpHeight: " << debug_Values->jumpHeight
	        << "\nmaxJumpHeight: " << debug_Values->maxJumpHeight
	        << "\nJumpHeightError: " << debug_Values->jumpHeightError;

	builder << "\nWallJumpTimer: " << app->gameState.player->walljumpWindow.value
	        << "\nWallJumpDuration: " << app->gameState.player->walljumpDuration.value;

	builder << '\n' << '\n';

	auto camera = &app->voxelState.camera;
	builder << "Camera:\n";
	builder.println( "Position: {}", camera->position );
	builder.println( "Look: {}", camera->look );
	builder.println( "Up: {}", camera->up );

	// output debug player state
	auto player = app->gameState.player;
	builder.println( "CollidableComponent: {}\nCollidableComponent Time: {}",
	                 getSpatialStateString( player->spatialState ), player->spatialStateTimer );
	builder.println( "Player pos: {}\nPlayer velocity: {}", player->position, player->velocity );
	builder.println( "Camera follow: {:.2}", app->gameState.cameraFollowRegion );

	builder << '\n' << '\n';
	builder.println( "Malloc Allocated: {}\nMalloc Free: {}\nMalloc Footprint: {}",
	                 info->mallocAllocated, info->mallocFree, info->mallocFootprint );

	return asStringView( builder );
}

#define TILE_CELLS_X 16
#define TILE_CELLS_Y 16
#define TILE_CELLS_Z 16
#define TILE_WIDTH ( CELL_WIDTH * TILE_CELLS_X )
#define TILE_HEIGHT ( CELL_HEIGHT * TILE_CELLS_Y )
#define TILE_DEPTH ( CELL_DEPTH * TILE_CELLS_Z )

static void showGameDebugGui( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto renderer  = &app->renderer;
	auto font      = &app->font;
	auto game      = &app->gameState;
	auto gui       = &game->debugGui;
	auto fadeSpeed = 0.25f * dt;
	auto imgui     = &gui->debugGuiState;

	if( !focus ) {
		return;
	}
	imguiBind( imgui );
	if( !gui->initialized ) {
		gui->debugGuiState = defaultImmediateModeGui();
		imguiLoadDefaultStyle( imgui, &app->platform );
		gui->mainDialog        = imguiGenerateContainer( &gui->debugGuiState );
		gui->debugOutputDialog = imguiGenerateContainer( &gui->debugGuiState, {200, 0, 600, 100} );
		gui->initialized       = true;
	}
	setProjection( renderer, ProjectionType::Orthogonal );
	if( isKeyPressed( inputs, KC_Oem_5 /*Key above tab*/ ) ) {
		auto mainDialog = imguiGetContainer( imgui, gui->mainDialog );
		if( mainDialog->isHidden() ) {
			gui->show = true;
			mainDialog->setHidden( false );
		} else {
			gui->show = !gui->show;
		}
	}
	gui->fadeProgress = clamp( gui->fadeProgress + fadeSpeed * ( ( gui->show ) ? ( 1 ) : ( -1 ) ) );
	if( !gui->show || gui->fadeProgress <= 0
	    || imguiGetContainer( imgui, gui->mainDialog )->isHidden() ) {
		renderer->color = Color::Black;
		reset( font );
		font->align = FontAlign::Right;
		static_string_builder< 64 > str;
		str.print( "Press [{}] to open debug dialog", toString( KC_Oem_5 ) );
		renderText( renderer, font, asStringView( str ), {0, 0, app->width, app->height} );
		return;
	}
	renderer->color = setAlpha( 0xFFFFFFFF, gui->fadeProgress );
	rectf guiBounds = {0, 0, app->width, app->height};
	imguiBind( imgui, renderer, font, inputs, app->stackAllocator.ptr, guiBounds );
	if( imguiDialog( "Debug Window", gui->mainDialog ) ) {
		if( imguiButton( "Reset Player Position" ) ) {
			game->player->position = {};
		}
		imguiCheckbox( "Lighting", &game->lighting );
		imguiCheckbox( "Camera turning", &app->settings.cameraTurning );
		imguiCheckbox( "View collision boxes", &app->gameState.debugCollisionBoxes );
	}
	if( imguiDialog( "Debug Output", gui->debugOutputDialog ) ) {
		imguiCheckbox( "Detailed Debug Output", &app->displayDebug );
		if( app->displayDebug ) {
			char buffer[1000];
			imguiText( detailedDebugOutput( app, buffer, countof( buffer ) ) );
		}

		// display held down keys
		SmallUninitializedArray< VirtualKeyEnumValues, 10 > keys;
		getDownKeys( inputs, &keys );
		FOR( key : keys ) {
			debugPrintln( "{:x} {} {}", (int32)key, toVirtualKeyString( key ), toString( key ) );
		}

		imguiText( debugPrintGetString() );
	}
	imguiUpdate( dt );
	imguiFinalize();
}
static void processGameCamera( AppData* app, float dt, bool frameBoundary )
{
	auto game              = &app->gameState;
	auto settings          = &app->settings;
	auto player            = game->player;
	vec3 cameraTranslation = {};
	auto playerAabb        = translate( player->aab, player->position );
	auto follow            = game->cameraFollowRegion;
	auto camera            = &game->camera;
	bool turnCamera        = false;

	if( playerAabb.right > follow.right ) {
		cameraTranslation.x = playerAabb.right - follow.right;
		if( settings->cameraTurning ) {
			if( !camera->turnTimer || !camera->turnedRight ) {
				turnCamera          = true;
				camera->turnedRight = true;
			}
		}
	} else if( playerAabb.left < follow.left ) {
		cameraTranslation.x = playerAabb.left - follow.left;
		if( settings->cameraTurning ) {
			if( !camera->turnTimer || camera->turnedRight ) {
				turnCamera          = true;
				camera->turnedRight = false;
			}
		}
	}
	if( playerAabb.bottom > follow.bottom ) {
		cameraTranslation.y = playerAabb.bottom - follow.bottom;
	} else if( playerAabb.top < follow.top ) {
		cameraTranslation.y = playerAabb.top - follow.top;
	}
	camera->position.x += cameraTranslation.x * VoxelCellSize.x;
	camera->position.y -= cameraTranslation.y * VoxelCellSize.y;
	game->cameraFollowRegion = translate( game->cameraFollowRegion, cameraTranslation.xy );
	if( settings->cameraTurning ) {
		if( turnCamera ) {
			camera->turnTimer = {1};
			camera->prevLook  = camera->look;
			camera->nextLook =
			    normalize( Vec3( player->position.x, camera->position.y, 0 ) - camera->position );
		}
		if( camera->turnTimer ) {
			auto t            = quadratic( camera->turnTimer.value, 0 );
			auto dir          = normalize( lerp( t, camera->nextLook, camera->prevLook ) );
			*camera           = cameraLookDirection( *camera, dir );
			camera->turnTimer = processTimer( camera->turnTimer, dt * 0.05f );
			if( !frameBoundary ) {
				debugPrintln( "camera prev look: {}", camera->prevLook );
				debugPrintln( "camera next look: {}", camera->nextLook );
				debugPrintln( "camera dir: {}", dir );
				debugPrintln( "camera turn timer: {}", camera->turnTimer.value );
			}
		}
	} else {
		*camera = cameraLookDirection( *camera, {0, 0, 1} );
	}
}

Array< VoxelCollection::Frame > getHeroActionAnimation( HeroVoxelCollection* collection,
                                                        CollidableComponent* collidable, float dt )
{
	auto animation = collidable->currentAnimation;
	bool shooting = (bool)collidable->shootingTimer;
	if( isCountdownTimerExpired( collidable->animationLockTimer ) ) {
		animation = ( shooting ) ? collection->idleShoot : collection->idle;
		collidable->animationSpeed = 0.1f;
		if( collidable->grounded ) {
			if( !shooting && collidable->spatialStateTimer <= 10.0f ) {
				collidable->animationSpeed = 0.2f;
				animation = collection->landing;
			} else {
				if( !shooting && collidable->faceDirection != collidable->prevFaceDirection ) {
					collidable->animationLockTimer = {5.0f};
					animation = collection->turn;
				} else {
					if( collidable->velocity.x != 0 ) {
						animation = ( shooting ) ? collection->walkShoot : collection->walk;
					}
				}
			}
		} else {
			if( collidable->walljumpWindow ) {
				animation = ( shooting ) ? collection->wallslideShoot : collection->wallslide;
			} else {
				if( collidable->velocity.y < 0 ) {
					animation = ( shooting ) ? collection->jumpRisingShoot : collection->jumpRising;
				} else {
					animation = ( shooting ) ? collection->jumpFallingShoot : collection->jumpFalling;
				}
			}
		}
		if( collidable->currentAnimation != animation ) {
			collidable->currentFrame = 0;
		}
		collidable->currentAnimation = animation;
		collidable->prevFaceDirection = collidable->faceDirection;
	}
	return getAnimationFrames( &collection->voxels, animation );
}

template < class Container >
void unordered_remove_if_helper( Container& container, Array< EntityHandle > handles )
{
	unordered_remove_if( container, [handles]( typename const Container::reference component ) {
		return exists( handles, component.entity );
	} );
}
template < class GenericSystem >
void removeEntities( GenericSystem* system, Array< EntityHandle > handles )
{
	unordered_remove_if_helper( system->entries, handles );
}
void removeEntities( CollidableSystem* system, Array< EntityHandle > handles )
{
	erase_if( system->entries, [system, handles]( const CollidableComponent& component ) {
		if( exists( handles, component.entity ) ) {
			if( !component.dynamic() ) {
				--system->entriesCount;
			}
			return true;
		}
		return false;
	} );
}
void processEntityRemovalQueue( GameState* game )
{
	assert( game );
	if( game->entityRemovalQueue.size() ) {
		auto handles = makeArrayView( game->entityRemovalQueue );
		removeEntities( &game->controlSystem, handles );
		removeEntities( &game->collidableSystem, handles );
		game->entityRemovalQueue.clear();
	}
}
void removeEntity( GameState* game, EntityHandle handle )
{
	append_unique( game->entityRemovalQueue, handle );
}

static void doGame( AppData* app, GameInputs* inputs, bool focus, float dt, bool frameBoundary,
                    bool enableRender = true )
{
	PROFILE_FUNCTION();

	auto renderer = &app->renderer;
	// auto font      = &app->font;
	auto game     = &app->gameState;
	auto settings = &app->settings;

	if( !game->initialized ) {
		auto allocator               = &app->stackAllocator;
		game->particleSystem         = makeParticleSystem( allocator, 200 );
		game->particleSystem.texture = app->platform.loadTexture( "Data/Images/dust.png" );
		loadVoxelCollection( allocator, "Data/voxels/projectile.json", &game->projectile );

		loadTileSet( allocator, "Data/voxels/default_tileset.json", &app->gameState.tileSet );
		loadHeroVoxelCollection( allocator, "Data/voxels/hero.json",
		                         &app->gameState.heroVoxelCollection );
		auto maxEntities         = 10;
		game->entityHandles      = makeHandleManager();
		game->collidableSystem   = makeCollidableSystem( allocator, maxEntities );
		game->controlSystem      = makeControlSystem( allocator, maxEntities );
		game->entityRemovalQueue = makeUArray( allocator, EntityHandle, maxEntities );

		auto playerHandle      = addEntity( &game->entityHandles );
		game->player           = addCollidableComponent( &game->collidableSystem, playerHandle );
		game->player->aab      = {-5, 0, 5, 24};
		game->player->position = {16 * 2};
		game->player->movement = CollidableMovement::Grounded;
		game->player->response = CollidableResponse::Bounce;
		game->player->airFrictionCoeffictient = 0.025f;
		addControlComponent( &game->controlSystem, playerHandle );

		auto addMovingPlatform = [&]( vec2arg pos ) {
			auto handle        = addEntity( &game->entityHandles );
			auto platform      = addCollidableComponent( &game->collidableSystem, handle, true );
			platform->aab      = {-8, 0, 8, 28};
			platform->position = pos;
			platform->movement = CollidableMovement::Straight;
			platform->response = CollidableResponse::Bounce;
			platform->bounceModifier = 2;
			platform->setForcedNormal( {1, 0} );
			platform->velocity        = {1.0f, 0};
			platform->gravityModifier = 0;
		};
		addMovingPlatform( {16 * 3} );
		addMovingPlatform( {16 * 8} );

		// auto followWidth         = app->width * 0.25f;
		// auto followHeight        = app->height * 0.25f;
		game->camera             = makeGameCamera( {0, -50, -200}, {0, 0, 1}, {0, 1, 0} );
		game->cameraFollowRegion = {-25, -50, 25, 50};
		game->useGameCamera      = true;
		game->lighting           = false;

		game->room = debugGetRoom( allocator, &game->tileSet );

		game->initialized = true;
	}

	if( !focus ) {
		return;
	}
	auto matrixStack = renderer->matrixStack;

	processCamera( inputs, settings, &app->voxelState.camera,
	               isKeyDown( inputs, KC_Shift ) ? dt : ( dt * 0.25f ) );

	if( !frameBoundary ) {
		debugPrintln( "Active entities: {}", game->collidableSystem.entries.size() );
	}

	if( !frameBoundary ) {
		if( isKeyPressed( inputs, KC_Key_H ) ) {
			game->paused = !game->paused;
		}
		if( isKeyPressed( inputs, KC_Key_0 ) ) {
			game->useGameCamera = !game->useGameCamera;
		}
	}

	// mouse lock
	if( isKeyPressed( inputs, KC_Key_L ) ) {
		app->mouseLocked = !app->mouseLocked;
	}
	if( app->focus == AppFocus::Game ) {
		inputs->mouse.locked = app->mouseLocked;
	}

	processGameCamera( app, dt, frameBoundary );
	processControlSystem( game, &game->controlSystem, &game->collidableSystem, inputs, dt,
	                      frameBoundary );
	doCollisionDetection( &game->room, &game->collidableSystem, dt, frameBoundary );
	processParticles( &game->particleSystem, dt );

	// emitting particles
	// TODO: factor this out into a processing function
	FOR( entry : game->collidableSystem.entries ) {
		// wallslide particles
		if( entry.wallslideCollidable && entry.movement == CollidableMovement::Grounded ) {
			auto feetPosition = entry.position;
			feetPosition.y += height( entry.aab );
			if( entry.walljumpLeft() ) {
				feetPosition.x += entry.aab.right;
			} else {
				feetPosition.x += entry.aab.left;
			}
			vec2 searchDir = {1, 0};
			if( !entry.walljumpLeft() ) {
				searchDir = {-1, 0};
			}
			auto region         = getSweptTileGridRegion( &entry, searchDir );
			auto oldBoundingBox = exchange( entry.aab, {-1, -1, 1, 1} );
			auto oldPosition    = exchange( entry.position, feetPosition );
			auto collisionAtFeet =
			    findCollision( &entry, searchDir, game->room.layers[RL_Main].grid, region,
			                   game->collidableSystem.dynamicEntries(), 1, false );
			entry.aab      = oldBoundingBox;
			entry.position = oldPosition;
			if( collisionAtFeet ) {
				entry.particleEmitTimer = processTimer( entry.particleEmitTimer, dt );
				if( !entry.particleEmitTimer ) {
					entry.particleEmitTimer = {6};
					emitParticles( &game->particleSystem, feetPosition, ParticleEmitterId::Dust );
				}
			}
		} else {
			entry.particleEmitTimer = {};
		}

		// landing particles
		if( entry.spatialState == SpatialState::Grounded
		    && floatEqZero( entry.spatialStateTimer ) ) {

			auto feetPosition = entry.position;
			feetPosition.y += height( entry.aab );
			auto emitted = emitParticles( &game->particleSystem, feetPosition,
			                              ParticleEmitterId::LandingDust );
		}
	}

	// entity behaviors
	// TODO: move this into a seperate component?
	FOR( entry : game->collidableSystem.staticEntries() ) {
		// TODO: make this dependant on a behavior type
		if( entry.renderType == RenderType::Projectile ) {
			if( entry.lastCollision || entry.dead() ) {
				removeEntity( game, entry.entity );
			}
		}
	}
	processEntityRemovalQueue( game );

	if( enableRender ) {
		setRenderState( renderer, RenderStateType::Lighting, game->lighting );
		setProjection( renderer, ProjectionType::Perspective );
		auto cameraTranslation = matrixTranslation( 0, -50, 0 );
		Camera* camera         = &app->voxelState.camera;
		if( game->useGameCamera ) {
			camera = &game->camera;
		}
		renderer->view = cameraTranslation * getViewMatrix( camera );

		const mat4 rotations[] = {
		    matrixIdentity(),
		    matrixRotationZOrigin( HalfPi32, 8, 8 ),
		    matrixRotationZOrigin( Pi32, 8, 8 ),
		    matrixRotationZOrigin( Pi32 + HalfPi32, 8, 8 ),
		};

		{
			auto tileWidth  = GameConstants::TileWidth;
			auto tileHeight = GameConstants::TileHeight;
			const float zTranslation[] = {0, TILE_DEPTH, -TILE_DEPTH};
			static_assert( countof( zTranslation ) == RL_Count, "" );
			for( auto i = 0; i < RL_Count; ++i ) {
				auto layer = &app->gameState.room.layers[i];
				auto grid = layer->grid;
				auto tileSet = &app->gameState.tileSet;
				setTexture( renderer, 0, tileSet->voxels.texture );
				for( auto y = 0; y < grid.height; ++y ) {
					for( auto x = 0; x < grid.width; ++x ) {
						auto tile = grid.at( x, y );
						if( tile ) {
							pushMatrix( matrixStack );
							translate( matrixStack, x * tileWidth, -y * tileHeight - tileHeight,
							           zTranslation[i] );
							assert( tile.rotation < countof( rotations ) );
							multMatrix( matrixStack, rotations[tile.rotation] );
							auto entry = &tileSet->voxels.frames[tile.frames.min];
							addRenderCommandMesh( renderer, entry->mesh );
							popMatrix( matrixStack );
						}
					}
				}
			}

			// render background
			setTexture( renderer, 0, null );
			MESH_STREAM_BLOCK( stream, renderer ) {
				stream->color = 0xFF46AEEB;
				pushQuad( stream, rectf{-500, 500, 500, -500} * TILE_WIDTH, 32 * CELL_DEPTH );
			}
		}
		// render entities
		for( auto& entry : game->collidableSystem.entries ) {
			VoxelCollection::Frame* currentFrame = nullptr;
			switch( entry.renderType ) {
				case RenderType::Hero: {
					setTexture( renderer, 0, game->heroVoxelCollection.voxels.texture );
					auto frames  = getHeroActionAnimation( &game->heroVoxelCollection, &entry, dt );
					currentFrame = &frames[(int32)entry.currentFrame % frames.size()];
					break;
				}
				case RenderType::Projectile: {
					setTexture( renderer, 0, game->projectile.texture );
					auto frames  = game->projectile.frames;
					currentFrame = &frames[0];
					break;
				}
					InvalidDefaultCase;
			}
			assert( currentFrame );

			// 2d game world coordinates to 3d space coordinates
			auto offset = currentFrame->offset;
			// offset.x = -offset.x;
			auto position = ( entry.position - offset ) * CELL_WIDTH;
			position.y    = -position.y - CELL_HEIGHT * height( entry.aab );
			// position.x -= CELL_WIDTH * entry.aab.right;
			pushMatrix( matrixStack );
			translate( matrixStack, position, 0 );
			if( entry.faceDirection == CollidableFaceDirection::Left ) {
				translate( matrixStack, 2 * currentFrame->offset.x, 0 );
				auto rot = matrixRotationY( Pi32 );
				multMatrix( matrixStack, rot );
			}
			auto mesh               = addRenderCommandMesh( renderer, currentFrame->mesh );
			mesh->screenDepthOffset = -0.01f;
			popMatrix( matrixStack );
		}

		// render particles
		renderParticles( renderer, &game->particleSystem );
#if 1
		if( game->debugCamera ) {
			// render camera follow region
			setTexture( renderer, 0, null );
			MESH_STREAM_BLOCK( stream, renderer ) {
				stream->color     = Color::Blue;
				stream->lineWidth = 2;
				pushQuadOutline( stream, gameToScreen( app->gameState.cameraFollowRegion ) );
			}
		}
#endif
		if( game->debugCollisionBoxes ) {
			// setRenderState( renderer, RenderStateType::DepthTest, false );
			setTexture( renderer, 0, null );
			MESH_STREAM_BLOCK( stream, renderer ) {
				stream->color   = setAlpha( Color::Blue, 0.5f );
				FOR( entry : game->collidableSystem.entries ) {
					auto screenRect = gameToScreen( translate( entry.aab, entry.position ) );
					pushAabb( stream, screenRect.left, screenRect.bottom, -16, screenRect.right,
					          screenRect.top, 16 );
				}
			}
			// setRenderState( renderer, RenderStateType::DepthTest, true );
		}
	}
}

void processIngameLogs( float dt )
{
	auto array = makeInitializedArrayView( GlobalIngameLog->entries, GlobalIngameLog->count,
	                                       countof( GlobalIngameLog->entries ) );
	for( auto it = array.begin(); it != array.end(); ) {
		it->alive -= dt;
		if( it->alive <= 0 ) {
			it = array.erase( it );
			continue;
		}
		++it;
	}
	GlobalIngameLog->count = array.size();
}

static void doBlock( Array< ProfilingInfo > infos, ProfilingBlock* block )
{
	auto info = &infos[block->infoIndex];
	debugPrintln( "Event: {{{} {:10} {:.2}%}", info->block, block->duration,
	              block->absRatio * 100 );
}
static void visitBlock( Array< ProfilingInfo > infos, ProfilingBlock* block,
                        int32 maxDepth = INT32_MAX );
static void visitBlockChildren( Array< ProfilingInfo > infos, ProfilingBlock* block,
                                int32 maxDepth )
{
	if( maxDepth > 0 ) {
		doBlock( infos, block );
	}
	if( maxDepth > 1 ) {
		for( auto child = block->child; child; child = child->next ) {
			visitBlock( infos, child, maxDepth - 1 );
		}
	}
};
static void visitBlock( Array< ProfilingInfo > infos, ProfilingBlock* block, int32 maxDepth )
{
	if( maxDepth <= 0 ) {
		return;
	}
	for( ; block; block = block->next ) {
		doBlock( infos, block );
		for( auto child = block->child; child; child = child->next ) {
			visitBlockChildren( infos, child, maxDepth - 1 );
		}
	}
}

void doEasing( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	if( !focus ) {
		return;
	}

	auto easing   = &app->easingState;
	auto gui      = &easing->gui;
	auto renderer = &app->renderer;
	auto font     = &app->font;

	const vec2 InitialPosition   = {0, 0};
	const float SimHeight        = 200;
	auto reset = [=]() {
		easing->position = InitialPosition;
		easing->velocity = {};
	};

	imguiBind( gui );
	if( !easing->initialized ) {
		*gui = defaultImmediateModeGui();
		imguiLoadDefaultStyle( gui, &app->platform );

		reset();

		easing->initialized = true;
	}
	imguiBind( gui, renderer, font, inputs, app, {0, 0, app->width, app->height} );
	setProjection( renderer, ProjectionType::Orthogonal );

	renderer->color = Color::White;
	renderer->clearColor = 0xFF1B2B34;
	imguiEditbox( "Duration", &easing->duration );

	imguiSameLine( 2 );
	auto simRect = imguiAddItem( 200, SimHeight );
	auto wipRect = imguiAddItem( 200, SimHeight );

	auto frameDelta = dt * GameConstants::DeltaToFrameTime;
	easing->t += frameDelta;
	if( easing->t > easing->duration ) {
		easing->t     = 0;
		easing->lastT = 0;
		reset();
	}

	if( easing->t - easing->lastT >= 1.0f ) {
		easing->velocity.y += GameConstants::Gravity * 2;
		easing->lastT = easing->t;
	}
	easing->position += easing->velocity * frameDelta;
	if( easing->position.y >= SimHeight ) {
		easing->velocity.y = easing->velocity.y * -0.5f;
		easing->position.y = SimHeight;
	}

	imguiSameLine( 3 );
	auto visualizer0        = imguiAddItem( 200, 200 );
	auto visualizer1        = imguiAddItem( 200, 200 );
	auto combinedVisualizer = imguiAddItem( 200, 200 );

	auto t                      = easing->t / easing->duration;
	auto tIndex                 = ( int32 )( t * countof( easing->yPositions0 ) );
	tIndex                      = clamp( tIndex, 0, countof( easing->yPositions0 ) - 1 );
	easing->yPositions0[tIndex] = ( easing->position.y - InitialPosition.y ) / SimHeight;

	auto easingWip = []( float t ) -> float {
		if( t <= 0.4f ) {
			t /= 0.4f;
			return t * t;
		} else if( t < 0.8f ) {
			t -= 0.4f;
			t /= ( 0.8f - 0.4f );
			t *= 2;
			t -= 1;
			return ( t * t ) * 0.25f + 0.75f;
		} else {
			t -= 0.8f;
			t /= ( 1 - 0.8f );
			t *= 2;
			t -= 1;
			return ( t * t ) * 0.25f * 0.25f + 1 - ( 0.25f * 0.25f );
		}
		return 0;
	};

	easing->yPositions1[tIndex] = easingWip( t );

	auto renderVisualizer = []( LineMeshStream* stream, const float* vals, rectfarg rect ) {
		assert( vals );
		if( !hasCapacity( stream, EasingState::Count, EasingState::Count + 1 ) ) {
			OutOfMemory();
			return;
		}
		for( auto i = 0; i < EasingState::Count; ++i ) {
			auto t   = i / (float)EasingState::Count;
			vec3 cur = {rect.left + t * width( rect ), rect.top + vals[i] * height( rect ), 0};
			pushLineStripVertexUnchecked( stream, cur.x, cur.y, cur.z );
		}
		pushEndLineStripUnchecked( stream );
	};

	if( isPointInside( visualizer0, inputs->mouse.position ) ) {
		auto mouse = inputs->mouse.position - visualizer0.leftTop;
		auto x = mouse.x / width( visualizer0 );
		auto number = toNumberString( x );
		renderer->color = Color::White;
		renderText( renderer, font, number,
		            RectWH( inputs->mouse.position.x, inputs->mouse.position.y - 16, 0, 0 ) );
	}

	setTexture( renderer, 0, null );
	MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::Red;
		vec2 simPos   = easing->position + simRect.leftTop;
		simPos.x += width( simRect ) * 0.5f;
		pushQuad( stream, RectHalfSize( simPos, 4, 4 ) );

		stream->color = Color::Blue;
		vec2 wipPos   = {center( wipRect ).x,
		               easing->yPositions1[tIndex] * height( wipRect ) + wipRect.top};
		pushQuad( stream, RectHalfSize( wipPos, 4, 4 ) );
	}
	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::Red;
		renderVisualizer( stream, easing->yPositions0, visualizer0 );
		stream->color = Color::Blue;
		renderVisualizer( stream, easing->yPositions1, visualizer1 );
		stream->color = Color::Red;
		renderVisualizer( stream, easing->yPositions0, combinedVisualizer );
		stream->color = Color::Blue;
		renderVisualizer( stream, easing->yPositions1, combinedVisualizer );
		if( isPointInside( visualizer0, inputs->mouse.position ) ) {
			stream->color = Color::Blue;
			pushLine( stream, {inputs->mouse.position.x, visualizer0.top, 0},
			          {inputs->mouse.position.x, visualizer0.bottom, 0} );
		}
	}

	imguiUpdate( dt );
	imguiFinalize();
}

GAME_STORAGE struct RenderCommands* updateAndRender( void* memory, struct GameInputs* inputs,
                                                     float dt );
UPDATE_AND_RENDER( updateAndRender )
{
	GlobalProfilingTable->eventsCount = 0;
	BEGIN_PROFILING_BLOCK( "updateAndRender" );

	auto app       = (AppData*)memory;
	auto renderer  = &app->renderer;
	auto font      = &app->font;
	auto allocator = &app->stackAllocator;

	inputs->disableEscapeForQuickExit = false;

	auto lastFrameTimeAcc = app->frameTimeAcc;
	app->frameTimeAcc += dt;

	if( !app->resourcesLoaded ) {
		app->font = app->platform.loadFont( allocator, "Arial", 11, 400, false,
		                                    getDefaultFontUnicodeRanges() );
		imguiLoadDefaultStyle( &app->guiState, &app->platform );
		app->resourcesLoaded = true;

		LOG( INFORMATION, "Resources Loaded" );
	}

	// beginning of the frame
	debugPrintClear();
	clear( renderer );
	renderer->ambientStrength = 0.1f;
	renderer->lightColor      = Color::White;

	setProjectionMatrix( renderer, ProjectionType::Perspective, app->perspective );
	setProjectionMatrix( renderer, ProjectionType::Orthogonal, app->orthogonal );

	debug_Clear();

	// options
	if( isHotkeyPressed( inputs, KC_Key_Z, KC_Control ) ) {
		debug_FillMeshStream = !debug_FillMeshStream;
	}
	if( isHotkeyPressed( inputs, KC_Key_I, KC_Control ) ) {
		renderer->wireframe = !renderer->wireframe;
	}

	if( isKeyPressed( inputs, KC_F1 ) ) {
		app->focus = AppFocus::Game;
	}
	if( isKeyPressed( inputs, KC_F2 ) ) {
		app->focus = AppFocus::Voxel;
	}
	if( isKeyPressed( inputs, KC_F3 ) ) {
		app->focus = AppFocus::TexturePack;
	}
	if( isKeyPressed( inputs, KC_F4 ) ) {
		app->focus = AppFocus::Animator;
	}
	if( isKeyPressed( inputs, KC_F8 ) ) {
		app->focus = AppFocus::Easing;
	}
	if( isHotkeyPressed( inputs, KC_Key_R, KC_Control ) ) {
		renderer->lightPosition = app->voxelState.camera.position;
	}

	// reset mouse lock, locking must be done on each frame, so that on focus change mouse becomes
	// unlocked automatically
	inputs->mouse.locked = false;

	// NOTE: assumption here is that we only cross a frame boundary once per gameloop, but in
	// reality we might move across multiple frame boundaries at once if the game is lagging for
	// whatever reason
	// TODO: think about whether we want frameskips or let the game lag behind if we cross multiple
	// frame boundaries
	constexpr const float FrameTimeTarget = 1000.0f / 60.0f;
	doVoxel( app, inputs, app->focus == AppFocus::Voxel, dt / FrameTimeTarget );
	if( app->frameTimeAcc < FrameTimeTarget ) {
		auto frameRatio = dt / FrameTimeTarget;
		// move entities along without altering their paths
		doGame( app, inputs, app->focus == AppFocus::Game, frameRatio, false );
	} else {
		// assert( lastFrameTimeAcc <= FrameTimeTarget );
		if( lastFrameTimeAcc > FrameTimeTarget ) {
			LOG( WARNING, "FrameSkipped" );
		}
		auto timeToFrameBoundary = FrameTimeTarget - lastFrameTimeAcc;
		if( timeToFrameBoundary != 0 ) {
			auto frameRatio = timeToFrameBoundary / FrameTimeTarget;
			// move entities along without altering their paths
			doGame( app, inputs, app->focus == AppFocus::Game, frameRatio, false, false );
		}
		app->frameTimeAcc -= FrameTimeTarget;
		auto frameRatio = app->frameTimeAcc / FrameTimeTarget;
		// recalculate entity velocities and update entity positions
		doGame( app, inputs, app->focus == AppFocus::Game, frameRatio, true );
	}
	doTexturePack( app, inputs, app->focus == AppFocus::TexturePack, dt );
	doAnimator( app, inputs, app->focus == AppFocus::Animator, dt );
	doEasing( app, inputs, app->focus == AppFocus::Easing, dt );

	addRenderCommandMesh( renderer, toMesh( debug_MeshStream ) );

	setProjection( renderer, ProjectionType::Orthogonal );
#if GAME_RENDER_DEBUG_OUTPUT == 1
	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		renderer->color = Color::Black;
		char buffer[500];
		auto str = detailedDebugOutput( app, buffer, countof( buffer ) );

		renderText( renderer, font, str, {300, 0, 0, 0} );
	}
#endif

	// render GlobalIngameLog
	processIngameLogs( dt );
	RENDER_COMMANDS_STATE_BLOCK( renderer ) {
		renderer->color = Color::Black;
		float step      = 16;
		float y         = app->height - step;
		reset( font );
		for( int32 i = GlobalIngameLog->count - 1; i >= 0; --i ) {
			auto entry         = &GlobalIngameLog->entries[i];
			StringView message = {entry->message, entry->messageLength};
			float x            = ( app->width - stringWidth( font, message ) ) * 0.5f;
			renderer->color    = Color::Black;
			renderText( renderer, font, message, {x, y} );
			y -= step;
		}
	}

	END_PROFILING_BLOCK( "updateAndRender" );

	TEMPORARY_MEMORY_BLOCK( &app->stackAllocator ) {
		auto state = processProfilingEvents( &app->stackAllocator, GlobalProfilingTable );
		debugPrintln( "Infos: {}", state.infos.size() );
		visitBlock( state.infos, state.blocks.head, 2 );
	}

	showGameDebugGui( app, inputs, true, dt );

	return renderer;
}