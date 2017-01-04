#pragma once

#ifndef _ANIMATOR_H_INCLUDED_
#define _ANIMATOR_H_INCLUDED_

struct AnimatorNode {
	vec3 translation;
	vec3 rotation;
	float length;
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

struct AnimatorEditor {
	vec3 rotation;
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
	UArray< AnimatorNode > nodes;
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
	AnimatorEditor editor;
};

#endif  // _ANIMATOR_H_INCLUDED_
