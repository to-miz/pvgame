#define TILE_CELLS_X 16
#define TILE_CELLS_Y 16
#define TILE_CELLS_Z 16
#define TILE_WIDTH ( CELL_WIDTH * TILE_CELLS_X )
#define TILE_HEIGHT ( CELL_HEIGHT * TILE_CELLS_Y )
#define TILE_DEPTH ( CELL_DEPTH * TILE_CELLS_Z )

struct TileInfo {
	float frictionCoefficient;
};
struct TileSet {
	VoxelCollection voxels;
	Array< TileInfo > infos;
};

struct GameTile {
	uint8 tileSet;
	uint8 rotation;
	trange< uint8 > frames;
	inline explicit operator bool() const { return length( frames ) != 0; }
};

const GameTile InvalidGameTile = {255, 255, 0, 255};

enum RoomLayerValues {
	RL_Main,
	RL_Back,
	RL_Front,

	RL_Count
};
typedef Grid< GameTile > TileGrid;

enum class RoomBackgroundType {
	BlueSky,
};
static const StringView RoomBackgroundTypeNames[] = {"BlueSky"};
StringView to_string( RoomBackgroundType type )
{
	assert( valueof( type ) >= 0 && valueof( type ) < countof( RoomBackgroundTypeNames ) );
	return RoomBackgroundTypeNames[valueof( type )];
}

struct Room {
	struct Layer {
		TileGrid grid;
	};
	Layer layers[RL_Count];
	TileSet* tileSet;
	RoomBackgroundType background;
};

void renderBackground( RenderCommands* renderer, RoomBackgroundType background )
{
	using namespace GameConstants;

	assert( renderer );
	switch( background ) {
		case RoomBackgroundType::BlueSky: {
			setTexture( renderer, 0, null );
			auto prev = exchange( renderer->color, 0xFF46AEEB );
			addRenderCommandSingleQuad( renderer, rectf{-500, 500, 500, -500} * TILE_WIDTH,
			                            32 * CELL_DEPTH );
			renderer->color = prev;
			break;
		}
	}
}

TileGrid getCollisionLayer( Room* room ) { return room->layers[RL_Main].grid; }

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

#define GAME_MAP_WIDTH 16
#define GAME_MAP_HEIGHT 16
static int8 GameDebugMapMain[] = {
    2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,
    2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    3, 0, 1, 3, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0,
    0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    4, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 2, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0,
    4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 2, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 1,
    4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    4, 0, 2, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    4, 0, 2, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 1, 1,
    4, 0, 0, 0, 0, 0, 0, 0, 7, 0, 1, 0, 0, 0, 0, 0,
    4, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
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