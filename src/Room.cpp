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