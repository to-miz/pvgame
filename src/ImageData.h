#pragma once

#ifndef _IMAGEDATA_H_INCLUDED_
#define _IMAGEDATA_H_INCLUDED_

struct ImageData {
	uint8* data;
	int32 width;
	int32 height;

	inline explicit operator bool() const { return data != nullptr; }
};

enum ImagePixelComponentValues : int32 {
	ImagePixelComponent_R,
	ImagePixelComponent_G,
	ImagePixelComponent_B,
	ImagePixelComponent_A,
};

union ImagePixel {
	struct {
		uint8 r;
		uint8 g;
		uint8 b;
		uint8 a;
	};
	uint8 elements[4];
};

Color getPixelColor( ImageData image, intmax x, intmax y )
{
	assert( x >= 0 && x < image.width );
	assert( y >= 0 && y < image.width );
	auto ptr = image.data + ( ( x + y * image.width ) * 4 );
	return Color::argb( (uint32)ptr[3], (uint32)ptr[0], (uint32)ptr[1], (uint32)ptr[2] );
}
ImagePixel getPixel( ImageData image, intmax x, intmax y )
{
	assert( x >= 0 && x < image.width );
	assert( y >= 0 && y < image.width );
	auto ptr = image.data + ( ( x + y * image.width ) * 4 );
	return {ptr[0], ptr[1], ptr[2], ptr[3]};
}

#endif // _IMAGEDATA_H_INCLUDED_
