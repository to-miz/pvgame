struct RoomEditorData {
	Room room;
};

struct RoomEditorState {
	ImmediateModeGui gui;
	bool initialized;

	int32 fileMenu;

	RoomEditorData data;

	enum FlagValues {
		ToolsExpanded   = BITFIELD( 0 ),
		TilesetExpanded = BITFIELD( 1 ),
	};
	uint32 flags;
};