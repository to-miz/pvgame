#pragma once

#ifndef _ANIMATOR_H_INCLUDED_
#define _ANIMATOR_H_INCLUDED_

struct AnimatorNode {
	vec3 translation;
	vec3 rotation;
	float length;

	int16 parentId;
	int16 id;
	AnimatorNode* parent;

	bool8 selected;

	mat4 base;
	mat4 world;

};

typedef int16 GroupId;
typedef trange< GroupId > GroupChildren;

struct AnimatorKeyframe {
	float t;
	bool8 selected;
	GroupId group;
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
		Voxels         = BITFIELD( 4 ),
	};
	uint32 expandedFlags;

	AnimatorMouseMode mouseMode;
	AnimatorTranslateOptions translateOptions;
	AnimatorMousePlane mousePlane;
	bool moving;
	AnimatorNode* clickedNode;
	vec2 mouseOffset;
};

struct AnimatorState {
	ImmediateModeGui gui;
	bool initialized;

	ImGuiScrollableRegion scrollableRegion;
	UArray< AnimatorKeyframe > keyframesPool;
	UArray< AnimatorKeyframe* > keyframes;
	UArray< AnimatorKeyframe* > selected;
	UArray< AnimatorGroup > groups;
	UArray< AnimatorGroupDisplay > visibleGroups;
	UArray< AnimatorNode* > nodes;
	FixedSizeAllocator nodeAllocator;

	VoxelCollection voxels;

	float duration;

	bool8 mouseSelecting;
	bool8 selectionRectValid;
	bool8 moveSelection;
	bool8 moving;
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
};

#endif  // _ANIMATOR_H_INCLUDED_
