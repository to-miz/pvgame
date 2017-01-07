constexpr const float PaddleWidth            = 5;
constexpr const float PaddleHeight           = 10;
constexpr const float KeyframePrecision      = 1.0f / 5.0f;
constexpr const float PropertiesColumnWidth  = 200;
constexpr const float NamesWidth             = 100;
constexpr const float RegionWidth            = 300;
constexpr const float RegionHeight           = 200;
static const vec3 AnimatorInitialRotation = {0, -0.2f, 0};

static const StringView NodeNames[] = {"Translation", "Rotation", "Scale"};

rectf getSelection( AnimatorState* animator )
{
	return correct( animator->selectionA, animator->selectionB );
}
rectf getAbsSelection( AnimatorState* animator, const ImGuiScrollableRegionResult* scrollable )
{
	auto selection = getSelection( animator );
	selection.left *= PaddleWidth;
	selection.right *= PaddleWidth;
	selection =
	    translate( selection, scrollable->inner.leftTop - animator->scrollableRegion.scrollPos );
	return selection;
}

bool compareKeyframes( const AnimatorKeyframe* a, const AnimatorKeyframe* b )
{
	return a->t < b->t;
}
void clearSelectedKeyframes( AnimatorState* animator )
{
	FOR( frame : animator->keyframes ) {
		frame->selected = false;
	}
	animator->selected.clear();
	animator->selectionRectValid = false;
	animator->clickedGroup       = -1;
}
void clearSelectedNodes( AnimatorState* animator )
{
	FOR( entry : animator->nodes ) {
		entry.selected = false;
	}
}

Array< AnimatorGroup > addAnimatorGroups( AnimatorState* animator, StringView parentName,
                                          Array< const StringView > childNames )
{
	// FIXME: the strings need to be stored in pools, otherwise dll hotloading breaks
	Array< AnimatorGroup > result = {};
	auto count                    = childNames.size() + 1;
	if( animator->groups.remaining() >= count ) {
		auto start = animator->groups.append( count, {} );
		result     = makeArrayView( start, animator->groups.end() );
		for( auto i = 0; i < count; ++i ) {
			auto entry      = &result[i];
			entry->name     = ( i == 0 ) ? parentName : childNames[i - 1];
			entry->id       = animator->ids;
			entry->expanded = true;
			++animator->ids;
		}
		if( count > 1 ) {
			result[0].children = {result[1].id, animator->ids};
		}
	}
	return result;
}

void doKeyframesNames( AppData* app, GameInputs* inputs, float width, float height )
{
	auto animator = &app->animatorState;
	auto renderer = &app->renderer;
	auto font     = ImGui->font;

	auto rect         = imguiAddItem( width, height );
	ClippingRect clip = {renderer, rect};

	const auto padding     = ImGui->style.innerPadding;
	const auto inner       = translate( rect, 0, -animator->scrollableRegion.scrollPos.y );
	const auto cellHeight  = PaddleHeight + padding * 2;
	const auto expandWidth = 10.0f;
	auto cell              = RectSetHeight( inner, cellHeight );
	cell.left += padding;

	renderer->color = 0xFF1D3E81;
	setTexture( renderer, 0, null );
	addRenderCommandSingleQuad( renderer, inner );

	renderer->color = Color::White;
	FOR( visible : animator->visibleGroups ) {
		auto textRect   = RectSetLeft( cell, cell.left + expandWidth + padding * 2 );
		if( visible.group < 0 || visible.children ) {
			auto expandRect = cell;
			expandRect.top += padding;
			expandRect.bottom = expandRect.top + expandWidth;
			expandRect.right  = expandRect.left + expandWidth;
			setTexture( renderer, 0, null );
			renderer->color = Color::White;
			addRenderCommandSingleQuad( renderer, expandRect );
		}
		if( auto group = find_first_where( animator->groups, it.id == visible.group ) ) {
			renderTextClipped( renderer, font, group->name, textRect );
		} else {
			renderTextClipped( renderer, font, "root", textRect );
		}
		cell = translate( cell, 0, cellHeight );
	}
}

void doKeyframesFrameNumbers( AppData* app, GameInputs* inputs, float width )
{
	auto animator = &app->animatorState;
	auto renderer = &app->renderer;
	auto font     = ImGui->font;

	const auto padding = ImGui->style.innerPadding;
	auto rect         = imguiInnerRect( imguiAddItem( width, stringHeight( font ) + padding * 2 ) );
	auto inner        = imguiInnerRect( rect );
	ClippingRect clip = {renderer, rect};

	setTexture( renderer, 0, null );
	renderer->color = Color::White;
	addRenderCommandSingleQuad( renderer, rect );

	const auto distance = PaddleWidth * 5;
	auto offset         = animator->scrollableRegion.scrollPos.x;
	auto start          = (int32)( offset / PaddleWidth );
	auto count          = width / distance + 1;
	auto frameNumber    = start - start % 5;
	auto x              = fmod( -offset, distance ) + inner.left;

	auto info = getFontInfo( font );
	setTexture( renderer, 0, info->ranges[0].texture );
	MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::Black;
		for( auto i = 0; i < count; ++i ) {
			renderText( stream, info, &font->renderOptions, font->verticalPadding,
			            toNumberString( frameNumber ), rectf{x, inner.top} );
			x += distance;
			frameNumber += 5;
		}
	}
}

void moveSelected( AnimatorState* animator, float movement )
{
	FOR( selected : animator->selected ) {
		selected->t += movement;
	}
	if( animator->selectionRectValid ) {
		animator->selectionA.x += movement;
		animator->selectionB.x += movement;
	}
}
void settleSelected( AnimatorState* animator, bool altDown )
{
	if( !altDown ) {
		FOR( selected : animator->selected ) {
			selected->t = round( selected->t );
		}
	} else {
		FOR( selected : animator->selected ) {
			selected->t -= fmod( selected->t, KeyframePrecision );
		}
	}
}

void doKeyframesSelection( AppData* app, GameInputs* inputs,
                           const ImGuiScrollableRegionResult* scrollable, bool clickedAny )
{
	auto animator = &app->animatorState;
	auto renderer = &app->renderer;
	auto width    = animator->scrollableRegion.dim.x;

	auto altDown = isKeyDown( inputs, KC_Alt );

	auto selectionHandle =
	    imguiMakeHandle( &animator->selectionRectValid, ImGuiControlType::Slider );
	if( isPointInside( scrollable->inner, inputs->mouse.position )
	    || imguiHasCapture( selectionHandle ) ) {

		if( !animator->mouseSelecting && isKeyPressed( inputs, KC_LButton ) && !clickedAny ) {
			imguiFocus( selectionHandle );
			imguiCapture( selectionHandle );
			auto absSelection = getAbsSelection( animator, scrollable );
			if( animator->selectionRectValid
			    && isPointInside( absSelection, inputs->mouse.position ) ) {
				// moving selection around
				animator->moveSelection = true;
				animator->mouseStart =
				    ( inputs->mouse.position - absSelection.leftTop ) / PaddleWidth;
			} else {
				animator->mouseSelecting     = true;
				animator->selectionRectValid = false;
				animator->selectionA         = inputs->mouse.position - scrollable->inner.leftTop
				                       - animator->scrollableRegion.scrollPos;
				animator->selectionA.x /= PaddleWidth;
				clearSelectedKeyframes( animator );
			}
		}

		if( animator->mouseSelecting ) {
			if( !isKeyDown( inputs, KC_LButton ) ) {
				animator->mouseSelecting = false;

				animator->selected.clear();
				// convert selection to time
				auto selection = getSelection( animator );
				auto allocator = &app->stackAllocator;
				TEMPORARY_MEMORY_BLOCK( allocator ) {
					// find groups that fall into selection
					auto groups = beginVector( allocator, GroupId );

					if( animator->visibleGroups.size() ) {
						auto groupHeight = PaddleHeight + ImGui->style.innerPadding;
						auto firstGroup  = (int32)floor( selection.top / groupHeight );
						auto lastGroup   = (int32)ceil( selection.bottom / groupHeight );
						firstGroup = clamp( firstGroup, 0, animator->visibleGroups.size() - 1 );
						lastGroup  = clamp( lastGroup, 0, animator->visibleGroups.size() - 1 );
						if( firstGroup == 0 ) {
							// top level group is every visible group
							FOR( visible : animator->visibleGroups ) {
								if( visible.group >= 0 ) {
									groups.push_back( visible.group );
								}
							}
						} else {
							for( auto i = firstGroup; i <= lastGroup; ++i ) {
								auto visible = &animator->visibleGroups[i];
								append_unique( groups, visible->group );
								if( visible->children && visible->group >= 0 ) {
									// group is parent group, add all child groups
									auto children = visible->children;
									for( auto i = children.min; i < children.max; ++i ) {
										append_unique( groups, i );
									}
								}
							}
						}
					}

					float minT = FLOAT_MAX;
					FOR( frame : animator->keyframes ) {
						if( frame->t + 1 >= selection.left && frame->t < selection.right
						    && exists( groups, frame->group ) ) {

							frame->selected = true;
							animator->selected.push_back( frame );
							animator->selectionRectValid = true;
							if( minT > frame->t ) {
								minT = frame->t;
							}
						} else {
							frame->selected = false;
						}
					}
					animator->selectedMinT =
					    minT - min( animator->selectionA.x, animator->selectionB.x );
				}
			} else {
				animator->selectionB = inputs->mouse.position - scrollable->inner.leftTop
				                       - animator->scrollableRegion.scrollPos;
				animator->selectionB.x /= PaddleWidth;
			}
		}
		if( !isKeyDown( inputs, KC_LButton ) ) {
			if( animator->moveSelection ) {
				settleSelected( animator, altDown );
			}
			animator->moveSelection = false;
		}
		if( animator->selectionRectValid && animator->moveSelection && animator->selected.size() ) {
			// convert absolute mouse coordinates to frame time
			auto framePos = ( inputs->mouse.position.x - scrollable->inner.left ) / PaddleWidth;
			// offset framePos by mouse start so that movement is relative to where we first clicked
			auto pos     = framePos - animator->mouseStart.x;
			// range of allowed movement
			auto start   = -animator->selectedMinT;
			auto end     = width / PaddleWidth + start;

			float offset = 0;
			if( pos >= start && pos <= end ) {
				offset = pos;
			} else if( pos < start ) {
				offset = start;
			} else {
				offset = end;
			}
			offset -= min( animator->selectionA.x, animator->selectionB.x );

			moveSelected( animator, offset );
		} else {
			animator->moveSelection = false;
		}
	}
	if( animator->mouseSelecting || animator->selectionRectValid ) {
		RENDER_COMMANDS_STATE_BLOCK( renderer ) {
			renderer->color = setAlpha( Color::Blue, 0x80 );
			auto selection = getAbsSelection( animator, scrollable );
			addRenderCommandSingleQuad( renderer, selection );
		}
	}
}
bool doKeyframesDisplay( AppData* app, GameInputs* inputs, const GroupId group )
{
	auto renderer = &app->renderer;
	auto animator = &app->animatorState;
	auto gui      = &animator->gui;

	auto width   = animator->scrollableRegion.dim.x;
	auto altDown = isKeyDown( inputs, KC_Alt );

	setTexture( renderer, 0, null );
	if( animator->selectionRectValid
	    && isPointInside( getSelection( animator ), inputs->mouse.position ) ) {
		gui->processInputs = false;
	}
	bool clickedAny = false;
	float clickedT  = 0;
	float movement  = 0;
	auto parent     = find_first_where( animator->groups, it.id == group );
	auto isParent   = ( parent && !isEmpty( parent->children ) );
	MESH_STREAM_BLOCK( stream, renderer ) {
		Color colors[] = {Color::White, Color::Red};
		FOR( frame : animator->keyframes ) {
			if( group >= 0 && !isParent && frame->group != group ) {
				continue;
			}
			if( isParent && !isInRange( parent->children, frame->group ) ) {
				continue;
			}
			auto oldT   = frame->t;
			auto custom = imguiCustomSlider( &frame->t, 0, width / PaddleWidth,
			                                 width + ImGui->style.innerPadding * 2 + PaddleWidth,
			                                 PaddleWidth, PaddleHeight, false );
			if( custom.clicked() ) {
				clickedAny = true;
				clickedT   = oldT;
				if( !isKeyDown( inputs, KC_Control ) ) {
					if( !frame->selected || group != animator->clickedGroup ) {
						clearSelectedKeyframes( animator );
						frame->selected = true;
						animator->selected.push_back( frame );
					}
				} else {
					frame->selected = !frame->selected;
					if( frame->selected ) {
						animator->selected.push_back( frame );
					} else {
						find_and_erase( animator->selected, frame );
					}
				}
				animator->clickedGroup = group;
				animator->moving = true;
			}
			if( custom.changed() ) {
				animator->moving = true;
				if( frame->selected ) {
					movement = frame->t - oldT;
				}
				frame->t = oldT;
			}
			assert( frame->selected.value < countof( colors ) );
			stream->color = colors[frame->selected.value];
			pushQuad( stream, custom.sliderRect );
		}
	}
	imguiAddItem( width, PaddleHeight + ImGui->style.innerPadding );
	bool isRoot = group < 0;
	if( ( isParent || isRoot ) && clickedAny ) {
		// select any frame which is whithin equal range of t
		FOR( frame : animator->keyframes ) {
			if( ( isRoot || isInRange( parent->children, group ) )
			    && floatEqSoft( frame->t, clickedT ) ) {

				frame->selected = true;
				append_unique( animator->selected, frame );
			}
		}
	}
	if( animator->moving ) {
		moveSelected( animator, movement );
	}
	if( !clickedAny && animator->moving && animator->selected.size()
	    && !isKeyDown( inputs, KC_LButton ) ) {

		animator->moving = false;
		settleSelected( animator, altDown );
	}
	if( !clickedAny && !isKeyDown( inputs, KC_LButton ) ) {
		animator->moving = false;
	}
	gui->processInputs = true;
	if( animator->selected.empty() ) {
		animator->selectionRectValid = false;
	}
	return clickedAny;
}

void populateVisibleGroups( AnimatorState* animator )
{
	animator->visibleGroups.clear();
	animator->visibleGroups.push_back( {-1, -1} );
	FOR( entry : animator->groups ) {
		animator->visibleGroups.push_back( {entry.id, entry.children} );
	}
}

void doKeyframesControl( AppData* app, GameInputs* inputs )
{
	auto animator = &app->animatorState;

	auto layout = imguiBeginColumn( NamesWidth );
	imguiAddItem( NamesWidth, ImGui->style.innerPadding * 2 + stringHeight( ImGui->font )  );
	doKeyframesNames( app, inputs, NamesWidth, RegionHeight );
	imguiNextColumn( &layout, RegionWidth );

	doKeyframesFrameNumbers( app, inputs, RegionWidth );
	auto scrollable =
	    imguiBeginScrollableRegion( &animator->scrollableRegion, RegionWidth, RegionHeight, true );
	bool clickedAny = false;
	FOR( visible : animator->visibleGroups ) {
		if( doKeyframesDisplay( app, inputs, visible.group ) ) {
			clickedAny = true;
		}
	}
	animator->duration = ( *max_element( animator->keyframes, compareKeyframes ) )->t;
	doKeyframesSelection( app, inputs, &scrollable, clickedAny );
	imguiEndScrollableRegion( &animator->scrollableRegion, &scrollable );
}

void doProperties( AppData* app, GameInputs* inputs, rectfarg rect )
{
	auto renderer = &app->renderer;
	auto animator = &app->animatorState;
	auto editor   = &animator->editor;

	setTexture( renderer, 0, null );
	renderer->color = 0xFF29373B;
	addRenderCommandSingleQuad( renderer, rect );

	auto doRotation = []( StringView text, float* val ) {
		imguiSameLine( 2 );
		auto handle = imguiMakeHandle( val, ImGuiControlType::None );
		float adjusted = radiansToDegrees( *val );
		if( imguiEditbox( handle, text, &adjusted ) ) {
			*val = degreesToRadians( adjusted );
		}
		if( imguiSlider( handle, &adjusted, -180, 180 ) ) {
			*val = degreesToRadians( adjusted );
		}
	};

	renderer->color = Color::White;
	if( imguiBeginDropGroup( "Editor", &editor->editorSettingsExpanded ) ) {
		imguiSlider( "Scale", &editor->scale, 0.1f, 10.0f );
		if( imguiButton( "Reset View" ) ) {
			editor->translation = {};
			editor->rotation    = AnimatorInitialRotation;
			editor->scale       = 1;
		}
		imguiEndDropGroup();
	}
	if( imguiBeginDropGroup( "Properties", &editor->propertiesExpanded ) ) {
		FOR( node : animator->nodes ) {
			if( node.selected ) {
				imguiEditbox( "Translation X", &node.translation.x );
				imguiEditbox( "Translation Y", &node.translation.y );
				imguiEditbox( "Translation Z", &node.translation.z );
				doRotation( "Rotation X", &node.rotation.x );
				doRotation( "Rotation Y", &node.rotation.y );
				doRotation( "Rotation Z", &node.rotation.z );
				break;
			}
		}
	}
}

void doEditor( AppData* app, GameInputs* inputs, rectfarg rect )
{
	auto renderer = &app->renderer;
	auto animator = &app->animatorState;
	auto editor   = &animator->editor;
	auto handle   = imguiMakeHandle( editor, ImGuiControlType::None );

	auto aspect = width( rect ) / height( rect );
	// we need to translate in homogeneous coordinates to offset the origin of the projection matrix
	// to the center of the editor frame, hence the division by app->width
	auto offset = matrixTranslation( rect.left / app->width, 0, 0 );
	auto perspective = matrixPerspectiveFovProjection( degreesToRadians( 65 ), aspect, -1, 1 );
	// translate origin first, then apply perspective matrix
	auto projection = perspective * offset;
	setProjectionMatrix( renderer, ProjectionType::Perspective, projection );
	setProjection( renderer, ProjectionType::Perspective );
	setRenderState( renderer, RenderStateType::Scissor, true );
	setScissorRect( renderer, Rect< int32 >( rect ) );

	auto mat = matrixRotationY( editor->rotation.x ) * matrixRotationX( editor->rotation.y )
	           * matrixTranslation( editor->translation )
	           * matrixScale( editor->scale, editor->scale, 1 ) * matrixTranslation( 0, 0, 200 );
	renderer->view = mat;

	auto handleMouse = [&]( uint8 button,
	                        void ( *proc )( AnimatorEditor * editor, GameInputs * inputs ) ) {
		auto pressed = isKeyPressed( inputs, button );
		if( ( isPointInside( rect, inputs->mouse.position ) && pressed )
		    || imguiHasCapture( handle ) ) {

			if( pressed ) {
				imguiFocus( handle );
				imguiCapture( handle, button );
			}
			if( imguiIsHover( handle ) && isKeyDown( inputs, button ) ) {
				proc( editor, inputs );
			}
		}
	};

	handleMouse( KC_RButton, []( AnimatorEditor* editor, GameInputs* inputs ) {
		auto delta = inputs->mouse.delta * 0.01f;
		editor->rotation.x -= delta.x;
		editor->rotation.y -= delta.y;
	} );
	handleMouse( KC_MButton, []( AnimatorEditor* editor, GameInputs* inputs ) {
		auto delta = inputs->mouse.delta * 0.5f / editor->scale;
		editor->translation.x += delta.x;
		editor->translation.y -= delta.y;
	} );
	if( ( isPointInside( rect, inputs->mouse.position ) || imguiHasFocus( handle ) )
	    && !floatEqZero( inputs->mouse.wheel ) ) {

		editor->scale += inputs->mouse.wheel * 0.1f;
	}

	// render grid
	setTexture( renderer, 0, null );
	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		rectf bounds  = {-8 * 12, -8 * 12, 8 * 12, 8 * 12};
		stream->color = 0x80FFFFFF;
		for( auto i = 0; i < 13; ++i ) {
			float x = i * 16.0f - 8 * 12.0f;
			pushLine( stream, {x, 0, -bounds.top}, {x, 0, -bounds.bottom} );
		}
		for( auto i = 0; i < 13; ++i ) {
			float z = i * 16.0f - 8 * 12;
			pushLine( stream, {bounds.left, 0, z}, {bounds.right, 0, z} );
		}
	}

	// update transforms
	FOR( node : animator->nodes ) {
		node.local = matrixRotation( node.rotation ) * matrixTranslation( node.translation );
		node.world = node.local;
	}

	// render skeleton
	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::White;
		FOR( node : animator->nodes ) {
			auto& mat     = node.world;
			vec3 origin   = {};
			vec3 base     = transformVector( mat, origin );
			vec3 head     = transformVector( mat, vec3{node.length} );
			stream->color = Color::Blue;
			pushLine( stream, origin, base );
			stream->color = Color::White;
			pushLine( stream, base, head );
		}
	}

	setProjection( renderer, ProjectionType::Orthogonal );

	// render controls
	auto viewProj = renderer->view * projection;
	// auto viewProj = mat * perspective;
	bool selectedAny = false;
	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::White;
		FOR( node : animator->nodes ) {
			auto mat      = node.world * viewProj;
			auto head     = toScreenSpace( mat, {node.length}, app->width, app->height );
			auto headRect = RectHalfSize( head, 5, 5 );
			if( isPointInside( headRect, inputs->mouse.position )
			    && isKeyPressed( inputs, KC_LButton ) ) {

				selectedAny = true;
				if( isKeyDown( inputs, KC_Control ) ) {
					node.selected = !node.selected;
				} else {
					if( !node.selected ) {
						clearSelectedNodes( animator );
						node.selected = true;
					} else {
						editor->moving      = true;
						editor->mouseOffset = head - inputs->mouse.position;
						editor->clickedNode = &node;
					}
				}
			}

			stream->color = ( node.selected ) ? ( Color::Red ) : ( Color::White );
			pushQuadOutline( stream, headRect );
		}
	}
	if( isPointInside( rect, inputs->mouse.position ) && isKeyPressed( inputs, KC_LButton )
	    && !selectedAny ) {

		clearSelectedNodes( animator );
		editor->moving = false;
	}
	if( isKeyUp( inputs, KC_LButton ) ) {
		editor->moving = false;
	}
	if( editor->moving && hasMagnitude( inputs->mouse.delta ) ) {
		// rotate selected nodes along plane
		if( auto node = editor->clickedNode ) {
			mat4 inv;
			inverse( viewProj, &inv );
			auto mat         = node->world;
			auto base        = transformVector3( mat, {} );
			vec3 head        = transformVector3( mat, {node->length} );
			vec3 planeNormal = {0, 0, -1};
			auto mouse       = inputs->mouse.position + editor->mouseOffset;
			vec3 rayStart    = toWorldSpace( inv, Vec3( mouse, 0 ), app->width, app->height );
			vec3 rayEnd      = toWorldSpace( inv, Vec3( mouse, 1 ), app->width, app->height );
			vec3 rayDir      = rayEnd - rayStart;
			vec3 rayOrigin   = rayStart;

			if( auto result = testRayVsPlane( rayOrigin, rayDir, base, planeNormal ) ) {
				auto intersection      = rayOrigin + rayDir * result.t;
				auto deltaIntersection = intersection - base;
				auto deltaHead         = head - base;
				auto deltaRotation     = angle( deltaIntersection, deltaHead, planeNormal );
				node->rotation.z       = simplifyAngle( node->rotation.z + deltaRotation );
			}
		}
	}

	setRenderState( renderer, RenderStateType::Scissor, false );
}

void doAnimator( AppData* app, GameInputs* inputs, bool focus, float dt )
{
	auto renderer = &app->renderer;
	auto font     = &app->font;
	auto animator = &app->animatorState;
	auto gui      = &animator->gui;

	if( !focus ) {
		return;
	}

	inputs->disableEscapeForQuickExit = false;
	imguiBind( gui );
	if( !animator->initialized ) {
		*gui = defaultImmediateModeGui();
		imguiLoadDefaultStyle( gui, &app->platform );

		auto allocator          = &app->stackAllocator;
		animator->keyframesPool = makeUArray( allocator, AnimatorKeyframe, 10 );
		animator->keyframes     = makeUArray( allocator, AnimatorKeyframe*, 10 );
		animator->selected      = makeUArray( allocator, AnimatorKeyframe*, 10 );
		animator->groups        = makeUArray( allocator, AnimatorGroup, 10 );
		animator->visibleGroups = makeUArray( allocator, AnimatorGroupDisplay, 10 );
		animator->nodes         = makeUArray( allocator, AnimatorNode, 10 );

		addAnimatorGroups( animator, "Test", makeArrayView( NodeNames ) );
		addAnimatorGroups( animator, "Test2", makeArrayView( NodeNames ) );
		populateVisibleGroups( animator );
		animator->keyframesPool.assign( {{0, 0, 1}, {20, 0, 1}, {35, 0, 3}, {200, 0, 1}} );
		auto pool = animator->keyframesPool;
		animator->keyframes.assign( {&pool[0], &pool[1], &pool[2], &pool[3]} );

		animator->duration = ( *max_element( animator->keyframes, compareKeyframes ) )->t;

		animator->editor.rotation = AnimatorInitialRotation;
		animator->editor.scale    = 1;

#if 1
		// debug
		animator->nodes.push_back( {{}, {0, 0, HalfPi32}, 40, {1}} );
#endif

		animator->initialized = true;
	}

	renderer->color      = Color::White;
	renderer->clearColor = 0xFF1B2B34;

	setProjection( renderer, ProjectionType::Orthogonal );
	setRenderState( renderer, RenderStateType::DepthTest, false );

	rectf guiBounds = {0, 0, app->width, app->height};
	imguiBind( gui, renderer, font, inputs, app->stackAllocator.ptr, guiBounds );

	auto width                     = animator->duration + RegionWidth;
	animator->scrollableRegion.dim = {width, RegionHeight};

	auto mainRowHeight  = app->height - RegionHeight - 30;
	auto editorWidth    = app->width - PropertiesColumnWidth;
	auto layout         = imguiBeginColumn( PropertiesColumnWidth );

	doProperties( app, inputs, RectSetHeight( imguiCurrentContainer()->rect, mainRowHeight ) );

	imguiNextColumn( &layout, editorWidth );
	auto editorRect = imguiAddItem( editorWidth, mainRowHeight );
	doEditor( app, inputs, editorRect );

	imguiEndColumn( &layout );

	doKeyframesControl( app, inputs );

	imguiUpdate( dt );
	imguiFinalize();
}