#include "DebugSwitches.h"

#include <cassert>
#include <Core/IntegerTypes.h>
#include <Core/CoreTypes.h>
#include "Warnings.h"
#include <stdarg.h>

#include <type_traits>
#include <Core/Macros.h>
#include <cstring>

#include <Core/Math.h>
#include <tm_utility_wrapper.cpp>
#include <Core/CoreFloat.h>
#include <Core/Math.cpp>
#include <Utility.cpp>
#include <Core/Log.h>

#include <Core/NumericLimits.h>
#include <Core/Truncate.h>
#include <Core/NullableInt.h>
#include <Core/Algorithm.h>

#ifdef GAME_DEBUG
	#define ARGS_AS_CONST_REF 1
#else
	#define ARGS_AS_CONST_REF 0
#endif
#include <Core/StackAllocator.cpp>
#define VEC3_ARGS_AS_CONST_REF ARGS_AS_CONST_REF
#define VEC4_ARGS_AS_CONST_REF ARGS_AS_CONST_REF
#include <Core/Vector.cpp>
#define RECT_ARGS_AS_CONST_REF ARGS_AS_CONST_REF
#include <Core/Rect.h>
#include <Core/Matrix.cpp>
#define AABB_ARGS_AS_CONST_REF ARGS_AS_CONST_REF
#include <Core/AABB.h>
#define RANGE_ARGS_AS_CONST_REF ARGS_AS_CONST_REF
#include <Core/Range.h>

#include <Core/ArrayView.cpp>
#include <Core/StringView.cpp>
#include <Core/String.cpp>
#include <Core/Unicode.cpp>
#include <tm_conversion_wrapper.cpp>
#include <tm_bin_packing_wrapper.cpp>

#include <Core/ScopeGuard.h>

#include <easing.cpp>

#include <Windows.h>
#include <Windowsx.h>
#undef far
#undef near

#include <Core/Color.cpp>
#include <Core/Normal.cpp>
#include <ImageData.h>

#include <Core/IntrusiveLinkedList.h>
#include <QuadTexCoords.cpp>
#include <Graphics.h>
#include <Graphics/Font.h>
#include "TextureMap.cpp"

#include "VirtualKeys.h"
#include "Inputs.cpp"
#include "GameDeclarations.h"
#include "Imgui.cpp"

#include "Graphics/ImageProcessing.cpp"

#include "JsonWriter.cpp"
#include "tm_json_wrapper.cpp"

namespace GameConstants
{
constexpr float MovementSpeed          = 1.1f;
constexpr float JumpingSpeed           = -2.0f;
constexpr float WalljumpingSpeed       = -2.0f;
constexpr float WalljumpMaxDuration    = 8;
constexpr float WalljumpFixDuration    = 4;
constexpr float WalljumpWindowDuration = 10;
constexpr float WalljumpMoveThreshold  = WalljumpMaxDuration - WalljumpFixDuration;
}

// globals
global PlatformServices* GlobalPlatformServices = nullptr;
global TextureMap* GlobalTextureMap             = nullptr;
global IngameLog* GlobalIngameLog               = nullptr;
global ImmediateModeGui* ImGui                  = nullptr;

global MeshStream* debug_MeshStream = nullptr;
global bool debug_FillMeshStream    = true;

global string_builder* debugPrinter = nullptr;
#ifdef GAME_DEBUG
	#define debugPrint( ... ) debugPrinter->print( __VA_ARGS__ );
	#define debugPrintln( ... ) debugPrinter->println( __VA_ARGS__ );
	#define debugPrintClear() debugPrinter->clear()
	#define debugPrintGetString() asStringView( *debugPrinter )
#else
	#define debugPrint( ... )
	#define debugPrintln( ... )
	#define debugPrintClear()
	#define debugPrintGetString() StringView{}
#endif

StringView toString( VirtualKeyEnumValues key )
{
	assert( GlobalPlatformServices );
	return GlobalPlatformServices->getKeyboardKeyName( key );
}

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
	result.look = normalize( position - camera.position );
	result.right = normalize( cross( camera.up, result.look ) );
	result.up = cross( result.look, result.right );
	return result;
}
Camera cameraLookDirection( const Camera& camera, vec3arg dir )
{
	assert( floatEqSoft( length( dir ), 1 ) );
	Camera result;
	result.position = camera.position;
	result.look = dir;
	result.right = normalize( cross( camera.up, dir ) );
	result.up = cross( dir, result.right );
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

enum VoxelFaceValues : uint32 {
	VF_Front,
	VF_Left,
	VF_Back,
	VF_Right,
	VF_Top,
	VF_Bottom,

	VF_Count,
};
static const char* VoxelFaceStrings[] = {
	"front",
	"left",
	"back",
	"right",
	"top",
	"bottom",
};

struct VoxelGridTextureMap {
	TextureId texture;
	struct Entry {
		QuadTexCoords texCoords;
	};
	Entry entries[VF_Count];
};

struct VoxelCollection {
	struct Frame {
		VoxelGridTextureMap textureMap;
		MeshId mesh;
		vec2 offset;
	};

	struct FrameInfo {
		recti textureRegion[VF_Count];
	};

	struct Animation {
		string name;
		rangei range;
	};

	TextureId texture;
	Array< Frame > frames;
	Array< FrameInfo > frameInfos;
	Array< Animation > animations;
	string voxelsFilename;
};

#include "VoxelGrid.cpp"
#include "VoxelEditor.h"

struct CountdownTimer {
	float value;

	inline explicit operator bool() const { return value > 0; }
};
bool isCountdownTimerExpired( CountdownTimer timer )
{
	return timer.value < Float::BigEpsilon;
}
CountdownTimer processCountdownTimer( CountdownTimer timer, float dt )
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
	inline bool operator == ( EntityHandle other ) const { return bits == other.bits; }
	inline bool operator != ( EntityHandle other ) const { return bits != other.bits; }
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
static const char* SpatilStateStrings[] = {
	"Grounded",
	"FallingOff",
	"Airborne",
};

struct CollidableComponent {
	vec2 position;
	vec2 velocity;
	rectf aab;
	int32 groundedTile;

	CountdownTimer walljumpWindow;       // time window in which we can perform a walljump
	CountdownTimer walljumpDuration;     // how long the player can't move towards the wall
	bool walljumpLeft;                   // whether we are doing a walljump to the left or right

	SpatialState spatialState;
	float spatialStateTimer;

	EntityHandle entity;
};
void setSpatialState( CollidableComponent* collidable, SpatialState state )
{
	if( state != collidable->spatialState ) {
		collidable->spatialState = state;
		collidable->spatialStateTimer = 0;	
		if( state != SpatialState::Grounded ) {
			collidable->groundedTile = -1;
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
	       ||*/ collidable->spatialState == SpatialState::Airborne;
}
const char* getSpatialStateString( SpatialState state )
{
	assert( valueof( state ) < countof( SpatilStateStrings ) );
	return SpatilStateStrings[valueof( state )];
}

struct CollidableSystem {
	UArray< CollidableComponent > entries;
};
CollidableSystem makeCollidableSystem( StackAllocator* allocator, int32 maxCount )
{
	CollidableSystem result = {};
	result.entries          = makeUArray( allocator, CollidableComponent, maxCount );
	return result;
}
CollidableComponent* addCollidableComponent( CollidableSystem* system, EntityHandle entity )
{
	assert( system );
	assert( entity );
	CollidableComponent* result = nullptr;
	if( system->entries.remaining() ) {
		result               = system->entries.emplace_back();
		*result              = {};
		result->groundedTile = -1;
		result->entity       = entity;
	}
	return result;
}
CollidableComponent* findCollidableComponent( CollidableSystem* system, EntityHandle entity )
{
	return find_first_where( system->entries, it.entity == entity );
}

struct ControlComponent {
	EntityHandle entity;

	CountdownTimer jumpInputBuffer;
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

void processControlSystem( ControlSystem* control, CollidableSystem* collidableSystem,
                           GameInputs* inputs, float dt, bool frameBoundary )
{
	assert( control );
	assert( collidableSystem );
	assert( inputs );

	using namespace GameConstants;

	for( auto& entry : control->entries ) {
		entry.jumpInputBuffer = processCountdownTimer( entry.jumpInputBuffer, dt );
		if( auto collidable = findCollidableComponent( collidableSystem, entry.entity ) ) {
			collidable->velocity.x = 0;
			// TODO: do keymapping to actions
			if( isKeyDown( inputs, KC_Left ) ) {
				if( isCountdownTimerExpired( collidable->walljumpDuration )
				    || collidable->walljumpLeft ) {

					collidable->velocity.x = -MovementSpeed;
				}
			}
			if( isKeyDown( inputs, KC_Right ) ) {
				if( isCountdownTimerExpired( collidable->walljumpDuration )
				    || !collidable->walljumpLeft ) {

					collidable->velocity.x = MovementSpeed;
				}
			}
			if( isKeyPressed( inputs, KC_Up ) && collidable->groundedTile < 0 ) {
				entry.jumpInputBuffer = {4};
			}
			// TODO: input buffering
			if( frameBoundary && isKeyDown( inputs, KC_Up )
			    && isSpatialStateJumpable( collidable ) ) {

				collidable->velocity.y = JumpingSpeed;
				setSpatialState( collidable, SpatialState::Airborne );
			}
			// walljump
			if( frameBoundary && entry.jumpInputBuffer && isSpatialStateWalljumpable( collidable )
			    && collidable->walljumpWindow ) {

				collidable->walljumpWindow   = {0};
				collidable->velocity.y       = WalljumpingSpeed;
				collidable->walljumpDuration = {WalljumpMaxDuration};
				LOG( INFORMATION, "Walljump attempted" );
			}
		}
	}
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
	GameCamera result = {};
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
struct GameState {
	MeshId tileMesh;
	TextureId tileTexture;
	MeshId heroMesh;
	TextureId heroTexture;

	HandleManager entityHandles;
	CollidableSystem collidableSystem;
	ControlSystem controlSystem;
	CollidableComponent* player;
	bool initialized;

	GameCamera camera;
	rectf cameraFollowRegion;
	bool useGameCamera;
	bool lighting;

	// debug fields
	GameDebugGuiState debugGui;

	float groundPosition;
	float jumpHeight;
	float maxJumpHeight;
	float lastJumpHeight;
	float jumpHeightError;

	bool paused;

	bool debugCamera;
	bool debugCollisionBoxes;
};

enum class AppFocus { Game, Voxel, TexturePack };

#include "Editor/TexturePack/TexturePack.h"

struct AppData {
	PlatformServices platform;
	PlatformInfo* platformInfo;

	// app storage for globals
	// globals get assigned pointers to these, so that dll hotloading doesn't break
	IngameLog log;
	TextureMap textureMap;
	ImmediateModeGui guiState;
	string_builder debugPrinter;

	float frameTimeAcc;

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

	TextureId texture;

	bool mouseLocked;
	bool displayDebug;
};

#include "Editor/TexturePack/TexturePack.cpp"

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
	for( intmax i = 0; i < 6; ++i ) {
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
                                              PlatformInfo* platformInfo, float viewportWidth,
                                              float viewportHeight );
INITIALIZE_APP( initializeApp )
{
	assert( getAlignmentOffset( memory, alignof( AppData ) ) == 0 );
	char* p  = (char*)memory;
	auto app = (AppData*)p;
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
	app->width    = viewportWidth;
	app->height   = viewportHeight;

	app->debugMeshStream = makeMeshStream( allocator, 4000, 12000, nullptr );
	app->textureMap      = {makeUArray( allocator, TextureMapEntry, 100 )};
	app->debugPrinter    = string_builder( allocateArray( allocator, char, 2048 ), 2048 );
	auto result          = reloadApp( memory, size );

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

	auto app               = (AppData*)memory;
	debug_MeshStream       = &app->debugMeshStream;
	GlobalPlatformServices = &app->platform;
	GlobalIngameLog        = &app->log;
	GlobalTextureMap       = &app->textureMap;
	debugPrinter           = &app->debugPrinter;

	result.success    = true;
	result.logStorage = GlobalIngameLog;
	result.textureMap = GlobalTextureMap;
	ImGui             = &app->guiState;
	return result;
}

#include "PhysicsHitTest.cpp"

bool loadVoxelCollection( StackAllocator* allocator, StringView filename, VoxelCollection* out )
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
		dest->range.min = currentFrame;

		FOR( frameVal : animation["frames"].getArray() ) {
			auto frame     = frameVal.getObject();
			auto destFrame = &out->frames[currentFrame];
			auto destInfo  = &out->frameInfos[currentFrame];
			++currentFrame;

			*destFrame = {};

			for( auto face = 0; face < VF_Count; ++face ) {
				auto faceObject = frame[VoxelFaceStrings[face]].getObject();

				destFrame->textureMap.texture = out->texture;
				serialize( faceObject["rect"], destInfo->textureRegion[face] );
				serialize( faceObject["texCoords"], destFrame->textureMap.entries[face].texCoords );
				FOR( vert : destFrame->textureMap.entries[face].texCoords.elements ) {
					vert.x *= itw;
					vert.y *= ith;
				}
			}
			serialize( frame["offset"], destFrame->offset );
		}

		dest->range.max = currentFrame;
	}
	out->voxelsFilename = makeString( primary, root["voxels"].getString() );

	partition.commit();
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
	if( isKeyDown( inputs, KC_Control ) ) {
		camera->position.y += -speed;
	}

	auto cameraDelta = inputs->mouse.delta * settings->mouseSensitivity;
	if( settings->mouseInvertY ) {
		cameraDelta.y = -cameraDelta.y;
	}
	updateCamera( camera, cameraDelta );
}

#include "VoxelEditor.cpp"

#define GAME_MAP_WIDTH 16
#define GAME_MAP_HEIGHT 16
static int8 GameTestMap[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

const float Gravity        = 0.08f;
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

static void doCollisionDetection( AppData* app, CollidableSystem* system, GameInputs* inputs,
                                  float dt, bool frameBoundary )
{
	using namespace GameConstants;

	auto game = &app->gameState;
	// TODO: air movement should be accelerated instead of instant
	vec2 rightBottom = {0, 0};
	auto rect = Rect( -1, 1, 1.0f, 0 );

	for( auto& entry : system->entries ) {
		entry.walljumpWindow      = processCountdownTimer( entry.walljumpWindow, dt );
		entry.walljumpDuration    = processCountdownTimer( entry.walljumpDuration, dt );

		// make entry move away from the wall if a walljump was executed
		if( entry.walljumpDuration ) {
			// move away only for 4 frames in total
			if( entry.walljumpDuration.value > WalljumpMoveThreshold ) {
				if( entry.walljumpLeft ) {
					entry.velocity.x = -MovementSpeed;
				} else {
					entry.velocity.x = MovementSpeed;
				}
			}
		}

		auto velocity = entry.velocity;
		// update
		{
			auto tileWidth  = 16.0f;
			auto tileHeight = 16.0f;

			if( frameBoundary ) {
				if( entry.groundedTile < 0 ) {
					entry.velocity.y += Gravity;
				}
			}

			processSpatialState( &entry, dt );

			auto vdt = dt;
			recti mapBounds = {0, 0, GAME_MAP_WIDTH, GAME_MAP_HEIGHT};

			// find position of gap and whether we should squeeze into it this frame
			if( velocity.x != 0 && velocity.y != 0 ) {
				auto entryGridX = (int32)floor( entry.position.x / tileWidth );
				auto entryGridY = (int32)floor( entry.position.y / tileHeight );
				auto entryGridYNext =
				    (int32)floor( ( entry.position.y + entry.velocity.y * vdt ) / tileHeight );
				if( entryGridY != entryGridYNext
				    && ( entryGridY >= 0 && entryGridY < GAME_MAP_HEIGHT ) ) {
					if( velocity.y > 0 ) {
						++entryGridY;
						++entryGridYNext;
					}

					bool gapExists = false;
					int32 gapTileIndex;
					int32 gapTileX = entryGridX;
					int32 gapTileY = entryGridY;
					if( velocity.x < 0 ) {
						gapTileX -= 1;
					} else {
						gapTileX += 1;
					}
					if( gapTileX >= 0 && gapTileX < GAME_MAP_WIDTH ) {
						int32 step = 1;
						if( velocity.y < 0 ) {
							step = -1;
						}
						for( gapTileY = entryGridY; gapTileY != entryGridYNext; gapTileY += step ) {
							if( gapTileY < 0 || gapTileY >= GAME_MAP_HEIGHT ) {
								continue;
							}
							gapTileIndex = gapTileX + gapTileY * GAME_MAP_WIDTH;
							auto gap     = GameTestMap[gapTileIndex];
							if( gap == 0 ) {
								auto neighborTileX = gapTileX;
								auto neighborTileY = gapTileY;
								if( velocity.y > 0 ) {
									neighborTileY -= 1;
								} else {
									neighborTileY += 1;
								}
								if( !isPointInside( mapBounds, neighborTileX, neighborTileY ) ) {
									continue;
								}
								auto neighborIndex = neighborTileX + neighborTileY * GAME_MAP_WIDTH;
								auto neighbor = GameTestMap[neighborIndex];
								// check whether neighbor is solid
								if( neighbor != 0 ) {
									// gap exists and is valid candidate for squeezing in
									gapExists = true;
									break;
								}
							}
						}
					}

					if( gapExists ) {
						rectf gapBounds = RectWH( gapTileX * tileWidth, gapTileY * tileHeight,
						                          tileWidth, tileHeight );
#if 0
						auto nextPlayerAab =
						    translate( entry.aab, entry.position + entry.velocity * vdt );
						assert( abs( velocity.x ) <= tileWidth );

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
						auto t = yDelta / velocity.y;
						entry.position += velocity * t;
						vdt -= t;
#endif
					}
				}
			}

			// collision detection begins here

			// broadphase

			auto currentPlayerAab = translate( entry.aab, entry.position );
			// sweep the bounding box along entry velocity to get bounding box of the sweep region
			auto entrySweptAab = sweep( currentPlayerAab, entry.velocity * vdt );
			// turn the swept bounding box to tile grid region that we need to check for collisions
			// tiles outside this region can't possibly collide with the entry
			auto tileGridRegion = RectTiledIndex( entrySweptAab, tileWidth, tileHeight );
			tileGridRegion =
			    RectMin( tileGridRegion, recti{0, 0, GAME_MAP_WIDTH, GAME_MAP_HEIGHT} );

			// check grounded state of entry
			if( entry.groundedTile >= 0 ) {
				assert_init( auto tile = GameTestMap[entry.groundedTile], tile );
				auto x             = entry.groundedTile % GAME_MAP_WIDTH;
				auto y             = entry.groundedTile / GAME_MAP_WIDTH;
				rectf tileBounds   = RectWH( x * tileWidth, y * tileHeight, tileWidth, tileHeight );
				CollisionInfo info = {};
				if( !testAabVsAab( entry.position, entry.aab, {0, 1}, tileBounds, 1, &info )
				    || info.t > SafetyDistance + 0.00001f || info.t < 0 ) {
					entry.groundedTile = -1;
				}
				if( entry.groundedTile < 0 ) {
					bool found = false;
					for( intmax y = tileGridRegion.top; y < tileGridRegion.bottom && !found; ++y ) {
						for( intmax x = tileGridRegion.left; x < tileGridRegion.right && !found;
						     ++x ) {
							auto index = x + y * GAME_MAP_WIDTH;
							assert( index >= 0 && index < countof( GameTestMap ) );
							auto tile = GameTestMap[index];
							if( tile ) {
								rectf tileBounds =
								    RectWH( x * tileWidth, y * tileHeight, tileWidth, tileHeight );
								CollisionInfo info = {};
								if( testAabVsAab( entry.position, entry.aab, {0, 1}, tileBounds, 1,
								                  &info ) ) {
									if( info.normal.y < 0 && info.t >= 0
									    && info.t < SafetyDistance + 0.00001f ) {
										entry.position.y += info.t - SafetyDistance;
										entry.groundedTile = (int32)index;
										found              = true;
										break;
									}
								}
							}
						}
					}
				}

				if( entry.groundedTile < 0 ) {
					setSpatialState( &entry, SpatialState::FallingOff );
				}
			}

			float remaining = 1;
			do {
				float t                 = remaining;
				vec2 normal             = {};
				vec2 push               = {};
				int32 collidedTileIndex = 0;
				bool collided           = false;
				for( intmax y = tileGridRegion.top; y < tileGridRegion.bottom; ++y ) {
					for( intmax x = tileGridRegion.left; x < tileGridRegion.right; ++x ) {
						auto index = x + y * GAME_MAP_WIDTH;
						assert( index >= 0 && index < countof( GameTestMap ) );
						auto tile = GameTestMap[index];
						if( tile ) {
							rectf tileBounds =
							    RectWH( x * tileWidth, y * tileHeight, tileWidth, tileHeight );
							CollisionInfo info = {};
							if( testAabVsAab( entry.position, entry.aab, entry.velocity * vdt,
							                  tileBounds, t, &info ) ) {
								if( info.t < t ) {
									collided          = true;
									t                 = info.t;
									collidedTileIndex = (int32)index;
									normal            = info.normal;
									push              = info.push;
								}
							}
						}
					}
				}
				if( !collided && !floatEqZero( entry.velocity.x ) ) {
					entry.walljumpWindow = {0};
				}
				if( t > 0 ) {
					entry.position += entry.velocity * vdt * t;
					if( collided ) {
						entry.position += normal * SafetyDistance;
					}
					remaining -= t;
				} else {
					entry.position += push + normal * SafetyDistance;
				}
				if( entry.groundedTile < 0 && entry.velocity.y < 0 ) {
					game->jumpHeight = entry.position.y - game->groundPosition;
				}
				if( normal.y < 0 ) {
					entry.groundedTile = collidedTileIndex;
					setSpatialState( &entry, SpatialState::Grounded );
					if( game->jumpHeight < game->maxJumpHeight ) {
						game->maxJumpHeight = game->jumpHeight;
					}
					game->jumpHeightError = abs( game->jumpHeight - game->lastJumpHeight );
					game->lastJumpHeight  = game->jumpHeight;
				}
				// reflect velocity based on the collision normal
				entry.velocity = entry.velocity - normal * dot( normal, entry.velocity );
				if( collided && normal.x != 0 && normal.y == 0 && entry.groundedTile < 0 ) {
					entry.walljumpLeft   = ( normal.x < 0 );
					entry.walljumpWindow = {WalljumpWindowDuration};
				}
				auto lengthSquared = dot( entry.velocity, entry.velocity );
				if( t >= 0 && ( t <= 0.000001f || lengthSquared < 0.00001f ) ) {
					break;
				}
			} while( remaining > 0 );
		}
	}
}

static StringView detailedDebugOutput( AppData* app, char* buffer, int32 size )
{
	auto builder  = string_builder( buffer, size );
	auto renderer = &app->renderer;

	auto info = app->platformInfo;

	builder << "FrameTime: " << info->frameTime
	        << "\nAverage FrameTime:" << info->averageFrameTime
	        << "\nMin FrameTime: " << info->minFrameTime
	        << "\nMax FrameTime: " << info->maxFrameTime;

	builder << "\n\n";
	builder.println( "Draw Calls: {}\nVertices: {}\nIndices: {}", info->drawCalls, info->vertices,
	                 info->indices );

	builder << '\n'
	        << "\nFPS: " << info->fps
	        << "\nAverage Fps:" << info->averageFps
	        << "\nMin Fps: " << info->minFps
	        << "\nMax Fps: " << info->maxFps;

	builder << '\n'
	        << "\nLight Position: " << renderer->lightPosition.x << ", "
	        << renderer->lightPosition.y << ", " << renderer->lightPosition.z;
	if( info->recordingInputs ) {
		builder << "\nRecording Inputs: " << info->recordingFrame;
	}
	if( info->replayingInputs ) {
		builder << "\nReplaying Inputs: " << info->recordingFrame;
	}

	builder << "\njumpHeight: " << app->gameState.jumpHeight
	        << "\nmaxJumpHeight: " << app->gameState.maxJumpHeight
	        << "\nJumpHeightError: " << app->gameState.jumpHeightError;

	builder << "\nWallJumpTimer: " << app->gameState.player->walljumpWindow.value
	        << "\nWallJumpDuration: " << app->gameState.player->walljumpDuration.value;

	builder << '\n' << '\n';

	auto camera = &app->voxelState.camera;
	builder << "Camera:\n";
	builder.println( "Position: {}", camera->position );
	builder.println( "Look: {}", camera->look );
	builder.println( "Up: {}", camera->up );
	auto player = app->gameState.player;
	builder.println( "CollidableComponent: {}\nCollidableComponent Time: {}",
	                 getSpatialStateString( player->spatialState ), player->spatialStateTimer );
	builder.println( "Player pos: {}, {}", player->position.x, player->position.y );
	builder.println( "Camera follow: {:.2}", app->gameState.cameraFollowRegion );

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
			auto t = quadratic( camera->turnTimer.value, 0 );
			auto dir          = normalize( lerp( t, camera->nextLook, camera->prevLook ) );
			*camera           = cameraLookDirection( *camera, dir );
			camera->turnTimer = processCountdownTimer( camera->turnTimer, dt * 0.05f );
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

static void doGame( AppData* app, GameInputs* inputs, bool focus, float dt, bool frameBoundary,
                    bool enableRender = true )
{
	auto renderer = &app->renderer;
	// auto font      = &app->font;
	auto game     = &app->gameState;
	auto settings = &app->settings;

	if( !game->initialized ) {
		auto allocator             = &app->stackAllocator;
		app->gameState.tileTexture = app->platform.loadTexture( "Data/Images/texture_map.png" );
		auto tileTextureMap        = makeDefaultVoxelGridTextureMap( app->gameState.tileTexture );
		app->gameState.tileMesh    = loadVoxelMeshFromFile( &app->platform, &app->stackAllocator,
		                                                 &tileTextureMap, "Data/tile.raw" );
		app->gameState.heroTexture = app->platform.loadTexture( "Data/Images/dude2.png" );
		auto heroTextureMap        = makeHeroVoxelGridTextureMap( app->gameState.heroTexture );
		app->gameState.heroMesh    = loadVoxelMeshFromFile( &app->platform, &app->stackAllocator,
		                                                 &heroTextureMap, "Data/hero.raw" );
		auto maxEntities       = 10;
		game->entityHandles    = makeHandleManager();
		game->collidableSystem = makeCollidableSystem( allocator, maxEntities );
		game->controlSystem    = makeControlSystem( allocator, maxEntities );

		auto playerHandle = addEntity( &game->entityHandles );
		game->player      = addCollidableComponent( &game->collidableSystem, playerHandle );
		game->player->aab = {-6, 0, 6, 26};
		addControlComponent( &game->controlSystem, playerHandle );

		auto enemyHandle = addEntity( &game->entityHandles );
		auto enemy       = addCollidableComponent( &game->collidableSystem, enemyHandle );
		enemy->aab       = {-8, 0, 8, 28};

		// auto followWidth         = app->width * 0.25f;
		// auto followHeight        = app->height * 0.25f;
		game->camera             = makeGameCamera( {0, -50, -200}, {0, 0, 1}, {0, 1, 0} );
		game->cameraFollowRegion = {-25, -50, 25, 50};
		game->useGameCamera      = true;
		game->lighting           = false;

		game->initialized = true;
	}

	if( !focus ) {
		return;
	}
	auto matrixStack     = renderer->matrixStack;

	processCamera( inputs, settings, &app->voxelState.camera,
	               isKeyDown( inputs, KC_Shift ) ? dt : ( dt * 0.25f ) );

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

	auto player = game->player;

	processGameCamera( app, dt, frameBoundary );
	processControlSystem( &game->controlSystem, &game->collidableSystem, inputs, dt,
	                      frameBoundary );
	doCollisionDetection( app, &app->gameState.collidableSystem, inputs, dt, frameBoundary );

	if( enableRender ) {
		setRenderState( renderer, RenderStateType::Lighting, game->lighting );
		setProjection( renderer, ProjectionType::Perspective );
		auto cameraTranslation = matrixTranslation( 0, -50, 0 );
		Camera* camera         = &app->voxelState.camera;
		if( game->useGameCamera ) {
			camera = &game->camera;
		}
		renderer->view = cameraTranslation * getViewMatrix( camera );

		{
			setTexture( renderer, 0, game->tileTexture );
			auto tileWidth = TILE_WIDTH;
			auto tileHeight = TILE_HEIGHT;
			for( intmax y = 0; y < GAME_MAP_HEIGHT; ++y ) {
				for( intmax x = 0; x < GAME_MAP_WIDTH; ++x ) {
					auto tile = GameTestMap[x + y * GAME_MAP_WIDTH];
					if( tile ) {
						pushMatrix( matrixStack );
						translate( matrixStack, x * tileWidth, -y * tileHeight - tileHeight, 0 );
						addRenderCommandMesh( renderer, game->tileMesh );
						popMatrix( matrixStack );
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
		// render player
		// setRenderState( renderer, RenderStateType::DepthTest, false );
		for( auto& entry : game->collidableSystem.entries ) {
			auto position = entry.position * CELL_WIDTH;
			position.y    = -position.y - CELL_HEIGHT * 28;
			position.x -= CELL_WIDTH * 8;
			setTexture( renderer, 0, game->heroTexture );
			pushMatrix( matrixStack );
			translate( matrixStack, position, 0 );
			auto mesh = addRenderCommandMesh( renderer, game->heroMesh );
			mesh->screenDepthOffset = -0.01f;
			popMatrix( matrixStack );
		}
		auto gameToScreen = []( rectfarg rect ) {
			rectf result;
			result.left   = rect.left;
			result.top    = -rect.top;
			result.right  = rect.right;
			result.bottom = -rect.bottom;
			return result;
		};
#if 1
		if( game->debugCamera ) {
			// render camera follow region
			setTexture( renderer, 0, null );
			MESH_STREAM_BLOCK( stream, renderer ) {
				stream->color = Color::Blue;
				stream->lineWidth = 2;
				pushQuadOutline( stream, gameToScreen( app->gameState.cameraFollowRegion ) );
			}
		}
#endif
		if( game->debugCollisionBoxes ) {
			// setRenderState( renderer, RenderStateType::DepthTest, true );
			// render collision box
			setRenderState( renderer, RenderStateType::DepthTest, false );
			setTexture( renderer, 0, null );
			MESH_STREAM_BLOCK( stream, renderer ) {
				stream->color   = setAlpha( Color::Blue, 0.5f );
				auto position   = player->position;
				position.y      = -position.y;
				aabb playerAabb = {
				    position.x + player->aab.left,  position.y + -player->aab.top,    -4,
				    position.x + player->aab.right, position.y + -player->aab.bottom, 4};
				pushAabb( stream, playerAabb * EDITOR_CELL_WIDTH );
			}
			setRenderState( renderer, RenderStateType::DepthTest, true );
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

const float FrameTimeTarget = 1000.0f / 60.0f;

GAME_STORAGE struct RenderCommands* updateAndRender( void* memory, struct GameInputs* inputs,
                                                     float dt );
UPDATE_AND_RENDER( updateAndRender )
{
	auto app       = (AppData*)memory;
	auto renderer  = &app->renderer;
	auto font      = &app->font;
	auto allocator = &app->stackAllocator;

	inputs->disableEscapeForQuickExit = false;

	auto lastFrameTimeAcc = app->frameTimeAcc;
	app->frameTimeAcc += dt;

	if( !app->resourcesLoaded ) {
		app->texture = app->platform.loadTexture( "Data/Images/test.png" );
		app->font    = app->platform.loadFont( allocator, "Arial", 11, 400, false,
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

	// gui code has to be outside of the frame independent movement code, so that we do not process
	// inputs twice on frame boundaries
	showGameDebugGui( app, inputs, true, dt );

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
		for( intmax i = GlobalIngameLog->count - 1; i >= 0; --i ) {
			auto entry         = &GlobalIngameLog->entries[i];
			StringView message = {entry->message, entry->messageLength};
			float x            = ( app->width - stringWidth( font, message ) ) * 0.5f;
			renderer->color    = Color::Black;
			renderText( renderer, font, message, {x, y} );
			y -= step;
		}
	}

	return renderer;
}