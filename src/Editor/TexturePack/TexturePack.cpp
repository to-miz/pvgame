/*
TODO:
	- finish up saving texture coordinates into VoxelGridTextureMap
	- implement saving/loading from file
	- add a scrollbar to the left side
	- implement texture packing of different sources into one single texture
*/

bool doTextureDisplay( AppData* app, TexturePackSource* source )
{
	assert( app );
	if( !source ) {
		return false;
	}

	auto renderer       = imguiRenderer();
	auto inputs         = imguiInputs();
	auto editor         = &app->texturePackState;
	auto textureDisplay = &editor->textureDisplay;
	rectf textureRect   = {0, 0, source->width * editor->textureScale,
						 source->height * editor->textureScale};
	auto rect = imguiScrollable( &textureDisplay->scrollPos, textureRect, 400, 400 );

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
			FOR( entry : source->regions ) {
				auto scaled = translate( entry * editor->textureScale, origin );
				if( isPointInside( scaled, inputs->mouse.position ) ) {
					addRenderCommandSingleQuad( renderer, scaled );

					if( isKeyPressed( inputs, KC_LButton ) ) {
						textureDisplay->selectedIndex  = indexof( source->regions, entry );
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
		if( editor->snapToPixels ) {
			textureDisplay->selectedRegion = floor( textureDisplay->selectedRegion );
		}

		debugPrintln( "{}", textureDisplay->selectedRegion );

		renderer->color = 0x80000000;
		setTexture( renderer, 0, null );
		auto scaled = translate( textureDisplay->selectedRegion * editor->textureScale, origin );
		addRenderCommandSingleQuad( renderer, scaled );
	}
	clip.clip();

	renderer->color = Color::White;
	if( imguiButton( "Assign" ) ) {
		auto normalized =
			scale( textureDisplay->selectedRegion, 1.0f / source->width, 1.0f / source->height );
		if( editor->bulkMode ) {
			FOR( textureMap : editor->textureMaps ) {
				FOR( frame : makeArrayView( textureMap.frames, textureMap.framesCount ) ) {
					for( auto i = 0; i < VF_Count; ++i ) {
						if( textureMap.bulkModeItems[i].selected ) {
							auto entry = &frame.textureMaps.entries[i];
							entry->texCoords = makeQuadTexCoords( normalized );
						}
					}
				}
			}
		} else {
			FOR( textureMap : editor->textureMaps ) {
				FOR( frame : makeArrayView( textureMap.frames, textureMap.framesCount ) ) {
					for( auto i = 0; i < VF_Count; ++i ) {
						auto item = &frame.textureMapItems[i];
						if( item->selected ) {
							auto entry = &frame.textureMaps.entries[i];
							entry->texCoords = makeQuadTexCoords( normalized );
						}
					}
				}
			}
		}
	}

	imguiSlider( "Texture Scale", &editor->textureScale, 1, 10 );
	imguiCheckbox( "Snap to Pixels", &editor->snapToPixels );

	if( editor->textureDisplay.selectedIndex >= 0 ) {
		auto rect = &editor->textureDisplay.selectedRegion;
		if( imguiBeginDropGroup( "Values", &editor->valuesExpanded ) ) {
			imguiEditbox( "Left", &rect->left );
			imguiEditbox( "Top", &rect->top );
			imguiEditbox( "Right", &rect->right );
			imguiEditbox( "Bottom", &rect->bottom );
			imguiEndDropGroup();
		}
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

TexturePackSource* doTexturePackSources( AppData* app )
{
	auto editor   = &app->texturePackState;

	imguiText( "Texture Sources" );
	auto textureSources = &editor->textureSources;
	auto selectedIndex  = imguiListboxIntrusive(
		&textureSources->scrollPos, textureSources->items.data(), sizeof( TexturePackSource ),
		textureSources->items.size(), 150, 150, false );
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
				auto item = textureSources->items.emplace_back();
				auto text = makeUninitializedArrayView( item->textStorage );
				copyToString( &text, "Unnamed" );
				item->textLength  = text.size();
				item->lb.text     = text;
				item->lb.selected = false;
				item->id          = app->platform.loadTexture( {filename, len} );
				if( item->id ) {
					auto info         = getTextureInfo( item->id );
					item->width       = info->width;
					item->height      = info->height;
					auto slice        = makeUninitializedArrayView( editor->textureRegions.end(),
															 editor->textureRegions.remaining() );
					imageFindRects( slice, info->image, {}, 0, false );
					item->regions = {slice.data(), slice.size()};
					editor->textureRegions.resize( editor->textureRegions.size() + slice.size() );
				} else {
					textureSources->items.pop_back();
				}
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
		{"Front"}, {"Left"}, {"Back"}, {"Right"}, {"Top"}, {"Bottom"},
	};
	if( imguiButton( "Add#1" ) ) {
		if( textureMaps->remaining() ) {
			auto entry = textureMaps->emplace_back();
			*entry = {};
			entry->expanded = false;
			entry->textLength =
				copyToString( entry->textStorage, countof( entry->textStorage ), "Unnamed" );
			copy( entry->bulkModeItems, entries, countof( entries ) );
		}
	}

	imguiCheckbox( "Bulk Mode", &editor->bulkMode );

	FOR( entry : *textureMaps ) {
		if( imguiBeginDropGroup( {entry.textStorage, entry.textLength}, &entry.expanded ) ) {
			imguiEditbox( "Name", entry.textStorage, &entry.textLength,
						  countof( entry.textStorage ) );
			if( editor->bulkMode ) {
				imguiListbox( &entry.bulkScrollPos, makeArrayView( entry.bulkModeItems ), 150,
							  6 * stringHeight( ImGui->font ), true );
			} else {
				if( imguiButton( "Add Frame" ) ) {
					if( entry.framesCount < countof( entry.frames ) ) {
						auto frame = &entry.frames[entry.framesCount++];
						copy( frame->textureMapItems, entries, countof( entries ) );
					}
				}
				auto framesView = makeInitializedArrayView( entry.frames, entry.framesCount,
															countof( entry.frames ) );
				for( auto i = 0; i < entry.framesCount; ++i ) {
					auto frame       = &entry.frames[i];
					auto indexString = toNumberString( i );
					if( imguiBeginDropGroup( indexString, &frame->expanded ) ) {
						imguiListbox( &frame->scrollPos, makeArrayView( frame->textureMapItems ), 150,
									  6 * stringHeight( ImGui->font ), true );
						if( imguiButton( "Remove Frame" ) ) {
							auto removeFrame = &set_variant( editor->removeVariant, removeFrame );
							removeFrame->entryIndex = indexof( *textureMaps, entry );
							removeFrame->frameIndex = i;
							imguiShowModal( editor->removeConfirm );
						}
					}
				}
				entry.framesCount = framesView.size();
				if( imguiButton( "Remove Entry" ) ) {
					auto removeEntry        = &set_variant( editor->removeVariant, removeEntry );
					removeEntry->entryIndex = indexof( *textureMaps, entry );
					imguiShowModal( editor->removeConfirm );
				}
			}
			imguiEndDropGroup();
		}
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
		editor->textureDisplay.selectedIndex = -1;
		editor->initialized                  = true;
	}
	setProjection( renderer, ProjectionType::Orthogonal );
	setRenderState( renderer, RenderStateType::DepthTest, false );
	renderer->color      = Color::White;
	renderer->clearColor = 0xFF1B2B34;

	rectf guiBounds = {0, 0, app->width, app->height};
	imguiBind( gui, renderer, font, inputs, app->stackAllocator.ptr, guiBounds );

	auto layout       = imguiBeginColumn( 150 );
	
	auto lastSelected = doTexturePackSources( app );

	doTexturePackEntries( app );

	imguiNextColumn( &layout, 400 );
	if( doTextureDisplay( app, lastSelected ) ) {
	}

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
					entry = textureSources->items.erase( entry );
					// TODO: update texureRegions pool so we do not "leak" region memory when
					// removing
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
		if( imguiButton( "Cancel" ) ) {
			imguiClose( editor->removeConfirm );
		}
	}
	imguiUpdate( dt );
	imguiFinalize();
}