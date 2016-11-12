#pragma once

#ifndef _FONT_H_INCLUDED_
#define _FONT_H_INCLUDED_

struct FontGlyph {
	rectf texCoords;
	float advance;
	float overhang;
	float ascend;
};

struct FontRange {
	uint32 min;
	uint32 max;  // non-inclusive
	bool visible;
	FontGlyph* glyphs;
	TextureId texture;
	float textureWidth;
	float textureHeight;
};

struct FontKerning {
	uint64* keys;
	float* amounts;
	int32 count;
};

struct FontInfo {
	float baseline;
	float newLineAdvance;
	float averageCharWidth;

	FontGlyph defaultGlyph;
	FontRange defaultRange;
	Array< FontRange > ranges;

	// TODO: use a hashmap instead
	// TODO: or sort keys and use binary search
	FontKerning kerning;
};

namespace FontStyles
{
enum Values : uint8 {
	Default    = 0,
	Bold       = ( 1 << 0 ),
	Italic     = ( 1 << 1 ),
	Underline  = ( 1 << 2 ),
	Strikeout  = ( 1 << 3 ),
	BoldItalic = Bold | Italic,
};
}
enum class FontAlign : int8 { Left, Center, Right };
enum class FontVerticalAlign : int8 { Top, Center, Bottom };
enum class WrappingMode : int8 { None, Character, Word, Clip };
struct FontRenderOptions {
	float scale;
	float sep;
	FontAlign align;
	uint8 kerning;
	WrappingMode wrappingMode;
	uint8 fontStyles;
	FontVerticalAlign verticalAlign;
};

constexpr inline FontRenderOptions defaultFontRenderOptions()
{
	return {1.0f, 0.0f, FontAlign::Left, true, WrappingMode::Character, 0, FontVerticalAlign::Top};
}

struct Font {
	union {
		struct {
			float scale;
			float sep;
			FontAlign align;
			uint8 kerning;
			WrappingMode wrappingMode;
			uint8 fontStyles;
			FontVerticalAlign verticalAlign;
		};
		FontRenderOptions renderOptions;
	};
	float verticalPadding;

	typedef FontInfo* FontInfoPtr;
	FontInfoPtr normal, bold, italic, boldItalic;
};

struct TextChain {
	vec2 pos;
	uint32 lastCodepoint;
};

struct FontUnicodeRequestRange {
	uint32 min;
	uint32 max;
	bool visible;
};
typedef Array< FontUnicodeRequestRange > FontUnicodeRequestRanges;

static FontUnicodeRequestRange DefaultFontUnicodeRanges[] = {
    {0x0000, 0x0020, false},  // control
    {0x0020, 0x0180, true},   // basic latin + latin-1 supplement + latin extended-a
    {0x2600, 0x2700, true},   // misc symbols
};

FontUnicodeRequestRanges getDefaultFontUnicodeRanges()
{
	return makeArrayView( DefaultFontUnicodeRanges );
}

void reset( Font* font ) { font->renderOptions = defaultFontRenderOptions(); }

FontRange* findFontRange( Array< FontRange > ranges, uint32 codepoint )
{
	PROFILE_FUNCTION();

	FOR( range : ranges ) {
		if( codepoint >= range.min && codepoint < range.max ) {
			return &range;
		}
	}
	return nullptr;
}
int32 findFontRangeIndex( Array< FontRange > ranges, uint32 codepoint )
{
	PROFILE_FUNCTION();

	for( intmax i = 0, count = ranges.size(); i < count; ++i ) {
		auto range = &ranges[i];
		if( codepoint >= range->min && codepoint < range->max ) {
			return (int32)i;
		}
	}
	return -1;
}
FontGlyph* getGlyph( FontRange* range, uint32 codepoint )
{
	assert( range );
	assert( range->glyphs );
	assert( codepoint >= range->min && codepoint < range->max );
	return &range->glyphs[codepoint - range->min];
}
FontGlyph* getGlyph( Array< FontRange > ranges, uint32 codepoint, FontGlyph* def )
{
	if( auto range = findFontRange( ranges, codepoint ) ) {
		return getGlyph( range, codepoint );
	}
	return def;
}
FontGlyph* getGlyph( FontInfo* info, uint32 codepoint )
{
	if( auto range = findFontRange( info->ranges, codepoint ) ) {
		return getGlyph( range, codepoint );
	}
	return &info->defaultGlyph;
}
uint64 toKerningKey( uint32 firstCodepoint, uint32 secondCodepoint )
{
	return ( ( (uint64)firstCodepoint ) << 32 ) | (uint64)secondCodepoint;
}
uint64 nextKerningKey( uint64 key, uint32 nextCodepoint )
{
	return ( key << 32 ) | (uint64)nextCodepoint;
}
uint32 getSecondCodepointFromKerningKey( uint64 key )
{
	return (uint32)( key & 0x00000000FFFFFFFFull );
}
float getKerningAmount( FontKerning* kerning, uint64 key )
{
	PROFILE_FUNCTION();

	assert( kerning );
	auto keys = kerning->keys;
	for( intmax i = 0, count = kerning->count; i < count; ++i ) {
		if( keys[i] == key ) {
			return kerning->amounts[i];
		}
	}
	return 0;
}

FontInfo* getFontInfo( Font* font, uint8 fontStyles )
{
	assert( font );
	bool bold = ( fontStyles & FontStyles::Bold ) != 0;
	bool italic = ( fontStyles & FontStyles::Italic ) != 0;

	auto ret = font->normal;
	if( bold ) {
		if( italic && font->boldItalic ) {
			ret = font->boldItalic;
		} else if( font->bold ) {
			ret = font->bold;
		}
	} else if( italic && font->italic ) {
		ret = font->italic;
	}

	assert( ret );
	return ret;
}
FontInfo* getFontInfo( Font* font )
{
	return getFontInfo( font, font->fontStyles );
}

float stringWidth( FontInfo* font, FontRenderOptions* options, StringView string )
{
	PROFILE_FUNCTION();

	auto result = 0.0f;
	auto sep = options->sep * options->scale;
	auto scale = options->scale;
	uint32 codepoint;
	auto ptr = string.data();
	auto remaining = string.size();
	if( options->kerning ) {
		uint64 key = 0;
		while( utf8::next( ptr, remaining, &codepoint ) ) {
			key = nextKerningKey( key, codepoint );
			result += getKerningAmount( &font->kerning, key ) * scale;
			result += getGlyph( font, codepoint )->advance * scale + sep;
		}
	} else {
		while( utf8::next( ptr, remaining, &codepoint ) ) {
			result += getGlyph( font, codepoint )->advance * scale + sep;
		}
	}
	return result;
}
float stringWidth( Font* font, StringView string )
{
	return stringWidth( getFontInfo( font ), &font->renderOptions, string );
}

float stringHeight( Font* font )
{
	auto info = getFontInfo( font );
	assert( info );
	return ( info->newLineAdvance + font->verticalPadding ) * font->scale;
}

float lineWidth( FontInfo* font, FontRenderOptions* options, bool kerning, StringView string,
                 int32 start, int32* end )
{
	PROFILE_FUNCTION();

	auto result = 0.0f;
	auto sep = options->sep * options->scale;
	auto scale = options->scale;
	string = StringView( string, start );
	uint32 codepoint;
	auto ptr = string.data();
	auto remaining = string.size();
	if( kerning ) {
		uint64 key = 0;
		while( utf8::next( ptr, remaining, &codepoint ) ) {
			++start;
			key = nextKerningKey( key, codepoint );
			if( codepoint == '\n' ) break;
			result += getKerningAmount( &font->kerning, key ) * scale;
			result += getGlyph( font, codepoint )->advance * scale + sep;
		}
	} else {
		while( utf8::next( ptr, remaining, &codepoint ) ) {
			++start;
			if( codepoint == '\n' ) break;
			result += getGlyph( font, codepoint )->advance * scale + sep;
		}
	}

	if( end ) {
		*end = start;
	}
	return result;
}
float stringHeight( FontInfo* font, FontRenderOptions* options, StringView string,
                    float textAreaWidth, float verticalPadding )
{
	PROFILE_FUNCTION();

	float result;
	auto scale = options->scale;
	auto advance = ( font->newLineAdvance + verticalPadding ) * scale;
	if( textAreaWidth > 0.0f && options->wrappingMode == WrappingMode::Character ) {
		auto newlines = 0;
		auto start = 0;
		// advance line by line bound by textAreaWidth
		do {
			auto width = lineWidth( font, options, false, string, start, &start );
			int32 wordWraps = safe_truncate< int32 >( width / textAreaWidth );
			newlines += wordWraps + 1;
			++start; // skip newline
		} while( start < string.size() );
		result = advance * newlines;
	} else {
		// just count all newlines
		auto newlines = count( string.begin(), string.end(), '\n' );
		result = advance * ( newlines + 1 );
	}
	return result;
}
float stringHeight( Font* font, StringView string, float textAreaWidth = 0 )
{
	PROFILE_FUNCTION();

	auto info = getFontInfo( font );
	assert( info );
	return stringHeight( info, &font->renderOptions, string, textAreaWidth, font->verticalPadding );
}

struct CharCountResult {
	int32 count;
	int32 index;
};
CharCountResult charCount( FontInfo* font, FontRenderOptions* options, StringView string,
                           float textAreaWidth )
{
	PROFILE_FUNCTION();

	CharCountResult result = {};
	float maxWidth = 0.0f;
	auto scale = options->scale;
	auto sep = options->sep * scale;
	uint32 codepoint;
	auto ptr = string.data();
	auto remaining = string.size();
	auto lastRemaining = remaining;
	float advance = 0.0f;

	if( options->kerning ) {
		uint64 key = 0;
		while( utf8::next( ptr, remaining, &codepoint ) ) {
			key = nextKerningKey( key, codepoint );
			maxWidth += getKerningAmount( &font->kerning, key ) * scale;
			advance = getGlyph( font, codepoint )->advance * scale + sep;
			maxWidth += advance;
			if( maxWidth > textAreaWidth ) break;
			lastRemaining = remaining;
			++result.count;
		}
	} else {
		while( utf8::next( ptr, remaining, &codepoint ) ) {
			advance = getGlyph( font, codepoint )->advance * scale + sep;
			maxWidth += advance;
			if( maxWidth > textAreaWidth ) break;
			lastRemaining = remaining;
			++result.count;
		}
	}
	if( remaining && maxWidth > textAreaWidth && advance > 0 ) {
		// decide whether we should advance one more time, since textAreaWidth extends into a glyph
		auto diff = maxWidth - textAreaWidth;
		auto ratio = diff / advance;
		if( ratio < 0.5f ) {
			lastRemaining = remaining;
			++result.count;
		}
	}
	result.index = string.size() - lastRemaining;
	return result;
}
CharCountResult charCount( Font* font, StringView string, float textAreaWidth )
{
	auto info = getFontInfo( font );
	assert( info );
	return charCount( info, &font->renderOptions, string, textAreaWidth );
}

TextChain renderText( MeshStream* stream, FontInfo* info, FontRenderOptions* options,
                      float verticalPadding, StringView text, rectfarg rect, float z = 0,
                      TextChain chain = {} )
{
	PROFILE_FUNCTION();

	assert( stream );
	assert( info );
	assert( options );

	auto origin         = rect.leftTop;
	auto pos            = floor( chain.pos );
	auto textAreaWidth  = ( rect.right == 0 ) ? ( 0.0f ) : ( width( rect ) );
	auto textAreaHeight = ( rect.bottom == 0 ) ? ( 0.0f ) : ( height( rect ) );

	auto scale = options->scale;
	auto newLineAdvance = ( info->newLineAdvance + verticalPadding ) * scale;
	auto sep = options->sep * scale;

	switch( options->verticalAlign ) {
		case FontVerticalAlign::Top: {
			break;
		}
		case FontVerticalAlign::Center: {
			auto height = stringHeight( info, options, text, textAreaWidth, verticalPadding );
			pos.y += floor( ( textAreaHeight - height ) * 0.5f );
			break;
		}
		case FontVerticalAlign::Bottom: {
			auto height = stringHeight( info, options, text, textAreaWidth, verticalPadding );
			pos.y += textAreaHeight - height;
			break;
		}
	}

	auto endOfLine          = 0;
	auto getAlignmentOffset = [&]() {
		float ret = {};
		switch( options->align ) {
			case FontAlign::Left: {
				break;
			}
			case FontAlign::Center: {
				auto substr = StringView( text, endOfLine );
				if( substr.size() ) {
					auto width = lineWidth( info, options, options->kerning != 0, text, endOfLine,
					                        &endOfLine );
					auto halfOffset = ( textAreaWidth - width ) * 0.5f;
					ret             = floor( halfOffset );
				}
				break;
			}
			case FontAlign::Right: {
				auto width =
				    lineWidth( info, options, options->kerning != 0, text, endOfLine, &endOfLine );
				ret = textAreaWidth - width;
				break;
			}
				InvalidDefaultCase;
		}
		return ret;
	};

	pos.x += getAlignmentOffset();
	pos.y += info->baseline;
	auto kerningKey = toKerningKey( chain.lastCodepoint, chain.lastCodepoint );
	auto isFirst = ( chain.lastCodepoint == 0 );

	Vertex vertices[4] = {
		{0, 0, z, stream->color, 0, 0},
		{0, 0, z, stream->color, 0, 0},
		{0, 0, z, stream->color, 0, 0},
		{0, 0, z, stream->color, 0, 0},
	};

	while( text.size() ) {
		auto codepoint = utf8::next( &text );
		kerningKey = nextKerningKey( kerningKey, codepoint );
		pos.x += getKerningAmount( &info->kerning, kerningKey ) * scale;
		auto range = findFontRange( info->ranges, codepoint );
		auto visible = true;
		const FontGlyph* glyph;
		if( !range ) {
			range = &info->defaultRange;
			glyph = &info->defaultGlyph;
			visible = codepoint > 0x20; // skip control chars
		} else {
			visible = range->visible;
			glyph = getGlyph( range, codepoint );
		}
		if( isFirst ) {
			pos.x -= glyph->overhang;
			isFirst = false;
		}
		auto advance = glyph->advance * scale + sep;

		// newline calculations
		auto isOutsideBounds = ( textAreaWidth > 0.001f && pos.x + advance > textAreaWidth );
		auto forceNewline = false;
		if( isOutsideBounds ) {
			switch( options->wrappingMode ) {
				case WrappingMode::None: {
					break;
				}
				case WrappingMode::Character: {
					forceNewline = true;
					break;
				}
				case WrappingMode::Word: {
					// TODO: implement
					break;
				}
				case WrappingMode::Clip: {
					text = substr( text, find( text, '\n' ) );
					if( !text.size() ) {
						visible = false;
					}
					break;
				}
				InvalidDefaultCase;
			}
		}
		auto overhang = glyph->overhang * scale;
		if( codepoint == '\n' || forceNewline ) {
			pos.x = getAlignmentOffset();
			if( codepoint == '\n' ) {
				isFirst = true;
			} else {
				pos.x -= overhang;
			}
			if( textAreaHeight > 0.001f && pos.y + newLineAdvance > textAreaHeight ) {
				break;
			}
			pos.y += newLineAdvance;
		}

		// make sure that glyph is visible and that pos is inside bounds (might be pushed out
		// because of FontAlign)
		if( visible && pos.x >= 0 ) {
			auto textureWidth  = range->textureWidth * scale;
			auto textureHeight = range->textureHeight * scale;

			auto renderCharWidth    = width( glyph->texCoords ) * textureWidth;
			auto renderCharHeight   = height( glyph->texCoords ) * textureHeight;
			auto left               = pos.x + overhang + origin.x;
			auto top                = pos.y - glyph->ascend + origin.y;
			vertices[0].position.x  = left;
			vertices[0].position.y  = top;
			vertices[0].texCoords.u = glyph->texCoords.left;
			vertices[0].texCoords.v = glyph->texCoords.top;

			vertices[1].position.x  = left + renderCharWidth;
			vertices[1].position.y  = top;
			vertices[1].texCoords.u = glyph->texCoords.right;
			vertices[1].texCoords.v = glyph->texCoords.top;

			vertices[2].position.x  = left;
			vertices[2].position.y  = top + renderCharHeight;
			vertices[2].texCoords.u = glyph->texCoords.left;
			vertices[2].texCoords.v = glyph->texCoords.bottom;

			vertices[3].position.x  = left + renderCharWidth;
			vertices[3].position.y  = top + renderCharHeight;
			vertices[3].texCoords.u = glyph->texCoords.right;
			vertices[3].texCoords.v = glyph->texCoords.bottom;

			pushQuad( stream, vertices );
		}
		if( codepoint != '\n' ) {
			pos.x += advance;
		}
	}

	pos.y -= info->baseline;
	return {pos, getSecondCodepointFromKerningKey( kerningKey )};
}
TextChain renderText( MeshStream* stream, Font* font, StringView text, rectfarg rect, float z = 0,
                      TextChain chain = {} )
{
	return renderText( stream, getFontInfo( font ), &font->renderOptions, font->verticalPadding,
	                   text, rect, z, chain );
}
TextChain renderText( RenderCommands* renderer, Font* font, StringView text, rectfarg rect,
                      float z = 0, TextChain chain = {} )
{
	PROFILE_FUNCTION();
	
	auto info = getFontInfo( font );
	setTexture( renderer, 0, info->ranges[0].texture );
	MESH_STREAM_BLOCK( stream, renderer ) {
		chain = renderText( stream, info, &font->renderOptions, font->verticalPadding, text, rect,
		                    z, chain );
	}
	return chain;
#if 0
	auto info    = getFontInfo( font );
	auto options = &font->renderOptions;

	auto pos            = floor( chain.pos );
	auto textAreaWidth  = ( rect.right == 0 ) ? ( 0.0f ) : ( width( rect ) );
	auto textAreaHeight = ( rect.bottom == 0 ) ? ( 0.0f ) : ( height( rect ) );

	auto scale = options->scale;
	auto newLineAdvance = ( info->newLineAdvance + font->verticalPadding ) * scale;
	auto sep = options->sep * scale;

	/*switch( verticalAlign ) {
		case FontVerticalAlign::Top: {
			break;
		}
		case FontVerticalAlign::Center: {
			auto height =
				info->stringHeight( string, renderOptions, textAreaWidth, verticalPadding );
			pos.y += floor( ( textAreaHeight - height ) * 0.5f );
			break;
		}
		case FontVerticalAlign::Bottom: {
			auto height =
				info->stringHeight( string, renderOptions, textAreaWidth, verticalPadding );
			pos.y += textAreaHeight - height;
			break;
		}
	}*/

	// auto endOfLine = 0;
	auto getAlignmentOffset = [&]() {
		float ret = {};
		/*switch( options->align ) {
			case FontAlign::Left: {
				break;
			}
			case FontAlign::Center: {
				auto substr = StringView( string, endOfLine );
				if( substr.size() ) {
					auto width = info->lineWidth( string, endOfLine, renderOptions,
												  renderOptions.kerning, &endOfLine );
					auto halfOffset = ( textAreaWidth - width ) * 0.5f;
					ret = floor( halfOffset );
				}
				break;
			}
			case FontAlign::Right: {
				auto width = info->lineWidth( string, endOfLine, renderOptions,
											  renderOptions.kerning, &endOfLine );
				ret = textAreaWidth - width;
				break;
			}
			InvalidDefaultCase;
		}*/
		return ret;
	};

	pos.x += getAlignmentOffset();
	pos.y += info->baseline;
	auto kerningKey = toKerningKey( chain.lastCodepoint, chain.lastCodepoint );
	auto isFirst = ( chain.lastCodepoint == 0 );

	Vertex vertices[4] = {
		{0, 0, z, renderer->color, 0, 0},
		{0, 0, z, renderer->color, 0, 0},
		{0, 0, z, renderer->color, 0, 0},
		{0, 0, z, renderer->color, 0, 0},
	};

	MeshStreamingBlock block = {};
	TextureId currentTexture = {};
	while( text.size() ) {
		auto codepoint = utf8::next( &text );
		kerningKey = nextKerningKey( kerningKey, codepoint );
		pos.x += getKerningAmount( &info->kerning, kerningKey ) * scale;
		auto range = findFontRange( info->ranges, codepoint );
		auto visible = true;
		const FontGlyph* glyph;
		if( !range ) {
			range = &info->defaultRange;
			glyph = &info->defaultGlyph;
			visible = codepoint > 0x20; // skip control chars
		} else {
			visible = range->visible;
			glyph = getGlyph( range, codepoint );
		}
		if( isFirst ) {
			pos.x -= glyph->overhang;
			isFirst = false;
		}
		auto advance = glyph->advance * scale + sep;

		// newline calculations
		auto isOutsideBounds = ( textAreaWidth > 0.001f && pos.x + advance > textAreaWidth );
		auto forceNewline = false;
		if( isOutsideBounds ) {
			switch( options->wrappingMode ) {
				case WrappingMode::None: {
					break;
				}
				case WrappingMode::Character: {
					forceNewline = true;
					break;
				}
				case WrappingMode::Word: {
					// TODO: implement
					break;
				}
				case WrappingMode::Clip: {
					text = substr( text, find( text, '\n' ) );
					if( !text.size() ) {
						visible = false;
					}
					break;
				}
				InvalidDefaultCase;
			}
		}
		auto overhang = glyph->overhang * scale;
		if( codepoint == '\n' || forceNewline ) {
			pos.x = getAlignmentOffset();
			if( codepoint == '\n' ) {
				isFirst = true;
			} else {
				pos.x -= overhang;
			}
			if( textAreaHeight > 0.001f && pos.y + newLineAdvance > textAreaHeight ) {
				break;
			}
			pos.y += newLineAdvance;
		}

		// make sure that glyph is visible and that pos is inside bounds (might be pushed out
		// because of FontAlign)
		if( visible && pos.x >= 0 ) {
			auto textureWidth  = range->textureWidth * scale;
			auto textureHeight = range->textureHeight * scale;

			if( currentTexture != range->texture ) {
				if( block ) {
					endMeshStreaming( renderer, &block );
				}
				setTexture( renderer, 0, range->texture );
				currentTexture = range->texture;
				block = beginMeshStreaming( renderer );
			}
			auto renderCharWidth    = width( glyph->texCoords ) * textureWidth;
			auto renderCharHeight   = height( glyph->texCoords ) * textureHeight;
			auto left               = pos.x + overhang;
			auto top                = pos.y - glyph->ascend;
			vertices[0].position.x  = left;
			vertices[0].position.y  = top;
			vertices[0].texCoords.u = glyph->texCoords.left;
			vertices[0].texCoords.v = glyph->texCoords.top;

			vertices[1].position.x  = left;
			vertices[1].position.y  = top + renderCharHeight;
			vertices[1].texCoords.u = glyph->texCoords.left;
			vertices[1].texCoords.v = glyph->texCoords.bottom;

			vertices[2].position.x  = left + renderCharWidth;
			vertices[2].position.y  = top;
			vertices[2].texCoords.u = glyph->texCoords.right;
			vertices[2].texCoords.v = glyph->texCoords.top;

			vertices[3].position.x  = left + renderCharWidth;
			vertices[3].position.y  = top + renderCharHeight;
			vertices[3].texCoords.u = glyph->texCoords.right;
			vertices[3].texCoords.v = glyph->texCoords.bottom;

			pushQuad( &block.stream, vertices );
		}
		if( codepoint != '\n' ) {
			pos.x += advance;
		}
	}
	if( block ) {
		endMeshStreaming( renderer, &block );
	}

	pos.y -= info->baseline;
	return {pos, getSecondCodepointFromKerningKey( kerningKey )};
#endif
}

#endif // _FONT_H_INCLUDED_
