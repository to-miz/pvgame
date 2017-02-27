struct win32FontDc {
	HDC hdc;
	HFONT oldFont;
	HFONT font;
};

static win32FontDc win32MakeFontDc( HDC screenDc, const wchar_t* fontName, StringView utf8Name,
                                    int32 size, int32 weight, bool italic )
{
	win32FontDc result = {};
	result.hdc         = CreateCompatibleDC( screenDc );
	SetMapMode( result.hdc, MM_TEXT );
	/*
	ANTIALIASED_QUALITY
	CLEARTYPE_QUALITY
	DEFAULT_QUALITY
	NONANTIALIASED_QUALITY
	*/
	result.font =
	    CreateFontW( -size, 0, 0, 0, weight, italic, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
	                 CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, fontName );
	if( !result.font ) {
		LOG( ERROR, "CreateFontW failed" );
		return result;
	}
	result.oldFont = SelectFont( result.hdc, result.font );
	SetTextColor( result.hdc, RGB( 255, 255, 255 ) );
	SetBkColor( result.hdc, RGB( 0, 0, 0 ) );
	// SetBkMode( hdc, TRANSPARENT );
	return result;
}

static void win32DestroyFontDc( win32FontDc dc )
{
	if( dc.font ) {
		SelectFont( dc.hdc, dc.oldFont );
		DeleteObject( dc.font );
	}
	DeleteDC( dc.hdc );
}

bool win32GenerateFont( StackAllocator* allocator, StackAllocator* scrap, win32FontDc fontDc,
                        FontUnicodeRequestRanges ranges, FontInfo* out )
{
	assert( isValid( allocator ) );
	assert( out );

	auto width  = 512;
	auto height = 512;

	auto hdc                       = fontDc.hdc;
	BITMAPINFO info                = {};
	info.bmiHeader.biSize          = sizeof( BITMAPINFOHEADER );
	info.bmiHeader.biWidth         = width;
	info.bmiHeader.biHeight        = -height;
	info.bmiHeader.biPlanes        = 1;
	info.bmiHeader.biBitCount      = 24;
	info.bmiHeader.biCompression   = BI_RGB;
	info.bmiHeader.biXPelsPerMeter = 0;
	info.bmiHeader.biYPelsPerMeter = 0;
	info.bmiHeader.biClrUsed       = 0;
	info.bmiHeader.biClrImportant  = 0;

	void* bits;
	HBITMAP bitmap = CreateDIBSection( hdc, &info, DIB_RGB_COLORS, &bits, nullptr, 0 );
	if( !bitmap ) {
		LOG( ERROR, "CreateDIBSection failed" );
		return false;
	}
	SCOPE_EXIT( bitmap ) {
		DeleteObject( bitmap );
	};
	SelectObject( hdc, bitmap );

	HBRUSH brush = CreateSolidBrush( RGB( 0, 0, 0 ) );
	RECT rect;
	rect.left   = 0;
	rect.right  = width;
	rect.top    = 0;
	rect.bottom = height;

	FillRect( hdc, &rect, brush );

	TEXTMETRICW metric;
	GetTextMetricsW( hdc, &metric );
	auto padding       = 1;
	auto x             = 0;
	auto y             = 0;
	auto charHeight    = metric.tmHeight;
	float oneOverWidth = 1.0f / ( width - 2 );

	// TODO: unicode support
	/*fontRanges = win32_GetFontUnicodeRanges( scrap, fontName, utf8Name, size, weight, italic,
	                                         ranges, hdc );
	if( !win32_MakeFontData( arena, fontRanges, out ) ) {
	    LOG_ERROR << "Failed to create font out, out of memory";
	    return false;
	}*/
	auto baseline = out->baseline = (float)( metric.tmAscent );
	out->newLineAdvance           = (float)( metric.tmHeight + metric.tmExternalLeading );
	out->averageCharWidth         = (float)( metric.tmAveCharWidth );

	auto drawDefaultGlyph = true;

	if( drawDefaultGlyph ) {
		// draw default glyph
		ABC defWidth;
		if( metric.tmPitchAndFamily & TMPF_TRUETYPE ) {
			GetCharABCWidthsW( hdc, metric.tmDefaultChar, metric.tmDefaultChar, &defWidth );
		} else {
			int32 bWidth;
			GetCharWidth32W( hdc, metric.tmDefaultChar, metric.tmDefaultChar, &bWidth );
			defWidth      = {};
			defWidth.abcB = bWidth;
		}
		int32 advance = 0;
		auto& w       = defWidth;
		auto overhang = 0;
		if( w.abcA < 0 ) {
			x -= w.abcA;
			overhang += w.abcA;
		} else {
			advance += w.abcA;
		}
		advance += w.abcB;
		if( x + advance > width ) {
			x = -overhang;
			y += charHeight;
		}

		assert( y < height );
		// 1 pixel offset from border because of bin packing padding
		auto absAdvance = abs( (int32)w.abcA ) + abs( (int32)w.abcB ) /* + abs( (int32)w.abcC )*/;
		auto pen        = CreatePen( PS_SOLID, 1, RGB( 255, 255, 255 ) );
		auto oldPen     = SelectObject( hdc, pen );
		auto oldBrush   = SelectObject( hdc, brush );
		Rectangle( hdc, x + 1, y + 1 + metric.tmAscent, x + 1 + absAdvance,
		           y + 1 + metric.tmDescent );
		SelectObject( hdc, oldPen );
		SelectObject( hdc, oldBrush );
		DeleteObject( pen );

		auto entry              = &out->defaultGlyph;
		auto leftMost           = x + overhang;
		entry->ascend           = baseline;
		entry->texCoords.left   = leftMost * oneOverWidth;
		entry->texCoords.top    = (float)y;
		entry->texCoords.right  = ( leftMost + absAdvance ) * oneOverWidth;
		entry->texCoords.bottom = (float)( y + charHeight );
		entry->advance          = (float)absAdvance;
		x += absAdvance + padding;
	}

	for( intmax i = 0, rangesCount = ranges.size(); i < rangesCount; ++i ) {
		auto current = ranges[i];
		assert( current.max - current.min > 0 );
		auto dest = &out->ranges[i];
		if( dest->max - dest->min <= 0 ) {
			dest->min             = current.min;
			dest->max             = current.max;
			dest->visible         = current.visible;
			size_t codepointCount = dest->max - dest->min;
			dest->glyphs          = allocateArray( allocator, FontGlyph, codepointCount );

			TEMPORARY_MEMORY_BLOCK( scrap ) {
				auto widths = allocateArray( scrap, ABC, codepointCount );
				// TrueType fonts require different method of acquiring char widths
				if( metric.tmPitchAndFamily & TMPF_TRUETYPE ) {
					GetCharABCWidthsW( hdc, dest->min, dest->max - 1, widths );
				} else {
					auto bWidths = allocateArray( scrap, int32, codepointCount );
					GetCharWidth32W( hdc, dest->min, dest->max - 1, bWidths );
					for( size_t j = 0; j < codepointCount; ++j ) {
						widths[j].abcA = 0;
						widths[j].abcB = bWidths[j];
						widths[j].abcC = 0;
					}
				}

				for( size_t j = 0; j < codepointCount; ++j ) {
					auto& w                  = widths[j];
					dest->glyphs[j].overhang = min( (float)w.abcA, 0.0f );
					dest->glyphs[j].advance  = (float)( w.abcA + w.abcB + w.abcC );
				}

				if( dest->visible ) {
					for( size_t j = 0; j < codepointCount; ++j ) {
						int32 advance = 0;
						auto& w       = widths[j];
						auto overhang = 0;
						if( w.abcA < 0 ) {
							x -= w.abcA;
							overhang += w.abcA;
						} else {
							advance += w.abcA;
						}
						advance += w.abcB;
						if( x + advance > width ) {
							x = -overhang;
							y += charHeight;
						}

						assert( y < height );
						wchar_t c = static_cast< wchar_t >( j + dest->min );
						// 1 pixel offset from border because of bin packing padding
						TextOutW( hdc, x + 1, y + 1, &c, 1 );

						auto entry = &dest->glyphs[j];
						auto absAdvance =
						    abs( (int32)w.abcA ) + abs( (int32)w.abcB ) /* + abs( (int32)w.abcC )*/;
						auto leftMost           = x + overhang;
						entry->ascend           = baseline;
						entry->texCoords.left   = leftMost * oneOverWidth;
						entry->texCoords.top    = (float)y;
						entry->texCoords.right  = ( leftMost + absAdvance ) * oneOverWidth;
						entry->texCoords.bottom = (float)( y + charHeight );
						x += advance + padding;
					}
				}
			}
		}
	}

	// Kerning Pairs
	size_t num = GetKerningPairsW( hdc, 0, nullptr );
	if( num != 0 ) {
		assert( !out->kerning.keys );
		assert( !out->kerning.amounts );

		out->kerning.keys    = allocateArray( allocator, uint64, num );
		out->kerning.amounts = allocateArray( allocator, float, num );
		out->kerning.count   = safe_truncate< int32 >( num );
		size_t currentIndex = 0;

		TEMPORARY_MEMORY_BLOCK( scrap ) {
			auto kerningpairs = allocateArray( scrap, KERNINGPAIR, num );
			if( kerningpairs && out->kerning.keys && out->kerning.amounts ) {
				GetKerningPairsW( hdc, (DWORD)num, kerningpairs );

				FontRange* second;
				for( size_t j = 0u; j < num; j++ ) {
					if( kerningpairs[j].iKernAmount != 0
					    && findFontRange( out->ranges, kerningpairs[j].wFirst )
					    && ( second = findFontRange( out->ranges, kerningpairs[j].wSecond ) )
					           != nullptr ) {
						// make sure that iKernAmount isn't already taken care of by the overhang
						// of second character
						auto glyph  = getGlyph( second, kerningpairs[j].wSecond );
						auto amount = kerningpairs[j].iKernAmount - (int32)round( glyph->overhang );
						if( amount != 0 ) {
							if( currentIndex >= num ) {
								OutOfMemory();
								break;
							}
							auto kerningKey    = &out->kerning.keys[currentIndex];
							auto kerningAmount = &out->kerning.amounts[currentIndex];
							++currentIndex;
							*kerningKey =
							    toKerningKey( kerningpairs[j].wFirst, kerningpairs[j].wSecond );
							*kerningAmount = (float)amount;
						} else {
							--out->kerning.count;
						}
					} else {
						--out->kerning.count;
					}
				}
			}
		}

		out->kerning.amounts = fitToSizeArrays( allocator, out->kerning.keys, out->kerning.count,
		                                        num, out->kerning.amounts, out->kerning.count, num );
	}

	auto pitch = width * 3;
	width -= 2;
	height -= 2;
	auto textureWidth = (float)( width );
	auto textureHeight = (float)( height );
	TextureId texture = {};
	TEMPORARY_MEMORY_BLOCK( scrap ) {
		// skip 1 pixel from left and top for now offset for now
		auto currentBits = (char*)bits + 3 + pitch;
		auto imageData   = allocateArray( scrap, uint8, width * height * 4 );
		ImageData image  = {imageData, width, height};
		for( auto y = 0; y < height; ++y ) {
			auto row = currentBits;
			for( auto x = 0; x < width; ++x ) {
				auto color   = row[0];
				imageData[0] = 0xFF;   // r
				imageData[1] = 0xFF;   // g
				imageData[2] = 0xFF;   // b
				imageData[3] = color;  // a
				imageData += 4;
				row += 3;
			}
			currentBits += pitch;
		}

		// glyphs are generated, create texture
		texture = toTextureId( win32UploadImageToGpu( image ) );
	}
	assert( texture );
	if( !texture ) {
		return false;
	}

	auto oneOverActualHeight = 1.0f / textureHeight;
	// auto oneOverActualWidth  = 1.0f / textureWidth;

	// correct chars.y with actual height
	if( drawDefaultGlyph ) {
		out->defaultGlyph.texCoords.top *= oneOverActualHeight;
		out->defaultGlyph.texCoords.bottom *= oneOverActualHeight;
		out->defaultRange = {0, 0, true, nullptr, texture, textureWidth, textureHeight};
	}
	/*if( supplement ) {
		FOR( range : out->ranges ) {
			if( !range.texture ) {
				range.texture = texture;
				range.textureWidth = textureWidth;
				range.textureHeight = textureHeight;
				auto codepointCount = range.count();
				for( auto i = 0; i < codepointCount; ++i ) {
					auto texCoords = &range.chars[i].texCoords;
					texCoords->top *= oneOverActualHeight;
					texCoords->bottom *= oneOverActualHeight;
				}
			}
		}
	} else*/ {
		FOR( range : out->ranges ) {
			range.texture = texture;
			range.textureWidth = textureWidth;
			range.textureHeight = textureHeight;
			auto codepointCount = range.max - range.min;
			for( uint32 i = 0; i < codepointCount; ++i ) {
				auto texCoords = &range.glyphs[i].texCoords;
				texCoords->top *= oneOverActualHeight;
				texCoords->bottom *= oneOverActualHeight;
			}
		}
	}

	return true;
}

static FontUnicodeRequestRanges win32RequestFontUnicodeRanges( StackAllocator* scrap, HDC hdc,
                                                               FontUnicodeRequestRanges ranges )
{
	const int32 MAX_RANGES = 500;
	auto fontRanges        = makeUArray( scrap, FontUnicodeRequestRange, MAX_RANGES );
	TEMPORARY_MEMORY_BLOCK( scrap ) {
		auto glyphSetCount = GetFontUnicodeRanges( hdc, nullptr );
		auto glyphSet      = allocateArray( scrap, GLYPHSET, glyphSetCount );
		if( GetFontUnicodeRanges( hdc, glyphSet ) ) {
			for( DWORD i = 0, count = glyphSet->cRanges; i < count; ++i ) {
				uint32 firstCodepoint = glyphSet->ranges[i].wcLow;
				uint32 lastCodepoint  = glyphSet->ranges[i].wcLow + glyphSet->ranges[i].cGlyphs - 1;

				FOR( range : ranges ) {
					// do ranges overlap?
					if( range.min <= lastCodepoint && firstCodepoint < range.max ) {
						// get overlap range
						auto overlapFirst = MAX( range.min, firstCodepoint );
						auto overlapLast  = MIN( range.max, lastCodepoint + 1 );

						// check if range is already in fontRanges (paranoid)
						bool isAlreadyInFontRange = false;
						FOR( fontRange : fontRanges ) {
							if( overlapFirst >= fontRange.min && overlapLast <= fontRange.max ) {
								isAlreadyInFontRange = true;
								break;
							}
						}
						if( !isAlreadyInFontRange ) {
							auto added = fontRanges.emplace_back();
							*added     = {overlapFirst, overlapLast, range.visible};
						}
					}
				}
			}
		} else {
			LOG( ERROR, "Failed to get unicode ranges" );
			assert( fontRanges.capacity() >= ranges.size() );
			fontRanges.assign( ranges.begin(), ranges.end() );
		}
	}
	return makeArrayView( fontRanges );
}

Font win32LoadFont( StackAllocator* allocator, StringView utf8Name, int32 size, int32 weight,
                    bool italic, FontUnicodeRequestRanges ranges )
{
	Font result = {};
	result.renderOptions = defaultFontRenderOptions();

	assert( isValid( allocator ) );
	WString fontName = WString::fromUtf8( utf8Name );

	auto hdc    = GetDC( Win32AppContext.window.hwnd );
	auto fontDc = win32MakeFontDc( hdc, fontName, utf8Name, size, weight, italic );
	SCOPE_EXIT( & ) {
		win32DestroyFontDc( fontDc );
		ReleaseDC( Win32AppContext.window.hwnd, hdc );
	};
	if( fontDc.font ) {
		auto partition = StackAllocatorPartition::ratio( allocator, 1 );
		auto corrected = win32RequestFontUnicodeRanges( partition.scrap(), fontDc.hdc, ranges );
		result.normal  = allocateStruct( partition.primary(), FontInfo );
		*result.normal = {};
		result.normal->ranges = makeArray( partition.primary(), FontRange, corrected.size() );
		zeroMemory( result.normal->ranges.data(), result.normal->ranges.size() );
		if( win32GenerateFont( partition.primary(), partition.scrap(), fontDc, corrected,
		                       result.normal ) ) {
			partition.commit();
		} else {
			result = {};
		}
	}

	return result;
}