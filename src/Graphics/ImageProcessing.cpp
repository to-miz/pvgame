void copyRegion( void* dest, rectiarg destRegion, int32 destStride, int32 elementSize, void* src,
                 rectiarg srcRegion, int32 srcStride )
{
	assert( width( destRegion ) == width( srcRegion ) );
	assert( height( destRegion ) == height( srcRegion ) );

	auto rowSize   = min( width( destRegion ), width( srcRegion ) ) * elementSize;
	auto minHeight = min( height( destRegion ), height( srcRegion ) );

	auto destRow = (char*)dest + destRegion.left * elementSize + destRegion.top * destStride;
	auto srcRow  = (char*)src + srcRegion.left * elementSize + srcRegion.top * srcStride;
	for( auto y = 0; y < minHeight; ++y, destRow += destStride, srcRow += srcStride ) {
		memcpy( destRow, srcRow, rowSize );
	}
}

static recti imageFindRect( uint8* pixels, int32 x, int32 y, int32 width, int32 height,
							int32 stride, int32 border )
{
	recti ret = {x, y, x + 1, y + 1};
	// MaxDist == 2 means that we allow 1 pixel non connectedness
	const int32 MaxDist = border + 1;

	auto expandDir = [MaxDist]( uint8* pixels, int32* start, int32 end, int32 min, int32 max,
	                            int32 columnStride, int32 rowStride, int32 step, int32 offset ) {
		auto changed  = false;
		auto i        = *start + offset;
		auto notFound = 0;
		auto row      = pixels + i * rowStride + min * columnStride;
		for( ; i != end; i += step ) {
			auto found  = false;
			auto column = row;
			for( auto j = min; j < max; ++j, column += columnStride ) {
				auto current = column;
				// rgba color, 3rd component is alpha
				if( current[3] != 0 ) {
					found = changed = true;
					*start          = i + 1 + offset;
					break;
				}
			}
			if( !found ) {
				++notFound;
				if( notFound >= MaxDist ) {
					break;
				}
			} else {
				notFound = 0;
			}
			row += rowStride * step;
		}
		return changed;
	};

	bool changed;
	do {
		changed = false;
		// expand right
		if( expandDir( pixels, &ret.right, width, ret.top, ret.bottom, stride, 4, 1, 0 ) ) {
			changed = true;
		}
		// expand bottom
		if( expandDir( pixels, &ret.bottom, height, ret.left, ret.right, 4, stride, 1, 0 ) ) {
			changed = true;
		}
		// expand left
		if( expandDir( pixels, &ret.left, -1, ret.top, ret.bottom, stride, 4, -1, -1 ) ) {
			changed = true;
		}
		// expand top
		if( expandDir( pixels, &ret.top, -1, ret.left, ret.right, 4, stride, -1, -1 ) ) {
			changed = true;
		}
	} while( changed );

	return ret;
}
void imageFindRects( UArray< recti >& rects, ImageData image, recti region, int32 border,
                     bool mergeOverlappingCells )
{
	if(::width( region ) == 0 ) {
		region.right = image.width;
	}
	if(::height( region ) == 0 ) {
		region.bottom = image.height;
	}
	auto stride     = image.width * 4;
	auto currentRow = image.data + region.left * 4 + region.top * image.width * 4;
	auto width      = ::width( region );
	auto height     = ::height( region );

	/*auto findFirstPixel = */ [&]() {
		for( auto y = 0; y < height; ++y ) {
			auto p = currentRow;
			for( auto x = 0; x < width; ++x, p += 4 ) {
				auto pixel = p;
				// pixel[3] is the alpha component of rgba
				if( pixel[3] != 0 ) {
					auto alreadyProcessed = false;
					FOR( entry : rects ) {
						if( isPointInside( entry, {x, y} ) ) {
							alreadyProcessed = true;
							break;
						}
					}
					if( !alreadyProcessed ) {
						if( !rects.remaining() ) {
							return;
						}
						rects.push_back(
						    imageFindRect( image.data, x, y, width, height, stride, border ) );
					}
				}
			}
			currentRow += stride;
		}
	}();

	if( mergeOverlappingCells ) {
		auto begin = rects.begin();
		auto end   = rects.end();
		for( auto it = begin; it != end; ) {
			bool erased = false;
			FOR( other : rects ) {
				if( &other == it ) {
					continue;
				}
				if( isOverlapping( *it, other ) ) {
					other.left   = MIN( it->left, other.left );
					other.top    = MIN( it->top, other.top );
					other.right  = MAX( it->right, other.right );
					other.bottom = MAX( it->bottom, other.bottom );

					it     = rects.erase( it );
					end    = rects.end();
					erased = true;
					break;
				}
			}
			if( !erased ) {
				++it;
			}
		}
	}
}
Array< recti > imageFindRects( StackAllocator* allocator, ImageData image, rectiarg region,
                               int32 border, bool mergeOverlappingCells )
{
	assert( image );

	auto rects = beginVector( allocator, recti );
	imageFindRects( rects, image, region, border, mergeOverlappingCells );
	endVector( allocator, &rects );
	return {rects.data(), rects.size()};
}