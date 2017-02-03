/*
TODO:
    - finish up saving texture coordinates into VoxelGridTextureMap
    - implement saving/loading from file
*/

#define STBIW_ASSERT( x ) assert( x )
#define STBIW_MALLOC( sz ) malloc( sz )
#define STBIW_REALLOC( p, newsz ) realloc( p, newsz )
#define STBIW_FREE( p ) free( p )
#define STBIW_MEMMOVE( a, b, sz ) memmove( a, b, sz )
#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

TexturePackRegion* getTexturePackRegion( TexturePackState* editor, int32 id )
{
	return find_first_where( editor->uniqueTextureRegions, entry.id == id );
}

QuadTexCoords texturePackGetTexCoords( rectfarg rect, TexturePackOrientation orientation )
{
	QuadTexCoords texCoords = makeQuadTexCoords( rect );
	switch( orientation ) {
		case TexturePackOrientation::Def: {
			break;
		}
		case TexturePackOrientation::Cw90: {
			texCoords = makeQuadTexCoordsCw90( texCoords );
			break;
		}
		case TexturePackOrientation::Ccw90: {
			texCoords = makeQuadTexCoordsCcw90( texCoords );
			break;
		}
		case TexturePackOrientation::Horizontal: {
			texCoords = makeQuadTexCoordsMirroredHorizontal( texCoords );
			break;
		}
		case TexturePackOrientation::Vertical: {
			texCoords = makeQuadTexCoordsMirroredVertical( texCoords );
			break;
		}
		case TexturePackOrientation::Diagonal: {
			texCoords = makeQuadTexCoordsMirroredDiagonal( texCoords );
			break;
		}
		InvalidDefaultCase;
	}
	return texCoords;
}
TexturePackRegion* texturePackAddUniqueTextureRegion( AppData* app, rectiarg rect,
                                                      TextureId source )
{
	auto editor = &app->texturePackState;

	auto textureRegion = find_first_where(
	    editor->uniqueTextureRegions,
	    entry.texture == source && memcmp( &entry.rect, &rect, sizeof( recti ) ) == 0 );

	if( !textureRegion ) {
		auto regionId = ++editor->regionIds;
		if( !editor->uniqueTextureRegions.remaining() ) {
			LOG( ERROR, "out of memory for unique texture regions" );
		} else {
			editor->uniqueTextureRegions.push_back( {regionId, rect, source} );
			textureRegion = &editor->uniqueTextureRegions.back();
		}
	}
	return textureRegion;
}

bool doTextureDisplay( AppData* app, TexturePackSource* source )
{
	assert( app );

	auto renderer       = imguiRenderer();
	auto inputs         = imguiInputs();
	auto editor         = &app->texturePackState;
	auto textureDisplay = &editor->textureDisplay;

	imguiSlider( "Texture Scale", &editor->textureScale, 1, 10 );

	if( source
	    && imguiBeginDropGroup( "Texture Region", &textureDisplay->textureRegionExpanded ) ) {

		rectf textureRect = {0, 0, source->width * editor->textureScale,
		                     source->height * editor->textureScale};
		auto rect = imguiScrollable( &textureDisplay->scrollPos, textureRect, 800, 600 );

		auto clip = ClippingRect( renderer, rect );
		setTexture( renderer, 0, source->id );
		renderer->color = Color::White;
		auto origin     = rect.leftTop - textureDisplay->scrollPos;
		addRenderCommandSingleQuad( renderer, translate( textureRect, origin ) );

		auto handle = imguiMakeHandle( &textureDisplay->scrollPos, ImGuiControlType::Scrollable );
		if( isPointInside( rect, inputs->mouse.position ) && imguiIsHover( handle )
		    && isKeyPressed( inputs, KC_RButton ) ) {

			textureDisplay->selectedIndex = -1;
		}
		if( textureDisplay->selectedIndex < 0 ) {
			if( isPointInside( rect, inputs->mouse.position ) && imguiIsHover( handle ) ) {
				setTexture( renderer, 0, null );
				renderer->color = 0x80000000;
				FOR( entry : makeRangeView( editor->textureRegions, source->regions ) ) {
					auto scaled = translate( entry * editor->textureScale, origin );
					if( isPointInside( scaled, inputs->mouse.position ) ) {
						addRenderCommandSingleQuad( renderer, scaled );

						if( isKeyPressed( inputs, KC_LButton ) ) {
							textureDisplay->selectedIndex =
							    indexof( editor->textureRegions, entry );
							textureDisplay->selectedRegion = Rect< float >( entry );
						}
					}
				}
			}
		} else {
			renderer->color = Color::White;
			auto translated = translate( textureRect, origin );
			imguiRect( &textureDisplay->selectedRegion, {0, 0, source->width, source->height},
			           translated );
			textureDisplay->selectedRegion = floor( textureDisplay->selectedRegion );

			debugPrintln( "Selected Region: {}", textureDisplay->selectedRegion );

			renderer->color = 0x80000000;
			setTexture( renderer, 0, null );
			auto scaled =
			    translate( textureDisplay->selectedRegion * editor->textureScale, origin );
			addRenderCommandSingleQuad( renderer, scaled );
		}
		clip.clip();

		renderer->color = Color::White;
		if( imguiButton( "Assign" ) ) {
			auto sourceIndex   = indexof( editor->textureSources.items, *source );
			auto orientation   = editor->orientation;
			auto rect          = Rect< int32 >( textureDisplay->selectedRegion );
			auto textureRegion = texturePackAddUniqueTextureRegion( app, rect, source->id );

			StringView assignedNames[] = {editor->front, editor->left, editor->back,
			                              editor->right, editor->top,  editor->bottom};
			FOR( entry : assignedNames ) {
				entry.pop_back();
			}

			if( textureRegion ) {
				if( editor->bulkMode ) {
					FOR( textureMap : editor->textureMaps ) {
						if( !textureMap.expanded ) {
							continue;
						}
						FOR( frame : makeArrayView( textureMap.frames, textureMap.framesCount ) ) {
							frame.source = sourceIndex;
							for( auto i = 0; i < VF_Count; ++i ) {
								if( textureMap.bulkModeItems[i].selected ) {
									auto face = &frame.faces[i];
									if( auto prev =
									        getTexturePackRegion( editor, face->regionId ) ) {
										if( prev != textureRegion ) {
											--prev->referenceCount;
										}
									}
									face->regionId    = textureRegion->id;
									face->orientation = orientation;
									++textureRegion->referenceCount;
									frame.textureMapItems[i].text = assignedNames[i];
								}
							}
						}
					}
				} else {
					FOR( textureMap : editor->textureMaps ) {
						if( !textureMap.expanded ) {
							continue;
						}
						FOR( frame : makeArrayView( textureMap.frames, textureMap.framesCount ) ) {
							if( !frame.expanded ) {
								continue;
							}
							frame.source = sourceIndex;
							for( auto i = 0; i < VF_Count; ++i ) {
								auto item = &frame.textureMapItems[i];
								if( item->selected ) {
									auto face = &frame.faces[i];
									if( auto prev =
									        getTexturePackRegion( editor, face->regionId ) ) {
										if( prev != textureRegion ) {
											--prev->referenceCount;
										}
									}
									face->regionId    = textureRegion->id;
									face->orientation = orientation;
									++textureRegion->referenceCount;
									frame.textureMapItems[i].text = assignedNames[i];
								}
							}
						}
					}
				}
				// remove regions with negative or zero ref count
				for( auto it = editor->uniqueTextureRegions.begin();
				     it != editor->uniqueTextureRegions.end(); ) {
					if( it->referenceCount <= 0 ) {
						it = editor->uniqueTextureRegions.erase( it );
						continue;
					}
					++it;
				}
			}
		}
		imguiEndDropGroup();
	}

	if( source && editor->textureDisplay.selectedIndex >= 0 ) {
		auto rect = &editor->textureDisplay.selectedRegion;
		if( imguiBeginDropGroup( "Values", &editor->valuesExpanded ) ) {
			imguiEditbox( "Left", &rect->left );
			imguiEditbox( "Top", &rect->top );
			imguiEditbox( "Right", &rect->right );
			imguiEditbox( "Bottom", &rect->bottom );
			imguiEndDropGroup();
		}
		if( imguiBeginDropGroup( "Orientation", &editor->orientationExpanded ) ) {
			auto layout = imguiBeginColumn( 100 );
			imguiRadiobox( "Def", &editor->orientation, TexturePackOrientation::Def );
			imguiRadiobox( "Cw90", &editor->orientation, TexturePackOrientation::Cw90 );
			imguiRadiobox( "Ccw90", &editor->orientation, TexturePackOrientation::Ccw90 );
			imguiRadiobox( "Horizontal", &editor->orientation, TexturePackOrientation::Horizontal );
			imguiRadiobox( "Vertical", &editor->orientation, TexturePackOrientation::Vertical );
			imguiRadiobox( "Diagonal", &editor->orientation, TexturePackOrientation::Diagonal );

			imguiNextColumn( &layout, 100 );
			auto rect = imguiRegion( 100, 100 );
			auto renderer = imguiRenderer();
			renderer->color = Color::White;
			setTexture( renderer, 0, source->id );
			auto normalized = scale( textureDisplay->selectedRegion, 1.0f / source->width,
			                         1.0f / source->height );
			auto texCoords = texturePackGetTexCoords( normalized, editor->orientation );
			addRenderCommandSingleQuad( renderer, rect, 0, texCoords );

			imguiEndColumn( &layout );
			imguiEndDropGroup();
		}
	}
	if( imguiBeginDropGroup( "Offset", &editor->offsetExpanded ) ) {
		auto firstExpanded = find_first_where( editor->textureMaps, entry.expanded );
		if( firstExpanded ) {
			auto frames = makeArrayView( firstExpanded->frames, firstExpanded->framesCount );
			if( auto frame = find_first_where( frames, entry.expanded ) ) {
				auto face = &frame->faces[VF_Front];
				auto region = getTexturePackRegion( editor, face->regionId );
				TextureMapEntry* info = nullptr;
				if( region && ( info = getTextureInfo( region->texture ) ) != nullptr ) {
					imguiEditbox( "offset x", &frame->offset.x );
					imguiEditbox( "offset y", &frame->offset.y );

					auto scale      = editor->textureScale;
					auto rect       = imguiRegion( 64 * scale, 64 * scale );
					auto w          = (float)width( region->rect );
					auto h          = (float)height( region->rect );
					auto renderer   = imguiRenderer();
					renderer->color = Color::White;
					setTexture( renderer, 0, info->id );
					auto normalized =
					    ::scale( region->rect, 1.0f / info->width, 1.0f / info->height );
					auto texCoords = texturePackGetTexCoords( normalized, editor->orientation );
					/*auto inner     = rect;
					inner.left += w * scale;
					inner.bottom -= h * scale;
					addRenderCommandSingleQuad( renderer, inner, 0, texCoords );*/

					auto halfW = 32.0f;
					auto halfH = 32.0f;
					rectf pointRect = {};
					imguiPoint( &frame->offset, {halfW, halfH, -halfW, -halfH}, rect,
					            {0, -h * scale, w * scale, 0}, &pointRect );
					frame->offset = round( frame->offset );

					// preview
					setTexture( renderer, 0, info->id );
					addRenderCommandSingleQuad( renderer, pointRect, 0, texCoords );

					setTexture( renderer, 0, null );
					LINE_MESH_STREAM_BLOCK( stream, renderer ) {
						stream->color = 0x80FFFFFF;
						auto c        = center( rect );
						pushLine( stream, {rect.left, c.y}, {rect.right, c.y} );
						pushLine( stream, {c.x, rect.top}, {c.x, rect.bottom} );
					}
				}
			}
		}

		imguiEndDropGroup();
	}

	// render debug cells
	/*setTexture( renderer, 0, null );
	Color colors[] = {0x80FF0000, 0x8000FF00, 0x800000FF};
	stream->color = Color::White;
	MESH_STREAM_BLOCK( stream, renderer ) {
		auto index = 0;
		FOR( entry : source->regions ) {
			stream->color = colors[index % 3];
			pushQuad( stream, translate( entry * editor->textureScale, origin ) );
			++index;
		}
	}*/
	return false;
}

TextureId texturePackAddTextureSource( AppData* app, StringView filename )
{
	auto editor         = &app->texturePackState;
	auto textureSources = &editor->textureSources;
	if( !textureSources->items.remaining() ) {
		return {};
	}

	auto item = textureSources->items.emplace_back();
	auto text = makeUninitializedArrayView( item->textStorage );
	copyToString( "Unnamed", &text );
	item->textLength  = text.size();
	item->lb.text     = text;
	item->lb.selected = false;
	item->id          = app->platform.loadTexture( filename );
	if( item->id ) {
		auto info         = getTextureInfo( item->id );
		item->width       = info->width;
		item->height      = info->height;
		item->regions.min = editor->textureRegions.size();
		auto slice        = makeUninitializedArrayView( editor->textureRegions.end(),
		                                         editor->textureRegions.remaining() );
		imageFindRects( slice, info->image, {}, 0, false );
		item->regions.max = item->regions.min + slice.size();
		editor->textureRegions.resize( editor->textureRegions.size() + slice.size() );
		return item->id;
	} else {
		textureSources->items.pop_back();
	}
	return {};
}

TexturePackSource* doTexturePackSources( AppData* app )
{
	auto editor   = &app->texturePackState;

	imguiText( "Texture Sources" );
	auto textureSources = &editor->textureSources;
	auto getter = []( TexturePackSource& source ) -> ImGuiListboxItem& { return source.lb; };
	auto items = makeArrayView( textureSources->items );
	auto selectedIndex =
	    imguiListboxIntrusive( &textureSources->scrollPos, {items, getter}, 150, 150, false );
	if( selectedIndex >= 0 ) {
		textureSources->lastSelected = selectedIndex;
	}
	if( textureSources->lastSelected >= 0 && textureSources->items.size() ) {
		if( !textureSources->items[textureSources->lastSelected].lb.selected ) {
			textureSources->lastSelected = -1;
		}
	}
	imguiSameLine( 2 );
	if( imguiButton( "Add", 70 ) ) {
		if( textureSources->items.remaining() ) {
			char filename[260];
			auto len = app->platform.getOpenFilename( nullptr, "Data/Images", false, filename,
													  countof( filename ) );
			if( len ) {
				texturePackAddTextureSource( app, {filename, len} );
			}
		}
	}
	TexturePackSource* lastSelected = nullptr;
	if( textureSources->lastSelected >= 0 && textureSources->items.size() ) {
		lastSelected = &textureSources->items[textureSources->lastSelected];
	}
	if( imguiButton( "Remove", 70 ) && lastSelected ) {
		auto removeSource = &set_variant( editor->removeVariant, removeSource );
		removeSource->sourceIndex = textureSources->lastSelected;
		imguiShowModal( editor->removeConfirm );
	}

	if( lastSelected ) {
		if( imguiEditbox( "Name", lastSelected->textStorage, &lastSelected->textLength,
					  countof( lastSelected->textStorage ) ) ) {
			lastSelected->lb.text = {lastSelected->textStorage, lastSelected->textLength};
		}
	}

	return lastSelected;
}

void doTexturePackEntries( AppData* app )
{
	auto editor = &app->texturePackState;
	auto textureMaps = &editor->textureMaps;
	imguiText( "Texture Maps" );
	const ImGuiListboxItem entries[] = {
	    {editor->front}, {editor->left}, {editor->back},
	    {editor->right}, {editor->top},  {editor->bottom},
	};
	if( imguiButton( "Add#1" ) ) {
		if( textureMaps->remaining() ) {
			auto entry = textureMaps->emplace_back();
			*entry = {};
			entry->expanded = false;
			entry->textLength =
			    copyToString( "Unnamed", entry->textStorage, countof( entry->textStorage ) );
			copy( entry->bulkModeItems, entries, countof( entries ) );
		}
	}

	imguiCheckbox( "Bulk Mode", &editor->bulkMode );

	auto regionState = imguiBeginScrollableRegion( &editor->textureMapsGuiRegion, 150, 400 );
	FOR( entry : *textureMaps ) {
		if( imguiBeginDropGroup( {entry.textStorage, entry.textLength}, &entry.expanded ) ) {
			imguiEditbox( "Name", entry.textStorage, &entry.textLength,
						  countof( entry.textStorage ) );
			if( editor->bulkMode ) {
				imguiListbox( &entry.bulkScrollPos, makeArrayView( entry.bulkModeItems ), 150,
							  6 * stringHeight( ImGui->font ), true );
				if( imguiButton( "Remove Entry" ) ) {
					auto removeEntry        = &set_variant( editor->removeVariant, removeEntry );
					removeEntry->entryIndex = indexof( *textureMaps, entry );
					imguiShowModal( editor->removeConfirm );
				}
			} else {
				imguiSameLine( 2 );
				if( imguiButton( "Add Frame", 50 ) ) {
					if( entry.framesCount < countof( entry.frames ) ) {
						auto frame = &entry.frames[entry.framesCount++];
						copy( frame->textureMapItems, entries, countof( entries ) );
					}
				}
				if( imguiButton( "Remove Entry", 50 ) ) {
					auto removeEntry        = &set_variant( editor->removeVariant, removeEntry );
					removeEntry->entryIndex = indexof( *textureMaps, entry );
					imguiShowModal( editor->removeConfirm );
				}
				auto framesView = makeInitializedArrayView( entry.frames, entry.framesCount,
															countof( entry.frames ) );
				for( auto i = 0; i < entry.framesCount; ++i ) {
					auto frame       = &entry.frames[i];
					auto indexString = toNumberString( i );
					if( imguiBeginDropGroup( indexString, &frame->expanded ) ) {
						auto selected = imguiListbox( &frame->scrollPos,
						                              makeArrayView( frame->textureMapItems ), 150,
						                              6 * stringHeight( ImGui->font ), true );
						if( selected >= 0 ) {
							auto face           = &frame->faces[selected];
							auto regionId       = face->regionId;
							editor->orientation = face->orientation;
							if( auto region = getTexturePackRegion( editor, regionId ) ) {
								auto textureDisplay            = &editor->textureDisplay;
								textureDisplay->selectedIndex  = 0;
								textureDisplay->selectedRegion = Rect< float >( region->rect );
							}
						}
						if( imguiButton( "Remove Frame" ) ) {
							auto removeFrame = &set_variant( editor->removeVariant, removeFrame );
							removeFrame->entryIndex = indexof( *textureMaps, entry );
							removeFrame->frameIndex = i;
							imguiShowModal( editor->removeConfirm );
						}
						imguiEndDropGroup();
					}
				}
				entry.framesCount = framesView.size();
			}
			imguiEndDropGroup();
		}
	}
	imguiEndScrollableRegion( &editor->textureMapsGuiRegion, &regionState );
}

void doBinPacking( AppData* app, StringView outputTextureFilename )
{
	auto editor     = &app->texturePackState;
	auto allocator  = &app->stackAllocator;
	auto cellBorder = editor->cellBorder;

	TEMPORARY_MEMORY_BLOCK( allocator ) {
		auto dimsCount = editor->uniqueTextureRegions.size();
		auto dims      = allocateArray( allocator, BinPackBatchDim, dimsCount );
		// fill in input structure for max rects algorithm
		for( auto i = 0; i < dimsCount; ++i ) {
			auto dest        = &dims[i];
			auto src         = &editor->uniqueTextureRegions[i];
			dest->dim.width  = width( src->rect ) + cellBorder;
			dest->dim.height = height( src->rect ) + cellBorder;
			dest->userData   = toPtr( src->id );
		}
		auto results   = allocateArray( allocator, BinPackBatchResult, dimsCount );

		MaxRectsFreeRectChoiceHeuristic bestHeuristic = MaxRectsBestShortSideFit;
		int32 bestTextureSize                         = 0;
		float bestFillingRatio                        = 0;

		struct PackBinsResult {
			int32 count;
			int32 usedArea;
			int32 textureSize;
			recti bounds;
		};
		auto packBins = [&]( MaxRectsFreeRectChoiceHeuristic heuristic,
		                     int32 initialTextureSize = 1 ) {
			int32 count        = 0;
			uint32 textureSize = (uint32)initialTextureSize;
			int32 usedArea     = 0;
			TEMPORARY_MEMORY_BLOCK( allocator ) {
				auto rectsCount =
				    safe_truncate< int32 >( getCapacityFor< regioni >( allocator ) / 2 );
				auto freeRects = allocateArray( allocator, regioni, rectsCount );
				auto usedRects = allocateArray( allocator, regioni, rectsCount );

				for( ;; ) {
					// try increasing powers of two until all rects are successfully packed
					auto bins = binPackCreateStatic( (int32)textureSize, (int32)textureSize,
					                                 freeRects, rectsCount, usedRects, rectsCount );
					count =
					    maxRectsInsertBatch( &bins, dims, results, dimsCount, heuristic, false );
					if( count == dimsCount ) {
						usedArea = bins.usedArea;
						break;
					}
					textureSize <<= 1;
				}
			}
			PackBinsResult result = {count, usedArea, (int32)textureSize};
			for( auto i = 0; i < count; ++i ) {
				auto entry    = &results[i];
				result.bounds = RectBounding( result.bounds, Rect( entry->result.rect ) );
			}
			return result;
		};

		auto findBestPacking = [&]( MaxRectsFreeRectChoiceHeuristic heuristic,
		                            uint32 textureSize ) {
			auto result       = packBins( heuristic, textureSize );
			auto area         = width( result.bounds ) * height( result.bounds );
			auto fillingRatio = result.usedArea / (float)area;
			if( fillingRatio > bestFillingRatio ) {
				bestHeuristic    = heuristic;
				bestTextureSize  = result.textureSize;
				bestFillingRatio = fillingRatio;
			}
			return result.textureSize;
		};

		for( auto i = 0; i < 5; ++i ) {
			auto heuristic = (MaxRectsFreeRectChoiceHeuristic)i;
			auto textureSize = findBestPacking( heuristic, 1 );
			findBestPacking( heuristic, textureSize << 1 );
			findBestPacking( heuristic, textureSize << 2 );
		}
		assert( bestTextureSize != 0 );
		auto result = packBins( bestHeuristic, bestTextureSize );

		// generate texture
		auto textureWidth  = width( result.bounds );
		auto textureHeight = height( result.bounds );
		// rgba texture
		auto texture = allocateArray( allocator, uint8, textureWidth * textureHeight * 4 );
		zeroMemory( texture, textureWidth * textureHeight * 4 );

		for( auto i = 0; i < result.count; ++i ) {
			auto entry       = &results[i];
			auto id          = fromPtr< int32 >( entry->userData );
			auto source      = getTexturePackRegion( editor, id );
			auto sourceInfo  = getTextureInfo( source->texture );
			auto sourceImage = sourceInfo->image;

			auto destRect = entry->result.rect;
			destRect.width -= cellBorder;
			destRect.height -= cellBorder;
			source->destRect = Rect( destRect );
			auto sourceRect  = source->rect;
			copyRegion( texture, Rect( destRect ), textureWidth * 4, 4, sourceImage.data,
			            sourceRect, sourceImage.width * 4 );
		}

		struct MemoryWriter {
			char* data;
			int32 size;
			int32 capacity;
		};
		MemoryWriter writer = {};
		writer.capacity     = safe_truncate< int32 >( remaining( allocator ) );
		writer.data         = allocateArray( allocator, char, remaining( allocator ) );

		auto memoryWriterCallback = []( void* context, void* data, int size ) {
			auto writer = (MemoryWriter*)context;
			auto len    = MIN( size, writer->capacity - writer->size );
			memcpy( writer->data + writer->size, data, len );
			writer->size += len;
		};

		stbi_write_png_to_func( memoryWriterCallback, &writer, textureWidth, textureHeight, 4,
		                        texture, textureWidth * 4 );
		GlobalPlatformServices->writeBufferToFile( outputTextureFilename, writer.data,
		                                           writer.size );
	}
}

void clear( TexturePackState* editor )
{
	editor->uniqueTextureRegions.clear();
	editor->textureMaps.clear();
	editor->textureRegions.clear();
	editor->textureScale                = 1;
	editor->valuesExpanded              = false;
	editor->bulkMode                    = false;
	editor->orientationExpanded         = false;
	editor->orientation                 = {};
	editor->textureSources.lastSelected = -1;

	FOR( item : editor->textureSources.items ) {
		GlobalPlatformServices->deleteTexture( item.id );
	}
	editor->textureSources.items.clear();
}

void texturePackLoad( AppData* app )
{
	assert( app );

	auto editor = &app->texturePackState;

	char filenameBuffer[260];
	auto filenameLength = GlobalPlatformServices->getOpenFilename(
	    "json\0*.json\0", "Data/Images", false, filenameBuffer, countof( filenameBuffer ) );
	if( !filenameLength ) {
		return;
	}
	StringView filename = {filenameBuffer, filenameLength};
	const ImGuiListboxItem entries[] = {
	    {editor->front}, {editor->left}, {editor->back},
	    {editor->right}, {editor->top},  {editor->bottom},
	};

	auto allocator = &app->stackAllocator;
	TEMPORARY_MEMORY_BLOCK( allocator ) {
		int32 jsonMaxSize = megabytes( 1 );
		auto jsonData = allocateArray( allocator, char, jsonMaxSize );

		auto jsonSize = GlobalPlatformServices->readFileToBuffer( filename, jsonData, jsonMaxSize );
		if( !jsonSize ) {
			return;
		}

		JsonStackAllocator jsonAlloc = {allocateArray( allocator, char, jsonMaxSize ), 0,
		                                (size_t)jsonMaxSize};
		auto doc = jsonMakeDocument( &jsonAlloc, jsonData, (int32)jsonSize, JSON_READER_STRICT );
		if( !doc ) {
			return;
		}
		auto root = doc.root;
		clear( editor );

		auto texture = texturePackAddTextureSource( app, root["texture"].getString() );
		if( !texture ) {
			return;
		}

		auto mapping = root["mapping"].getArray();
		FOR( entry : mapping ) {
			if( !editor->textureMaps.remaining() ) {
				break;
			}
			auto dest = editor->textureMaps.emplace_back();
			*dest = {};

			auto animation = entry.getObject();
			auto frames = animation["frames"].getArray();

			dest->textLength = copyToString( animation["name"].getString(), dest->textStorage,
			                                 countof( dest->textStorage ) );
			dest->framesCount = min( frames.size(), countof( dest->frames ) );
			copy( dest->bulkModeItems, entries, countof( dest->bulkModeItems ) );

			for( auto i = 0; i < dest->framesCount; ++i ) {
				auto frame = frames[i].getObject();
				auto destFrame = &dest->frames[i];
				copy( destFrame->textureMapItems, entries, countof( destFrame->textureMapItems ) );

				for( auto j = 0; j < VF_Count; ++j ) {
					auto face = frame[VoxelFaceStrings[j]].getObject();
					if( !face ) {
						continue;
					}
					destFrame->textureMapItems[j].text.pop_back();

					auto rectObject = face["rect"].getObject();
					recti rect      = {
					    rectObject["left"].getInt(), rectObject["top"].getInt(),
					    rectObject["right"].getInt(), rectObject["bottom"].getInt(),
					};
					auto textureRegion = texturePackAddUniqueTextureRegion( app, rect, texture );
					++textureRegion->referenceCount;
					destFrame->faces[j].regionId = textureRegion->id;
					destFrame->faces[j].orientation =
					    (TexturePackOrientation)clamp( face["orientation"].getInt(), 0, 5 );
				}
				serialize( frame["offset"], destFrame->offset );
			}
		}
	}
}

void texturePackSave( AppData* app )
{
	auto editor = &app->texturePackState;

	auto filename = getSaveFilename( nullptr, "Data/Images" );
	if( !filename.size() ) {
		return;
	}

	auto allocator = &app->stackAllocator;
	TEMPORARY_MEMORY_BLOCK( allocator ) {
		auto filenameStub    = getFilenameAndPathWithoutExtension( filename );
		auto textureFilename = snprint( allocator, "{}.png", filenameStub );
		auto jsonFilename    = snprint( allocator, "{}.json", filenameStub );
		auto voxelsFilename  = snprint( allocator, "{}.raw", filenameStub );

		doBinPacking( app, textureFilename );

		// output json
		auto bufferSize                 = safe_truncate< int32 >( remaining( allocator ) );
		auto buffer                     = allocateArray( allocator, char, bufferSize );
		auto writer = makeJsonWriter( buffer, bufferSize );
		writer.builder.format.precision = 9;

		writeStartObject( &writer );
		// TODO: write the relative path instead
		writeProperty( &writer, "texture", textureFilename );
		writeProperty( &writer, "voxels", voxelsFilename );
		writePropertyName( &writer, "mapping" );
		writeStartArray( &writer );
		FOR( textureMap : editor->textureMaps ) {
			writeStartObject( &writer );
				auto name = StringView{textureMap.textStorage, textureMap.textLength};
				writeProperty( &writer, "name", name );
				writePropertyName( &writer, "frames" );
				writeStartArray( &writer );
					FOR( frame : makeArrayView( textureMap.frames, textureMap.framesCount ) ) {
						writeStartObject( &writer );
						for( auto i = 0; i < VF_Count; ++i ) {
							writePropertyName( &writer, VoxelFaceStrings[i] );

					        auto face          = &frame.faces[i];
					        auto id            = face->regionId;
					        auto textureRegion = getTexturePackRegion( editor, id );
					        if( textureRegion ) {
								writeStartObject( &writer );
						        // TODO: do we normalize textures before saving them to file?
#if 0
						        auto info          = getTextureInfo( textureRegion->texture );
						        auto rect = scale( textureRegion->destRect, 1.0f / info->width,
						                           1.0f / info->height );
#else
						        auto rect = Rect< float >( textureRegion->destRect );
#endif
						        auto texCoords = texturePackGetTexCoords( rect, face->orientation );
						        writePropertyName( &writer, "texCoords" );
						        writeStartArray( &writer );
						        	FOR( element : texCoords.elements ) {
						        		writeValue( &writer, element );
						        	}
						        writeEndArray( &writer );

						        writeProperty( &writer, "orientation", (int32)face->orientation );
						        writeProperty( &writer, "rect", textureRegion->destRect );

						        writeEndObject( &writer );
					        } else {
					        	writeValue( &writer, null );
					        }
				        }
				        writeProperty( &writer, "offset", frame.offset );
				        writeEndObject( &writer );
					}
				writeEndArray( &writer );
			writeEndObject( &writer );
		}
		writeEndArray( &writer );
		writeEndObject( &writer );

		app->platform.writeBufferToFile( jsonFilename, buffer, writer.builder.size() );
	}
}

void doTexturePack( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto renderer = &app->renderer;
	auto font     = &app->font;
	auto editor   = &app->texturePackState;
	auto gui      = &editor->gui;

	if( !focus ) {
		return;
	}

	debugPrintln( "Unique rects: {}", editor->uniqueTextureRegions.size() );

	inputs->disableEscapeForQuickExit = true;
	imguiBind( gui );
	if( !editor->initialized ) {
		*gui = defaultImmediateModeGui();
		imguiLoadDefaultStyle( gui, &app->platform );

		editor->removeConfirm = imguiGenerateContainer( gui );
		imguiGetContainer( gui, editor->removeConfirm )->setHidden( true );

		auto allocator                       = &app->stackAllocator;
		editor->textureRegions               = makeUArray( allocator, recti, 100 );
		editor->textureSources.items         = makeUArray( allocator, TexturePackSource, 10 );
		editor->textureMaps                  = makeUArray( allocator, TexturePackEntry, 100 );
		editor->uniqueTextureRegions         = makeUArray( allocator, TexturePackRegion, 200 );
		editor->textureDisplay.selectedIndex = -1;

		auto pool      = makeStringPool( editor->stringPool, countof( editor->stringPool ) );
		editor->front  = pushString( &pool, "Front*" );
		editor->left   = pushString( &pool, "Left*" );
		editor->back   = pushString( &pool, "Back*" );
		editor->right  = pushString( &pool, "Right*" );
		editor->top    = pushString( &pool, "Top*" );
		editor->bottom = pushString( &pool, "Bottom*" );

		editor->initialized = true;
	}
	setProjection( renderer, ProjectionType::Orthogonal );
	setRenderState( renderer, RenderStateType::DepthTest, false );
	renderer->color      = Color::White;
	renderer->clearColor = 0xFF1B2B34;

	rectf guiBounds = {0, 0, app->width, app->height};
	imguiBind( gui, renderer, font, inputs, app->stackAllocator.ptr, guiBounds );

	auto layout = imguiBeginColumn( 150 );

	auto lastSelected = doTexturePackSources( app );

	doTexturePackEntries( app );

	imguiNextColumn( &layout, 800 );
	if( doTextureDisplay( app, lastSelected ) ) {
	}

	imguiNextColumn( &layout, 400 );
	if( imguiButton( "Load" ) ) {
		texturePackLoad( app );
	}
	if( imguiButton( "Save" ) ) {
		texturePackSave( app );
	}

	imguiEditbox( "Border", &editor->cellBorder );

	if( imguiDialog( "Remove", editor->removeConfirm ) ) {
		auto variant = &editor->removeVariant;
		switch( variant->type ) {
			case TexturePackRemoveVariant::type_removeFrame: {
				imguiText( "Frame will be removed." );
				imguiSameLine( 2 );
				if( imguiButton( "OK" ) || isKeyPressed( imguiInputs(), KC_Return ) ) {
					auto removeFrame = &get_variant( *variant, removeFrame );
					auto entry = &editor->textureMaps[removeFrame->entryIndex];
					entry->framesCount =
						erase_index( entry->frames, entry->framesCount, removeFrame->frameIndex );
					imguiClose( editor->removeConfirm );
				}
				break;
			}
			case TexturePackRemoveVariant::type_removeEntry: {
				imguiText( "Entry will be removed." );
				imguiSameLine( 2 );
				if( imguiButton( "OK" ) || isKeyPressed( imguiInputs(), KC_Return ) ) {
					auto removeEntry = &get_variant( *variant, removeEntry );
					erase_index( editor->textureMaps, removeEntry->entryIndex );
					imguiClose( editor->removeConfirm );
				}
				break;
			}
			case TexturePackRemoveVariant::type_removeSource: {
				imguiText( "Texture source will be removed." );
				imguiSameLine( 2 );
				if( imguiButton( "OK" ) || isKeyPressed( imguiInputs(), KC_Return ) ) {
					auto removeSource = &get_variant( *variant, removeSource );
					auto textureSources = &editor->textureSources;
					auto entry = &textureSources->items[removeSource->sourceIndex];
					app->platform.deleteTexture( entry->id );
					auto range = entry->regions;
					auto rangeLength = length( range );
					erase_range( editor->textureRegions, range );
					FOR( other : textureSources->items ) {
						if( other.regions.min > entry->regions.min ) {
							other.regions = offsetBy( other.regions, -rangeLength );
						}
					}
					entry = textureSources->items.erase( entry );
					if( entry == textureSources->items.end() ) {
						if( textureSources->items.size() ) {
							entry                        = &textureSources->items.back();
							entry->lb.selected           = true;
							textureSources->lastSelected = indexof( textureSources->items, *entry );
						} else {
							entry                        = nullptr;
							textureSources->lastSelected = -1;
						}
					} else {
						textureSources->lastSelected = indexof( textureSources->items, *entry );
					}
					imguiClose( editor->removeConfirm );
				}
				break;
			}
			InvalidDefaultCase;
		}
		if( imguiButton( "Cancel" ) || isKeyPressed( imguiInputs(), KC_Escape ) ) {
			imguiClose( editor->removeConfirm );
		}
	}
	imguiUpdate( dt );
	imguiFinalize();
}