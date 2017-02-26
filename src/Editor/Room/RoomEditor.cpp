namespace RoomEditor
{

void doSidebar( AppData* app, GameInputs* inputs, rectfarg rect )
{
	auto editor = &app->roomEditorState;
	auto renderer = &app->renderer;

	setTexture( renderer, 0, null );
	renderer->color = 0xFF29373B;
	addRenderCommandSingleQuad( renderer, rect );

	renderer->color = Color::White;
	if( imguiBeginDropGroup( "Tools", &editor->flags, RoomEditorState::ToolsExpanded ) ) {
		imguiEndDropGroup();
	}
	if( imguiBeginDropGroup( "Tileset", &editor->flags, RoomEditorState::TilesetExpanded ) ) {
		imguiEndDropGroup();
	}
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

	// inputs->disableEscapeForQuickExit = true;

	imguiBind( gui );
	if( !editor->initialized ) {
		*gui = defaultImmediateModeGui();
		imguiLoadDefaultStyle( gui, &app->platform, font );

		editor->fileMenu = imguiGenerateContainer( gui, {}, ImGuiVisibility::Hidden );

		editor->initialized = true;;
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
		imguiMenuItem( itemHandle, "View" );
		imguiMenuItem( itemHandle, "Preferences" );
		imguiEndContainer();
	}

	imguiEndColumn( &layout );

	const float MenuLeft = 200;
	imguiNextColumn( &layout, MenuLeft );

	doSidebar( app, inputs, imguiPeekItem( MenuLeft ) );

	const float MappingClient = app->width - MenuLeft;
	imguiNextColumn( &layout, MappingClient );

	auto rect = imguiAddItem( MappingClient, app->height );

	imguiEndColumn( &layout );

	if( imguiContextMenu( editor->fileMenu ) ) {
		imguiContextMenuEntry( "New" );
		imguiContextMenuEntry( "Open" );
		imguiEndContainer();
	}

	imguiUpdate( dt );
	imguiFinalize();
}

} // namespace RoomEditor