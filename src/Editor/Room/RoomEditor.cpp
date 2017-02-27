namespace RoomEditor
{

const float MinScale = 0;
const float MaxScale = 1;

void clearIntermediateRoom( State* editor )
{
	FOR( layer : editor->intermediateRoom.layers ) {
		zeroMemory( layer.data, MaxTiles );
	}
}

recti getSelection( State* editor )
{
	auto result = correct( editor->selectionStart, editor->selectionEnd );
	++result.right;
	++result.bottom;
	return result;
}

void resizeRoom( State* editor, int32 width, int32 height )
{
	auto view = &editor->view;
	width     = min( width, MaxWidth );
	height    = min( height, MaxHeight );
	for( int32 i : rangei{0, RL_Count} ) {
		view->room.layers[i].width  = width;
		view->room.layers[i].height = height;

		editor->intermediateRoom.layers[i].width  = width;
		editor->intermediateRoom.layers[i].height = height;
	}
}

void doSidebar( AppData* app, GameInputs* inputs, rectfarg rect )
{
	auto editor   = &app->roomEditorState;
	auto view     = &editor->view;
	auto renderer = &app->renderer;

	setTexture( renderer, 0, null );
	renderer->color = 0xFF29373B;
	addRenderCommandSingleQuad( renderer, rect );

	renderer->color = Color::White;
	if( imguiBeginDropGroup( "Tools", &editor->flags, State::ToolsExpanded ) ) {
		imguiSameLine( 2 );
		if( imguiPushButton( "Select", editor->mouseMode == MouseMode::Select, imguiRatio() ) ) {
			editor->mouseMode = MouseMode::Place;
		}
		if( imguiPushButton( "Place", editor->mouseMode == MouseMode::Place, imguiRatio() ) ) {
			editor->mouseMode = MouseMode::Place;
		}
		imguiSameLine( 1 );
		if( imguiPushButton( "Place Rectangular", editor->mouseMode == MouseMode::PlaceRectangular,
		                     imguiRatio( 0.5f, 0 ) ) ) {
			editor->mouseMode = MouseMode::PlaceRectangular;
		}

		auto listboxSize = imguiSize( imgui::Ratio{1}, imgui::Absolute{50} );
		imguiListbox( &editor->layersScrollPos, makeArrayView( view->layers ), listboxSize );
		imguiEndDropGroup();
	}

	if( imguiBeginDropGroup( "Tileset", &editor->flags, State::TilesetExpanded ) ) {
		imguiText( "Tiles" );
		auto listboxSize = imguiSize( imgui::Ratio{1}, imgui::Absolute{100} );
		if( auto tileSet = view->tileSet ) {
			ImGuiTextGetter< VoxelCollection::Animation > getter =
			    []( const auto& entry ) -> StringView { return entry.name; };
			auto listboxHandle = imguiMakeHandle( &editor->tilesScrollPos );
			int32 index        = view->placingTile.frames.min;
			auto listboxRect   = imguiAddItem( listboxSize.width, listboxSize.height );
			if( imguiListboxSingleSelect( listboxHandle, &editor->tilesScrollPos,
			                              tileSet->voxels.animations, &index, listboxRect, getter,
			                              false ) ) {
				view->placingTile.frames.min = auto_truncate( index );
				view->placingTile.frames.max = view->placingTile.frames.min + 1;
			}
		}

		listboxSize                             = imguiSize( imgui::Ratio{1}, imgui::Absolute{60} );
		static ImGuiListboxItem RotationNames[] = {{"0"}, {"90"}, {"180"}, {"270"}};
		auto names                                  = makeArrayView( RotationNames );
		FOR( entry : names ) {
			entry.selected = false;
		}
		names[view->placingTile.rotation].selected = true;
		auto index = imguiListbox( &editor->rotationScrollPos, names, listboxSize, false );
		if( index >= 0 ) {
			view->placingTile.rotation = auto_truncate( index );
		}
		imguiEndDropGroup();
	}

	if( imguiBeginDropGroup( "Properties", &editor->flags, State::PropertiesExpanded ) ) {
		imguiSameLine( 2 );
		imguiText( "Background Type" );
		auto comboHandle = imguiMakeHandle( &editor->view.room.background );
		int32 bgValue    = valueof( editor->view.room.background );
		if( imguiCombo( comboHandle, &bgValue, makeArrayView( RoomBackgroundTypeNames ) ) ) {
			editor->view.room.background = (RoomBackgroundType)bgValue;
		}

		imguiText( "Room Size" );
		imguiSameLine( 2 );
		if( imguiEditbox( "Width", &view->room.layers[RL_Main].width ) ) {
			auto w = view->room.layers[RL_Main].width;
			w = min( w, MaxWidth );
			resizeRoom( editor, w, view->room.layers[RL_Main].height );
		}
		if( imguiEditbox( "Height", &view->room.layers[RL_Main].height ) ) {
			auto h = view->room.layers[RL_Main].height;
			h = min( h, MaxHeight );
			resizeRoom( editor, view->room.layers[RL_Main].width, h );
		}
		imguiEndDropGroup();
	}

#ifdef GAME_DEBUG
	if( imguiButton( "Play" ) ) {
		// copy room data into game room and switch focus
		auto gameRoom = &app->gameState.room;
		auto pool     = &editor->playTilePool;
		auto data     = pool->data;

		for( auto i = 0; i < RL_Count; ++i ) {
			auto src  = &view->room.layers[i];
			auto dest = &gameRoom->layers[i];

			dest->grid = makeGridView( data, src->width, src->height );
			for( auto y = 0; y < src->height; ++y ) {
				copy( dest->grid.ptr + y * src->width, src->data + y * MaxWidth, src->width );
			}
			data += src->width * src->height;
		}

		app->gameState.player->grounded            = {};
		app->gameState.player->wallslideCollidable = {};
		app->gameState.player->lastCollision       = {};
		setSpatialState( app->gameState.player, SpatialState::Airborne );
		app->focus = AppFocus::Game;
	}
#endif
}

void doMapping( AppData* app, GameInputs* inputs, rectfarg rect )
{
	using GameConstants::TileWidth;
	using GameConstants::TileHeight;
	using GameConstants::TileDepth;

	auto editor   = &app->roomEditorState;
	auto renderer = &app->renderer;
	auto view     = &editor->view;

#if 0
	setTexture( renderer, 0, null );
	renderer->color = 0xFF29373B;
	addRenderCommandSingleQuad( renderer, rect );
#endif

	auto handle = imguiMakeHandle( view );

	renderer->color = Color::White;
	setScissorRect( renderer, Rect< int32 >( rect ) );
	setRenderState( renderer, RenderStateType::Scissor, true );
	auto projection = matrixPerspectiveFovProjection( rect, app->width, app->height,
	                                                  degreesToRadians( 65 ), -1, 1 );
	auto viewMatrix =
	    matrixTranslation( Vec3( view->translation, lerp( view->scale, 100.0f, 400.0f ) ) );

	setProjectionMatrix( renderer, ProjectionType::Perspective, projection );
	setProjection( renderer, ProjectionType::Perspective );

	auto invViewProj = inverse( viewMatrix * projection );

	const auto roomWidth   = view->room.layers[RL_Main].width;
	const auto roomHeight  = view->room.layers[RL_Main].height;
	const rectf roomBounds = {0, 0, TileWidth * 16, TileHeight * 16};
	vec3 planeBase         = {0, 0, TileDepth};

	auto layers         = makeArrayView( view->layers );
	auto isOnlySelected = []( Array< ImGuiListboxItem > layers, int32 index ) {
		return layers[index].selected
		       && count_if( layers.begin(), layers.end(), []( const ImGuiListboxItem& item ) {
			          return item.selected;
			      } ) == 1;
	};

	if( isOnlySelected( layers, RL_Front ) ) {
		planeBase.z = 0;
	}
	if( isOnlySelected( layers, RL_Back ) ) {
		planeBase.z = TileDepth * 2;
	}

	if( view->flags & View::DrawGridInFront ) {
		planeBase.z -= TileDepth;
	}

	if( imguiIsHover( handle ) && isPointInside( rect, inputs->mouse.position ) ) {
		int8 button = 0;
		if( isKeyPressed( inputs, KC_LButton ) ) {
			button = KC_LButton;
		}
		if( isKeyPressed( inputs, KC_MButton ) ) {
			button = KC_MButton;
		}
		if( isKeyPressed( inputs, KC_RButton ) ) {
			button = KC_RButton;
		}

		if( button ) {
			imguiFocus( handle );
			imguiCapture( handle, button );
		}
	}

	// process mouse down
	if( imguiHasCapture( handle ) && imguiIsHover( handle ) && invViewProj ) {
		auto current = pointToWorldSpaceRay( invViewProj.matrix, inputs->mouse.position, app->width,
		                                     app->height );
		auto currentPosition = intersectionRayVsPlane( current, planeBase, {0, 0, -1} );

		vec2i gridPos;
		gridPos.x = (int32)floor( ( currentPosition.x - roomBounds.left ) / TileWidth );
		gridPos.y = (int32)floor( ( roomBounds.top - currentPosition.y ) / TileHeight );

		if( hasMagnitude( inputs->mouse.delta ) && isKeyDown( inputs, KC_MButton ) ) {
			auto prev = pointToWorldSpaceRay( invViewProj.matrix, inputs->mouse.prev, app->width,
			                                  app->height );
			auto prevPosition = intersectionRayVsPlane( prev, planeBase, {0, 0, -1} );

			auto delta = currentPosition - prevPosition;

			view->translation.x += delta.x;
			view->translation.y += delta.y;
		}

		auto placeTiles = [&]( View::Room* room, GameTile tile ) {
			if( room ) {
				clearIntermediateRoom( editor );
				auto selection = getSelection( editor );
				for( auto i = 0; i < RL_Count; ++i ) {
					if( view->layers[i].selected ) {
						auto& grid = room->layers[i];
						for( auto y = selection.top; y < selection.bottom; ++y ) {
							for( auto x = selection.left; x < selection.right; ++x ) {

								if( grid.isInBounds( x, y ) ) {
									grid.at( x, y ) = tile;
								}
							}
						}
					}
				}
			}
		};
		auto processMouse = [&]( uint8 key, GameTile pressedTile, GameTile releasedTile ) {
			View::Room* room = nullptr;
			auto tile        = InvalidGameTile;
			if( editor->mouseMode == MouseMode::PlaceRectangular ) {
				if( isKeyDown( inputs, key ) ) {
					if( isKeyPressed( inputs, key ) ) {
						editor->selectionStart = gridPos;
					}
					editor->selectionEnd = gridPos;
					room                 = &editor->intermediateRoom;
					tile                 = pressedTile;
				} else if( isKeyReleased( inputs, key ) && imguiHasFocus( handle ) ) {
					editor->selectionEnd = gridPos;
					room                 = &view->room;
					tile                 = releasedTile;
				}
			} else if( isKeyDown( inputs, key ) ) {
				editor->selectionStart = gridPos;
				editor->selectionEnd   = gridPos;
				tile                   = releasedTile;
				room                   = &view->room;
			}

			placeTiles( room, tile );
		};

		processMouse( KC_RButton, InvalidGameTile, {} );
		processMouse( KC_LButton, view->placingTile, view->placingTile );
	}

	// process mouse wheel
	if( ( isPointInside( rect, inputs->mouse.position ) || imguiHasFocus( handle ) )
	    && !floatEqZero( inputs->mouse.wheel ) ) {

		view->scale = clamp( view->scale - inputs->mouse.wheel * 0.1f, MinScale, MaxScale );
	}

	renderer->view = viewMatrix;

	static const mat4 Rotations[] = {
	    matrixIdentity(), matrixRotationZOrigin( HalfPi32, 8, 8 ),
	    matrixRotationZOrigin( Pi32, 8, 8 ), matrixRotationZOrigin( Pi32 + HalfPi32, 8, 8 ),
	};

	setRenderState( renderer, RenderStateType::DepthTest, true );
	{
		// render tiles
		auto matrixStack = renderer->matrixStack;
		auto tileWidth             = TileWidth;
		auto tileHeight            = TileHeight;
		const float zTranslation[] = {0, TILE_DEPTH, -TILE_DEPTH};
		static_assert( countof( zTranslation ) == RL_Count, "" );
		pushMatrix( matrixStack );
		for( auto i = 0; i < RL_Count; ++i ) {
			if( ( view->flags & View::DrawSelectedLayersOnly ) && !layers[i].selected ) {
				continue;
			}
			auto intermediate = &editor->intermediateRoom.layers[i];
			auto grid         = &view->room.layers[i];
			auto tileSet      = &app->gameState.tileSet;
			setTexture( renderer, 0, tileSet->voxels.texture );
			for( auto y = 0; y < grid->height; ++y ) {
				for( auto x = 0; x < grid->width; ++x ) {
					auto intermediateTile = intermediate->at( x, y );
					auto existing         = grid->at( x, y );
					auto tile = ( intermediateTile ) ? ( intermediateTile ) : ( existing );
					if( tile && compare( &tile, &InvalidGameTile, 1 ) != 0 ) {
						pushMatrix( matrixStack );
						translate( matrixStack, x * tileWidth, -y * tileHeight - tileHeight,
						           zTranslation[i] );
						assert( tile.rotation < countof( Rotations ) );
						multMatrix( matrixStack, Rotations[tile.rotation] );
						auto entry = &tileSet->voxels.frames[tile.frames.min];
						addRenderCommandMesh( renderer, entry->mesh );
						popMatrix( matrixStack );
					}
				}
			}
		}
		popMatrix( matrixStack );

		renderBackground( renderer, view->room.background );
	}

	// draw grid
	// setRenderState( renderer, RenderStateType::DepthTest, false );
	if( view->flags & View::DrawGrid ) {
		const float z = planeBase.z;
		LINE_MESH_STREAM_BLOCK( stream, renderer ) {
			stream->color = 0x80FFFFFF;
			for( auto i = 0; i <= roomHeight; ++i ) {
				// horizontal line
				pushLine( stream, {0, -i * TileHeight, z},
				          {roomWidth * TileWidth, -i * TileHeight, z} );
			}

			for( auto i = 0; i <= roomWidth; ++i ) {
				// vertical line
				pushLine( stream, {i * TileWidth, 0, z},
				          {i * TileWidth, -roomHeight * TileHeight, z} );
			}
		}
	}

	setProjection( renderer, ProjectionType::Orthogonal );
	setRenderState( renderer, RenderStateType::Scissor, false );
}

void doRoomEditor( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	if( !focus ) {
		return;
	}

	auto editor   = &app->roomEditorState;
	auto gui      = &editor->gui;
	auto renderer = &app->renderer;
	auto font     = &app->font;
	auto view     = &editor->view;

	// inputs->disableEscapeForQuickExit = true;

	imguiBind( gui );
	if( !editor->initialized ) {
		auto allocator = &app->stackAllocator;

		*gui = defaultImmediateModeGui();
		imguiLoadDefaultStyle( gui, &app->platform, font );

		editor->fileMenu = imguiGenerateContainer( gui, {}, ImGuiVisibility::Hidden );
		editor->viewMenu = imguiGenerateContainer( gui, {}, ImGuiVisibility::Hidden );

		{
			StringPool pool = {beginVector( allocator, char )};

			view->layers[RL_Main].text  = pushString( &pool, "Main" );
			view->layers[RL_Front].text = pushString( &pool, "Front" );
			view->layers[RL_Back].text  = pushString( &pool, "Back" );

			endVector( allocator, &pool.memory );
		}

#ifdef GAME_DEBUG
		view->tileSet = &app->gameState.tileSet;
		resizeRoom( editor, 16, 16 );
		view->placingTile = {0, 0, {0, 1}};
#endif

		editor->initialized = true;
	}

	renderer->color      = Color::White;
	renderer->clearColor = 0xFF1B2B34;

	rectf guiBounds = {0, 0, app->width, app->height};
	setProjection( renderer, ProjectionType::Orthogonal );
	setRenderState( renderer, RenderStateType::DepthTest, false );
	imguiBind( gui, renderer, font, inputs, app->stackAllocator.ptr, guiBounds );

	auto layout        = imguiBeginColumn( app->width );

	if( imguiMenu( true ) ) {
		auto itemHandle = imguiMakeHandle( editor );
		imguiMenuItem( itemHandle, "File", editor->fileMenu );
		imguiMenuItem( itemHandle, "Edit" );
		imguiMenuItem( itemHandle, "View", editor->viewMenu );
		imguiMenuItem( itemHandle, "Preferences" );
		imguiEndContainer();
	}

	imguiEndColumn( &layout );

	const float MenuLeft = 200;
	imguiNextColumn( &layout, MenuLeft );

	doSidebar( app, inputs, imguiPeekItem( MenuLeft ) );

	const float MappingClient = app->width - MenuLeft;
	imguiNextColumn( &layout, MappingClient );

	doMapping( app, inputs, imguiAddItem( MappingClient ) );

	imguiEndColumn( &layout );

	if( imguiContextMenu( editor->fileMenu ) ) {
		imguiContextMenuEntry( "New" );
		imguiContextMenuEntry( "Open" );
		imguiEndContainer();
	}

	if( imguiContextMenu( editor->viewMenu ) ) {
		imguiCheckboxFlag( "Grid", &editor->view.flags, View::DrawGrid );
		imguiCheckboxFlag( "Grid In Front", &editor->view.flags, View::DrawGridInFront );
		imguiCheckboxFlag( "Selected Layers Only", &editor->view.flags,
		                   View::DrawSelectedLayersOnly );
		imguiEndContainer();
	}

	imguiUpdate( dt );
	imguiFinalize();
}

} // namespace RoomEditor
