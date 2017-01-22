#pragma once

#ifndef _ANIMATOR_H_INCLUDED_
#define _ANIMATOR_H_INCLUDED_

typedef int16 GroupId;
typedef trange< GroupId > GroupChildren;

struct AnimatorNode {
	vec3 translation;
	vec3 rotation;
	vec3 scale;
	float length;

	int16 parentId;
	int16 id;
	AnimatorNode* parent;

	int32 childrenCount;

	bool8 selected;
	bool8 marked;
	int16 voxel;
	int16 frame;
	GroupId group;

	mat4 base;
	mat4 world;

	short_string< 10 > name;
};

struct AnimatorKeyframeData {
	enum { type_none, type_translation, type_rotation, type_scale, type_frame } type;
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
	bool8 expanded;
	GroupChildren children;
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
};

struct AnimatorVoxelCollection {
	VoxelCollection voxels;
	Array< StringView > names;
};

struct AnimatorAnimation {
	Array< AnimatorNode > nodes;
	Array< AnimatorKeyframe > keyframes;
	short_string< 10 > name;
	int32 id;
};

struct AnimatorState {
	ImmediateModeGui gui;
	bool initialized;

	ImGuiScrollableRegion scrollableRegion;
	UArray< AnimatorKeyframe* > keyframes;
	UArray< AnimatorKeyframe* > selected;
	UArray< AnimatorGroup > groups;
	UArray< AnimatorGroupDisplay > visibleGroups;
	UArray< AnimatorNode* > baseNodes;
	UArray< AnimatorNode* > nodes;
	UArray< AnimatorAnimation > animations;
	FixedSizeAllocator nodeAllocator;
	FixedSizeAllocator keyframesAllocator;

	AnimatorVoxelCollection voxels;
	AnimatorAnimation* currentAnimation;

	float duration;
	float currentFrame;
	bool8 timelineRootExpanded;
	bool8 showRelativeProperties;

	bool8 mouseSelecting;
	bool8 selectionRectValid;
	bool8 moveSelection;
	bool8 moving;
	bool8 keyframesMoved;
	vec2 selectionA;
	vec2 selectionB;
	vec2 mouseStart;
	float selectedMinT;
	GroupId clickedGroup;

	float startTime;  // start frame time when dragging

	float scale;

	GroupId ids;
	int16 nodeIds;
	AnimatorEditor editor;

	StringPool stringPool;
	StringView fieldNames[4];
};

#endif  // _ANIMATOR_H_INCLUDED_
