#pragma once

#ifndef _ANIMATOR_H_INCLUDED_
#define _ANIMATOR_H_INCLUDED_

constexpr const int32 SkeletonMaxParents = 1024;

typedef int32 GroupId;
typedef trange< GroupId > GroupChildren;

struct AnimatorParticleEmitter {
	ParticleEmitter emitter;
	float interval;
};

struct AnimatorAsset {
	ImGuiListboxItem item = {};

	enum Type {
		type_none,
		type_collection,
		type_collision,
		type_hitbox,
		type_hurtbox,
		type_emitter,
	} type = type_none;

	union {
		DynamicVoxelCollection* collection = nullptr;
		rectf collision;
		rectf hitbox;
		rectf hurtbox;
		AnimatorParticleEmitter emitter;
	};
	int16 id = -1;
	char name[10];
	int32 nameLength = 0;

	char itemText[30];
	int32 itemTextLength = 0;

	void setName( StringView str );
	StringView getName() const { return {name, nameLength}; }

	AnimatorAsset() = default;
	~AnimatorAsset();
	AnimatorAsset( const AnimatorAsset& other );
	AnimatorAsset( AnimatorAsset&& other );
	AnimatorAsset& operator=( const AnimatorAsset& other );
	AnimatorAsset& operator=( AnimatorAsset&& other );

	void assign( const AnimatorAsset& other );
	void assign( AnimatorAsset&& other );
	void destroy();

	explicit operator bool() const { return type != type_none; }
};
static_assert( std::is_standard_layout< AnimatorAsset >::value,
               "AnimatorAsset must be standard layout for imguiListboxIntrusive" );

AnimatorAsset animatorLoadVoxelCollectionAsset( StringView filename, int16 id );

struct AnimatorNode {
	vec3 translation = {};
	vec3 rotation    = {};
	vec3 scale       = {1, 1, 1};
	Color flashColor = {};
	float length     = {};

	int16 id             = -1;
	int16 parentId       = -1;
	AnimatorNode* parent = nullptr;

	int16 childrenCount = {};

	struct {
		uint32 selected : 1;
		uint32 marked : 1;
		uint32 visible : 1;
		uint32 interactible : 1;
	} flags = {0, 0, 1, 1};

	int16 assetId                 = -1;
	AnimatorAsset::Type assetType = {};
	AnimatorAsset* asset          = nullptr;
	struct {
		int8 animation;
		int8 frame;
	} voxel      = {-1, 0};
	bool8 active = {1};  // whether asset is active (valid for hitboxes and emitters)
	struct {
		float time;
	} emitter = {};

	mat4 base             = {};
	mat4 world            = {};
	Color worldFlashColor = {};

	short_string< 25 > name = {};

	bool selectable() const { return flags.interactible && flags.visible; }
};

struct AnimatorCurveData {
	bool used = false;
	vec2 curve0;
	vec2 curve1;
	BezierForwardDifferencerData differencer;
};
struct AnimatorKeyframeData {
	enum Type : int8 {
		type_none,
		type_translation,
		type_rotation,
		type_scale,
		type_flashColor,
		type_frame,
		type_active,
		type_event,

		type_count
	} type;
	enum EaseType : int8 { Lerp, Step, Smoothstep, EaseOutBounce, EaseOutElastic, Curve } easeType;
	int16 curveIndex; // index to curve data or -1
	union {
		vec3 translation;
		vec3 rotation;
		vec3 scale;
		int8 frame;
		bool8 active;
		Color flashColor;
		SkeletonEventType event;
	};
};

struct AnimatorKeyframe {
	float t;
	bool8 selected;
	GroupId group;
	AnimatorKeyframeData data;
};

struct AnimatorGroup {
	StringView name;
	GroupId id;
	bool8 expanded;
};
struct AnimatorGroupDisplay {
	GroupId group;
};

enum class AnimatorMouseMode : int8 {
	Select,
	Translate,
	Rotate,
};
enum class AnimatorTranslateOptions : int8 {
	World,
	LocalAlong,
	LocalPerpendicular,
	ParentAlong,
	ParentPerpendicular
};
enum class AnimatorMousePlane : int8 { XY, YZ, XZ };
enum class AnimatorEditorViewType { Perspective, Orthogonal };
enum class AnimatorEditorEditType { Node, Hitbox };

enum class AnimatorEditorHitboxFeature {
	None,
	LeftTop,
	Top,
	RightTop,
	Right,
	RightBottom,
	Bottom,
	LeftBottom,
	Left
};

struct AnimatorEditor {
	EditorView view;
	AnimatorEditorViewType viewType;
	AnimatorEditorEditType editType;

	enum : uint32 {
		EditorSettings   = BITFIELD( 0 ),
		Properties       = BITFIELD( 1 ),
		Options          = BITFIELD( 2 ),
		Plane            = BITFIELD( 3 ),
		Animations       = BITFIELD( 4 ),
		Keying           = BITFIELD( 5 ),
		Nodes            = BITFIELD( 6 ),
		Assets           = BITFIELD( 7 ),
		AssetsAdd        = BITFIELD( 8 ),
		AssetsProperties = BITFIELD( 9 )
	};
	uint32 expandedFlags;

	struct {
		uint32 visibleNodes : 1;
		uint32 interactibleNodes : 1;
	} viewFlags = {1, 1};

	AnimatorMouseMode mouseMode;
	AnimatorTranslateOptions translateOptions;
	AnimatorMousePlane mousePlane;
	bool8 moving;
	bool8 clickedVoxel;
	AnimatorNode* clickedNode;
	vec2 mouseOffset;
	vec3 clickedPlanePos;
	vec3 clickedRotation;

	int32 contextMenu;

	vec2 rightClickedPos;
	float nodesListboxScroll;
	float assetsScrollPos;

	struct {
		AnimatorEditorHitboxFeature selectedFeature;
	} hitboxView;
};

struct AnimatorAnimation {
	std::vector< AnimatorNode > nodes;
	std::vector< AnimatorKeyframe > keyframes;
	std::vector< AnimatorCurveData > curves;
	short_string< 25 > name;
};

typedef std::vector< std::unique_ptr< AnimatorKeyframe > > AnimatorKeyframes;
typedef std::unique_ptr< AnimatorNode > UniqueAnimatorNode;
typedef std::unique_ptr< AnimatorAsset > UniqueAnimatorAsset;
typedef std::vector< std::unique_ptr< AnimatorNode > > AnimatorNodes;
typedef std::vector< AnimatorAnimation > AnimatorAnimations;

struct AppData;
typedef void AnimatorMessageBoxAction( AppData* );

struct AnimatorState {
	ImmediateModeGui gui;
	bool initialized;

	int32 fileMenu;
	int32 viewMenu;
	int32 editMenu;
	TextureId controlIcons;
	int32 assetContextMenu;

	ImGuiScrollableRegion scrollableRegion;
	AnimatorKeyframes keyframes;
	std::vector< AnimatorKeyframe* > selected;
	std::vector< AnimatorGroup > groups;
	std::vector< AnimatorGroupDisplay > visibleGroups;
	std::vector< AnimatorCurveData > curves;
	std::vector< UniqueAnimatorAsset > assets;
	AnimatorNodes baseNodes;
	AnimatorNodes nodes;
	AnimatorAnimations animations;
	ParticleSystem particleSystem;

	AnimatorAnimation* currentAnimation;

	float duration;
	float currentFrame;

	struct {
		uint32 timelineRootExpanded : 1;
		uint32 showRelativeProperties : 1;
		uint32 mirror : 1;
		uint32 repeat : 1;
		uint32 playing : 1;
		uint32 mouseSelecting : 1;
		uint32 selectionRectValid : 1;
		uint32 moveSelection : 1;
		uint32 moving : 1;
		uint32 keyframesMoved : 1;
		uint32 unsavedChanges : 1;
		uint32 uncommittedChanges : 1;
	} flags;

	vec2 selectionA;
	vec2 selectionB;
	vec2 mouseStart;
	float selectedMinT;
	GroupId clickedGroup;

	float startTime;  // start frame time when dragging

	float scale;

	int16 nodeIds;
	int16 assetIds;
	AnimatorEditor editor;

	StringPool stringPool;
	StringView fieldNames[7];

	FilenameString filename;

	struct MessageBox {
		int32 container;
		enum { OkCancel, YesNoCancel } type;
		short_string< 100 > text;
		short_string< 50 > title;

		struct {
			AnimatorMessageBoxAction* onYes;
			AnimatorMessageBoxAction* onNo;

			struct {
				AnimatorAnimations::iterator toDelete;
			} data;
		} action;
	} messageBox;
};

#endif  // _ANIMATOR_H_INCLUDED_
