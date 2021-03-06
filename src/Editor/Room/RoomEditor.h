namespace RoomEditor
{
using namespace EditorCommon;
typedef ::Entity::Type EntityType;

const int32 MaxWidth  = 64;
const int32 MaxHeight = 64;
const int32 MaxTiles  = MaxWidth * MaxHeight;

struct TilesPool {
	GameTile data[RL_Count * MaxTiles];
};

struct View {
	struct Room {
		struct Layer {
			GameTile data[MaxTiles];
			int32 width;
			int32 height;

			GameTile& at( int32 x, int32 y )
			{
				assert( isInBounds( x, y ) );
				return data[x + y * MaxWidth];
			}
			GameTile& at( vec2i pos ) { return at( pos.x, pos.y ); }
			bool isInBounds( int32 x, int32 y )
			{
				return x >= 0 && x < width && y >= 0 && y < height;
			}
			bool isInBounds( vec2i pos ) { return isInBounds( pos.x, pos.y ); }
		} layers[RL_Count];

		RoomBackgroundType background;
	} room;

	struct Entity {
		EntityType type;
		vec2i position;
		Skeleton* skeleton;
	};
	std::vector< Entity > entities;

	vec2 translation = {};
	float scale = 1;

	enum FlagValues {
		DrawGrid               = BITFIELD( 0 ),
		DrawGridInFront        = BITFIELD( 1 ),
		DrawSelectedLayersOnly = BITFIELD( 2 ),

		UnsavedChanges = BITFIELD( 31 ),
	};
	uint32 flags = DrawGrid;

	TileSet* tileSet           = nullptr;
	GameTile placingTile       = {};
	EntityType placingEntity = {};

	ImGuiListboxItem layers[RL_Count];
	FilenameString filename;
};

enum class MouseMode { Select, Place, PlaceRectangular, Entity };

struct State {
	ImmediateModeGui gui;
	bool initialized;

	int32 fileMenu;
	int32 viewMenu;
	float layersScrollPos;
	float rotationScrollPos;
	float entitiesScrollPos;
	ImGuiScrollableRegion tilesScollableRegion;

	View view;
	TilesPool tilePool;

	MouseMode mouseMode;
	vec2i selectionStart;
	vec2i selectionEnd;
	View::Room intermediateRoom;

	const SkeletonDefinition* definitions[Entity::type_count];

	enum FlagValues {
		ToolsExpanded      = BITFIELD( 0 ),
		TilesetExpanded    = BITFIELD( 1 ),
		PropertiesExpanded = BITFIELD( 2 ),
	};
	uint32 flags = ToolsExpanded | TilesetExpanded;

	EditorCommon::MessageBox messageBox;
};

} // namespace RoomEditor