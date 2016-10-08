#pragma once

#ifndef _TEXTUREPACK_H_INCLUDED_
#define _TEXTUREPACK_H_INCLUDED_

struct TexturePackTextureDisplay {
	vec2 scrollPos;
	int32 selectedIndex;
	rectf selectedRegion;
};

struct TexturePackFrame {
	bool expanded;
	VoxelGridTextureMap textureMaps;
	float scrollPos;
	ImGuiListboxItem textureMapItems[VF_Count];
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
	Array< recti > regions;
	char textStorage[20];
	int32 textLength;
};
template < class T >
struct TexturePackListbox {
	float scrollPos;
	int32 lastSelected;
	UArray< T > items;
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

struct TexturePackState {
	ImmediateModeGui gui;
	bool initialized;

	ImGuiContainerId removeConfirm;
	TexturePackRemoveVariant removeVariant;

	TexturePackTextureDisplay textureDisplay;
	float textureScale;
	bool snapToPixels;
	bool valuesExpanded;
	bool bulkMode;

	UArray< recti > textureRegions;

	TexturePackListbox< TexturePackSource > textureSources;
	UArray< TexturePackEntry > textureMaps;
};

#endif // _TEXTUREPACK_H_INCLUDED_
