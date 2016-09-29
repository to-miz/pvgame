#define STBI_ASSERT( x ) assert( x )
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#pragma warning( push )
#pragma warning( disable : 4244 4505 )
#include <stb_image.h>
#pragma warning( pop )

ImageData loadImageToMemory( StringView filename )
{
	ImageData result = {};
	auto file        = win32ReadWholeFileInternal( filename );
	if( file ) {
		result.data = stbi_load_from_memory( (const stbi_uc*)file.data, (int32)file.size,
		                                     &result.width, &result.height, nullptr, 4 );
	} else {
		LOG( ERROR, "Failed to loadImageToMemory: {}", filename );
	}
	return result;
}
void freeImageData( ImageData* image )
{
	assert( image );
	stbi_image_free( image->data );
}