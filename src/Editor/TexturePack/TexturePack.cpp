/*
TODO:
	- finish up saving texture coordinates into VoxelGridTextureMap
	- implement saving/loading from file
*/

bool doTextureDisplay( AppData* app )
{
	assert( app );

	auto renderer       = imguiRenderer();
	auto inputs         = imguiInputs();
	auto editor         = &app->texturePackState;
	auto textureDisplay = &editor->textureDisplay;
	rectf textureRect   = {0, 0, editor->textureWidth * editor->textureScale,
	                     editor->textureHeight * editor->textureScale};
	auto rect           = imguiScrollable( &textureDisplay->scrollPos, textureRect, 400, 400 );

	auto clip = ClippingRect( renderer, rect );
	setTexture( renderer, 0, editor->texture );
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
			FOR( entry : editor->textureRegions ) {
				auto scaled = translate( entry * editor->textureScale, origin );
				if( isPointInside( scaled, inputs->mouse.position ) ) {
					addRenderCommandSingleQuad( renderer, scaled );

					if( isKeyPressed( inputs, KC_LButton ) ) {
						textureDisplay->selectedIndex  = indexof( editor->textureRegions, entry );
						textureDisplay->selectedRegion = Rect< float >( entry );
					}
				}
			}
		}
	} else {
		renderer->color = Color::White;
		auto translated = translate( textureRect, origin );
		imguiRect( &textureDisplay->selectedRegion,
		           {0, 0, editor->textureWidth, editor->textureHeight}, translated );
		if( editor->snapToPixels ) {
			textureDisplay->selectedRegion = floor( textureDisplay->selectedRegion );
		}

		debugPrintln( "{}", textureDisplay->selectedRegion );

		renderer->color = 0x80000000;
		setTexture( renderer, 0, null );
		auto scaled = translate( textureDisplay->selectedRegion * editor->textureScale, origin );
		addRenderCommandSingleQuad( renderer, scaled );
	}

	// render debug cells
	/*setTexture( renderer, 0, null );
	Color colors[] = {0x80FF0000, 0x8000FF00, 0x800000FF};
	stream->color = Color::White;
	MESH_STREAM_BLOCK( stream, renderer ) {
		auto index = 0;
		FOR( entry : editor->textureRegions ) {
			stream->color = colors[index % 3];
			pushQuad( stream, translate( entry * editor->textureScale, origin ) );
			++index;
		}
	}*/
	return false;
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
	if( !editor->initialized ) {
		*gui = defaultImmediateModeGui();
		imguiLoadDefaultStyle( gui, &app->platform );

		editor->textureRegions               = makeUArray( &app->stackAllocator, recti, 100 );
		editor->textureDisplay.selectedIndex = -1;
		editor->initialized                  = true;
	}
	setProjection( renderer, ProjectionType::Orthogonal );
	setRenderState( renderer, RenderStateType::DepthTest, false );
	renderer->color      = Color::White;
	renderer->clearColor = 0xFF1B2B34;

	rectf guiBounds = {0, 0, app->width, app->height};
	imguiBind( gui, renderer, font, inputs, app->stackAllocator.ptr, guiBounds );

	auto layout = imguiBeginColumn( 150 );
	if( imguiButton( "Load Texture" ) ) {
		char filename[260];
		auto len = app->platform.getOpenFilename( nullptr, "Data/Images", false, filename,
		                                          countof( filename ) );
		if( len ) {
			if( editor->texture ) {
				// app->platform.deleteTexture( editor->texture );
			}
			editor->texture       = app->platform.loadTexture( {filename, len} );
			auto info             = getTextureInfo( editor->texture );
			editor->textureWidth  = info->width;
			editor->textureHeight = info->height;
			editor->textureRegions.clear();
			imageFindRects( editor->textureRegions, info->image, {}, 0, false );
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

	imguiNextColumn( &layout, 400 );
	if( doTextureDisplay( app ) ) {
	}

	imguiUpdate( dt );
	imguiFinalize();
}