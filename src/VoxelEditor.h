enum class EditMode {
	Build,
	Select,

	Count
};
static const char* const EditModeStrings[] = {
    "Build", "Select",
};

enum class DragAction {
	None,
	DragSelection,
	MoveSelection,
	MoveVoxels,
};

enum class VoxelFocus { Voxel, Gui };
struct VoxelGuiState {
	int32 editMode;
	int32 rendering;
	int32 textureIndex;
	ImGuiContainerId textureMapDialog;

	float fadeProgress;
	bool initialized;

	bool lightingChecked;
	bool noLightingChecked;

	bool fileExpanded;
	bool sizesExpanded;
	bool texturesExpanded;

	bool collectionSaveLoadExpanded;
	bool collectionExpanded;
	bool collectionOffsetExpanded;
	struct Animation {
		bool expanded;
	};
	UArray< Animation > animations;

	int32 mappingType;
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
	int32 currentFrame;
	Array< VoxelGrid > frameVoxels;

	StackAllocator collectionArena;
	VoxelCollection collection;

	bool renderWithOffset;
};