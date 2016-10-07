#pragma once

#ifndef _TEXTUREPACK_H_INCLUDED_
#define _TEXTUREPACK_H_INCLUDED_

struct TexturePackTextureDisplay {
	vec2 scrollPos;
	int32 selectedIndex;
	rectf selectedRegion;
};

struct TexturePackState {
	ImmediateModeGui gui;
	bool initialized;

	TextureId texture;
	float textureWidth, textureHeight;
	TexturePackTextureDisplay textureDisplay;
	float textureScale;
	bool snapToPixels;
	bool valuesExpanded;

	UArray< recti > textureRegions;
};

#endif // _TEXTUREPACK_H_INCLUDED_
