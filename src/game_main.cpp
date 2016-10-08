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
#include <Core/Unicode.cpp>
#include <tm_conversion_wrapper.cpp>

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

#define CELL_MAX_X 32
#define CELL_MAX_Y 32
#define CELL_MAX_Z 32
#define CELL_MAX_IN_PLANE ( CELL_MAX_X * CELL_MAX_Y )
#define CELL_MAX_COUNT ( CELL_MAX_X * CELL_MAX_Y * CELL_MAX_Z )

#define EDITOR_CELL_WIDTH 32.0f
#define EDITOR_CELL_HEIGHT 32.0f
#define EDITOR_CELL_DEPTH 32.0f

const vec3 EditorVoxelCellSize = {EDITOR_CELL_WIDTH, EDITOR_CELL_HEIGHT, EDITOR_CELL_DEPTH};

#define CELL_WIDTH 1.0f
#define CELL_HEIGHT 1.0f
#define CELL_DEPTH 1.0f

const vec3 VoxelCellSize = {CELL_WIDTH, CELL_HEIGHT, CELL_DEPTH};

#define TILE_CELLS_X 16
#define TILE_CELLS_Y 16
#define TILE_CELLS_Z 16
#define TILE_WIDTH ( CELL_WIDTH * TILE_CELLS_X )
#define TILE_HEIGHT ( CELL_HEIGHT * TILE_CELLS_Y )
#define TILE_DEPTH ( CELL_DEPTH * TILE_CELLS_Z )

#define CELL_ONE_OVER_WIDTH ( 1.0f / EDITOR_CELL_WIDTH )
#define CELL_ONE_OVER_HEIGHT ( 1.0f / EDITOR_CELL_HEIGHT )
#define CELL_ONE_OVER_DEPTH ( 1.0f / EDITOR_CELL_DEPTH )

#define SET_VOXEL_FACE( front, left, back, right, top, bottom )                              \
	( ( ( (VoxelCell)front + 1 ) << ( 0 * 4 ) ) | ( ( (VoxelCell)left + 1 ) << ( 1 * 4 ) )   \
	  | ( ( (VoxelCell)back + 1 ) << ( 2 * 4 ) ) | ( ( (VoxelCell)right + 1 ) << ( 3 * 4 ) ) \
	  | ( ( (VoxelCell)top + 1 ) << ( 4 * 4 ) ) | ( ( (VoxelCell)bottom + 1 ) << ( 5 * 4 ) ) )

typedef uint32 VoxelCell;
constexpr const VoxelCell EmptyCell   = {0};
constexpr const VoxelCell DefaultCell = {SET_VOXEL_FACE( 0, 1, 2, 3, 4, 5 )};
struct VoxelGrid {
	VoxelCell data[CELL_MAX_COUNT];
	union {
		struct {
			int32 width;
			int32 height;
			int32 depth;
		};
		vec3i dim;
	};

	int32 size() { return width * height * depth; }
	int32 max_size() { return CELL_MAX_COUNT; }
};
enum VoxelFaceValues : uint32 {
	VF_Front,
	VF_Left,
	VF_Back,
	VF_Right,
	VF_Top,
	VF_Bottom,

	VF_Count,
};
inline vec2i getTexelPlaneByFace( int32 face )
{
	constexpr vec2i texelPlaneByFace[] = {
		{VectorComponent_X, VectorComponent_Y},
		{VectorComponent_Z, VectorComponent_Y},
		{VectorComponent_X, VectorComponent_Y},
		{VectorComponent_Z, VectorComponent_Y},
		{VectorComponent_X, VectorComponent_Z},
		{VectorComponent_X, VectorComponent_Z},
	};
	assert( face >= 0 && face < countof( texelPlaneByFace ) );
	return texelPlaneByFace[face];
}

uint32 getVoxelFaceTexture( VoxelCell cell, VoxelFaceValues face )
{
	auto value = valueof( face );
	auto textureIndex = ( ( cell >> ( value * 4 ) ) & 0xFu ) - 1;
	return textureIndex;
}
VoxelCell setVoxelFaceTexture( VoxelCell cell, VoxelFaceValues face, int32 index )
{
	uint32 shiftAmount = ( valueof( face ) * 4 );
	VoxelCell masked   = ( ( VoxelCell )( ( index + 1 ) & 0xFu ) ) << shiftAmount;
	return ( ( cell & ~( 0xFu << shiftAmount ) ) | masked );
}

bool isPointInsideVoxelBounds( VoxelGrid* grid, int32 x, int32 y, int32 z )
{
	return ( x >= 0 && x < grid->width ) && ( y >= 0 && y < grid->height )
	       && ( z >= 0 && z < grid->depth );
}
bool isPointInsideVoxelBounds( VoxelGrid* grid, vec3iarg position )
{
	return ( position.x >= 0 && position.x < grid->width )
	       && ( position.y >= 0 && position.y < grid->height )
	       && ( position.z >= 0 && position.z < grid->depth );
}
VoxelCell& getCell( VoxelGrid* grid, int32 x, int32 y, int32 z )
{
	assert( isPointInsideVoxelBounds( grid, x, y, z ) );
	int32 index = x + y * grid->width + z * grid->width * grid->height;
	return grid->data[index];
}
VoxelCell& getCell( VoxelGrid* grid, vec3iarg position )
{
	assert( isPointInsideVoxelBounds( grid, position ) );
	int32 index =
	    position.x + position.y * grid->width + position.z * grid->width * grid->height;
	return grid->data[index];
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

enum class EditMode {
	Build,
	Select,

	Count
};
static const char* EditModeStrings[] = {
    "Build", "Select",
};

enum class SelectedAxis { None, xAxis, yAxis, zAxis };
enum class DragAction {
	None,
	DragSelection,
	MoveSelection,
	MoveVoxels,
};

enum class VoxelFocus {
	Voxel,
	Gui
};
struct VoxelGuiState {
	int32 editMode;
	int32 rendering;
	int32 textureIndex;
	float fadeProgress;
	bool initialized;

	bool lightingChecked;
	bool noLightingChecked;

	bool fileExpanded;
	bool sizesExpanded;
	bool texturesExpanded;

	int32 mappingType;
};
struct VoxelGridTextureMap {
	TextureId texture;
	struct Entry {
		QuadTexCoords texCoords;
	};
	Entry entries[6];
};

struct VoxelState {
	vec3 position;
	Camera camera;
	MeshStream meshStream;
	VoxelGrid voxels;
	VoxelGrid voxelsMoving;
	VoxelGrid voxelsCombined;
	VoxelCell placingCell;
	bool lighting;
	bool initialized;

	VoxelFocus focus;
	EditMode editMode;
	aabbi selection;
	VoxelGuiState gui;

	bool isFaceSelected;
	vec3i selectionNormal;
	vec3 selectionOrigin;
	aabb selectionWorld;
	DragAction dragAction;
	vec3 lastAxisPosition;

	VoxelGridTextureMap textureMap;
};

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

void generateMeshFromVoxelGridNaive( MeshStream* stream, VoxelGrid* grid )
{
	assert( isValid( stream ) );
	assert( grid );

	stream->color = 0xFF000000;
	auto yStart   = grid->height * EDITOR_CELL_HEIGHT;

	for( intmax z = 0; z < grid->depth; ++z ) {
		for( intmax y = 0; y < grid->height; ++y ) {
			for( intmax x = 0; x < grid->width; ++x ) {
				intmax index = ( x ) + ( y * grid->width ) + ( z * grid->width * grid->height );
				intmax cell  = grid->data[index];
				if( cell != EmptyCell ) {
					stream->color = 0xFF000000;
					float left   = x * EDITOR_CELL_WIDTH;
					float bottom = yStart - y * EDITOR_CELL_HEIGHT - EDITOR_CELL_HEIGHT;
					float near   = z * EDITOR_CELL_DEPTH;
					float right  = left + EDITOR_CELL_WIDTH;
					float top    = bottom + EDITOR_CELL_HEIGHT;
					float far    = near + EDITOR_CELL_DEPTH;
					pushAabb( stream, left, bottom, near, right, top, far );
				}
			}
		}
	}
}

VoxelGridTextureMap makeDefaultVoxelGridTextureMap( TextureId texture )
{
	VoxelGridTextureMap result = {};
	result.texture = texture;
	const float subdivisionWidth = 1.0f / 6.0f;
	for( intmax i = 0; i < 6; ++i ) {
		auto entry = &result.entries[i];
		auto left = i * subdivisionWidth;
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
void generateMeshFromVoxelGrid( MeshStream* stream, VoxelGrid* grid, VoxelGridTextureMap* textures,
                                vec3arg cellSize )
{
	assert( isValid( stream ) );
	assert( grid );

	uint8 map[CELL_MAX_IN_PLANE];

	struct FindFirstQuadResult {
		bool found;
		vec3i position;
	};
	struct PlaneDescriptor {
		vec3 hAxis;
		vec3 vAxis;
		vec3 zAxis;
		vec3 origin;
		int8 hComponent;
		int8 vComponent;
		int8 zComponent;
		float hSize;
		float vSize;
		float zSize;
		int32 hCellCount;
		int32 vCellCount;
		int32 zCount;
		int8 frontOffset;
		Normal normal;
		bool isBackFacing;

		VoxelFaceValues face;
	};
	auto findFirstUnprocessedQuad = []( VoxelGrid* grid, uint8* map, PlaneDescriptor* plane,
	                                    int32 layer ) -> FindFirstQuadResult {
		// assume we are searching on the xy plane
		for( int32 y = 0; y < plane->vCellCount; ++y ) {
			for( int32 x = 0; x < plane->hCellCount; ++x ) {
				auto mapIndex = x + y * plane->hCellCount;
				vec3i currentCell;
				currentCell.elements[plane->hComponent] = x;
				currentCell.elements[plane->vComponent] = y;
				currentCell.elements[plane->zComponent] = layer;
				vec3i frontCell                         = currentCell;
				frontCell.elements[plane->zComponent] += plane->frontOffset;

				if( getCell( grid, currentCell ) != EmptyCell && map[mapIndex] == 0 ) {
					// does the quad we are currently at need to be generated?
					if( !isPointInsideVoxelBounds( grid, frontCell )
					    || getCell( grid, frontCell ) == EmptyCell ) {
						FindFirstQuadResult result;
						result.found                                = true;
						result.position.elements[plane->hComponent] = x;
						result.position.elements[plane->vComponent] = y;
						result.position.elements[plane->zComponent] = layer;
						return result;
					}
				}
			}
		}
		return {false};
	};

	auto isGeneratingQuad = []( VoxelGrid* grid, PlaneDescriptor* plane, uint8* map, int32 x,
	                            int32 y, int32 z, uint32 textureIndex ) {
		auto mapIndex = x + y * plane->hCellCount;
		vec3i currentCell;
		currentCell.elements[plane->hComponent] = x;
		currentCell.elements[plane->vComponent] = y;
		currentCell.elements[plane->zComponent] = z;
		vec3i frontCell                         = currentCell;
		frontCell.elements[plane->zComponent] += plane->frontOffset;

		return getCell( grid, currentCell ) != EmptyCell
		       && getVoxelFaceTexture( getCell( grid, currentCell ), plane->face ) == textureIndex
		       && map[mapIndex] == 0 && ( !isPointInsideVoxelBounds( grid, frontCell )
		                                  || getCell( grid, frontCell ) == EmptyCell );
	};

	auto processPlane = [findFirstUnprocessedQuad, isGeneratingQuad](
	    MeshStream* stream, VoxelGrid* grid, VoxelGridTextureMap* textures, PlaneDescriptor* plane,
	    uint8* map, size_t mapSize ) {

		for( int32 z = 0; z < plane->zCount; ++z ) {
			memset( map, 0, mapSize * sizeof( uint8 ) );
			for( ;; ) {
				auto first = findFirstUnprocessedQuad( grid, map, plane, z );
				if( first.found ) {
					auto position = swizzle( first.position, plane->hComponent, plane->vComponent,
					                         plane->zComponent );
					auto dim = swizzle( grid->dim, plane->hComponent, plane->vComponent,
					                    plane->zComponent );
					auto textureIndex =
					    getVoxelFaceTexture( getCell( grid, first.position ), plane->face );
					map[position.x + position.y * plane->hCellCount] = 1;

					vec3 startVertex = plane->hAxis * ( plane->hSize * position.x )
					                   + plane->vAxis * ( plane->vSize * position.y )
					                   + plane->zAxis * ( plane->zSize * position.z )
					                   + plane->origin;
					assert( textureIndex < (uint32)countof( textures->entries ) );
					auto textureEntry = &textures->entries[textureIndex];
					auto texelPlane   = getTexelPlaneByFace( textureIndex );
					auto tw           = getAxisAlignedWidth( textureEntry->texCoords );
					auto th           = getAxisAlignedHeight( textureEntry->texCoords );
					auto texelWidth  = tw / grid->dim[texelPlane.x];
					auto texelHeight = th / grid->dim[texelPlane.y];
					auto tu          = first.position[texelPlane.x] * texelWidth
					          + textureEntry->texCoords.elements[0].u;
					auto tv = first.position[texelPlane.y] * texelHeight
					          + textureEntry->texCoords.elements[0].v;
					Vertex quad[4] = {
					    {startVertex, 0xFFFFFFFF, tu, tv, plane->normal},
					    {startVertex + plane->hAxis * plane->hSize, 0xFFFFFFFF, tu + texelWidth, tv,
					     plane->normal},
					    {startVertex + plane->vAxis * plane->vSize, 0xFFFFFFFF, tu,
					     tv + texelHeight, plane->normal},
					    {startVertex + plane->vAxis * plane->vSize + plane->hAxis * plane->hSize,
					     0xFFFFFFFF, tu + texelWidth, tv + texelHeight, plane->normal}};
					if( plane->face != textureIndex ) {
						// plane face doesn't match texture index
						// that means that the voxel face plane doesn't match the texelPlane
						// (ie the right face of the voxel taking the texture of the front face)
						// in that case we just repeat the quad over and over since the texture
						// doesn't depend on our current position in the plane
						
						// we end up outputting a single quad for each voxel that is like this
					} else {
						// texture coordinates depends on our current position in the plane
						// we greedily adjust the quad and texture coordinates as we move
						int32 y    = position.y;
						int32 xEnd = position.x + 1;
						for( int32 x = position.x + 1; x < plane->hCellCount; ++x ) {
							if( isGeneratingQuad( grid, plane, map, x, y, z, textureIndex ) ) {
								quad[1].position += plane->hAxis * plane->hSize;
								quad[1].texCoords.x += texelWidth;
								quad[3].position += plane->hAxis * plane->hSize;
								quad[3].texCoords.x += texelWidth;
								map[x + y * plane->hCellCount] = 1;
								++xEnd;
							} else {
								break;
							}
						}
						// we expanded the quad as much as we can in the x direction
						// now we expand in y
						++y;
						for( ; y < plane->vCellCount; ++y ) {
							bool generating = true;
							for( int32 x = position.x; x < xEnd; ++x ) {
								if( !isGeneratingQuad( grid, plane, map, x, y, z, textureIndex ) ) {
									generating = false;
									break;
								}
							}
							if( generating ) {
								for( int32 x = position.x; x < xEnd; ++x ) {
									map[x + y * plane->hCellCount] = 1;
								}
								quad[2].position += plane->vAxis * plane->vSize;
								quad[2].texCoords.y += texelHeight;
								quad[3].position += plane->vAxis * plane->vSize;
								quad[3].texCoords.y += texelHeight;
							} else {
								break;
							}
						}
					}

					if( plane->isBackFacing ) {
						// swap positions of 1 and 2 so that quad is ccw winded
						swap( quad[1], quad[2] );
					}
					pushQuad( stream, quad );
				} else {
					break;
				}
			}
		}
	};

	PlaneDescriptor plane;
	// front face
	plane.hAxis        = {1, 0, 0};
	plane.vAxis        = {0, -1, 0};
	plane.zAxis        = {0, 0, 1};
	plane.hComponent   = VectorComponent_X;
	plane.vComponent   = VectorComponent_Y;
	plane.zComponent   = VectorComponent_Z;
	plane.hSize        = cellSize.x;
	plane.vSize        = cellSize.y;
	plane.zSize        = cellSize.y;
	plane.hCellCount   = grid->width;
	plane.vCellCount   = grid->height;
	plane.zCount       = grid->depth;
	plane.frontOffset  = -1;
	plane.origin       = {0, grid->height * cellSize.y, 0};
	plane.face         = VF_Front;
	plane.normal       = normal_neg_z_axis;
	plane.isBackFacing = false;

	stream->color = 0xFFFF0000;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );
	// back face
	plane.frontOffset  = 1;
	plane.origin       = {0, grid->height * cellSize.y, cellSize.y};
	stream->color      = 0xFF00FF00;
	plane.face         = VF_Back;
	plane.normal       = normal_pos_z_axis;
	plane.isBackFacing = true;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );

	// right face
	plane.hAxis        = {0, 0, 1};
	plane.vAxis        = {0, -1, 0};
	plane.zAxis        = {1, 0, 0};
	plane.hComponent   = VectorComponent_Z;
	plane.vComponent   = VectorComponent_Y;
	plane.zComponent   = VectorComponent_X;
	plane.hSize        = cellSize.y;
	plane.vSize        = cellSize.y;
	plane.zSize        = cellSize.x;
	plane.hCellCount   = grid->depth;
	plane.vCellCount   = grid->height;
	plane.zCount       = grid->width;
	plane.frontOffset  = 1;
	plane.origin       = {cellSize.x, grid->height * cellSize.y, 0};
	plane.face         = VF_Right;
	stream->color      = 0xFF0000FF;
	plane.normal       = normal_pos_x_axis;
	plane.isBackFacing = false;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );
	// left face
	plane.frontOffset  = -1;
	plane.origin       = {0, grid->height * cellSize.y, 0};
	stream->color      = 0xFFFF00FF;
	plane.face         = VF_Left;
	plane.normal       = normal_neg_x_axis;
	plane.isBackFacing = true;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );

	// top face
	plane.hAxis        = {1, 0, 0};
	plane.vAxis        = {0, 0, 1};
	plane.zAxis        = {0, -1, 0};
	plane.hComponent   = VectorComponent_X;
	plane.vComponent   = VectorComponent_Z;
	plane.zComponent   = VectorComponent_Y;
	plane.hSize        = cellSize.x;
	plane.vSize        = cellSize.y;
	plane.zSize        = cellSize.y;
	plane.hCellCount   = grid->width;
	plane.vCellCount   = grid->depth;
	plane.zCount       = grid->height;
	plane.frontOffset  = -1;
	plane.face         = VF_Top;
	plane.origin       = {0, grid->height * cellSize.y, 0};
	stream->color      = 0xFFFFFF00;
	plane.normal       = normal_pos_y_axis;
	plane.isBackFacing = true;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );

	// bottom face
	plane.frontOffset  = 1;
	plane.origin       = {0, grid->height * cellSize.y - cellSize.y, 0};
	stream->color      = 0xFF00FFFF;
	plane.face         = VF_Bottom;
	plane.normal       = normal_neg_y_axis;
	plane.isBackFacing = false;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );
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

struct ShortestLineBetweenLinesResult {
	float tA, tB;
};
ShortestLineBetweenLinesResult shortestLineBetweenLines( vec3arg aStart, vec3arg aDir,
                                                         vec3arg bStart, vec3arg bDir )
{
	ShortestLineBetweenLinesResult result;
	auto diffStart = aStart - bStart;
	auto aa        = dot( aDir, aDir );
	auto ab        = dot( aDir, bDir );
	auto bb        = dot( bDir, bDir );
	auto pa        = dot( aDir, diffStart );
	auto pb        = dot( bDir, diffStart );

	auto denom = 1.0f / ( aa * bb - ab * ab );
	result.tA  = ( ab * pb - bb * pa ) * denom;
	result.tB  = ( aa * pb - ab * pa ) * denom;
	return result;
}
bool testRayVsPlane( vec3arg rayOrigin, vec3arg rayDir, vec3arg planeOrigin, vec3arg planeNormal,
                     float* t = nullptr )
{
	auto denom = dot( rayDir, planeNormal );
	if( denom == 0 ) {
		auto projection = dot( planeOrigin - rayOrigin, planeNormal );
		if( projection > -0.0001f && projection < 0.0001f ) {
			if( t ) {
				*t = 0;
			}
			return true;
		}
		return false;
	}

	auto relative = planeOrigin - rayOrigin;
	auto t_       = dot( relative, planeNormal ) / denom;
	if( t_ >= 0 ) {
		if( t ) {
			*t = t_;
		}
		return true;
	}
	return false;
}
bool testRayVsAabb( vec3arg rayOrigin, vec3arg rayDir, aabbarg box, float* t = nullptr )
{
	auto oneOverDirX = 1.0f / rayDir.x;
	auto oneOverDirY = 1.0f / rayDir.y;
	auto oneOverDirZ = 1.0f / rayDir.z;

	auto tMinX = ( box.min.x - rayOrigin.x ) * oneOverDirX;
	auto tMinY = ( box.min.y - rayOrigin.y ) * oneOverDirY;
	auto tMinZ = ( box.min.z - rayOrigin.z ) * oneOverDirZ;

	auto tMaxX = ( box.max.x - rayOrigin.x ) * oneOverDirX;
	auto tMaxY = ( box.max.y - rayOrigin.y ) * oneOverDirY;
	auto tMaxZ = ( box.max.z - rayOrigin.z ) * oneOverDirZ;

	auto mmX  = minmax( tMinX, tMaxX );
	auto mmY  = minmax( tMinY, tMaxY );
	auto mmZ  = minmax( tMinZ, tMaxZ );
	auto tMin = max( mmX.min, mmY.min, mmZ.min );
	auto tMax = min( mmX.max, mmY.max, mmZ.max );

	if( t ) {
		*t = tMin;
	}
	return tMin <= tMax;
}

struct TestRayVsAabbOption {
	float t;
	vec3i normal;
};
static bool operator<( const TestRayVsAabbOption& a, const TestRayVsAabbOption& b )
{
	return a.t < b.t;
}
struct TestRayVsAabbResult {
	TestRayVsAabbOption enter; // intersection with aabb when entering
	TestRayVsAabbOption leave; // intersection with aabb when leaving
};
bool testRayVsAabb( vec3arg rayOrigin, vec3arg rayDir, aabbarg box, TestRayVsAabbResult* out )
{
	assert( out );

	auto oneOverDirX = 1.0f / rayDir.x;
	auto oneOverDirY = 1.0f / rayDir.y;
	auto oneOverDirZ = 1.0f / rayDir.z;

	TestRayVsAabbOption tMinX = {( box.min.x - rayOrigin.x ) * oneOverDirX, {-1, 0, 0}};
	TestRayVsAabbOption tMinY = {( box.min.y - rayOrigin.y ) * oneOverDirY, {0, -1, 0}};
	TestRayVsAabbOption tMinZ = {( box.min.z - rayOrigin.z ) * oneOverDirZ, {0, 0, -1}};

	TestRayVsAabbOption tMaxX = {( box.max.x - rayOrigin.x ) * oneOverDirX, {1, 0, 0}};
	TestRayVsAabbOption tMaxY = {( box.max.y - rayOrigin.y ) * oneOverDirY, {0, 1, 0}};
	TestRayVsAabbOption tMaxZ = {( box.max.z - rayOrigin.z ) * oneOverDirZ, {0, 0, 1}};

	auto mmX  = minmax( tMinX, tMaxX );
	auto mmY  = minmax( tMinY, tMaxY );
	auto mmZ  = minmax( tMinZ, tMaxZ );
	auto tMin = max( mmX.min, mmY.min, mmZ.min );
	auto tMax = min( mmX.max, mmY.max, mmZ.max );

	out->enter.t      = tMin.t;
	out->enter.normal = tMin.normal;
	out->leave.t      = tMax.t;
	out->leave.normal = tMax.normal;
	return tMin.t <= tMax.t;
}

struct RayCastResult {
	bool found;
	vec3i position;
	vec3i normal;
	vec3 intersection;
};

// raycasting into 3d grid algorithm based on this paper:
// http://www.cse.yorku.ca/~amana/research/grid.pdf
RayCastResult raycastIntoVoxelGrid( VoxelGrid* grid, vec3arg rayOrigin, vec3 rayDir, float tMax )
{
	assert( grid );
	RayCastResult result = {};

	// find first grid intersection point
	aabb gridBoundingBox = {0,
	                        0,
	                        0,
	                        grid->width * EDITOR_CELL_WIDTH,
	                        grid->height * EDITOR_CELL_HEIGHT,
	                        grid->depth * EDITOR_CELL_DEPTH};

	auto originalDir = rayDir;
	TestRayVsAabbResult rayIntersection;
	if( !testRayVsAabb( rayOrigin, rayDir, gridBoundingBox, &rayIntersection ) ) {
		return result;
	}
	auto rayIntersectionT = rayIntersection.enter.t;
	result.normal = rayIntersection.enter.normal;

	vec3 start;
	if( rayIntersectionT >= 0 ) {
		start = rayOrigin + rayDir * rayIntersectionT;
	} else {
		rayIntersectionT = 0;
		start            = rayOrigin;
	}
	start.y = grid->height * EDITOR_CELL_HEIGHT - start.y;
	start.x = clamp( start.x, gridBoundingBox.min.x, gridBoundingBox.max.x - 1 );
	start.y = clamp( start.y, gridBoundingBox.min.y, gridBoundingBox.max.y - 1 );
	start.z = clamp( start.z, gridBoundingBox.min.z, gridBoundingBox.max.z - 1 );

	result.normal.y = -result.normal.y;
	rayDir.y        = -rayDir.y;

	int32 stepX = ( rayDir.x >= 0 ) ? ( 1 ) : ( -1 );
	int32 stepY = ( rayDir.y >= 0 ) ? ( 1 ) : ( -1 );
	int32 stepZ = ( rayDir.z >= 0 ) ? ( 1 ) : ( -1 );

	int32 x = (int32)floor( start.x * CELL_ONE_OVER_WIDTH );
	int32 y = (int32)floor( start.y * CELL_ONE_OVER_HEIGHT );
	int32 z = (int32)floor( start.z * CELL_ONE_OVER_DEPTH );

	float nextVoxelX = ( x + stepX ) * EDITOR_CELL_WIDTH;
	float nextVoxelY = ( y + stepY ) * EDITOR_CELL_HEIGHT;
	float nextVoxelZ = ( z + stepZ ) * EDITOR_CELL_DEPTH;
	if( rayDir.x < 0 ) {
		nextVoxelX += EDITOR_CELL_WIDTH;
	}
	if( rayDir.y < 0 ) {
		nextVoxelY += EDITOR_CELL_HEIGHT;
	}
	if( rayDir.z < 0 ) {
		nextVoxelZ += EDITOR_CELL_DEPTH;
	}

	auto oneOverRayDirX = 1.0f / rayDir.x;
	auto oneOverRayDirY = 1.0f / rayDir.y;
	auto oneOverRayDirZ = 1.0f / rayDir.z;

	float tMaxX = ( rayDir.x != 0 ) ? ( ( nextVoxelX - start.x ) * oneOverRayDirX ) : ( FLOAT_MAX );
	float tMaxY = ( rayDir.y != 0 ) ? ( ( nextVoxelY - start.y ) * oneOverRayDirY ) : ( FLOAT_MAX );
	float tMaxZ = ( rayDir.z != 0 ) ? ( ( nextVoxelZ - start.z ) * oneOverRayDirZ ) : ( FLOAT_MAX );

	float tDeltaX =
	    ( rayDir.x != 0 ) ? ( EDITOR_CELL_WIDTH * oneOverRayDirX * stepX ) : ( FLOAT_MAX );
	float tDeltaY =
	    ( rayDir.y != 0 ) ? ( EDITOR_CELL_HEIGHT * oneOverRayDirY * stepY ) : ( FLOAT_MAX );
	float tDeltaZ =
	    ( rayDir.z != 0 ) ? ( EDITOR_CELL_DEPTH * oneOverRayDirZ * stepZ ) : ( FLOAT_MAX );

	result.intersection = rayOrigin + originalDir * rayIntersectionT;
	if( getCell( grid, x, y, z ) != EmptyCell ) {
		result.position = {x, y, z};
		result.found    = true;
	} else {
		float prevT = 0;
		float t     = 0;
		while( t < tMax ) {
			if( !isPointInsideVoxelBounds( grid, x, y, z ) ) {
				result.found = false;
				break;
			}

			if( getCell( grid, x, y, z ) != EmptyCell ) {
				result.intersection = rayOrigin + originalDir * ( t + rayIntersectionT );
				result.position     = {(int32)x, (int32)y, (int32)z};
				result.found        = true;
				break;
			}

			prevT = t;
			if( tMaxX < tMaxY ) {
				if( tMaxX < tMaxZ ) {
					// tMaxX was smallest
					x += stepX;
					t             = tMaxX;
					result.normal = {-stepX, 0, 0};
					tMaxX += tDeltaX;
				} else {
					// tMaxZ was smallest
					z += stepZ;
					t             = tMaxZ;
					result.normal = {0, 0, -stepZ};
					tMaxZ += tDeltaZ;
				}
			} else {
				if( tMaxY < tMaxZ ) {
					// tMaxY was smallest
					y += stepY;
					t             = tMaxY;
					result.normal = {0, -stepY, 0};
					tMaxY += tDeltaY;
				} else {
					// tMaxZ was smallest
					z += stepZ;
					t             = tMaxZ;
					result.normal = {0, 0, -stepZ};
					tMaxZ += tDeltaZ;
				}
			}
		}
	}

	return result;
}

static void processBuildMode( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto voxel  = &app->voxelState;
	auto camera = &voxel->camera;
	// auto renderer     = &app->renderer;
	auto cameraCenter = center( camera );

	auto generateVoxelMesh = false;
	if( isKeyPressed( inputs, KC_LButton ) ) {
		auto result = raycastIntoVoxelGrid( &voxel->voxels, cameraCenter, camera->look, 10000 );
		if( result.found ) {
			auto destinationCell = result.position + result.normal;
			if( isPointInsideVoxelBounds( &voxel->voxels, destinationCell ) ) {
				getCell( &voxel->voxels, destinationCell ) = voxel->placingCell;
				generateVoxelMesh = true;
			}
		}
	}
	if( isKeyPressed( inputs, KC_RButton ) ) {
		auto result = raycastIntoVoxelGrid( &voxel->voxels, cameraCenter, camera->look, 10000 );
		if( result.found ) {
			auto destinationCell = result.position;
			if( isPointInsideVoxelBounds( &voxel->voxels, destinationCell ) ) {
				getCell( &voxel->voxels, destinationCell ) = EmptyCell;
				generateVoxelMesh = true;
			}
		}
	}

	if( generateVoxelMesh ) {
		clear( &voxel->meshStream );
		generateMeshFromVoxelGrid( &voxel->meshStream, &voxel->voxels, &voxel->textureMap,
		                           EditorVoxelCellSize );
	}
}

aabb calculateSelectionWorld( VoxelState* voxel )
{
	auto grid             = &voxel->voxels;
	auto selectionWorld   = AabbScaled( voxel->selection, EditorVoxelCellSize );
	auto voxelGridTop     = EDITOR_CELL_HEIGHT * grid->height;
	selectionWorld        = translate( selectionWorld, {0, voxelGridTop, 0} );
	selectionWorld.bottom = voxelGridTop * 2 - selectionWorld.bottom;
	selectionWorld.top    = voxelGridTop * 2 - selectionWorld.top;
	swap( selectionWorld.bottom, selectionWorld.top );
	return selectionWorld;
}

aabbi getSelection( VoxelState* state )
{
	aabbi result;
	auto grid    = &state->voxels;
	result.min.x = max( min( state->selection.min.x, state->selection.max.x ), 0 );
	result.max.x = min( max( state->selection.min.x, state->selection.max.x ), grid->width );

	result.min.y = max( min( state->selection.min.y, state->selection.max.y ), 0 );
	result.max.y = min( max( state->selection.min.y, state->selection.max.y ), grid->height );

	result.min.z = max( min( state->selection.min.z, state->selection.max.z ), 0 );
	result.max.z = min( max( state->selection.min.z, state->selection.max.z ), grid->depth );
	return result;
}
static void processSelectMode( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto voxel        = &app->voxelState;
	auto camera       = &voxel->camera;
	auto cameraCenter = center( camera );
	auto grid         = &voxel->voxels;

	if( isKeyPressed( inputs, KC_MButton ) ) {
		auto result = raycastIntoVoxelGrid( &voxel->voxels, cameraCenter, camera->look, 10000 );
		if( result.found ) {
			voxel->selection = AabbWHD( result.position, 1, 1, 1 );
		}
	}

	if( isKeyPressed( inputs, KC_Key_J ) ) {
		swap( voxel->selection.min.x, voxel->selection.max.x );
		swap( voxel->selection.min.y, voxel->selection.max.y );
		swap( voxel->selection.min.z, voxel->selection.max.z );
	}
	if( voxel->dragAction == DragAction::None ) {
		auto& selectionWorld = voxel->selectionWorld = calculateSelectionWorld( voxel );
		TestRayVsAabbResult result;
		voxel->isFaceSelected =
		    testRayVsAabb( cameraCenter, camera->look, selectionWorld, &result );
		if( voxel->isFaceSelected ) {
			TestRayVsAabbOption* option;
			if( isKeyDown( inputs, KC_Alt ) ) {
				option = &result.leave;
			} else {
				option = &result.enter;
			}
			voxel->selectionOrigin = cameraCenter + camera->look * option->t;
			voxel->selectionNormal = option->normal;
		}
	}

	auto generateVoxelMesh = false;
	switch( voxel->dragAction ) {
		case DragAction::None: {
			break;
		}
		case DragAction::DragSelection: {
			if( isKeyUp( inputs, KC_LButton ) ) {
				voxel->dragAction = DragAction::None;
			}
			break;
		}
		case DragAction::MoveVoxels: {
			if( isKeyUp( inputs, KC_RButton ) ) {
				voxel->dragAction = DragAction::None;
				auto selection    = getSelection( voxel );
				for( int32 z = selection.min.z; z < selection.max.z; ++z ) {
					for( int32 y = selection.min.y; y < selection.max.y; ++y ) {
						for( int32 x = selection.min.x; x < selection.max.x; ++x ) {
							auto& cell     = getCell( &voxel->voxels, x, y, z );
							auto& selected = getCell( &voxel->voxelsMoving, x - selection.min.x,
							                          y - selection.min.y, z - selection.min.z );
							if( selected != EmptyCell ) {
								cell = selected;
							}
							selected = EmptyCell;
						}
					}
				}
				generateVoxelMesh = true;
			}
			break;
		}
		InvalidDefaultCase;
	}

	auto startedDragging = ( voxel->dragAction == DragAction::None );
	if( voxel->dragAction == DragAction::None && voxel->isFaceSelected ) {
		if( isKeyPressed( inputs, KC_LButton ) ) {
			voxel->dragAction = DragAction::DragSelection;
		} else if( isKeyPressed( inputs, KC_RButton ) ) {
			voxel->dragAction = DragAction::MoveVoxels;
		}
	}

	if( voxel->dragAction != DragAction::None ) {
		vec3 axisOrigin          = voxel->selectionOrigin;
		vec3 axisDir             = {};
		intmax componentAabbMin  = 0;
		intmax componentAabbMax  = 0;
		intmax componentIndexMin = 0;
		intmax componentIndexMax = 0;
		intmax componentVec      = 0;
		float scale              = 0;
		float offset             = 0;
		auto selectionNormal     = voxel->selectionNormal;
		if( selectionNormal.x != 0 ) {
			axisDir = {1, 0, 0};
			if( selectionNormal.x < 0 ) {
				componentAabbMin  = 0;
				componentIndexMin = 0;
				componentAabbMax  = 3;
				componentIndexMax = 3;
			} else {
				componentAabbMin  = 3;
				componentIndexMin = 3;
				componentAabbMax  = 0;
				componentIndexMax = 0;
			}
			componentVec = 0;
			scale        = CELL_ONE_OVER_WIDTH;
		} else if( selectionNormal.y != 0 ) {
			axisDir = {0, 1, 0};
			if( selectionNormal.y < 0 ) {
				componentAabbMin  = 1;
				componentIndexMin = 4;
				componentAabbMax  = 4;
				componentIndexMax = 1;
			} else {
				componentAabbMin  = 4;
				componentIndexMin = 1;
				componentAabbMax  = 1;
				componentIndexMax = 4;
			}
			componentVec = 1;
			scale        = -CELL_ONE_OVER_HEIGHT;
			offset       = -EDITOR_CELL_HEIGHT * grid->height;
		} else if( selectionNormal.z != 0 ) {
			axisDir = {0, 0, 1};
			if( selectionNormal.z < 0 ) {
				componentAabbMin  = 2;
				componentIndexMin = 2;
				componentAabbMax  = 5;
				componentIndexMax = 5;
			} else {
				componentAabbMin  = 5;
				componentIndexMin = 5;
				componentAabbMax  = 2;
				componentIndexMax = 2;
			}
			componentVec = 2;
			scale        = CELL_ONE_OVER_DEPTH;
		} else {
			InvalidCodePath();
		}

		auto result = shortestLineBetweenLines( cameraCenter, camera->look, axisOrigin, axisDir );
		auto axisResult = axisOrigin + axisDir * result.tB;
		if( startedDragging ) {
			voxel->lastAxisPosition = axisResult;
		}
		switch( voxel->dragAction ) {
			case DragAction::DragSelection: {
				if( !startedDragging ) {
					auto delta = ( axisResult - voxel->lastAxisPosition ).elements[componentVec];
					voxel->selectionWorld.elements[componentAabbMin] += delta;
					auto index = (int32)floor(
					    ( voxel->selectionWorld.elements[componentAabbMin] + offset ) * scale );
					voxel->selection.elements[componentIndexMin] = index;
				}
				break;
			}
			case DragAction::MoveVoxels: {
				if( startedDragging ) {
					auto duplicate = isKeyDown( inputs, KC_Shift );
					auto selection = getSelection( voxel );
					for( int32 z = selection.min.z; z < selection.max.z; ++z ) {
						for( int32 y = selection.min.y; y < selection.max.y; ++y ) {
							for( int32 x = selection.min.x; x < selection.max.x; ++x ) {
								auto& cell = getCell( &voxel->voxels, x, y, z );
								auto& selected =
								    getCell( &voxel->voxelsMoving, x - selection.min.x,
								             y - selection.min.y, z - selection.min.z );
								selected = cell;
								if( !duplicate ) {
									cell = EmptyCell;
								}
							}
						}
					}
					generateVoxelMesh = true;
				} else {
					auto delta = ( axisResult - voxel->lastAxisPosition ).elements[componentVec];
					voxel->selectionWorld.elements[componentAabbMin] += delta;
					voxel->selectionWorld.elements[componentAabbMax] += delta;
					auto indexMin = (int32)floor(
					    ( voxel->selectionWorld.elements[componentAabbMin] + offset ) * scale );
					auto indexMax = (int32)floor(
					    ( voxel->selectionWorld.elements[componentAabbMax] + offset ) * scale );
					if( voxel->selection.min.elements[componentVec] != indexMin
					    || voxel->selection.max.elements[componentVec] != indexMax ) {
						generateVoxelMesh = true;
					}
					voxel->selection.min.elements[componentVec] = indexMin;
					voxel->selection.max.elements[componentVec] = indexMax;
				}
				break;
			}
			InvalidDefaultCase;
		}
		voxel->lastAxisPosition = axisResult;
	}

	if( isKeyPressed( inputs, KC_Delete ) ) {
		generateVoxelMesh = true;
		auto selection    = getSelection( voxel );
		for( int32 z = selection.min.z; z < selection.max.z; ++z ) {
			for( int32 y = selection.min.y; y < selection.max.y; ++y ) {
				for( int32 x = selection.min.x; x < selection.max.x; ++x ) {
					getCell( &voxel->voxels, x, y, z ) = EmptyCell;
				}
			}
		}
	}

	if( generateVoxelMesh ) {
		auto selection = getSelection( voxel );
		voxel->voxelsCombined = voxel->voxels;
		for( int32 z = selection.min.z; z < selection.max.z; ++z ) {
			for( int32 y = selection.min.y; y < selection.max.y; ++y ) {
				for( int32 x = selection.min.x; x < selection.max.x; ++x ) {
					auto src = getCell( &voxel->voxelsMoving, x - selection.min.x,
					                    y - selection.min.y, z - selection.min.z );
					auto& dest = getCell( &voxel->voxelsCombined, x, y, z );
					if( src != EmptyCell ) {
						dest = src;
					}
				}
			}
		}
		clear( &voxel->meshStream );
		generateMeshFromVoxelGrid( &voxel->meshStream, &voxel->voxelsCombined, &voxel->textureMap,
		                           EditorVoxelCellSize );
	}
}
static void processEditMode( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto voxel = &app->voxelState;

	switch( voxel->editMode ) {
		case EditMode::Build: {
			processBuildMode( app, inputs, focus, dt );
			break;
		}
		case EditMode::Select: {
			processSelectMode( app, inputs, focus, dt );
			break;
		}
		InvalidDefaultCase;
	}
}

void saveVoxelGridToFile( PlatformServices* platform, StringView filename, VoxelGrid* grid )
{
	assert( platform );
	assert( grid );
	// TODO: write the path to the used texture map also to the file
	platform->writeBufferToFile( filename, grid, sizeof( VoxelGrid ) );
}
bool loadVoxelGridFromFile( PlatformServices* platform, StringView filename, VoxelGrid* grid )
{
	auto bytesToRead = sizeof( VoxelGrid );
	if( platform->readFileToBuffer( filename, grid, bytesToRead ) != bytesToRead ) {
		LOG( ERROR, "Failed to load voxel grid from file {}", filename );
		return false;
	}
	return true;
}
VoxelGrid getVoxelGridFromTextureMap( VoxelGridTextureMap* map, Color colorkey )
{
	assert( map );
	VoxelGrid result = {};
	auto info        = getTextureInfo( map->texture );
	auto tw          = getAxisAlignedWidth( map->entries[VF_Front].texCoords );
	auto th          = getAxisAlignedHeight( map->entries[VF_Front].texCoords );
	auto td          = getAxisAlignedWidth( map->entries[VF_Right].texCoords );

	// convert texture coordinates to pixel coordinates to calculate grid dimensions
	result.width  = (int32)round( tw * info->image.width );
	result.height = (int32)round( th * info->image.height );
	result.depth  = (int32)round( td * info->image.width );

	auto isEmpty = [info, map, &colorkey, &result]( vec3iarg index, uint32 face ) {
		auto plane     = getTexelPlaneByFace( face );
		auto texCoords = &map->entries[face].texCoords;
		float tx       = index.elements[plane.x] / (float)result.dim[plane.x];
		float ty       = index.elements[plane.y] / (float)result.dim[plane.y];
		auto textureX  = ( intmax )( lerp( tx, texCoords->elements[0].u, texCoords->elements[1].u )
		                            * info->image.width );
		auto textureY = ( intmax )( lerp( ty, texCoords->elements[0].v, texCoords->elements[2].v )
		                            * info->image.height );
		auto pixel = getPixelColor( info->image, textureX, textureY );
		return ( getAlpha( pixel ) == 0 || pixel == colorkey );
	};

	int32 strideY = result.width;
	int32 strideZ = result.width * result.height;
	for( vec3i pos = {}; pos.z < result.depth; ++pos.z ) {
		for( pos.y = 0; pos.y < result.height; ++pos.y ) {
			for( pos.x = 0; pos.x < result.width; ++pos.x ) {
				int32 index = pos.x + pos.y * strideY + pos.z * strideZ;
				auto& cell   = result.data[index];
				cell         = DefaultCell;

				for( uint32 face = 0; face < VF_Count; ++face ) {
					if( isEmpty( pos, face ) ) {
						cell = EmptyCell;
						break;
					}
				}
			}
		}
	}
	return result;
}
VoxelGrid getVoxelGridFromTextureMapTopLeftColorKey( VoxelGridTextureMap* map )
{
	assert( map );
	assert( map->texture );
	auto info = getTextureInfo( map->texture );
	return getVoxelGridFromTextureMap( map, getPixelColor( info->image, 0, 0 ) );
}

static void doVoxelGui( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto voxel     = &app->voxelState;
	auto gui       = &voxel->gui;
	auto renderer  = &app->renderer;
	auto font      = &app->font;
	auto fadeSpeed = 0.25f * dt;
	if( !focus && gui->fadeProgress <= 0 ) {
		return;
	}
	gui->fadeProgress = clamp( gui->fadeProgress + fadeSpeed * ( ( focus ) ? ( 1 ) : ( -1 ) ) );

	if( !gui->initialized ) {
		gui->editMode  = imguiGenerateContainer( &app->guiState );
		gui->rendering = imguiGenerateContainer( &app->guiState, {ImGui->style.containerWidth} );
		gui->textureIndex =
		    imguiGenerateContainer( &app->guiState, {ImGui->style.containerWidth, 200} );
		gui->noLightingChecked = true;
		gui->initialized       = true;
	}
	if( isKeyPressed( inputs, KC_Key_K ) ) {
		imguiGetContainer( gui->editMode )->setHidden( false );
		imguiGetContainer( gui->rendering )->setHidden( false );
		imguiGetContainer( gui->textureIndex )->setHidden( false );
	}

	setProjection( renderer, ProjectionType::Orthogonal );
	/*setTexture( renderer, 0, voxel->textureMap );
	MESH_STREAM_BLOCK( stream, renderer ) {
	    stream->color    = 0xFFFFFFFF;
	    vec3 vertices[4] = {{0, 0, 0}, {100, 0, 0}, {0, 100, 0}, {100, 100, 0}};
	    pushQuad( stream, vertices );
	}*/
	renderer->color = setAlpha( 0xFFFFFFFF, gui->fadeProgress );

	rectf guiBounds = {0, 0, app->width, app->height};
	imguiBind( &app->guiState, renderer, font, inputs, app->stackAllocator.ptr, guiBounds, focus );
	if( imguiDialog( "EditMode", gui->editMode ) ) {
		char buffer[200];
		string_builder builder( buffer, 200 );
		builder.print( "Current: {}", EditModeStrings[valueof( voxel->editMode )] );
		imguiText( asStringView( builder ) );
		imguiSameLine( 3 );
		for( intmax i = 0; i < valueof( EditMode::Count ); ++i ) {
			if( imguiButton( EditModeStrings[i], 16, 16 ) ) {
				voxel->editMode = (EditMode)i;
			}
		}
		
		auto generateVoxelMesh = false;
		if( imguiButton( "New" ) ) {
			fill( voxel->voxels.data, DefaultCell, voxel->voxels.max_size() );
			generateVoxelMesh = true;
		}

		if( imguiBeginDropGroup( "Sizes", &gui->sizesExpanded ) ) {
			if( imguiEditbox( "Width", &voxel->voxels.width ) ) {
				voxel->voxels.width = clamp( voxel->voxels.width, 0, CELL_MAX_X );
				generateVoxelMesh = true;
			}
			if( imguiEditbox( "Height", &voxel->voxels.height ) ) {
				voxel->voxels.height = clamp( voxel->voxels.height, 0, CELL_MAX_X );
				generateVoxelMesh = true;
			}
			if( imguiEditbox( "Depth", &voxel->voxels.depth ) ) {
				voxel->voxels.depth = clamp( voxel->voxels.depth, 0, CELL_MAX_X );
				generateVoxelMesh   = true;
			}
			imguiEndDropGroup();
		}

		if( imguiBeginDropGroup( "Textures & Mapping", &gui->texturesExpanded ) ) {
			imguiSameLine( 2 );
			if( imguiButton( "Load Texture" ) ) {
				char filenameBuffer[260];
				auto filenameSize = app->platform.getOpenFilename(
				    "All\0*.*\0", nullptr, false, filenameBuffer, countof( filenameBuffer ) );
				if( filenameSize ) {
					voxel->textureMap.texture =
					    app->platform.loadTexture( {filenameBuffer, filenameSize} );
				}
			}
			if( imguiButton( "Generate Grid" ) ) {
				voxel->voxels     = getVoxelGridFromTextureMapTopLeftColorKey( &voxel->textureMap );
				generateVoxelMesh = true;
			}
#if 1
			auto combo = imguiCombo( "Mapping Type", &gui->mappingType );
			if( imguiComboEntry( combo, "Hero Mapping" ) ) {
				voxel->textureMap = makeHeroVoxelGridTextureMap( voxel->textureMap.texture );
				generateVoxelMesh = true;
			}
			if( imguiComboEntry( combo, "TileMapping" ) ) {
				voxel->textureMap = makeDefaultVoxelGridTextureMap( voxel->textureMap.texture );
				generateVoxelMesh = true;
			}
#else
			imguiSameLine( 2 );
			if( imguiButton( "Hero Mapping" ) ) {
				voxel->textureMap = makeHeroVoxelGridTextureMap( voxel->textureMap.texture );
				generateVoxelMesh = true;
			}
			if( imguiButton( "Tile Mapping" ) ) {
				voxel->textureMap = makeDefaultVoxelGridTextureMap( voxel->textureMap.texture );
				generateVoxelMesh = true;
			}
#endif
			imguiEndDropGroup();
		}

		if( imguiBeginDropGroup( "Save & Load", &gui->fileExpanded ) ) {
			imguiSameLine( 2 );
			if( imguiButton( "Save" ) ) {
				char filenameBuffer[260];
				auto filenameSize = app->platform.getSaveFilename(
				    "All\0*.*\0", nullptr, filenameBuffer, countof( filenameBuffer ) );
				if( filenameSize ) {
					saveVoxelGridToFile( &app->platform, {filenameBuffer, filenameSize},
					                     &voxel->voxels );
				}
			}

			if( imguiButton( "Load" ) ) {
				char filenameBuffer[260];
				auto filenameSize = app->platform.getOpenFilename(
				    "All\0*.*\0", nullptr, false, filenameBuffer, countof( filenameBuffer ) );
				if( filenameSize ) {
					loadVoxelGridFromFile( &app->platform, {filenameBuffer, filenameSize},
					                       &voxel->voxels );
					generateVoxelMesh = true;
				}
			}
			imguiEndDropGroup();
		}

		if( generateVoxelMesh ) {
			voxel->voxelsCombined = voxel->voxels;
			voxel->voxelsMoving.width = voxel->voxels.width;
			voxel->voxelsMoving.height = voxel->voxels.height;
			voxel->voxelsMoving.depth = voxel->voxels.depth;
			clear( &voxel->meshStream );
			generateMeshFromVoxelGrid( &voxel->meshStream, &voxel->voxels, &voxel->textureMap,
			                           EditorVoxelCellSize );
		}
	}

	if( imguiDialog( "Rendering Settings", gui->rendering ) ) {
		if( imguiRadiobox( "Lighting", &gui->lightingChecked ) ) {
			voxel->lighting = gui->lightingChecked;
		}
		if( imguiRadiobox( "No Lighting", &gui->noLightingChecked ) ) {
			voxel->lighting = !gui->noLightingChecked;
		}
	}

	auto doButtons = [voxel]( StringView faceLabel, VoxelFaceValues face ) {
		imguiSameLine( 7 );
		imguiText( faceLabel, 35, 16 );
		int32 index = -1;
		if( imguiButton( "front", 16, 16 ) ) {
			index = 0;
		}
		if( imguiButton( "left", 16, 16 ) ) {
			index = 1;
		}
		if( imguiButton( "back", 16, 16 ) ) {
			index = 2;
		}
		if( imguiButton( "right", 16, 16 ) ) {
			index = 3;
		}
		if( imguiButton( "top", 16, 16 ) ) {
			index = 4;
		}
		if( imguiButton( "bottom", 16, 16 ) ) {
			index = 5;
		}
		if( index >= 0 ) {
			voxel->placingCell = setVoxelFaceTexture( voxel->placingCell, face, index );
		}
	};
	if( imguiDialog( "Voxel Texture Index", gui->textureIndex ) ) {
		doButtons( "front", VF_Front );
		doButtons( "left", VF_Left );
		doButtons( "back", VF_Back );
		doButtons( "right", VF_Right );
		doButtons( "top", VF_Top );
		doButtons( "bottom", VF_Bottom );
		if( imguiButton( "Reset" ) ) {
			voxel->placingCell = DefaultCell;
		}
	}

	imguiUpdate( dt );

	imguiFinalize();
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

static MeshId loadVoxelMeshFromFile( PlatformServices* platform, StackAllocator* allocator,
                                     VoxelGridTextureMap* textures, StringView filename )
{
	// TODO: implement loading of texture maps too
	MeshId result = {};
	VoxelGrid grid;
	if( loadVoxelGridFromFile( platform, filename, &grid ) ) {
		TEMPORARY_MEMORY_BLOCK( allocator ) {
			auto meshStream = makeMeshStream( allocator, 10000, 40000, nullptr );
			generateMeshFromVoxelGrid( &meshStream, &grid, textures, VoxelCellSize );
			result = platform->uploadMesh( toMesh( &meshStream ) );
		}
	}
	return result;
}

static void doVoxel( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto voxel    = &app->voxelState;
	auto camera   = &voxel->camera;
	auto renderer = &app->renderer;
	// auto allocator = &app->stackAllocator;
	auto settings = &app->settings;

	if( !focus ) {
		return;
	}
	if( !voxel->initialized ) {
		auto textureMapId = app->platform.loadTexture( "Data/Images/texture_map.png" );
		voxel->textureMap = makeDefaultVoxelGridTextureMap( textureMapId );

		voxel->placingCell = DefaultCell;
		// initial voxel grid size
		voxel->voxels.width = 16;
		voxel->voxels.height = 16;
		voxel->voxels.depth = 16;
		voxel->initialized = true;
	}

	// movement
	if( isKeyPressed( inputs, KC_Tab ) ) {
		if( voxel->focus == VoxelFocus::Gui ) {
			voxel->focus = VoxelFocus::Voxel;
		} else {
			voxel->focus = VoxelFocus::Gui;
		}
	}
	
	// mouse lock
	if( isKeyPressed( inputs, KC_Key_L ) ) {
		app->mouseLocked = !app->mouseLocked;
	}
	if( voxel->focus == VoxelFocus::Voxel ) {
		inputs->mouse.locked = app->mouseLocked;
	}

	if( voxel->focus == VoxelFocus::Voxel ) {
		processCamera( inputs, settings, &voxel->camera, dt );
		voxel->position = camera->position;
		processEditMode( app, inputs, focus, dt );
	}

	setProjection( renderer, ProjectionType::Perspective );
	auto cameraTranslation = matrixTranslation( 0, -50, 0 );
	renderer->view         = cameraTranslation * getViewMatrix( camera );

	setTexture( renderer, 0, null );

	if( isKeyPressed( inputs, KC_Key_U ) ) {
		clear( &voxel->meshStream );
		generateMeshFromVoxelGridNaive( &voxel->meshStream, &voxel->voxels );
	}

	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		auto grid     = &voxel->voxels;
		stream->color = 0xFF0000FF;
		aabb box      = {0,
		            0,
		            0,
		            grid->width * EDITOR_CELL_WIDTH,
		            grid->height * EDITOR_CELL_HEIGHT,
		            grid->depth * EDITOR_CELL_DEPTH};
		pushAabbOutline( stream, box );
	}

	setRenderState( renderer, RenderStateType::Lighting, voxel->lighting );
	setTexture( renderer, 0, voxel->textureMap.texture );
	addRenderCommandMesh( renderer, toMesh( &voxel->meshStream ) );

	if( voxel->editMode == EditMode::Select ) {
		setTexture( renderer, 0, null );
		setRenderState( renderer, RenderStateType::DepthTest, false );
		LINE_MESH_STREAM_BLOCK( stream, renderer ) {
			auto selectionWorld = calculateSelectionWorld( voxel );
			stream->color       = Color::Blue;
			pushAabbOutline( stream, selectionWorld );
		}
		setRenderState( renderer, RenderStateType::DepthTest, true );
	}

	doVoxelGui( app, inputs, voxel->focus == VoxelFocus::Gui, dt );
}

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

	builder << "FrameTime: " << app->platformInfo->frameTime
	        << "\nAverage FrameTime:" << app->platformInfo->averageFrameTime
	        << "\nMin FrameTime: " << app->platformInfo->minFrameTime
	        << "\nMax FrameTime: " << app->platformInfo->maxFrameTime
	        << "\nFPS: " << app->platformInfo->fps
	        << "\nAverage Fps:" << app->platformInfo->averageFps
	        << "\nMin Fps: " << app->platformInfo->minFps
	        << "\nMax Fps: " << app->platformInfo->maxFps
	        << "\nLight Position: " << renderer->lightPosition.x << ", "
	        << renderer->lightPosition.y << ", " << renderer->lightPosition.z;
	if( app->platformInfo->recordingInputs ) {
		builder << "\nRecording Inputs: " << app->platformInfo->recordingFrame;
	}
	if( app->platformInfo->replayingInputs ) {
		builder << "\nReplaying Inputs: " << app->platformInfo->recordingFrame;
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

template < class T >
void* toPtr( T v )
{
	return (void*)( (uintmax)unsignedof( v ) );
}
template < class T >
T fromPtr( void* p )
{
	return ( T )( (uintmax)p );
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
	if( isKeyPressed( inputs, KC_Key_Z ) ) {
		debug_FillMeshStream = !debug_FillMeshStream;
	}
	if( isKeyPressed( inputs, KC_Key_I ) ) {
		renderer->wireframe = !renderer->wireframe;
	}

	if( isKeyPressed( inputs, KC_Key_1 ) ) {
		app->focus = AppFocus::Game;
	}
	if( isKeyPressed( inputs, KC_Key_2 ) ) {
		app->focus = AppFocus::Voxel;
	}
	if( isKeyPressed( inputs, KC_Key_3 ) ) {
		app->focus = AppFocus::TexturePack;
	}
	if( isKeyPressed( inputs, KC_Key_R ) ) {
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
		string_builder builder = string_builder( buffer, countof( buffer ) );
		builder << "FrameTime: " << app->platformInfo->frameTime
		        << "\nAverage FrameTime:" << app->platformInfo->averageFrameTime
		        << "\nMin FrameTime: " << app->platformInfo->minFrameTime
		        << "\nMax FrameTime: " << app->platformInfo->maxFrameTime
		        << "\nFPS: " << app->platformInfo->fps
		        << "\nAverage Fps:" << app->platformInfo->averageFps
		        << "\nMin Fps: " << app->platformInfo->minFps
		        << "\nMax Fps: " << app->platformInfo->maxFps
		        << "\nLight Position: " << renderer->lightPosition.x << ", "
		        << renderer->lightPosition.y << ", " << renderer->lightPosition.z;
		if( app->platformInfo->recordingInputs ) {
			builder << "\nRecording Inputs: " << app->platformInfo->recordingFrame;
		}
		if( app->platformInfo->replayingInputs ) {
			builder << "\nReplaying Inputs: " << app->platformInfo->recordingFrame;
		}

		builder << "\njumpHeight: " << app->gameState.jumpHeight
				<< "\nmaxJumpHeight: " << app->gameState.maxJumpHeight
				<< "\nJumpHeightError: " << app->gameState.jumpHeightError;

		builder << "\nWallJumpTimer: " << app->gameState.player->walljumpWindow.value
		        << "\nWallJumpDuration: " << app->gameState.player->walljumpDuration.value;

		renderText( renderer, font, asStringView( builder ), {300, 0, 0, 0} );
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