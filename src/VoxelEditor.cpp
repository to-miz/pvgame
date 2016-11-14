#define EDITOR_CELL_WIDTH 32.0f
#define EDITOR_CELL_HEIGHT 32.0f
#define EDITOR_CELL_DEPTH 32.0f

const vec3 EditorVoxelCellSize = {EDITOR_CELL_WIDTH, EDITOR_CELL_HEIGHT, EDITOR_CELL_DEPTH};

#define CELL_ONE_OVER_WIDTH ( 1.0f / EDITOR_CELL_WIDTH )
#define CELL_ONE_OVER_HEIGHT ( 1.0f / EDITOR_CELL_HEIGHT )
#define CELL_ONE_OVER_DEPTH ( 1.0f / EDITOR_CELL_DEPTH )

void generateMeshFromVoxelGridNaive( MeshStream* stream, VoxelGrid* grid, vec3arg cellSize )
{
	assert( isValid( stream ) );
	assert( grid );

	stream->color = 0xFF000000;
	auto yStart   = grid->height * cellSize.y;

	for( int32 z = 0; z < grid->depth; ++z ) {
		for( int32 y = 0; y < grid->height; ++y ) {
			for( int32 x = 0; x < grid->width; ++x ) {
				int32 index = ( x ) + ( y * grid->width ) + ( z * grid->width * grid->height );
				int32 cell  = grid->data[index];
				if( cell != EmptyCell ) {
					stream->color = 0xFF000000;
					float left    = x * cellSize.x;
					float bottom  = yStart - y * cellSize.y - cellSize.y;
					float near    = z * cellSize.z;
					float right   = left + cellSize.x;
					float top     = bottom + cellSize.y;
					float far     = near + cellSize.z;
					pushAabb( stream, left, bottom, near, right, top, far );
				}
			}
		}
	}
}

struct RayCastResult {
	bool found;
	vec3i position;
	vec3i normal;
	vec3 intersection;
};

// raycasting into 3d grid algorithm based on this paper:
// http://www.cse.yorku.ca/~amana/research/grid.pdf
RayCastResult raycastIntoVoxelGrid( VoxelGrid* grid, vec3arg rayOrigin, vec3 rayDir, float tMax )
{
	assert( grid );
	RayCastResult result = {};

	// find first grid intersection point
	aabb gridBoundingBox = {0,
	                        0,
	                        0,
	                        grid->width * EDITOR_CELL_WIDTH,
	                        grid->height * EDITOR_CELL_HEIGHT,
	                        grid->depth * EDITOR_CELL_DEPTH};

	auto originalDir = rayDir;
	TestRayVsAabbResult rayIntersection;
	if( !testRayVsAabb( rayOrigin, rayDir, gridBoundingBox, &rayIntersection ) ) {
		return result;
	}
	auto rayIntersectionT = rayIntersection.enter.t;
	result.normal         = rayIntersection.enter.normal;

	vec3 start;
	if( rayIntersectionT >= 0 ) {
		start = rayOrigin + rayDir * rayIntersectionT;
	} else {
		rayIntersectionT = 0;
		start            = rayOrigin;
	}
	start.y = grid->height * EDITOR_CELL_HEIGHT - start.y;
	start.x = clamp( start.x, gridBoundingBox.min.x, gridBoundingBox.max.x - 1 );
	start.y = clamp( start.y, gridBoundingBox.min.y, gridBoundingBox.max.y - 1 );
	start.z = clamp( start.z, gridBoundingBox.min.z, gridBoundingBox.max.z - 1 );

	result.normal.y = -result.normal.y;
	rayDir.y        = -rayDir.y;

	int32 stepX = ( rayDir.x >= 0 ) ? ( 1 ) : ( -1 );
	int32 stepY = ( rayDir.y >= 0 ) ? ( 1 ) : ( -1 );
	int32 stepZ = ( rayDir.z >= 0 ) ? ( 1 ) : ( -1 );

	int32 x = (int32)floor( start.x * CELL_ONE_OVER_WIDTH );
	int32 y = (int32)floor( start.y * CELL_ONE_OVER_HEIGHT );
	int32 z = (int32)floor( start.z * CELL_ONE_OVER_DEPTH );

	float nextVoxelX = ( x + stepX ) * EDITOR_CELL_WIDTH;
	float nextVoxelY = ( y + stepY ) * EDITOR_CELL_HEIGHT;
	float nextVoxelZ = ( z + stepZ ) * EDITOR_CELL_DEPTH;
	if( rayDir.x < 0 ) {
		nextVoxelX += EDITOR_CELL_WIDTH;
	}
	if( rayDir.y < 0 ) {
		nextVoxelY += EDITOR_CELL_HEIGHT;
	}
	if( rayDir.z < 0 ) {
		nextVoxelZ += EDITOR_CELL_DEPTH;
	}

	auto oneOverRayDirX = 1.0f / rayDir.x;
	auto oneOverRayDirY = 1.0f / rayDir.y;
	auto oneOverRayDirZ = 1.0f / rayDir.z;

	float tMaxX = ( rayDir.x != 0 ) ? ( ( nextVoxelX - start.x ) * oneOverRayDirX ) : ( FLOAT_MAX );
	float tMaxY = ( rayDir.y != 0 ) ? ( ( nextVoxelY - start.y ) * oneOverRayDirY ) : ( FLOAT_MAX );
	float tMaxZ = ( rayDir.z != 0 ) ? ( ( nextVoxelZ - start.z ) * oneOverRayDirZ ) : ( FLOAT_MAX );

	float tDeltaX =
	    ( rayDir.x != 0 ) ? ( EDITOR_CELL_WIDTH * oneOverRayDirX * stepX ) : ( FLOAT_MAX );
	float tDeltaY =
	    ( rayDir.y != 0 ) ? ( EDITOR_CELL_HEIGHT * oneOverRayDirY * stepY ) : ( FLOAT_MAX );
	float tDeltaZ =
	    ( rayDir.z != 0 ) ? ( EDITOR_CELL_DEPTH * oneOverRayDirZ * stepZ ) : ( FLOAT_MAX );

	result.intersection = rayOrigin + originalDir * rayIntersectionT;
	if( getCell( grid, x, y, z ) != EmptyCell ) {
		result.position = {x, y, z};
		result.found    = true;
	} else {
		float t     = 0;
		while( t < tMax ) {
			if( !isPointInsideVoxelBounds( grid, x, y, z ) ) {
				result.found = false;
				break;
			}

			if( getCell( grid, x, y, z ) != EmptyCell ) {
				result.intersection = rayOrigin + originalDir * ( t + rayIntersectionT );
				result.position     = {(int32)x, (int32)y, (int32)z};
				result.found        = true;
				break;
			}

			if( tMaxX < tMaxY ) {
				if( tMaxX < tMaxZ ) {
					// tMaxX was smallest
					x += stepX;
					t             = tMaxX;
					result.normal = {-stepX, 0, 0};
					tMaxX += tDeltaX;
				} else {
					// tMaxZ was smallest
					z += stepZ;
					t             = tMaxZ;
					result.normal = {0, 0, -stepZ};
					tMaxZ += tDeltaZ;
				}
			} else {
				if( tMaxY < tMaxZ ) {
					// tMaxY was smallest
					y += stepY;
					t             = tMaxY;
					result.normal = {0, -stepY, 0};
					tMaxY += tDeltaY;
				} else {
					// tMaxZ was smallest
					z += stepZ;
					t             = tMaxZ;
					result.normal = {0, 0, -stepZ};
					tMaxZ += tDeltaZ;
				}
			}
		}
	}

	return result;
}

static void processBuildMode( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto voxel  = &app->voxelState;
	auto camera = &voxel->camera;
	// auto renderer     = &app->renderer;
	auto cameraCenter = center( camera );

	auto generateVoxelMesh = false;
	if( isKeyPressed( inputs, KC_LButton ) ) {
		auto result = raycastIntoVoxelGrid( &voxel->voxels, cameraCenter, camera->look, 10000 );
		if( result.found ) {
			auto destinationCell = result.position + result.normal;
			if( isPointInsideVoxelBounds( &voxel->voxels, destinationCell ) ) {
				getCell( &voxel->voxels, destinationCell ) = voxel->placingCell;
				generateVoxelMesh = true;
			}
		}
	}
	if( isKeyPressed( inputs, KC_RButton ) ) {
		auto result = raycastIntoVoxelGrid( &voxel->voxels, cameraCenter, camera->look, 10000 );
		if( result.found ) {
			auto destinationCell = result.position;
			if( isPointInsideVoxelBounds( &voxel->voxels, destinationCell ) ) {
				getCell( &voxel->voxels, destinationCell ) = EmptyCell;
				generateVoxelMesh = true;
			}
		}
	}

	if( generateVoxelMesh ) {
		clear( &voxel->meshStream );
		generateMeshFromVoxelGrid( &voxel->meshStream, &voxel->voxels, &voxel->textureMap,
		                           EditorVoxelCellSize );
	}
}

aabb calculateSelectionWorld( VoxelState* voxel )
{
	auto grid             = &voxel->voxels;
	auto selectionWorld   = AabbScaled( voxel->selection, EditorVoxelCellSize );
	auto voxelGridTop     = EDITOR_CELL_HEIGHT * grid->height;
	selectionWorld        = translate( selectionWorld, {0, voxelGridTop, 0} );
	selectionWorld.bottom = voxelGridTop * 2 - selectionWorld.bottom;
	selectionWorld.top    = voxelGridTop * 2 - selectionWorld.top;
	swap( selectionWorld.bottom, selectionWorld.top );
	return selectionWorld;
}

aabbi getSelection( VoxelState* state )
{
	aabbi result;
	auto grid    = &state->voxels;
	result.min.x = max( min( state->selection.min.x, state->selection.max.x ), 0 );
	result.max.x = min( max( state->selection.min.x, state->selection.max.x ), grid->width );

	result.min.y = max( min( state->selection.min.y, state->selection.max.y ), 0 );
	result.max.y = min( max( state->selection.min.y, state->selection.max.y ), grid->height );

	result.min.z = max( min( state->selection.min.z, state->selection.max.z ), 0 );
	result.max.z = min( max( state->selection.min.z, state->selection.max.z ), grid->depth );
	return result;
}
static void processSelectMode( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto voxel        = &app->voxelState;
	auto camera       = &voxel->camera;
	auto cameraCenter = center( camera );
	auto grid         = &voxel->voxels;

	if( isKeyPressed( inputs, KC_MButton ) ) {
		auto result = raycastIntoVoxelGrid( &voxel->voxels, cameraCenter, camera->look, 10000 );
		if( result.found ) {
			voxel->selection = AabbWHD( result.position, 1, 1, 1 );
		}
	}

	if( isKeyPressed( inputs, KC_Key_J ) ) {
		swap( voxel->selection.min.x, voxel->selection.max.x );
		swap( voxel->selection.min.y, voxel->selection.max.y );
		swap( voxel->selection.min.z, voxel->selection.max.z );
	}
	if( voxel->dragAction == DragAction::None ) {
		auto& selectionWorld = voxel->selectionWorld = calculateSelectionWorld( voxel );
		TestRayVsAabbResult result;
		voxel->isFaceSelected =
		    testRayVsAabb( cameraCenter, camera->look, selectionWorld, &result );
		if( voxel->isFaceSelected ) {
			TestRayVsAabbOption* option;
			if( isKeyDown( inputs, KC_Alt ) ) {
				option = &result.leave;
			} else {
				option = &result.enter;
			}
			voxel->selectionOrigin = cameraCenter + camera->look * option->t;
			voxel->selectionNormal = option->normal;
		}
	}

	auto generateVoxelMesh = false;
	switch( voxel->dragAction ) {
		case DragAction::None: {
			break;
		}
		case DragAction::DragSelection: {
			if( isKeyUp( inputs, KC_LButton ) ) {
				voxel->dragAction = DragAction::None;
			}
			break;
		}
		case DragAction::MoveVoxels: {
			if( isKeyUp( inputs, KC_RButton ) ) {
				voxel->dragAction = DragAction::None;
				auto selection    = getSelection( voxel );
				for( int32 z = selection.min.z; z < selection.max.z; ++z ) {
					for( int32 y = selection.min.y; y < selection.max.y; ++y ) {
						for( int32 x = selection.min.x; x < selection.max.x; ++x ) {
							auto& cell     = getCell( &voxel->voxels, x, y, z );
							auto& selected = getCell( &voxel->voxelsMoving, x - selection.min.x,
							                          y - selection.min.y, z - selection.min.z );
							if( selected != EmptyCell ) {
								cell = selected;
							}
							selected = EmptyCell;
						}
					}
				}
				generateVoxelMesh = true;
			}
			break;
		}
			InvalidDefaultCase;
	}

	auto startedDragging = ( voxel->dragAction == DragAction::None );
	if( voxel->dragAction == DragAction::None && voxel->isFaceSelected ) {
		if( isKeyPressed( inputs, KC_LButton ) ) {
			voxel->dragAction = DragAction::DragSelection;
		} else if( isKeyPressed( inputs, KC_RButton ) ) {
			voxel->dragAction = DragAction::MoveVoxels;
		}
	}

	if( voxel->dragAction != DragAction::None ) {
		vec3 axisOrigin          = voxel->selectionOrigin;
		vec3 axisDir             = {};
		intmax componentAabbMin  = 0;
		intmax componentAabbMax  = 0;
		intmax componentIndexMin = 0;
		intmax componentVec      = 0;
		float scale              = 0;
		float offset             = 0;
		auto selectionNormal     = voxel->selectionNormal;
		if( selectionNormal.x != 0 ) {
			axisDir = {1, 0, 0};
			if( selectionNormal.x < 0 ) {
				componentAabbMin  = 0;
				componentIndexMin = 0;
				componentAabbMax  = 3;
			} else {
				componentAabbMin  = 3;
				componentIndexMin = 3;
				componentAabbMax  = 0;
			}
			componentVec = 0;
			scale        = CELL_ONE_OVER_WIDTH;
		} else if( selectionNormal.y != 0 ) {
			axisDir = {0, 1, 0};
			if( selectionNormal.y < 0 ) {
				componentAabbMin  = 1;
				componentIndexMin = 4;
				componentAabbMax  = 4;
			} else {
				componentAabbMin  = 4;
				componentIndexMin = 1;
				componentAabbMax  = 1;
			}
			componentVec = 1;
			scale        = -CELL_ONE_OVER_HEIGHT;
			offset       = -EDITOR_CELL_HEIGHT * grid->height;
		} else if( selectionNormal.z != 0 ) {
			axisDir = {0, 0, 1};
			if( selectionNormal.z < 0 ) {
				componentAabbMin  = 2;
				componentIndexMin = 2;
				componentAabbMax  = 5;
			} else {
				componentAabbMin  = 5;
				componentIndexMin = 5;
				componentAabbMax  = 2;
			}
			componentVec = 2;
			scale        = CELL_ONE_OVER_DEPTH;
		} else {
			InvalidCodePath();
		}

		auto result = shortestLineBetweenLines( cameraCenter, camera->look, axisOrigin, axisDir );
		auto axisResult = axisOrigin + axisDir * result.tB;
		if( startedDragging ) {
			voxel->lastAxisPosition = axisResult;
		}
		switch( voxel->dragAction ) {
			case DragAction::DragSelection: {
				if( !startedDragging ) {
					auto delta = ( axisResult - voxel->lastAxisPosition ).elements[componentVec];
					voxel->selectionWorld.elements[componentAabbMin] += delta;
					auto index = (int32)floor(
					    ( voxel->selectionWorld.elements[componentAabbMin] + offset ) * scale );
					voxel->selection.elements[componentIndexMin] = index;
				}
				break;
			}
			case DragAction::MoveVoxels: {
				if( startedDragging ) {
					auto duplicate = isKeyDown( inputs, KC_Shift );
					auto selection = getSelection( voxel );
					for( int32 z = selection.min.z; z < selection.max.z; ++z ) {
						for( int32 y = selection.min.y; y < selection.max.y; ++y ) {
							for( int32 x = selection.min.x; x < selection.max.x; ++x ) {
								auto& cell = getCell( &voxel->voxels, x, y, z );
								auto& selected =
								    getCell( &voxel->voxelsMoving, x - selection.min.x,
								             y - selection.min.y, z - selection.min.z );
								selected = cell;
								if( !duplicate ) {
									cell = EmptyCell;
								}
							}
						}
					}
					generateVoxelMesh = true;
				} else {
					auto delta = ( axisResult - voxel->lastAxisPosition ).elements[componentVec];
					voxel->selectionWorld.elements[componentAabbMin] += delta;
					voxel->selectionWorld.elements[componentAabbMax] += delta;
					auto indexMin = (int32)floor(
					    ( voxel->selectionWorld.elements[componentAabbMin] + offset ) * scale );
					auto indexMax = (int32)floor(
					    ( voxel->selectionWorld.elements[componentAabbMax] + offset ) * scale );
					if( voxel->selection.min.elements[componentVec] != indexMin
					    || voxel->selection.max.elements[componentVec] != indexMax ) {
						generateVoxelMesh = true;
					}
					voxel->selection.min.elements[componentVec] = indexMin;
					voxel->selection.max.elements[componentVec] = indexMax;
				}
				break;
			}
				InvalidDefaultCase;
		}
		voxel->lastAxisPosition = axisResult;
	}

	if( isKeyPressed( inputs, KC_Delete ) ) {
		generateVoxelMesh = true;
		auto selection    = getSelection( voxel );
		for( int32 z = selection.min.z; z < selection.max.z; ++z ) {
			for( int32 y = selection.min.y; y < selection.max.y; ++y ) {
				for( int32 x = selection.min.x; x < selection.max.x; ++x ) {
					getCell( &voxel->voxels, x, y, z ) = EmptyCell;
				}
			}
		}
	}

	if( generateVoxelMesh ) {
		auto selection        = getSelection( voxel );
		voxel->voxelsCombined = voxel->voxels;
		for( int32 z = selection.min.z; z < selection.max.z; ++z ) {
			for( int32 y = selection.min.y; y < selection.max.y; ++y ) {
				for( int32 x = selection.min.x; x < selection.max.x; ++x ) {
					auto src = getCell( &voxel->voxelsMoving, x - selection.min.x,
					                    y - selection.min.y, z - selection.min.z );
					auto& dest = getCell( &voxel->voxelsCombined, x, y, z );
					if( src != EmptyCell ) {
						dest = src;
					}
				}
			}
		}
		clear( &voxel->meshStream );
		generateMeshFromVoxelGrid( &voxel->meshStream, &voxel->voxelsCombined, &voxel->textureMap,
		                           EditorVoxelCellSize );
	}
}
static void processEditMode( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto voxel = &app->voxelState;

	switch( voxel->editMode ) {
		case EditMode::Build: {
			processBuildMode( app, inputs, focus, dt );
			break;
		}
		case EditMode::Select: {
			processSelectMode( app, inputs, focus, dt );
			break;
		}
			InvalidDefaultCase;
	}
}

void saveVoxelGridToFile( PlatformServices* platform, StringView filename, VoxelGrid* grid )
{
	assert( platform );
	assert( grid );
	// TODO: write the path to the used texture map also to the file
	platform->writeBufferToFile( filename, grid, sizeof( VoxelGrid ) );
}
void saveVoxelGridsToFile( PlatformServices* platform, StringView filename,
                           Array< VoxelGrid > grids )
{
	assert( platform );
	if( grids.size() ) {
		// TODO: write the path to the used texture map also to the file
		platform->writeBufferToFile( filename, grids.data(), grids.size() * sizeof( VoxelGrid ) );
	}
}

VoxelGrid getVoxelGridFromTextureMap( VoxelGridTextureMap* map, Color colorkey )
{
	assert( map );
	VoxelGrid result = {};
	auto info        = getTextureInfo( map->texture );
	auto tw          = getAxisAlignedWidth( map->entries[VF_Front].texCoords );
	auto th          = getAxisAlignedHeight( map->entries[VF_Front].texCoords );
	auto td          = getAxisAlignedWidth( map->entries[VF_Right].texCoords );

	// convert texture coordinates to pixel coordinates to calculate grid dimensions
	result.width  = (int32)round( tw * info->image.width );
	result.height = (int32)round( th * info->image.height );
	result.depth  = (int32)round( td * info->image.width );

	/*auto halfTexelW = ( 1.0f / info->image.width ) * 0.5f;
	auto halfTexelH = ( 1.0f / info->image.height ) * 0.5f;*/
	auto isEmpty = [&]( vec3iarg index, uint32 face ) {
		auto plane     = getTexelPlaneByFace( face );
		auto texCoords = map->entries[face].texCoords.elements;
		float tx       = ( (float)index.elements[plane.x] + 0.5f ) / (float)result.dim[plane.x];
		float ty       = ( (float)index.elements[plane.y] + 0.5f ) / (float)result.dim[plane.y];
		auto posX      = lerp( tx, texCoords[2].u, texCoords[3].u ) * info->image.width;
		auto posY      = lerp( ty, texCoords[1].v, texCoords[3].v ) * info->image.height;
		auto textureX  = (int32)floor( posX );
		auto textureY  = (int32)floor( posY );
		auto pixel     = getPixelColor( info->image, textureX, textureY );
		auto alpha     = getAlpha( pixel );
		bool ret       = ( alpha == 0 || pixel == colorkey );
		return ret;
	};

	int32 strideY = result.width;
	int32 strideZ = result.width * result.height;
	for( vec3i pos = {}; pos.z < result.depth; ++pos.z ) {
		for( pos.y = 0; pos.y < result.height; ++pos.y ) {
			for( pos.x = 0; pos.x < result.width; ++pos.x ) {
				int32 index = pos.x + pos.y * strideY + pos.z * strideZ;
				auto& cell  = result.data[index];
				cell        = DefaultCell;

				for( uint32 face = 0; face < VF_Count; ++face ) {
					if( isEmpty( pos, face ) ) {
						cell = EmptyCell;
						break;
					}
				}
			}
		}
	}
	return result;
}
VoxelGrid getVoxelGridFromTextureMapTopLeftColorKey( VoxelGridTextureMap* map )
{
	assert( map );
	assert( map->texture );
	auto info = getTextureInfo( map->texture );
	return getVoxelGridFromTextureMap( map, getPixelColor( info->image, 0, 0 ) );
}

static void doVoxelGui( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	PROFILE_FUNCTION();

	auto voxel     = &app->voxelState;
	auto gui       = &voxel->gui;
	auto renderer  = &app->renderer;
	auto font      = &app->font;
	auto fadeSpeed = 0.25f * dt;
	if( !focus && gui->fadeProgress <= 0 ) {
		return;
	}
	gui->fadeProgress = clamp( gui->fadeProgress + fadeSpeed * ( ( focus ) ? ( 1 ) : ( -1 ) ) );

	auto imgui = &app->guiState;
	imguiBind( imgui );
	if( !gui->initialized ) {
		gui->editMode         = imguiGenerateContainer( imgui );
		gui->rendering        = imguiGenerateContainer( imgui, {ImGui->style.containerWidth} );
		gui->textureIndex     = imguiGenerateContainer( imgui, RectWH( ImGui->style.containerWidth, 200, 240, 200 ) );
		gui->textureMapDialog = imguiGenerateContainer( imgui, {0, 300} );
		gui->noLightingChecked = true;

		gui->animations = makeUArray( &app->stackAllocator, VoxelGuiState::Animation, 100 );

		gui->initialized = true;
	}
	if( isKeyPressed( inputs, KC_Key_K ) ) {
		imguiGetContainer( gui->editMode )->setHidden( false );
		imguiGetContainer( gui->rendering )->setHidden( false );
		imguiGetContainer( gui->textureIndex )->setHidden( false );
	}

	setProjection( renderer, ProjectionType::Orthogonal );
	/*setTexture( renderer, 0, voxel->textureMap );
	MESH_STREAM_BLOCK( stream, renderer ) {
	    stream->color    = 0xFFFFFFFF;
	    vec3 vertices[4] = {{0, 0, 0}, {100, 0, 0}, {0, 100, 0}, {100, 100, 0}};
	    pushQuad( stream, vertices );
	}*/
	renderer->color        = setAlpha( 0xFFFFFFFF, gui->fadeProgress );
	auto generateVoxelMesh = false;

	rectf guiBounds = {0, 0, app->width, app->height};
	imguiBind( &app->guiState, renderer, font, inputs, app->stackAllocator.ptr, guiBounds, focus );
	if( imguiDialog( "EditMode", gui->editMode ) ) {
		char buffer[200];
		string_builder builder( buffer, 200 );
		builder.print( "Current: {}", EditModeStrings[valueof( voxel->editMode )] );
		imguiText( asStringView( builder ) );
		imguiSameLine( 3 );
		for( intmax i = 0; i < valueof( EditMode::Count ); ++i ) {
			if( imguiButton( EditModeStrings[i], 16, 16 ) ) {
				voxel->editMode = (EditMode)i;
			}
		}

		if( imguiButton( "New" ) ) {
			fill( voxel->voxels.data, DefaultCell, voxel->voxels.max_size() );
			generateVoxelMesh = true;
		}

		if( imguiBeginDropGroup( "Sizes", &gui->sizesExpanded ) ) {
			if( imguiEditbox( "Width", &voxel->voxels.width ) ) {
				voxel->voxels.width = clamp( voxel->voxels.width, 0, CELL_MAX_X );
				generateVoxelMesh   = true;
			}
			if( imguiEditbox( "Height", &voxel->voxels.height ) ) {
				voxel->voxels.height = clamp( voxel->voxels.height, 0, CELL_MAX_X );
				generateVoxelMesh    = true;
			}
			if( imguiEditbox( "Depth", &voxel->voxels.depth ) ) {
				voxel->voxels.depth = clamp( voxel->voxels.depth, 0, CELL_MAX_X );
				generateVoxelMesh   = true;
			}
			imguiEndDropGroup();
		}

		if( imguiBeginDropGroup( "Load & Save", &gui->fileExpanded ) ) {
			imguiSameLine( 2 );
			if( imguiButton( "Load" ) ) {
				char filenameBuffer[260];
				auto filenameSize = app->platform.getOpenFilename(
				    "All\0*.*\0", nullptr, false, filenameBuffer, countof( filenameBuffer ) );
				if( filenameSize ) {
					loadVoxelGridFromFile( &app->platform, {filenameBuffer, filenameSize},
					                       &voxel->voxels );
					generateVoxelMesh = true;
				}
			}

			if( imguiButton( "Save" ) ) {
				char filenameBuffer[260];
				auto filenameSize = app->platform.getSaveFilename(
				    "All\0*.*\0", nullptr, filenameBuffer, countof( filenameBuffer ) );
				if( filenameSize ) {
					saveVoxelGridToFile( &app->platform, {filenameBuffer, filenameSize},
					                     &voxel->voxels );
				}
			}

			imguiEndDropGroup();
		}
	}

	if( imguiDialog( "Rendering Settings", gui->rendering ) ) {
		if( imguiRadiobox( "Lighting", &gui->lightingChecked ) ) {
			voxel->lighting = gui->lightingChecked;
		}
		if( imguiRadiobox( "No Lighting", &gui->noLightingChecked ) ) {
			voxel->lighting = !gui->noLightingChecked;
		}
		imguiCheckbox( "Frame Offset", &voxel->renderWithOffset );
	}

	if( imguiDialog( "Texture Mapping", gui->textureMapDialog ) ) {
		if( imguiBeginDropGroup( "Load & Save", &gui->collectionSaveLoadExpanded ) ) {
			imguiSameLine( 2 );
			if( imguiButton( "Load Collection" ) ) {
				char filenameBuffer[260];
				auto filenameLength =
				    app->platform.getOpenFilename( "json\0*.json\0", "Data/voxel", false,
				                                   filenameBuffer, countof( filenameBuffer ) );
				if( filenameLength ) {
					clear( &voxel->collectionArena );
					auto collection     = &voxel->collection;
					*collection         = {};
					voxel->currentFrame = -1;
					gui->animations.clear();
					if( loadVoxelCollectionTextureMapping( &voxel->collectionArena,
					                                       {filenameBuffer, filenameLength},
					                                       collection ) ) {
						gui->animations.resize( collection->animations.size() );
						auto framesCount = collection->frames.size();
						voxel->frameVoxels =
						    makeArray( &voxel->collectionArena, VoxelGrid, framesCount );
						if( !loadVoxelGridsFromFile( &app->platform, collection->voxelsFilename,
						                             voxel->frameVoxels ) ) {
							for( auto frame = 0; frame < framesCount; ++frame ) {
								auto textureMap = &collection->frameInfos[frame].textureMap;
								auto voxels     = &voxel->frameVoxels[frame];
								*voxels = getVoxelGridFromTextureMapTopLeftColorKey( textureMap );
							}
						}
						zeroMemory( gui->animations.data(), gui->animations.size() );
					}
				}
			}
			if( imguiButton( "Save Collection" ) ) {
				if( voxel->collection.texture ) {
					saveVoxelGridsToFile( &app->platform, voxel->collection.voxelsFilename,
					                      voxel->frameVoxels );
				}
			}
			imguiEndDropGroup();
		}

		imguiSeperator();
		imguiSameLine( 2 );
		if( imguiButton( "Assign to current" ) ) {
			auto collection = &voxel->collection;
			auto index      = voxel->currentFrame;
			if( collection->texture && index >= 0 && index < voxel->frameVoxels.size() ) {
				voxel->frameVoxels[index] = voxel->voxels;
			}
		}
		/*if( imguiBeginDropGroup( "Offset", &gui->collectionOffsetExpanded ) ) {
		    auto collection = &voxel->collection;
		    auto index      = voxel->currentFrame;
		    if( collection->texture && index >= 0 && index < voxel->frameVoxels.size() ) {
		        auto current = &collection->frames[index];
		        imguiEditbox( "x", &current->offset.x );
		        imguiEditbox( "y", &current->offset.y );
		    }
		    imguiEndDropGroup();
		}*/
		if( imguiButton( "Generate Grid" ) ) {
			voxel->voxels     = getVoxelGridFromTextureMapTopLeftColorKey( &voxel->textureMap );
			generateVoxelMesh = true;
		}

		imguiSeperator();
		if( voxel->collection.texture
		    && imguiBeginDropGroup( "Collection", &gui->collectionExpanded ) ) {

			auto collection = &voxel->collection;
			for( auto i = 0, count = collection->animations.size(); i < count; ++i ) {
				auto entry = &collection->animations[i];
				auto state = &gui->animations[i];
				if( imguiBeginDropGroup( entry->name, &state->expanded ) ) {
					auto frameCount = length( entry->range );
					for( auto frame = 0; frame < frameCount; ++frame ) {
						auto frameIndex = entry->range.min + frame;
						StringView str  = "Current";
						auto number     = toNumberString( frame );
						if( voxel->currentFrame != frameIndex ) {
							str = number;
						}
						auto handle =
						    imguiMakeHandle( entry->name.data(), ImGuiControlType::Button, frame );
						if( imguiButton( handle, str, ImGui->style.buttonWidth,
						                 ImGui->style.buttonHeight ) ) {
							voxel->textureMap   = collection->frameInfos[frameIndex].textureMap;
							voxel->voxels       = voxel->frameVoxels[frameIndex];
							voxel->currentFrame = frameIndex;
							generateVoxelMesh   = true;
						}
					}
					imguiEndDropGroup();
				}
			}
			imguiEndDropGroup();
		}
	}

	if( generateVoxelMesh ) {
		voxel->voxelsCombined      = voxel->voxels;
		voxel->voxelsMoving.width  = voxel->voxels.width;
		voxel->voxelsMoving.height = voxel->voxels.height;
		voxel->voxelsMoving.depth  = voxel->voxels.depth;
		clear( &voxel->meshStream );
		generateMeshFromVoxelGrid( &voxel->meshStream, &voxel->voxels, &voxel->textureMap,
		                           EditorVoxelCellSize );
	}

	auto doButtons = [voxel]( StringView faceLabel, VoxelFaceValues face ) {
		imguiSameLine( 8 );
		imguiText( faceLabel, 35, 16 );

		auto innerHandle = imguiMakeHandle( &voxel->placingCell, ImGuiControlType::Button );
		innerHandle.shortIndex = safe_truncate< uint8 >( valueof( face ) );
		auto cell              = voxel->placingCell;
		bool inner             = isVoxelFaceInner( cell, face );
		bool front             = getVoxelFaceTexture( cell, face ) == VF_Front;
		bool left              = getVoxelFaceTexture( cell, face ) == VF_Left;
		bool back              = getVoxelFaceTexture( cell, face ) == VF_Back;
		bool right             = getVoxelFaceTexture( cell, face ) == VF_Right;
		bool top               = getVoxelFaceTexture( cell, face ) == VF_Top;
		bool bottom            = getVoxelFaceTexture( cell, face ) == VF_Bottom;
		imguiPushButton( "inner", &inner, 16, 16 );
		int32 index = -1;
		if( imguiPushButton( "front", &front, 16, 16 ) && front ) {
			index = 0;
		}
		if( imguiPushButton( "left", &left, 16, 16 ) && left ) {
			index = 1;
		}
		if( imguiPushButton( "back", &back, 16, 16 ) && back ) {
			index = 2;
		}
		if( imguiPushButton( "right", &right, 16, 16 ) && right ) {
			index = 3;
		}
		if( imguiPushButton( "top", &top, 16, 16 ) && top ) {
			index = 4;
		}
		if( imguiPushButton( "bottom", &bottom, 16, 16 ) && bottom ) {
			index = 5;
		}
		if( index >= 0 ) {
			voxel->placingCell = setVoxelFaceTexture( voxel->placingCell, face, index );
		}
		voxel->placingCell = setVoxelFaceInner( voxel->placingCell, face, inner );
	};
	if( imguiDialog( "Voxel Texture Index", gui->textureIndex ) ) {
		doButtons( "front", VF_Front );
		doButtons( "left", VF_Left );
		doButtons( "back", VF_Back );
		doButtons( "right", VF_Right );
		doButtons( "top", VF_Top );
		doButtons( "bottom", VF_Bottom );
		if( imguiButton( "Reset" ) ) {
			voxel->placingCell = DefaultCell;
		}
	}

	imguiUpdate( dt );

	imguiFinalize();
}

static void doVoxel( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	PROFILE_FUNCTION();

	auto voxel    = &app->voxelState;
	auto camera   = &voxel->camera;
	auto renderer = &app->renderer;
	// auto allocator = &app->stackAllocator;
	auto settings = &app->settings;

	if( !focus ) {
		return;
	}
	if( !voxel->initialized ) {
		auto textureMapId = app->platform.loadTexture( "Data/Images/texture_map.png" );
		voxel->textureMap = makeDefaultVoxelGridTextureMap( textureMapId );

		voxel->collectionArena = makeStackAllocator( &app->stackAllocator, megabytes( 5 ) );

		voxel->placingCell  = DefaultCell;
		voxel->currentFrame = -1;
		// initial voxel grid size
		voxel->voxels.width  = 16;
		voxel->voxels.height = 16;
		voxel->voxels.depth  = 16;
		voxel->initialized   = true;
	}

	// movement
	if( isKeyPressed( inputs, KC_Tab ) ) {
		if( voxel->focus == VoxelFocus::Gui ) {
			voxel->focus = VoxelFocus::Voxel;
		} else {
			voxel->focus = VoxelFocus::Gui;
		}
	}

	// mouse lock
	if( isKeyPressed( inputs, KC_Key_L ) ) {
		app->mouseLocked = !app->mouseLocked;
	}
	if( voxel->focus == VoxelFocus::Voxel ) {
		inputs->mouse.locked = app->mouseLocked;
	}

	if( voxel->focus == VoxelFocus::Voxel ) {
		processCamera( inputs, settings, &voxel->camera, dt );
		voxel->position = camera->position;
		processEditMode( app, inputs, focus, dt );
	}

	setProjection( renderer, ProjectionType::Perspective );
	auto cameraTranslation = matrixTranslation( 0, -50, 0 );
	renderer->view         = cameraTranslation * getViewMatrix( camera );

	setTexture( renderer, 0, null );

	if( isHotkeyPressed( inputs, KC_Key_U, KC_Control ) ) {
		clear( &voxel->meshStream );
		generateMeshFromVoxelGridNaive( &voxel->meshStream, &voxel->voxels, EditorVoxelCellSize );
	}

	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		auto grid     = &voxel->voxels;
		stream->color = 0xFF0000FF;
		aabb box      = {0,
		            0,
		            0,
		            grid->width * EDITOR_CELL_WIDTH,
		            grid->height * EDITOR_CELL_HEIGHT,
		            grid->depth * EDITOR_CELL_DEPTH};
		pushAabbOutline( stream, box );
	}

	setRenderState( renderer, RenderStateType::Lighting, voxel->lighting );
	setTexture( renderer, 0, voxel->textureMap.texture );
	if( voxel->renderWithOffset && voxel->collection.texture ) {
		pushMatrix( renderer->matrixStack );
		auto collection = &voxel->collection;
		auto index      = voxel->currentFrame;
		if( auto current = queryElement( collection->frames, index ) ) {
			translate( renderer->matrixStack, -current->offset.x * EDITOR_CELL_WIDTH,
			           current->offset.y * EDITOR_CELL_HEIGHT );
			addRenderCommandMeshTransformed( renderer, toMesh( &voxel->meshStream ) );
		} else {
			addRenderCommandMesh( renderer, toMesh( &voxel->meshStream ) );
		}
		popMatrix( renderer->matrixStack );
	} else {
		addRenderCommandMesh( renderer, toMesh( &voxel->meshStream ) );
	}

	if( voxel->editMode == EditMode::Select ) {
		setTexture( renderer, 0, null );
		setRenderState( renderer, RenderStateType::DepthTest, false );
		LINE_MESH_STREAM_BLOCK( stream, renderer ) {
			auto selectionWorld = calculateSelectionWorld( voxel );
			stream->color       = Color::Blue;
			pushAabbOutline( stream, selectionWorld );
		}
		setRenderState( renderer, RenderStateType::DepthTest, true );
	}

	doVoxelGui( app, inputs, voxel->focus == VoxelFocus::Gui, dt );
}