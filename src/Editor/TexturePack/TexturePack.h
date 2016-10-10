#pragma once

#ifndef _TEXTUREPACK_H_INCLUDED_
#define _TEXTUREPACK_H_INCLUDED_

struct TexturePackTextureDisplay {
	vec2 scrollPos;
	int32 selectedIndex;
	rectf selectedRegion;
};

enum class TexturePackOrientation {
	Def,
	Cw90,
	Ccw90,
	Horizontal,
	Vertical,
	Diagonal,
};
struct TexturePackFace {
	int32 regionId;
	TexturePackOrientation orientation;
};
struct TexturePackFrame {
	bool expanded;
	TexturePackFace faces[VF_Count];
	float scrollPos;
	ImGuiListboxItem textureMapItems[VF_Count];
	int32 source;
};
struct TexturePackEntry {
	char textStorage[20];
	int32 textLength;
	bool expanded;
	TexturePackFrame frames[10];
	int32 framesCount;
	float bulkScrollPos;
	ImGuiListboxItem bulkModeItems[VF_Count];
};
struct TexturePackSource {
	ImGuiListboxItem lb;
	TextureId id;
	float width;
	float height;
	rangei regions;
	char textStorage[20];
	int32 textLength;
};
struct TextureSourcesListbox {
	float scrollPos;
	int32 lastSelected;
	UArray< TexturePackSource > items;
};

struct TexturePackRemoveVariant {
	enum { type_removeFrame, type_removeEntry, type_removeSource } type;
	union {
		struct {
			int32 entryIndex;
			int32 frameIndex;
		} removeFrame;
		struct {
			int32 entryIndex;
		} removeEntry;
		struct {
			int32 sourceIndex;
		} removeSource;
	};
};

struct TexturePackRegion {
	int32 id;
	recti rect;
	TextureId texture;
	int32 referenceCount;
	recti destRect;
};

struct TexturePackState {
	ImmediateModeGui gui;
	bool initialized;

	ImGuiContainerId removeConfirm;
	TexturePackRemoveVariant removeVariant;

	ImGuiScrollableRegion textureMapsGuiRegion;

	TexturePackTextureDisplay textureDisplay;
	float textureScale;
	bool valuesExpanded;
	bool bulkMode;
	bool orientationExpanded;
	TexturePackOrientation orientation;

	UArray< recti > textureRegions;

	TextureSourcesListbox textureSources;
	UArray< TexturePackEntry > textureMaps;

	int32 regionIds;
	UArray< TexturePackRegion > uniqueTextureRegions;
};

#endif // _TEXTUREPACK_H_INCLUDED_
