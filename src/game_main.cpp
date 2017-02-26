#include "DebugSwitches.h"

#include <type_traits>
#include <cassert>
// #include "Core/CoreAssert.h"
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
#include "Core/Ring.h"
#include "Core/StringView.cpp"
#include "Core/String.cpp"
#include "Core/Unicode.cpp"
#include "Core/Color.cpp"
#include "tm_conversion_wrapper.cpp"
#include "tm_bin_packing_wrapper.cpp"

#include "Core/ScopeGuard.h"

#include "easing.cpp"

#include "Core/Normal.cpp"
#include "ImageData.h"

#include "Core/IntrusiveLinkedList.h"
#include "Profiling.cpp"

#include "QuadTexCoords.cpp"
#include "Quad.cpp"
#include "Graphics.h"
#include "Graphics/Font.h"
#include "TextureMap.cpp"

#include "string_logger.cpp"

global_var string_builder* GlobalDebugPrinter = nullptr;
global_var string_logger* GlobalDebugLogger   = nullptr;
#if defined( GAME_DEBUG ) || ( GAME_DEBUG_PRINTING )
	#define debugPrint( ... ) GlobalDebugPrinter->print( __VA_ARGS__ );
	#define debugPrintln( ... ) GlobalDebugPrinter->println( __VA_ARGS__ );
	#define debugPrintClear() GlobalDebugPrinter->clear()
	#define debugPrintGetString() asStringView( *GlobalDebugPrinter )

	#define debugLog( ... ) GlobalDebugLogger->log( __VA_ARGS__ );
	#define debugLogln( ... ) GlobalDebugLogger->logln( __VA_ARGS__ );
	#define debugLogClear() GlobalDebugLogger->clear()
	#define debugLogGetString() asStringView( *GlobalDebugLogger )
#else
	#define debugPrint( ... ) ( (void)0 )
	#define debugPrintln( ... ) ( (void)0 )
	#define debugPrintClear() ( (void)0 )
	#define debugPrintGetString() ( StringView{} )

	#define debugLog( ... ) ( (void)0 )
	#define debugLogln( ... ) ( (void)0 )
	#define debugLogClear() ( (void)0 )
	#define debugLogGetString() ( StringView{} )
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
#include "tm_bezier_wrapper.cpp"

#include "GameConstants.h"

struct DebugValues;
struct VoxelCollectionMeshCache;

// globals
global_var PlatformServices* GlobalPlatformServices                 = nullptr;
global_var TextureMap* GlobalTextureMap                             = nullptr;
global_var VoxelCollectionMeshCache* GlobalVoxelCollectionMeshCache = nullptr;
global_var IngameLog* GlobalIngameLog                               = nullptr;
global_var ImmediateModeGui* ImGui                                  = nullptr;
global_var ProfilingTable* GlobalProfilingTable                     = nullptr;

#include "PlatformServicesHelpers.cpp"

// logging needs some definitions to exists, but those definitions may need to log
// this could be solved by splitting everything into .h/.cpp pairs, but instead its easier to only
// split log.h
#define _LOG_IMPLEMENTATION_
#include "Core/Log.h"

// TODO: GlobalScrapSize can be 1 megabyte once VoxelGrids are dense
constexpr const size_t GlobalScrapSize = megabytes( 3 );
global_var StackAllocator* GlobalScrap = nullptr;

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

#include <vector>
#include <new>
#include <memory>

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

#include "Camera.cpp"
#include "VoxelGrid.cpp"
#include "VoxelCollection.cpp"
#include "Room.cpp"

#include "Editor/Common/DynamicVoxelCollection.h"
#include "Editor/Common/EditorView.h"
#include "Editor/Common/EditorView.cpp"
#include "VoxelEditor.h"

struct CountdownTimer {
	float value;

	inline explicit operator bool() const { return value >= Float::BigEpsilon; }
};
bool isCountdownTimerExpired( CountdownTimer timer ) { return timer.value < Float::BigEpsilon; }
bool isCountdownActive( CountdownTimer timer ) { return timer.value >= Float::BigEpsilon; }
CountdownTimer processTimer( CountdownTimer timer, float dt )
{
	CountdownTimer result = timer;
	result.value -= dt;
	if( result.value < 0 ) {
		result.value = 0;
	}
	return result;
}

#include "Entity.cpp"
#include "Collision.cpp"

struct ControlComponent {
	EntityHandle entity;

	CountdownTimer jumpInputBuffer;
	CountdownTimer attackInputBuffer;
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
	int32 debugLogDialog;
	ImGuiScrollableRegion debugLogRegion;
	bool show;
	bool windowsExpanded;
	float fadeProgress;
	bool showFrameStepCounts;
	bool initialized;
};

vec2 gameToScreen( vec2 pos ) { return {pos.x, -pos.y}; }
rectf gameToScreen( rectfarg rect )
{
	rectf result;
	result.left   = rect.left;
	result.top    = -rect.top;
	result.right  = rect.right;
	result.bottom = -rect.bottom;
	return result;
}

#include "ParticleSystem.cpp"

enum class AppFocus { Game, Voxel, TexturePack, Animator, Easing, RoomEditor };

#include "PhysicsHitTest.cpp"

enum class SkeletonEventType : int8 {
	None,
	Attack,
};
StringView to_string( SkeletonEventType type )
{
	static const StringView Names[] = {
		"None", "Attack"
	};
	assert( valueof( type ) >= 0 && valueof( type ) < countof( Names ) );
	return Names[valueof( type )];
}
template <>
SkeletonEventType convert_to< SkeletonEventType >( StringView str, SkeletonEventType def )
{
	auto result = def;
	if( str.size() ) {
		switch( str[0] ) {
			case 'n':
			case 'N': {
				result = SkeletonEventType::None;
				break;
			}
			case 'a':
			case 'A': {
				result = SkeletonEventType::Attack;
				break;
			}
		}
	}
	return result;
}

#include "Editor/TexturePack/TexturePack.h"
#include "Editor/Animator/Animator.h"
#include "Editor/Room/RoomEditor.h"

#include "Skeleton.h"
#include "Skeleton.cpp"

struct HitboxSystem {
	struct HitboxPair {
		EntityHandle attacker;
		EntityHandle defender;
	};
	HitboxPair pairsData[20];
	int8 pairsCount;

	UArray< HitboxPair > hitboxPairs()
	{
		return makeInitializedArrayView( pairsData, pairsCount, countof( pairsData ) );
	};

	void setPairsCount( int32 size )
	{
		assert( size >= 0 && size <= countof( pairsData ) );
		pairsCount = auto_truncate( size );
	}
};

template < class GenericSystem >
void removeEntities( GenericSystem* system, Array< EntityHandle > handles )
{
	unordered_remove_if( system->entries, [handles]( const auto& component ) {
		return exists( handles, component.entity );
	} );
}
void removeEntities( EntitySystem* system, Array< EntityHandle > handles )
{
	erase_if( system->entries, [system, handles]( const Entity& component ) {
		if( exists( handles, component.handle ) ) {
			if( !component.dynamic() ) {
				--system->entriesCount;
			}
			return true;
		}
		return false;
	} );
}
void removeEntities( HitboxSystem* system, Array< EntityHandle > handles )
{
	auto pairs = system->hitboxPairs();
	erase_if( pairs, [handles]( const auto& pair ) {
		return exists_if( handles, [pair]( const auto& handle ) {
			return handle == pair.attacker || handle == pair.defender;
		} );
	} );
	system->setPairsCount( pairs.size() );
}
void removeEntities( EntitySystem* entitySystem, SkeletonSystem* system,
                     Array< EntityHandle > handles )
{
	FOR( entity : entitySystem->entries ) {
		if( entity.skeleton && exists( handles, entity.handle ) ) {
			deleteSkeleton( system, entity.skeleton );
			entity.skeleton = nullptr;
		}
	}
}

#include "Projectile.h"

struct GameState {
	TileSet tileSet;
	ParticleSystem particleSystem;
	SkeletonSystem skeletonSystem;
	VoxelCollection projectile;
	HitboxSystem hitboxSystem;
	ProjectileSystem projectileSystem;

	HandleManager entityHandles;
	EntitySystem entitySystem;
	ControlSystem controlSystem;
	Entity* player;
	UArray< EntityHandle > entityRemovalQueue;
	bool initialized;

	GameCamera camera;
	GameCamera prevCamera;
	rectf cameraFollowRegion;
	bool useGameCamera;
	bool lighting;

	Room room;

	float prevBlendFactor;

	// debug fields
	GameDebugGuiState debugGui;

	bool paused;

	bool debugCamera;
	bool debugCollisionBoxes;

	ShaderId outlineShader;
};

void processEntityRemovalQueue( GameState* game )
{
	assert( game );
	if( game->entityRemovalQueue.size() ) {
		auto handles = makeArrayView( game->entityRemovalQueue );
		removeEntities( &game->entitySystem, &game->skeletonSystem, handles );
		removeEntities( &game->controlSystem, handles );
		removeEntities( &game->entitySystem, handles );
		removeEntities( &game->hitboxSystem, handles );
		game->entityRemovalQueue.clear();
	}
}
void removeEntity( GameState* game, EntityHandle handle )
{
	append_unique( game->entityRemovalQueue, handle );
}

#include "Projectile.cpp"

void processControlSystem( GameState* game, ControlSystem* controlSystem,
                           EntitySystem* entitySystem, GameInputs* inputs, float dt )
{
	assert( controlSystem );
	assert( entitySystem );
	assert( inputs );

	for( auto& control : controlSystem->entries ) {
		if( auto entity = findEntity( entitySystem, control.entity ) ) {
			auto entityControl = &entity->control;
			// TODO: do keymapping to actions
			// TODO: input buffering

			entityControl->horizontal = EntityHorizontalMovement::None;
			if( isKeyDown( inputs, KC_Left ) ) {
				entityControl->horizontal = EntityHorizontalMovement::Left;
			}
			if( isKeyDown( inputs, KC_Right ) ) {
				entityControl->horizontal = EntityHorizontalMovement::Right;
			}

			// break_if( isKeyPressed( inputs, KC_Up ) );
			auto jumpDown = isKeyDown( inputs, KC_Up );
			if( !jumpDown || entityControl->verticalAction == EntityVerticalAction::ConsumeInput ) {
				control.jumpInputBuffer = {};
			}
			if( isKeyPressed( inputs, KC_Up ) ) {
				control.jumpInputBuffer = {4};
			}
			if( isKeyPressed( inputs, KC_Space ) ) {
				control.attackInputBuffer = {1};
			}

			entityControl->action = EntityActionState::None;
			if( control.jumpInputBuffer ) {
				entityControl->verticalAction = EntityVerticalAction::Jump;
			}
			if( !jumpDown ) {
				entityControl->vertical       = EntityVerticalMovement::None;
				entityControl->verticalAction = EntityVerticalAction::None;
			} else {
				entityControl->vertical = EntityVerticalMovement::Jump;
			}

			if( control.attackInputBuffer ) {
				entityControl->action = EntityActionState::Attack;
			}
		}
		control.jumpInputBuffer   = processTimer( control.jumpInputBuffer, dt );
		control.attackInputBuffer = processTimer( control.attackInputBuffer, dt );
	}
}

struct EasingState {
	bool initialized;
	ImmediateModeGui gui;
	float t;
	float lastT;

	vec2 position;
	vec2 velocity;
	vec2 acceleration;
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
	VoxelCollectionMeshCache voxelCollectionMeshCache;
	ImmediateModeGui guiState;
	string_builder debugPrinter;
	string_logger debugLogger;
	ProfilingTable profilingTable;
	DebugValues debugValues;

	mat4 perspective;
	mat4 orthogonal;

	StackAllocator stackAllocator;
	StackAllocator scrapAllocator;
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
	RoomEditorState roomEditorState;

	bool mouseLocked;
	bool displayDebug;
};

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
	app->debugLogger     = string_logger( allocateArray( allocator, char, 2048 ), 2048, 200 );
	app->scrapAllocator  = makeStackAllocator( &app->stackAllocator, GlobalScrapSize );
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
	GlobalVoxelCollectionMeshCache = &app->voxelCollectionMeshCache;
	GlobalScrap                    = &app->scrapAllocator;
	GlobalDebugPrinter             = &app->debugPrinter;
	GlobalDebugLogger              = &app->debugLogger;
	GlobalProfilingTable           = &app->profilingTable;
	debug_Values                   = &app->debugValues;
	app->profilingTable.infosCount = 0;

	result.success     = true;
	result.logStorage  = GlobalIngameLog;
	result.debugLogger = GlobalDebugLogger;
	result.textureMap  = GlobalTextureMap;
	ImGui              = &app->guiState;
	return result;
}

static void processCamera( GameInputs* inputs, GameSettings* settings, Camera* camera, float dt )
{
	auto speed = 20.0f * dt;
	if( isKeyDown( inputs, KC_W ) ) {
		auto dir = camera->look;
		camera->position += dir * speed;
	}
	if( isKeyDown( inputs, KC_S ) ) {
		auto dir = camera->look;
		camera->position += -dir * speed;
	}
	if( isKeyDown( inputs, KC_D ) ) {
		auto dir = camera->right;
		camera->position += dir * speed;
	}
	if( isKeyDown( inputs, KC_A ) ) {
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

#include "Editor/Common/DynamicVoxelCollection.cpp"
#include "Editor/TexturePack/TexturePack.cpp"
#include "Editor/Animator/Animator.cpp"
#include "Editor/Room/RoomEditor.cpp"
#include "VoxelEditor.cpp"

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
	builder.println( "Uploaded Meshes: {}\nDraw Calls: {}\nVertices: {}\nIndices: {}",
	                 info->uploadedMeshes, info->drawCalls, info->vertices, info->indices );

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

	if( app->gameState.player ) {
		builder << "\nWallJumpTimer: " << app->gameState.player->walljumpWindow.value
		        << "\nWallJumpDuration: " << app->gameState.player->walljumpDuration.value;
	}

	builder << '\n' << '\n';

	auto camera = &app->voxelState.camera;
	builder << "Camera:\n";
	builder.println( "Position: {}", camera->position );
	builder.println( "Look: {}", camera->look );
	builder.println( "Up: {}", camera->up );

	// output debug player state
	if( auto player = app->gameState.player ) {
		builder.println( "SpatialState: {}\nSpatialStateTimer: {}",
		                 getSpatialStateString( player->spatialState ), player->spatialStateTimer );
		builder.println( "Player pos: {}\nPlayer velocity: {}", player->position,
		                 player->velocity );
		builder.println( "Camera follow: {:.2}", app->gameState.cameraFollowRegion );
		if( player->skeleton->prevAnimations.size() ) {
			builder.println( "Current Animation Frame: {}",
			                 player->skeleton->prevAnimations[0].currentFrame );
		}
	}

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
		imguiLoadDefaultStyle( imgui, &app->platform, font );
		gui->mainDialog        = imguiGenerateContainer( &gui->debugGuiState );
		gui->debugOutputDialog = imguiGenerateContainer( &gui->debugGuiState, {200, 0, 600, 100} );
		gui->debugLogDialog    = imguiGenerateContainer( &gui->debugGuiState, {600, 0, 1000, 100} );
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
			game->player->position      = {};
			game->player->positionDelta = {};
		}
		if( imguiBeginDropGroup( "Debug Windows", &gui->windowsExpanded ) ) {
			auto debugOutputDialog = imguiGetContainer( gui->debugOutputDialog );
			auto showOutput = imguiCheckbox( "Debug Output", !debugOutputDialog->isHidden() );
			debugOutputDialog->setHidden( !showOutput );

			auto debugLogDialog = imguiGetContainer( gui->debugLogDialog );
			auto showLog = imguiCheckbox( "Debug Log", !debugLogDialog->isHidden() );
			debugLogDialog->setHidden( !showLog );
			imguiEndDropGroup();
		}
		imguiCheckbox( "Lighting", &game->lighting );
		imguiCheckbox( "Camera turning", &app->settings.cameraTurning );
		imguiCheckbox( "View collision boxes", &app->gameState.debugCollisionBoxes );
		imguiCheckbox( "Frame Step Counts", &gui->showFrameStepCounts );
	}

	if( imguiDialog( "Debug Log", gui->debugLogDialog ) ) {
		imguiSameLine( 2 );
		if( imguiButton( "Clear Debug Log" ) ) {
			debugLogClear();
		}
		imguiText( getTimeStampString() );
		const float LogHeight = 400;
		bool wasAtEnd   = gui->debugLogRegion.scrollPos.y >= gui->debugLogRegion.dim.y - LogHeight;
		auto scrollable = imguiBeginScrollableRegion( &gui->debugLogRegion, 400, LogHeight, true );

		imguiText( debugLogGetString() );

		if( wasAtEnd ) {
			auto container = imguiCurrentContainer();
			gui->debugLogRegion.scrollPos.y = height( container->rect );
		}
		imguiEndScrollableRegion( &gui->debugLogRegion, &scrollable );
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
static void processGameCamera( AppData* app, float dt )
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
			debugPrintln( "camera prev look: {}", camera->prevLook );
			debugPrintln( "camera next look: {}", camera->nextLook );
			debugPrintln( "camera dir: {}", dir );
			debugPrintln( "camera turn timer: {}", camera->turnTimer.value );
		}
	} else {
		*camera = cameraLookDirection( *camera, {0, 0, 1} );
	}
}

void setHeroActionAnimation( SkeletonSystem* system, Entity* entity, float dt )
{
	auto hero       = &get_variant( *entity, hero );
	const auto& ids = system->hero.animationIds;

	auto skeleton  = entity->skeleton;
	auto animation = hero->currentAnimationIndex;
	bool shooting  = (bool)hero->shootingAnimationTimer;
	bool repeating = true;

	if( !entity->grounded ) {
		hero->animationLockTimer = {};
	}
	hero->animationLockTimer     = processTimer( hero->animationLockTimer, dt );
	hero->shootingAnimationTimer = processTimer( hero->shootingAnimationTimer, dt );

	if( isCountdownTimerExpired( hero->animationLockTimer ) ) {
		animation = ( shooting ) ? ids.idleShoot : ids.idle;
		if( entity->grounded ) {
			if( !shooting && entity->spatialStateTimer < 10.0f ) {
				animation = ids.landing;
				repeating = false;
			} else {
				if( !shooting && entity->faceDirection != entity->prevFaceDirection ) {
					auto lock = skeleton->definition->animations[ids.turn].duration;
					hero->animationLockTimer = {lock};
					animation                = ids.turn;
				} else {
					if( entity->velocity.x != 0 ) {
						animation = ( shooting ) ? ids.walkShoot : ids.walk;
					}
				}
			}
		} else {
			if( entity->walljumpWindow ) {
				animation = ( shooting ) ? ids.wallslideShoot : ids.wallslide;
			} else {
				if( entity->velocity.y < 0 ) {
					animation = ( shooting ) ? ids.jumpRisingShoot : ids.jumpRising;
				} else {
					animation = ( shooting ) ? ids.jumpFallingShoot : ids.jumpFalling;
				}
			}
		}
		if( hero->currentAnimationIndex != animation ) {
			stopAnimations( skeleton );
			hero->currentAnimation = playAnimation( skeleton, animation, repeating );
		}
		hero->currentAnimationIndex = animation;
		entity->prevFaceDirection   = entity->faceDirection;
	}
}

void emitEntityParticles( GameState* game, float dt )
{
	FOR( entry : game->entitySystem.entries ) {
		auto traits = getEntityTraits( entry.type );
		if( traits->movement == EntityMovement::Grounded ) {
			// wallslide particles
			if( entry.wallslideCollidable && entry.skeleton && entry.type == Entity::type_hero ) {
				auto feetPosIndex = game->skeletonSystem.hero.nodeIds.feetPos;
				auto feetPosition = getNode( entry.skeleton, feetPosIndex ).xy;
				feetPosition.y    = -feetPosition.y;
				vec2 searchDir    = {1, 0};
				if( !entry.walljumpLeft() ) {
					searchDir = {-1, 0};
				}
				auto region = getSweptTileGridRegion( entry.aab, entry.position, searchDir );
				auto collisionAtFeet = findCollision(
				    {-1, -1, 1, 1}, feetPosition, searchDir, getCollisionLayer( &game->room ),
				    region, game->entitySystem.dynamicEntries(), 1, false );
				if( collisionAtFeet ) {
					auto hero               = &entry.hero;
					hero->particleEmitTimer = processTimer( hero->particleEmitTimer, dt );
					if( !hero->particleEmitTimer ) {
						hero->particleEmitTimer = {6};
						emitParticles( &game->particleSystem, feetPosition,
						               ParticleEmitterId::Dust );
					}
				}
			} else {
				if( auto hero = query_variant( entry, hero ) ) {
					hero->particleEmitTimer = {};
				}
			}

			// landing particles
			if( entry.spatialState == SpatialState::Grounded
			    && floatEqZero( entry.spatialStateTimer ) ) {

				auto feetPosition = entry.position;
				auto emitted      = emitParticles( &game->particleSystem, feetPosition,
				                              ParticleEmitterId::LandingDust );
			}
		}
	}
}

typedef void BehaviorFunctionType( GameState*, Entity*, float );
void processHeroEntity( GameState* game, Entity* entity, float dt )
{
	using namespace GameConstants;

	// process controls
	auto hero       = &get_variant( *entity, hero );
	const auto& ids = game->skeletonSystem.hero.animationIds;

	if( entity->flags.hurt ) {
		entity->control.responsiveness = EntityResponsiveness::NoControl;
		auto animation                 = ids.hurt;
		stopAnimations( entity->skeleton );
		hero->currentAnimation      = playAnimation( entity->skeleton, animation, false );
		hero->currentAnimationIndex = animation;
	}
	if( hero->currentAnimationIndex != ids.hurt
	    || isAnimationFinished( entity->skeleton, hero->currentAnimation ) ) {
		entity->control.responsiveness = EntityResponsiveness::Responsive;
	}

	auto control    = entity->control;

	entity->velocity.x = 0;
	if( control.responsiveness == EntityResponsiveness::Responsive ) {
		switch( control.horizontal ) {
			case EntityHorizontalMovement::Left: {
				if( isCountdownTimerExpired( entity->walljumpDuration )
				    || entity->walljumpLeft() ) {
					entity->velocity.x    = -MovementSpeed;
					entity->faceDirection = EntityFaceDirection::Left;
				}
				break;
			}
			case EntityHorizontalMovement::Right: {
				if( isCountdownTimerExpired( entity->walljumpDuration )
				    || !entity->walljumpLeft() ) {
					entity->velocity.x    = MovementSpeed;
					entity->faceDirection = EntityFaceDirection::Right;
				}
				break;
			}
		}
		if( entity->walljumpWindow ) {
			if( !entity->walljumpLeft() ) {
				entity->faceDirection = EntityFaceDirection::Right;
			} else {
				entity->faceDirection = EntityFaceDirection::Left;
			}
		}

		// jump
		if( control.vertical == EntityVerticalMovement::Jump
		    && isSpatialStateJumpable( entity ) ) {
			entity->velocity.y = JumpingSpeed;
			setSpatialState( entity, SpatialState::Airborne );

			entity->control.verticalAction = EntityVerticalAction::ConsumeInput;
		}

		// variable jump height
		if( entity->spatialState == SpatialState::Airborne
		    && control.vertical == EntityVerticalMovement::Jump && entity->velocity.y < 0 ) {
		} else {
			if( entity->velocity.y < 0 ) {
				entity->velocity.y = 0;
			}
		}

		// walljump
		if( control.verticalAction == EntityVerticalAction::Jump
		    && isSpatialStateWalljumpable( entity ) && entity->walljumpWindow ) {

			entity->wallslideCollidable.clear();
			entity->walljumpWindow         = {};
			entity->velocity.y             = WalljumpingSpeed;
			entity->walljumpDuration       = {WalljumpMaxDuration};
			entity->control.verticalAction = EntityVerticalAction::ConsumeInput;
		}

		// shooting
		if( control.action == EntityActionState::Attack ) {
			hero->shootingAnimationTimer = {20};
		}

		// update skeleton
		setHeroActionAnimation( &game->skeletonSystem, entity, dt );

		// emit projectile after skeleton update, since we need correct node positions
		if( control.action == EntityActionState::Attack ) {
			if( entity->skeleton->dirty ) {
				setMirrored( entity->skeleton, entity->faceDirection == EntityFaceDirection::Left );
				update( entity->skeleton, nullptr, 0 );
			}
			auto shootPosIndex = game->skeletonSystem.hero.nodeIds.shootPos;
			auto gunPosition   = getNode( entity->skeleton, shootPosIndex ).xy;
			gunPosition.y      = -gunPosition.y;
			vec2 direction     = {1, 0};
			if( entity->faceDirection == EntityFaceDirection::Left ) {
				direction.x = -direction.x;
			}
			if( !emitProjectile( game, gunPosition, direction, ProjectileType::Hero,
			                     entity->team ) ) {
				hero->shootingAnimationTimer = {};
			}
		}
	} else {
		if( entity->velocity.y < 0 ) {
			entity->velocity.y = 0;
		}
	}
}
void processWheelsEntity( GameState* game, Entity* entity, float dt )
{
	auto wheels      = &entity->wheels;
	bool stateChange = false;

	if( entity->flags.hurt ) {
		--wheels->stats.hp;
		if( wheels->stats.hp <= 0 ) {
			entity->flags.deathFlag = true;
			removeEntity( game, entity->handle );
		}
	}

	auto doHurt = [&]() {
		if( entity->flags.hurt ) {
			stopAnimations( entity->skeleton );
			wheels->currentAnimation = playAnimation(
			    entity->skeleton, game->skeletonSystem.wheels.animationIds.hurt, false );
			entity->acceleration.x = 0;
			entity->velocity.x     = -entity->hurtNormal.x * 0.1f;
			if( entity->hurtNormal.x < 0 ) {
				entity->faceDirection = EntityFaceDirection::Left;
			} else {
				entity->faceDirection = EntityFaceDirection::Right;
			}
			entity->wheels.state = Entity::Wheels::Hurt;
			stateChange          = true;
		}
	};
	auto doAttack = [&]() {
		wheels->attackTimer = processTimer( wheels->attackTimer, dt );
		if( isCountdownTimerExpired( wheels->attackTimer ) ) {
			wheels->attackTimer  = {120};
			entity->wheels.state = Entity::Wheels::Attacking;
			stateChange          = true;
		}
	};
	do {
		bool stateJustChanged = stateChange;
		stateChange           = false;
		switch( entity->wheels.state ) {
			case Entity::Wheels::Idle: {
				entity->maxSpeed      = {1, 0};
				entity->wheels.state  = Entity::Wheels::Moving;
				entity->faceDirection = EntityFaceDirection::Right;
				stateChange           = true;
				break;
			}
			case Entity::Wheels::Moving: {
				if( stateJustChanged ) {
					stopAnimations( entity->skeleton );
					wheels->currentAnimation = playAnimation(
					    entity->skeleton, game->skeletonSystem.wheels.animationIds.move, true );
				}
				if( floatEqZero( entity->acceleration.x ) ) {
					entity->acceleration.x =
					    ( entity->faceDirection == EntityFaceDirection::Right ) ? 0.1f : -0.1f;
				}
				if( entity->lastCollision && entity->lastCollision != entity->grounded ) {

					entity->wheels.state = Entity::Wheels::Turning;
					stateChange          = true;
				}
				doAttack();
				doHurt();
				break;
			}
			case Entity::Wheels::Turning: {
				if( stateJustChanged ) {
					wheels->shouldTurn     = false;
					entity->acceleration.x = 0;
					entity->velocity.x     = 0;
					stopAnimations( entity->skeleton );
					wheels->currentAnimation = playAnimation(
					    entity->skeleton, game->skeletonSystem.wheels.animationIds.turn );
				}
				if( ( !stateJustChanged
				      && isAnimationFinished( entity->skeleton, wheels->currentAnimation ) )
				    || entity->flags.hurt ) {
					if( entity->faceDirection == EntityFaceDirection::Right ) {
						entity->faceDirection = EntityFaceDirection::Left;
					} else {
						entity->faceDirection = EntityFaceDirection::Right;
					}
					entity->wheels.state = Entity::Wheels::Moving;
					stateChange          = true;
				}
				if( !entity->grounded ) {
					entity->wheels.state = Entity::Wheels::Moving;
					stateChange          = true;
				}
				doHurt();
				break;
			}
			case Entity::Wheels::Attacking: {
				if( stateJustChanged ) {
					entity->acceleration.x = -sign( entity->velocity.x ) * 0.02f;
					stopAnimations( entity->skeleton );
					wheels->currentAnimation = playAnimation(
					    entity->skeleton, game->skeletonSystem.wheels.animationIds.attack );
				} else if( isAnimationFinished( entity->skeleton, wheels->currentAnimation ) ) {
					if( wheels->shouldTurn ) {
						entity->wheels.state = Entity::Wheels::Turning;
					} else {
						entity->wheels.state = Entity::Wheels::Moving;
					}
					stateChange = true;
				}

				auto events = getAnimationEvents( entity->skeleton, wheels->currentAnimation );
				FOR( event : events ) {
					if( event.type == SkeletonEventType::Attack ) {
						vec2 velocity = {};
						velocity.x    = ( entity->faceDirection == EntityFaceDirection::Left )
						                 ? ( -1.0f )
						                 : 1.0f;
						auto origin = getNode( entity->skeleton,
						                       game->skeletonSystem.wheels.nodeIds.attackOrigin )
						                  .xy;
						origin.y = -origin.y;
						emitProjectile( game, origin, velocity, ProjectileType::Wheels,
						                entity->team );
					}
				}

				if( entity->lastCollision && entity->grounded
				    && entity->lastCollision != entity->grounded ) {
					entity->velocity.x     = -entity->prevVeloctiy.x;
					entity->acceleration.x = -entity->acceleration.x;
					wheels->shouldTurn     = true;
				}
				if( entity->flags.hurt ) {
					entity->velocity.x     = 0;
					entity->acceleration.x = 0;
				}
				if( floatEqZero( entity->velocity.x )
				    || signbit( entity->velocity.x ) == signbit( entity->acceleration.x ) ) {

					entity->acceleration.x = 0;
					entity->velocity.x     = 0;
				}
				break;
			}
			case Entity::Wheels::Hurt: {
				if( !stateJustChanged ) {
					doHurt();
				}
				if( isAnimationFinished( entity->skeleton, wheels->currentAnimation ) ) {
					entity->wheels.state = Entity::Wheels::Moving;
					stateChange          = true;
				}
				doAttack();
				break;
			}
			InvalidDefaultCase;
		}
	} while( stateChange );
}

void processEntityBehaviors( GameState* game, float dt )
{
	assert( game );

	emitEntityParticles( game, dt );

	static BehaviorFunctionType* const EntityBehaviors[] = {
	    processHeroEntity, processWheelsEntity,
	};
	static_assert( Entity::type_hero == 1, "Wrong entity type order" );
	static_assert( Entity::type_wheels == 2, "Wrong entity type order" );
	static_assert( countof( EntityBehaviors ) == Entity::type_count - 1,
	               "Not all behaviors defined" );

	FOR( entry : game->entitySystem.staticEntries() ) {
		assert( entry.type != Entity::type_none );
		EntityBehaviors[entry.type - 1]( game, &entry, dt );
	}
}

static void updateGame( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	PROFILE_FUNCTION();

	// auto font      = &app->font;
	auto game     = &app->gameState;
	auto settings = &app->settings;

	if( !focus ) {
		return;
	}
	if( !game->useGameCamera ) {
		float speed = dt * 0.25f;
		if( isKeyDown( inputs, KC_Shift ) ) {
			speed = dt;
		}
		if( isKeyDown( inputs, KC_Alt ) ) {
			speed = dt * 0.01f;
		}
		processCamera( inputs, settings, &app->voxelState.camera, speed );
	}

	if( isKeyPressed( inputs, KC_H ) ) {
		game->paused = !game->paused;
	}
	if( isKeyPressed( inputs, KC_0 ) ) {
		game->useGameCamera = !game->useGameCamera;
	}
	if( isKeyPressed( inputs, KC_1 ) ) {
		auto handle = addEntityHandle( &game->entityHandles );
		addEntity( &game->entitySystem, &game->skeletonSystem, handle, Entity::type_wheels,
		           {16 * 6, 16 * 8} );
	}

	// mouse lock
	if( isKeyPressed( inputs, KC_L ) ) {
		app->mouseLocked = !app->mouseLocked;
	}
	if( app->focus == AppFocus::Game ) {
		inputs->mouse.locked = app->mouseLocked;
	}

	game->prevCamera = game->camera;
	processGameCamera( app, dt );
	processControlSystem( game, &game->controlSystem, &game->entitySystem, inputs, dt );
	// update aab's
	FOR( entity : game->entitySystem.entries ) {
		if( entity.skeleton ) {
			auto traits = getEntityTraits( entity.type );
			if( !traits->flags.noFaceDirection ) {
				setMirrored( entity.skeleton, entity.faceDirection == EntityFaceDirection::Left );
				update( entity.skeleton, nullptr, 0 );
			}

			auto skeletonTraits = getSkeletonTraits( &game->skeletonSystem, entity.type );
			auto collisionIds = skeletonTraits->collisionIds();
			if( collisionIds.size() ) {
				entity.aab = getHitboxRelative( entity.skeleton, collisionIds[0] ).first;
			}
		}
	}
	doCollisionDetection( &game->room, &game->entitySystem, dt );
	processProjectiles( game, getCollisionLayer( &game->room ), game->entitySystem.dynamicEntries(),
	                    dt );

	// hit detection
	{
		auto hitboxSystem = &game->hitboxSystem;
		auto pairs        = hitboxSystem->hitboxPairs();
		auto testHit      = [&]( Entity* entity, EntityHandle attacker, rectfarg hitbox,
		                    vec2arg position, vec2arg delta, rectf* hitBounds ) {
			if( entity->skeleton ) {
				auto defender     = entity->handle;
				auto existingPair = find_index_if( pairs, [=]( const auto& entry ) {
					return entry.attacker == attacker && entry.defender == defender;
				} );
				if( !existingPair ) {
					auto otherSkeletonTraits =
					    getSkeletonTraits( &game->skeletonSystem, entity->type );
					auto combinedDelta = delta - entity->positionDelta;
					FOR( hurtboxId : otherSkeletonTraits->hurtboxIds() ) {
						auto otherHurtbox = getHitboxAbsolute( entity->skeleton, hurtboxId );
						if( !otherHurtbox.second ) {
							continue;
						}
						auto otherBounds = translate( otherHurtbox.first, -entity->positionDelta );
						CollisionInfo info;
						if( testAabVsAab( hitbox, position, combinedDelta, otherBounds, 1,
						                  &info ) ) {
							if( !entity->flags.deflects && !entity->flags.invincible ) {
								entity->flags.hurt = true;
								entity->hurtNormal = info.normal;
								pairs.push_back( {attacker, defender} );
							}

							if( hitBounds ) {
								*hitBounds = otherBounds;
							}
							return true;
						}
					}
				}
			}
			return false;
		};
		// clear hurt flags
		auto staticEntries = game->entitySystem.staticEntries();
		FOR( entity : staticEntries ) {
			entity.flags.hurt = false;
		}
		for( auto i = 0, count = staticEntries.size(); i < count; ++i ) {
			auto entity = &staticEntries[i];
			if( entity->skeleton ) {
				auto skeletonTraits = getSkeletonTraits( &game->skeletonSystem, entity->type );
				auto delta          = entity->positionDelta;
				auto prevPosition   = entity->position - entity->positionDelta;
				FOR( hitboxId : skeletonTraits->hitboxIds() ) {
					auto hitbox = getHitboxRelative( entity->skeleton, hitboxId );
					if( !hitbox.second ) {
						continue;
					}
					FOR( other : staticEntries ) {
						if( &other == entity || other.team == entity->team
						    || other.flags.deathFlag ) {
							continue;
						}
						testHit( &other, entity->handle, hitbox.first, prevPosition, delta,
						         nullptr );
					}
				}
			}
		}

		FOR( entry : game->projectileSystem.entries ) {
			if( entry.durability <= 0 ) {
				continue;
			}
			auto data = getProjectileData( &game->projectileSystem, entry.type );
			const auto& hitbox = data->hitbox;
			auto delta         = entry.positionDelta;
			auto prevPosition  = entry.position - entry.positionDelta;
			FOR( other : staticEntries ) {
				if( other.team == entry.team || other.flags.deathFlag || other.flags.invincible ) {
					continue;
				}
				if( other.flags.hurt ) {
					// FIXME: find a better solution for not hurting hurt entities multiple
					// times. If an entity turns invincible after a hit, removing the continue
					// means they might still get hit multiple times, if the hits all happen on
					// the exact same frame because invincibility only starts after hit
					// detection
					continue;
				}
				rectf hitBounds = {};
				if( testHit( &other, entry.handle, hitbox, prevPosition, delta, &hitBounds ) ) {
					if( other.flags.deflects ) {
						entry.deflected = true;
						entry.velocity  = safeNormalize( prevPosition - center( hitBounds ) )
						                 * length( entry.velocity );
					} else {
						--entry.durability;
						if( entry.durability <= 0 ) {
							entry.aliveCountdown = {};
						}
					}
					break;
				};
			}
		}
		hitboxSystem->setPairsCount( pairs.size() );
	}

	// hurt flashing
	FOR( entry : game->entitySystem.staticEntries() ) {
		entry.hurtFlashCountdown = processTimer( entry.hurtFlashCountdown, dt );
		if( entry.flags.hurt ) {
			entry.hurtFlashCountdown = {3};
		}
		if( entry.skeleton ) {
			if( entry.hurtFlashCountdown ) {
				entry.skeleton->rootTransform.flashColor = 0xD0D9EFFF;
			} else {
				entry.skeleton->rootTransform.flashColor = 0;
			}
		}
	}

	// update skeleton transform
	FOR( entity : game->entitySystem.entries ) {
		if( entity.skeleton ) {
			vec3 origin = Vec3( gameToScreen( entity.position ), 0 );
			setTransform( entity.skeleton, matrixTranslation( origin ) );
		}
	}
	processEntityBehaviors( game, dt );
	processEntityRemovalQueue( game );

	// entity behaviors might have changed face direction
	FOR( entity : game->entitySystem.entries ) {
		if( entity.skeleton ) {
			auto traits = getEntityTraits( entity.type );
			if( !traits->flags.noFaceDirection ) {
				setMirrored( entity.skeleton, entity.faceDirection == EntityFaceDirection::Left );
			}
		}
	}
	advanceSkeletons( &game->skeletonSystem, dt );
}

static void initializeGame( AppData* app, GameInputs* inputs )
{
	auto game = &app->gameState;
	if( game->initialized ) {
		return;
	}
	auto allocator               = &app->stackAllocator;
	game->particleSystem         = makeParticleSystem( allocator, 200 );
	game->particleSystem.texture = app->platform.loadTexture( "Data/Images/particles.png" );

	game->projectileSystem = makeProjectileSystem( allocator, 200 );

	game->outlineShader =
	    app->platform.loadShader( "Shaders/scale_by_normal.vsh", "Shaders/single_color.fsh" );

	loadTileSet( allocator, "Data/voxels/default_tileset.json", &app->gameState.tileSet );
	game->skeletonSystem = makeSkeletonSystem();

	auto maxEntities         = 10;
	game->entityHandles      = makeHandleManager();
	game->entitySystem       = makeEntitySystem( allocator, maxEntities );
	game->controlSystem      = makeControlSystem( allocator, maxEntities );
	game->entityRemovalQueue = makeUArray( allocator, EntityHandle, maxEntities );

	auto addHeroEntity = [&]( vec2arg pos ) {
		auto handle = addEntityHandle( &game->entityHandles );
		auto entity =
		    addEntity( &game->entitySystem, &game->skeletonSystem, handle, Entity::type_hero, pos );
		return entity;
	};
	game->player = addHeroEntity( {16 * 2, 0} );
	addControlComponent( &game->controlSystem, game->player->handle );

	/*auto addMovingPlatform = [&]( vec2arg pos ) {
	    auto platform            = addHeroEntity( pos, true );
	    platform->movement       = EntityMovement::Straight;
	    platform->response       = CollisionResponse::Bounce;
	    platform->bounceModifier = 2;
	    platform->setForcedNormal( {1, 0} );
	    platform->velocity                = {1.0f, 0};
	    platform->gravityModifier         = 0;
	    platform->airFrictionCoeffictient = 0;
	};
	addMovingPlatform( {16 * 3} );
	addMovingPlatform( {16 * 8} );*/

	game->camera             = makeGameCamera( {0, -50, -200}, {0, 0, 1}, {0, 1, 0} );
	game->cameraFollowRegion = {-25, -50, 25, 50};
	game->useGameCamera      = true;
	game->lighting           = false;

	game->room        = debugGetRoom( allocator, &game->tileSet );
	game->initialized = true;

	updateGame( app, inputs, true, 1 );
}

void renderGame( AppData* app, GameInputs* inputs, float blendFactor, bool focus, int32 stepCount )
{
	if( !focus ) {
		return;
	}

	auto renderer = &app->renderer;
	auto game     = &app->gameState;

	setRenderState( renderer, RenderStateType::Lighting, game->lighting );
	setProjection( renderer, ProjectionType::Perspective );
	auto cameraTranslation = matrixTranslation( 0, -50, 0 );
	Camera camera          = app->voxelState.camera;
	if( game->useGameCamera ) {
		auto& prev      = game->prevCamera;
		auto& next      = game->camera;
		camera.position = lerp( blendFactor, prev.position, next.position );
		camera.look     = lerp( blendFactor, prev.look, next.look );
		camera.right    = lerp( blendFactor, prev.right, next.right );
		camera.up       = lerp( blendFactor, prev.up, next.up );
	}
	renderer->view = cameraTranslation * getViewMatrix( &camera );

	const mat4 rotations[] = {
	    matrixIdentity(), matrixRotationZOrigin( HalfPi32, 8, 8 ),
	    matrixRotationZOrigin( Pi32, 8, 8 ), matrixRotationZOrigin( Pi32 + HalfPi32, 8, 8 ),
	};

	auto matrixStack = renderer->matrixStack;
	{
		// render tiles
		auto tileWidth             = GameConstants::TileWidth;
		auto tileHeight            = GameConstants::TileHeight;
		const float zTranslation[] = {0, TILE_DEPTH, -TILE_DEPTH};
		static_assert( countof( zTranslation ) == RL_Count, "" );
		for( auto i = 0; i < RL_Count; ++i ) {
			auto layer   = &app->gameState.room.layers[i];
			auto grid    = layer->grid;
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
	for( auto& entry : game->entitySystem.entries ) {
		auto position = entry.position - entry.positionDelta * ( 1 - blendFactor );
		setTransform( entry.skeleton, matrixTranslation( position.x, -position.y, 0 ) );
		update( entry.skeleton, &game->particleSystem, blendFactor - 1 );
		render( renderer, entry.skeleton );
	}
	processParticles( &game->particleSystem,
	                  (float)stepCount + blendFactor - game->prevBlendFactor );
	game->prevBlendFactor = blendFactor;

	// render projectiles
	FOR( entry : game->projectileSystem.entries ) {
		auto position = entry.position - entry.positionDelta * ( 1 - blendFactor );
		auto data = getProjectileData( &game->projectileSystem, entry.type );
		setTexture( renderer, 0, data->texture );
		auto offset   = data->frame.offset;
		position = gameToScreen( position - offset );
		pushMatrix( matrixStack );
		translate( matrixStack, position, data->z );
		auto mesh               = addRenderCommandMesh( renderer, data->frame.mesh );
		mesh->screenDepthOffset = -0.02f;
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
			stream->color = setAlpha( Color::Blue, 0.5f );
			FOR( entry : game->entitySystem.entries ) {
				auto screenRect = gameToScreen( translate( entry.aab, entry.position ) );
				pushAabb( stream, screenRect.left, screenRect.bottom, -16, screenRect.right,
				          screenRect.top, 16 );
			}
		}
		// setRenderState( renderer, RenderStateType::DepthTest, true );
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

	const vec2 InitialPosition = {0, 0};
	const vec2 TargetPosition  = {0, 200};
	const float SimHeight      = 200;
	auto reset = [=]() {
		easing->position     = InitialPosition;
		easing->velocity     = {};
		easing->acceleration = {};
	};

	imguiBind( gui );
	if( !easing->initialized ) {
		*gui = defaultImmediateModeGui();
		imguiLoadDefaultStyle( gui, &app->platform, font );

		reset();

		easing->initialized = true;
	}
	imguiBind( gui, renderer, font, inputs, app, {0, 0, app->width, app->height} );
	setProjection( renderer, ProjectionType::Orthogonal );

	renderer->color = Color::White;
	renderer->clearColor = 0xFF1B2B34;
	imguiEditbox( "Duration", &easing->duration );

	imguiSameLine( 3 );
	auto simRect      = imguiAddItem( 200, SimHeight );
	auto wipRect      = imguiAddItem( 200, SimHeight );
	auto combinedRect = imguiAddItem( 200, SimHeight );

	auto frameDelta = dt * GameConstants::DeltaToFrameTime;
	easing->t += frameDelta;
	if( easing->t > easing->duration ) {
		easing->t     = 0;
		easing->lastT = 0;
		reset();
	}

	if( easing->t - easing->lastT >= 1.0f ) {
		const float Tightness = 0.055f;
		const float Damper = 0.14f;
		easing->acceleration =
		    -Tightness * ( easing->position - TargetPosition ) - Damper * easing->velocity;
		easing->velocity += easing->acceleration;
		easing->lastT = easing->t;
	}
	easing->position += easing->velocity * frameDelta;

	imguiAddItem( 100, 50 );

	imguiSameLine( 3 );
	auto visualizer0        = imguiAddItem( 200, 200 );
	auto visualizer1        = imguiAddItem( 200, 200 );
	auto combinedVisualizer = imguiAddItem( 200, 200 );

	auto t                      = easing->t / easing->duration;
	auto tIndex                 = ( int32 )( t * countof( easing->yPositions0 ) );
	tIndex                      = clamp( tIndex, 0, countof( easing->yPositions0 ) - 1 );
	easing->yPositions0[tIndex] = ( easing->position.y - InitialPosition.y ) / SimHeight;

	auto easingWip = []( float t ) -> float {
		return easeOutElastic( t );
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

	auto doVisualizer = [&]( rectfarg visualizer, float ( *easer )( float ) ) {
		char buffer[100];
		if( isPointInside( visualizer, inputs->mouse.position ) ) {
			auto mouse      = inputs->mouse.position - visualizer.leftTop;
			auto x          = mouse.x / width( visualizer );
			renderer->color = Color::White;

			int32 len = 0;
			if( easer ) {
				len = snprint( buffer, countof( buffer ), "X: {} Y: {}", x, easer( x ) );
			} else {
				auto tIndex = ( int32 )( x * countof( easing->yPositions0 ) );
				tIndex      = clamp( tIndex, 0, countof( easing->yPositions0 ) - 1 );
				auto y      = easing->yPositions0[tIndex];
				len         = snprint( buffer, countof( buffer ), "X: {} Y: {}", x, y );
			}
			renderText( renderer, font, {buffer, len},
			            RectWH( inputs->mouse.position.x, inputs->mouse.position.y - 16, 0, 0 ) );
		}
		if( easer ) {
			auto rect = translate( visualizer, 0, height( visualizer ) );
			auto len =
			    snprint( buffer, countof( buffer ), "X: 0 Y: {}\nX: 0.9999 Y: {}\nX: 1 Y: {}",
			             easer( 0 ), easer( 0.9999f ), easer( 1 ) );
			renderText( renderer, font, {buffer, len}, rect );
		}
	};
	doVisualizer( visualizer0, nullptr );
	doVisualizer( visualizer1, easingWip );
	doVisualizer( combinedVisualizer, easingWip );

	auto renderDot = []( MeshStream* stream, rectfarg rect, vec2arg pos ) {
		vec2 simPos = pos + rect.leftTop;
		simPos.x += width( rect ) * 0.5f;
		pushQuad( stream, RectHalfSize( simPos, 4, 4 ) );
	};

	setTexture( renderer, 0, null );
	MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::Red;
		renderDot( stream, simRect, easing->position );
		renderDot( stream, combinedRect, easing->position );

		stream->color = Color::Blue;
		vec2 wipPos   = {0, easing->yPositions1[tIndex] * height( wipRect )};
		renderDot( stream, wipRect, wipPos );
		renderDot( stream, combinedRect, wipPos );
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
		} else if( isPointInside( visualizer1, inputs->mouse.position ) ) {
			stream->color = Color::Red;
			pushLine( stream, {inputs->mouse.position.x, visualizer1.top, 0},
			          {inputs->mouse.position.x, visualizer1.bottom, 0} );
		} else if( isPointInside( combinedVisualizer, inputs->mouse.position ) ) {
			stream->color = Color::Red;
			pushLine( stream, {inputs->mouse.position.x, combinedVisualizer.top, 0},
			          {inputs->mouse.position.x, combinedVisualizer.bottom, 0} );
		}
	}

	imguiUpdate( dt );
	imguiFinalize();
}

GAME_STORAGE struct RenderCommands* updateAndRender( void* memory, struct GameInputs* inputs,
                                                     struct GameInputs* fixedInputs, float dt,
                                                     int32 stepCount, float blendFactor );
UPDATE_AND_RENDER( updateAndRender )
{
	GlobalProfilingTable->eventsCount = 0;
	BEGIN_PROFILING_BLOCK( "updateAndRender" );

	auto app       = (AppData*)memory;
	auto renderer  = &app->renderer;
	auto font      = &app->font;
	auto allocator = &app->stackAllocator;

	inputs->disableEscapeForQuickExit = false;

	if( !app->resourcesLoaded ) {
		app->font = app->platform.loadFont( allocator, "Arial", 11, 400, false,
		                                    getDefaultFontUnicodeRanges() );
		imguiLoadDefaultStyle( &app->guiState, &app->platform, font );
		app->resourcesLoaded = true;

		LOG( INFORMATION, "Resources Loaded" );
	}

	// beginning of the frame
	debugPrintClear();
	clear( renderer );
	renderer->ambientStrength = 0.1f;
	renderer->lightColor      = Color::White;
	renderer->flashColor      = 0;

	setProjectionMatrix( renderer, ProjectionType::Perspective, app->perspective );
	setProjectionMatrix( renderer, ProjectionType::Orthogonal, app->orthogonal );

	debug_Clear();

	// options
	if( isHotkeyPressed( inputs, KC_Z, KC_Control ) ) {
		debug_FillMeshStream = !debug_FillMeshStream;
	}
	if( isHotkeyPressed( inputs, KC_I, KC_Control ) ) {
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
	if( isKeyPressed( inputs, KC_F5 ) ) {
		app->focus = AppFocus::RoomEditor;
	}
	if( isKeyPressed( inputs, KC_F8 ) ) {
		app->focus = AppFocus::Easing;
	}
	if( isHotkeyPressed( inputs, KC_R, KC_Control ) ) {
		renderer->lightPosition = app->voxelState.camera.position;
	}

	// reset mouse lock, locking must be done on each frame, so that on focus change mouse becomes
	// unlocked automatically
	inputs->mouse.locked = false;

	if( app->gameState.debugGui.showFrameStepCounts ) {
		debugLogln( "{}", stepCount );
	}
	initializeGame( app, fixedInputs );
	for( auto i = 0; i < stepCount; ++i ) {
		updateGame( app, fixedInputs, app->focus == AppFocus::Game, 1.0f );
		resetInputs( fixedInputs );
	}
	renderGame( app, inputs, blendFactor, app->focus == AppFocus::Game, stepCount );

	doVoxel( app, inputs, app->focus == AppFocus::Voxel, dt );
	doTexturePack( app, inputs, app->focus == AppFocus::TexturePack, dt );
	doAnimator( app, inputs, app->focus == AppFocus::Animator, dt );
	doEasing( app, inputs, app->focus == AppFocus::Easing, dt );
	RoomEditor::doRoomEditor( app, inputs, app->focus == AppFocus::RoomEditor, dt );

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

#ifndef NO_PROFILING
	TEMPORARY_MEMORY_BLOCK( &app->stackAllocator ) {
		auto state = processProfilingEvents( &app->stackAllocator, GlobalProfilingTable );
		debugPrintln( "Infos: {}", state.infos.size() );
		visitBlock( state.infos, state.blocks.head, 2 );
	}
#endif

	showGameDebugGui( app, inputs, true, dt );

	return renderer;
}