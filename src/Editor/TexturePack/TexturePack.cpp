void doTexturePack( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto renderer = &app->renderer;
	auto font     = &app->font;
	auto editor   = &app->texturePackState;
	auto gui      = &editor->gui;

	if( !focus ) {
		return;
	}
	static int32 modal;
	static int32 other;
	if( !editor->initialized ) {
		*gui = defaultImmediateModeGui();
		imguiLoadDefaultStyle( gui, &app->platform );
		modal = imguiGenerateContainer( gui );
		other = imguiGenerateContainer( gui );
		imguiGetContainer( gui, modal )->setHidden( true );
		editor->initialized = true;
	}
	setProjection( renderer, ProjectionType::Orthogonal );
	setRenderState( renderer, RenderStateType::DepthTest, false );
	renderer->color      = Color::White;
	renderer->clearColor = 0xFF1B2B34;

	rectf guiBounds = {0, 0, app->width, app->height};
	imguiBind( gui, renderer, font, inputs, app->stackAllocator.ptr, guiBounds );

	auto layout = imguiBeginColumn( 150 );
	if( imguiButton( "Reset Player Position" ) ) {
		imguiShowModal( modal );
	}
	if( imguiButton( "Reset Player Position" ) ) {
	}
	if( imguiButton( "Reset Player Position" ) ) {
	}

	imguiFitContainerToSize();
	imguiNextColumn( &layout, 150 );
	if( imguiButton( "Reset Player Position" ) ) {
	}
	if( imguiButton( "Reset Player Position" ) ) {
	}
	if( imguiButton( "Reset Player Position" ) ) {
	}

	static ImGuiListboxItem items[] = {
	    {"a"}, {"b"},  {"0"},  {"1"},  {"2"},  {"3"},  {"4"},  {"5"},  {"6"},  {"7"},  {"8"},
	    {"9"}, {"10"}, {"11"}, {"12"}, {"13"}, {"14"}, {"15"}, {"16"}, {"17"}, {"18"}, {"19"},
	};
	static float scrollPos;
	imguiListbox( &scrollPos, makeArrayView( items ), 150, 100, false );

	imguiNextColumn( &layout, 400 );
	{
		static vec2 scrollPos = {};
		imguiScrollable( &scrollPos, {0, 0, 800, 800}, 400, 400 );
	}

	if( imguiDialog( "Modal", modal ) ) {
		if( imguiButton( "Close" ) ) {
			imguiClose( modal );
		}
	}

	if( imguiDialog( "other", other ) ) {
		if( imguiButton( "Close" ) ) {
			imguiClose( other );
		}
	}

	imguiUpdate( dt );
	imguiFinalize();
}