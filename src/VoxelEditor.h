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
	int32 meshInfo;
	ImGuiContainerId textureMapDialog;

	float fadeProgress;
	bool initialized;

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

enum class VoxelCameraType { Fixed, Free };

struct VoxelState {
	vec3 position;
	VoxelCameraType cameraType;
	Camera camera;
	EditorView view;
	MeshStream meshStream;
	VoxelGrid voxels;
	VoxelGrid voxelsIntermediate;
	VoxelGrid voxelsCombined;
	VoxelCell placingCell;
	bool lighting;
	bool initialized;

	VoxelFocus focus;
	EditMode editMode;
	aabbi selection;
	VoxelGuiState gui;

	uint8 capture;

	bool isFaceSelected;
	vec3i selectionNormal;
	vec3 selectionOrigin;
	aabb selectionWorld;
	DragAction dragAction;
	vec3 lastAxisPosition;

	VoxelGridTextureMap textureMap;
	DynamicVoxelCollection collection;
	int32 currentFrame;

	bool renderWithOffset;
	bool renderEdges;
};