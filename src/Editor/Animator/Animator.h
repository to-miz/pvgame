#pragma once

#ifndef _ANIMATOR_H_INCLUDED_
#define _ANIMATOR_H_INCLUDED_

typedef int32 GroupId;
typedef trange< GroupId > GroupChildren;

struct AnimatorNode {
	vec3 translation = {};
	vec3 rotation    = {};
	vec3 scale       = {1, 1, 1};
	float length     = {};

	int16 id             = -1;
	int16 parentId       = -1;
	AnimatorNode* parent = nullptr;

	int32 childrenCount = {};

	bool8 selected = {};
	bool8 marked   = {};
	int16 voxel    = -1;
	int16 frame    = {};

	mat4 base  = {};
	mat4 world = {};

	short_string< 10 > name = {};
};

struct AnimatorCurveData {
	bool used = false;
	vec2 curve0;
	vec2 curve1;
	BezierForwardDifferencerData differencer;
};
struct AnimatorKeyframeData {
	enum : int8 { type_none, type_translation, type_rotation, type_scale, type_frame } type;
	enum EaseType : int8 { Lerp, Step, Smoothstep, EaseOutBounce, EaseOutElastic, Curve } easeType;
	int16 curveIndex; // index to curve data or -1
	union {
		vec3 translation;
		vec3 rotation;
		vec3 scale;
		int16 frame;
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
	GroupChildren children;
	bool8 expanded;
};
struct AnimatorGroupDisplay {
	GroupId group;
	GroupChildren children;
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
enum class AnimatorMousePlane : int8 {
	XY,
	YZ,
	XZ
};

struct AnimatorEditor {
	vec3 rotation;
	vec3 translation;
	float scale;

	enum : uint32 {
		EditorSettings = BITFIELD( 0 ),
		Properties     = BITFIELD( 1 ),
		Options        = BITFIELD( 2 ),
		Plane          = BITFIELD( 3 ),
		Animations     = BITFIELD( 4 ),
		Keying         = BITFIELD( 5 ),
		Nodes          = BITFIELD( 6 ),
	};
	uint32 expandedFlags;

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
};

struct AnimatorVoxelCollection {
	VoxelCollection voxels;
	Array< StringView > names;
};

struct AnimatorAnimation {
	std::vector< AnimatorNode > nodes;
	std::vector< AnimatorKeyframe > keyframes;
	short_string< 10 > name;
};

typedef std::vector< std::unique_ptr< AnimatorKeyframe > > AnimatorKeyframes;
typedef std::unique_ptr< AnimatorNode > UniqueAnimatorNode;
typedef std::vector< std::unique_ptr< AnimatorNode > > AnimatorNodes;
typedef std::vector< AnimatorAnimation > AnimatorAnimations;

struct AppData;
typedef void AnimatorMessageBoxAction( AppData* );

struct AnimatorState {
	ImmediateModeGui gui;
	bool initialized;

	int32 fileMenu;
	TextureId controlIcons;

	ImGuiScrollableRegion scrollableRegion;
	AnimatorKeyframes keyframes;
	std::vector< AnimatorKeyframe* > selected;
	std::vector< AnimatorGroup > groups;
	std::vector< AnimatorGroupDisplay > visibleGroups;
	std::vector< AnimatorCurveData > curves;
	AnimatorNodes baseNodes;
	AnimatorNodes nodes;
	AnimatorAnimations animations;

	AnimatorVoxelCollection voxels;
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
	AnimatorEditor editor;

	StringPool stringPool;
	StringView fieldNames[4];

	short_string< MAX_PATH > filename;

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
