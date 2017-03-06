namespace RoomEditor
{

const float MinScale     = 0;
const float MaxScale     = 1;
const char* const Filter = "Room\0*.room\0All\0*.*\0";

Room toRoom( TilesPool* pool, View* view )
{
	Room result    = {};
	result.tileSet = view->tileSet;
	auto data      = pool->data;

	for( auto i = 0; i < RL_Count; ++i ) {
		auto src  = &view->room.layers[i];
		auto dest = &result.layers[i];

		dest->grid = makeGridView( data, src->width, src->height );
		for( auto y = 0; y < src->height; ++y ) {
			copy( dest->grid.ptr + y * src->width, src->data + y * MaxWidth, src->width );
		}
		data += src->width * src->height;
	}
	return result;
}
Room makeRoom( TilesPool* pool, int32 width, int32 height )
{
	Room result = {};
	auto data   = pool->data;
	FOR( layer : result.layers ) {
		layer.grid = makeGridView( data, width, height );
		data += width * height;
	}
	return result;
}
void fromRoom( View* view, const Room& room )
{
	for( auto i = 0; i < RL_Count; ++i ) {
		auto src  = &room.layers[i].grid;
		auto dest = &view->room.layers[i];

		dest->width  = src->width;
		dest->height = src->height;
		for( auto y = 0; y < src->height; ++y ) {
			copy( dest->data + y * MaxWidth, src->data() + y * src->width, src->width );
		}
	}
	view->room.background = room.background;
}

void clear( View* view )
{
	FOR( layer : view->room.layers ) {
		zeroMemory( layer.data, countof( layer.data ) );
		layer.width  = 16;
		layer.height = 16;
	}
	view->room.background = {};
	view->filename.clear();
	view->flags &= ~View::UnsavedChanges;
	FOR( entity : view->entities ) {
		deleteSkeleton( entity.skeleton );
	}
	view->entities.clear();
}
void setUnsavedChanges( View* view )
{
	view->flags |= View::UnsavedChanges;
}

void save( State* editor, View* view, StringView filename )
{
	auto allocator = GlobalScrap;
	TEMPORARY_MEMORY_BLOCK( allocator ) {
		auto room = toRoom( &editor->tilePool, view );
		if( !room.tileSet ) {
			LOG( ERROR, "Can't save file {}: No tile set defined", filename );
			return;
		}
		auto writer = makeMemoryWriter( allocator );

		write( &writer, "ROOM" );
		writePascalString( &writer, room.tileSet->voxels.filename );
		write( &writer, &room.background, 1 );
		write( &writer, room.layers[RL_Main].grid.width );
		write( &writer, room.layers[RL_Main].grid.height );
		FOR( layer : room.layers ) {
			write( &writer, makeArrayView( layer.grid ) );
		}

		GlobalPlatformServices->writeBufferToFile( filename, writer.data(), writer.size() );
		view->filename = filename;
		view->flags &= ~View::UnsavedChanges;
	}
}
void load( State* editor, View* view, StringView filename )
{
	auto allocator = GlobalScrap;
	TEMPORARY_MEMORY_BLOCK( allocator ) {
		auto data = readFile( allocator, filename );
		if( data.size() ) {
			auto reader = makeMemoryReader( data.data(), data.size() );

			if( !read( &reader, "ROOM" ) ) {
				LOG( ERROR, "{}: Invalid room file", filename );
				return;
			}
			FilenameString voxelsFilename;
			readPascalString( &reader, voxelsFilename );
			auto background = read< RoomBackgroundType >( &reader );
			auto width      = read< int32 >( &reader );
			auto height     = read< int32 >( &reader );
			if( !width || !height ) {
				LOG( ERROR, "{}: Invalid room file", filename );
				return;
			}

			auto room = makeRoom( &editor->tilePool, width, height );
			FOR( layer : room.layers ) {
				if( !read( &reader, makeArrayView( layer.grid ) ) ) {
					LOG( ERROR, "{}: Invalid room file", filename );
					return;
				}
			}

			room.background = background;
			fromRoom( view, room );

			view->filename = filename;
		}
	}
}

FilenameString getSaveFilename()
{
	auto result = ::getSaveFilename( Filter, nullptr );
	if( result.size() && !equalsIgnoreCase( getFilenameExtension( result ), ".room" ) ) {
		result.append( ".room" );
	}
	return result;
}
bool menuSave( AppData* app )
{
	assert( app );
	auto editor = &app->roomEditorState;
	auto view   = &editor->view;

	if( !view->filename.size() ) {
		view->filename = getSaveFilename();
	}
	if( view->filename.size() ) {
		save( editor, view, view->filename );
	}
	return view->filename.size() != 0;
}
bool menuSaveAs( AppData* app )
{
	assert( app );
	auto editor = &app->roomEditorState;
	auto view   = &editor->view;

	auto filename = getSaveFilename();
	if( filename.size() ) {
		save( editor, view, filename );
	}
	return view->filename.size() != 0;
}
bool menuOpen( AppData* app )
{
	auto editor = &app->roomEditorState;
	auto view   = &editor->view;

	auto saveAndOpen = []( AppData* app ) {
		if( menuSave( app ) ) {
			menuOpen( app );
		}
	};

	auto open = []( AppData* app ) {
		auto view = &app->roomEditorState.view;
		view->flags &= ~View::UnsavedChanges;
		if( !menuOpen( app ) ) {
			view->flags |= View::UnsavedChanges;
		}
	};

	if( view->flags & View::UnsavedChanges ) {
		showMessageBox( &editor->messageBox, "Save Changes?", "UnsavedChanges", saveAndOpen, open );
	} else {
		auto filename = getOpenFilename( Filter, nullptr, false );
		if( filename.size() ) {
			load( editor, view, filename );
		}
	}
	return view->filename.size() != 0;
}

void updateIntermediateRoom( State* editor, View* view )
{
	for( int32 i : rangei{0, RL_Count} ) {
		editor->intermediateRoom.layers[i].width  = view->room.layers[i].width;
		editor->intermediateRoom.layers[i].height = view->room.layers[i].height;
	}
}
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
	setUnsavedChanges( view );
}

vec2 gridPosToEntityPos( vec2i gridPos )
{
	using namespace GameConstants;
	vec2 position = {gridPos.x * TileWidth + TileWidth * 0.5f, gridPos.y * TileHeight + TileHeight};
	return position;
}

void playRoom( AppData* app, View* view )
{
	auto editor = &app->roomEditorState;
	auto game   = &app->gameState;

	restartGame( game );
	// copy room data into game room and switch focus
	game->room                        = toRoom( &editor->tilePool, view );
	game->player->grounded            = {};
	game->player->wallslideCollidable = {};
	game->player->lastCollision       = {};
	setSpatialState( game->player, SpatialState::Airborne );

	FOR( entity : view->entities ) {
		auto position = gridPosToEntityPos( entity.position );
		auto handle   = addEntityHandle( &game->entityHandles );
		assert( entity.type != Entity::type_none );
		addEntity( &game->entitySystem, &game->skeletonSystem, handle, entity.type, position );
	}

	app->focus = AppFocus::Game;
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
			editor->mouseMode = MouseMode::Select;
		}
		if( imguiPushButton( "Place", editor->mouseMode == MouseMode::Place, imguiRatio() ) ) {
			editor->mouseMode = MouseMode::Place;
		}
		imguiSameLine( 2 );
		if( imguiPushButton( "Place Rectangular", editor->mouseMode == MouseMode::PlaceRectangular,
		                     imguiRatio() ) ) {
			editor->mouseMode = MouseMode::PlaceRectangular;
		}
		if( imguiPushButton( "Entity", editor->mouseMode == MouseMode::Entity, imguiRatio() ) ) {
			editor->mouseMode = MouseMode::Entity;
		}

		auto listboxSize = imguiSize( imgui::Ratio{1}, imgui::Absolute{50} );
		imguiListbox( &editor->layersScrollPos, makeArrayView( view->layers ), listboxSize );
		imguiEndDropGroup();
	}

	if( imguiBeginDropGroup( "Tileset", &editor->flags, State::TilesetExpanded ) ) {
		if( editor->mouseMode != MouseMode::Entity ) {
			imguiText( "Tiles" );
			auto tilesRectSize = imguiSize( imgui::Ratio{1}, imgui::Absolute{400} );
			auto scrollable    = imguiBeginScrollableRegion(
			    &editor->tilesScollableRegion, tilesRectSize.width, tilesRectSize.height );

			auto handle = imguiMakeHandle( &editor->tilesScollableRegion );
			renderer->color = Color::White;
			/*setTexture( renderer, 0, null );
			addRenderCommandSingleQuad( renderer, scrollable.inner );*/

			if( auto tileSet = view->tileSet ) {
				setTexture( renderer, 0, tileSet->voxels.texture );
				auto thumbRect = translate( rectf{0, 0, 32, 32}, scrollable.inner.leftTop );
				MESH_STREAM_BLOCK( stream, renderer ) {
					for( int32 i : rangei{0, tileSet->voxels.animations.size()} ) {
						auto& animation = tileSet->voxels.animations[i];
						if( animation.range ) {
							auto range = Range< uint8 >( animation.range );
							if( isKeyPressed( inputs, KC_LButton )
							    && isPointInside( thumbRect, inputs->mouse.position )
							    && imguiIsHover( handle ) ) {

								imguiFocus( handle );
								view->placingTile.frames = range;
							}

							stream->color = ( view->placingTile.frames == range )
							                    ? ( 0xFF808080 )
							                    : ( Color::White );

							auto frame      = &tileSet->voxels.frameInfos[animation.range.min];
							auto& texCoords = frame->textureMap.entries[VF_Front].texCoords;
							pushQuad( stream, thumbRect, 0, texCoords );

							thumbRect = translate( thumbRect, 32, 0 );
							if( thumbRect.right >= scrollable.inner.right ) {
								thumbRect = RectSetLeft( thumbRect, scrollable.inner.left );
								thumbRect = translate( thumbRect, 0, 32 );
							}
						}
					}
				}
			}

			imguiEndScrollableRegion( &editor->tilesScollableRegion, &scrollable );
		} else {
			imguiText( "Entities" );
			auto listboxSize = imguiSize( imgui::Ratio{1}, imgui::Absolute{100} );
			if( auto tileSet = view->tileSet ) {
				auto listboxHandle = imguiMakeHandle( &editor->entitiesScrollPos );
				int32 index        = view->placingEntity;
				auto listboxRect   = imguiAddItem( listboxSize.width, listboxSize.height );
				if( imguiListboxSingleSelect( listboxHandle, &editor->entitiesScrollPos,
				                              makeArrayView( EntityTypeNames ), &index, listboxRect,
				                              false ) ) {
					view->placingEntity = (EntityType)index;
				}
			}
		}

		auto listboxSize                        = imguiSize( imgui::Ratio{1}, imgui::Absolute{60} );
		static ImGuiListboxItem RotationNames[] = {{"0"}, {"90"}, {"180"}, {"270"}};
		auto names                              = makeArrayView( RotationNames );
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
		if( imguiEditbox( "Width", &view->room.layers[RL_Main].width, imguiRatio( 0.2f, 0 ) ) ) {
			auto w = view->room.layers[RL_Main].width;
			w = min( w, MaxWidth );
			resizeRoom( editor, w, view->room.layers[RL_Main].height );
		}
		if( imguiEditbox( "Height", &view->room.layers[RL_Main].height, imguiRatio( 0.2f, 0 ) ) ) {
			auto h = view->room.layers[RL_Main].height;
			h = min( h, MaxHeight );
			resizeRoom( editor, view->room.layers[RL_Main].width, h );
		}
		imguiEndDropGroup();
	}

	if( imguiButton( "Play" ) ) {
		playRoom( app, view );
	}
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
			setUnsavedChanges( view );
		};

		if( editor->mouseMode != MouseMode::Entity ) {
			processMouse( KC_RButton, InvalidGameTile, {} );
			processMouse( KC_LButton, view->placingTile, view->placingTile );
		} else {
			bool placeEntity  = false;
			EntityType type = {};
			if( isKeyDown( inputs, KC_LButton ) ) {
				if( isKeyPressed( inputs, KC_LButton ) ) {
					placeEntity = true;
					type = view->placingEntity;
				} else {
					placeEntity = editor->selectionStart != gridPos;
				}
				editor->selectionStart = gridPos;
			}
			if( isKeyDown( inputs, KC_RButton ) ) {
				if( isKeyPressed( inputs, KC_RButton ) ) {
					placeEntity = true;
				} else {
					placeEntity = editor->selectionStart != gridPos;
				}
				editor->selectionStart = gridPos;
			}

			if( placeEntity ) {
				if( auto index = find_index_if( view->entities, [gridPos]( const auto& entry ) {
					    return entry.position == gridPos;
					} ) ) {

					auto entry = &view->entities[index.get()];
					if( entry->type != type ) {
						if( entry->skeleton ) {
							deleteSkeleton( entry->skeleton );
							entry->skeleton = nullptr;
						}
						if( type != Entity::type_none ) {
							entry->type = type;
							entry->skeleton = makeSkeleton( *editor->definitions[type] );
						} else {
							unordered_erase( view->entities, view->entities.begin() + index.get() );
						}
					}
				} else if( type != Entity::type_none ) {
					view->entities.push_back(
					    {type, gridPos, makeSkeleton( *editor->definitions[type] )} );
				}
			}
		}
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

		// render entities
		FOR( entity : view->entities ) {
			auto position = gameToScreen( gridPosToEntityPos( entity.position ) );
			setTransform( entity.skeleton, matrixTranslation( Vec3( position, 0 ) ) );
			update( entity.skeleton, nullptr, 0 );
			render( renderer, entity.skeleton );
		}

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

		editor->fileMenu             = imguiGenerateContainer( gui, {}, ImGuiVisibility::Hidden );
		editor->viewMenu             = imguiGenerateContainer( gui, {}, ImGuiVisibility::Hidden );
		editor->messageBox.container =
		    imguiGenerateContainer( gui, {0, 0, 300, 10}, ImGuiVisibility::Hidden );

		// TODO: better way of getting to SkeletonSystem without digging through an unrelated
		// subsystem
		{
			auto skeletonSystem = &app->gameState.skeletonSystem;
			for( auto i = 0; i < Entity::type_count; ++i ) {
				editor->definitions[i] =
				    getSkeletonTraits( skeletonSystem, (EntityType)i )->definition;
			}
		}

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

	updateIntermediateRoom( editor, view );

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

	if( isHotkeyPressed( inputs, KC_S, KC_Control ) ) {
		menuSave( app );
	}
	if( isHotkeyPressed( inputs, KC_S, KC_Control, KC_Shift ) ) {
		menuSaveAs( app );
	}

	if( imguiContextMenu( editor->fileMenu ) ) {
		if( imguiContextMenuEntry( "New" ) ) {
			auto saveAndClear = []( AppData* app ) {
				if( menuSave( app ) ) {
					clear( &app->roomEditorState.view );
				}
			};
			auto clear = []( AppData* app ) { RoomEditor::clear( &app->roomEditorState.view ); };
			if( view->flags & View::UnsavedChanges ) {
				showMessageBox( &editor->messageBox, "Save changes?", "Unsaved changes",
				                saveAndClear, clear );
			} else {
				clear( app );
			}
		}
		if( imguiContextMenuEntry( "Open" ) ) {
			menuOpen( app );
		}
		if( imguiContextMenuEntry( "Save" ) ) {
			menuSave( app );
		}
		if( imguiContextMenuEntry( "Save As" ) ) {
			menuSaveAs( app );
		}
		imguiEndContainer();
	}

	if( imguiContextMenu( editor->viewMenu ) ) {
		imguiCheckboxFlag( "Grid", &editor->view.flags, View::DrawGrid );
		imguiCheckboxFlag( "Grid In Front", &editor->view.flags, View::DrawGridInFront );
		imguiCheckboxFlag( "Selected Layers Only", &editor->view.flags,
		                   View::DrawSelectedLayersOnly );
		imguiEndContainer();
	}

	handleMessageBox( app, inputs, editor->messageBox );

	imguiUpdate( dt );
	imguiFinalize();
}

} // namespace RoomEditor
