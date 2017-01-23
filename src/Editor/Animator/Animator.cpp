constexpr const float PaddleWidth            = 5;
constexpr const float PaddleHeight           = 10;
constexpr const float KeyframePrecision      = 1.0f / 5.0f;
constexpr const float PropertiesColumnWidth  = 200;
constexpr const float NamesWidth             = 100;
constexpr const float RegionHeight           = 200;

static const vec3 AnimatorInitialRotation = {0, -0.2f, 0};
constexpr const float AnimatorMinScale = 0.1f;
constexpr const float AnimatorMaxScale = 10.0f;

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
		entry->selected = false;
	}
	animator->editor.clickedNode = nullptr;
}

Array< AnimatorGroup > addAnimatorGroups( AnimatorState* animator, StringView parentName,
                                          Array< const StringView > childNames )
{
	Array< AnimatorGroup > result = {};
	auto count                    = childNames.size() + 1;
	if( animator->groups.remaining() >= count ) {
		auto start = animator->groups.append( count, {} );
		result     = makeArrayView( start, animator->groups.end() );
		for( auto i = 0; i < count; ++i ) {
			auto entry      = &result[i];
			entry->name     = ( i == 0 ) ? parentName : childNames[i - 1];
			entry->id       = animator->ids;
			entry->expanded = false;
			++animator->ids;
		}
		if( count > 1 ) {
			result[0].children = {result[1].id, animator->ids};
		}
	}
	return result;
}
void clearAnimatorGroups( AnimatorState* animator )
{
	animator->ids = 0;
	animator->visibleGroups.clear();
	animator->groups.clear();
}

void populateVisibleGroups( AnimatorState* animator )
{
	animator->visibleGroups.clear();
	animator->visibleGroups.push_back( {-1, -1} );
	if( animator->timelineRootExpanded ) {
		for( auto it = animator->groups.begin(), end = animator->groups.end(); it < end; ++it ) {
			animator->visibleGroups.push_back( {it->id, it->children} );
			if( !it->expanded && it->children ) {
				it += width( it->children );
			}
		}
	}
}

AnimatorKeyframeData lerp( float t, const AnimatorKeyframeData& a, const AnimatorKeyframeData& b )
{
	AnimatorKeyframeData result = {};
	assert( a.type == b.type );
	switch( a.type ) {
		case AnimatorKeyframeData::type_translation: {
			result.type = AnimatorKeyframeData::type_translation;
			result.translation = lerp( t, a.translation, b.translation );
			break;
		}
		case AnimatorKeyframeData::type_rotation: {
			result.type = AnimatorKeyframeData::type_rotation;
			result.rotation = lerp( t, a.rotation, b.rotation );
			break;
		}
		case AnimatorKeyframeData::type_scale: {
			result.type = AnimatorKeyframeData::type_scale;
			result.scale = lerp( t, a.scale, b.scale );
			break;
		}
		case AnimatorKeyframeData::type_frame: {
			result.type = AnimatorKeyframeData::type_frame;
			result.frame = (int16)lerp( t, (float)a.frame, (float)b.frame );
			break;
		}
			InvalidDefaultCase;
	}
	return result;
}
AnimatorKeyframe** lowerBoundKeyframeIt( AnimatorState* animator, float t, GroupId group )
{
	auto first = animator->keyframes.begin();
	auto last = animator->keyframes.end();
	auto it = lower_bound( first, last, t, []( const AnimatorKeyframe* keyframe, float t ) {
		return keyframe->t < t;
	} );
	if( first != last ) {
		if( it == last ) {
			--it;
		}
		if( ( *it )->group == group && ( *it )->t <= t ) {
			return it;
		}
		--it;
		while( it >= first && ( *it )->group != group ) {
			--it;
		}
		if( it < first ) {
			it = last;
		}
		return it;
	} else {
		return it;
	}
}
AnimatorKeyframe* lowerBoundKeyframe( AnimatorState* animator, float t, GroupId group )
{
	auto last = animator->keyframes.end();
	auto it   = lowerBoundKeyframeIt( animator, t, group );
	if( it != last ) {
		return *it;
	}
	return nullptr;
}
AnimatorKeyframeData getInterpolatedKeyframe( AnimatorState* animator, float t, GroupId group )
{
	AnimatorKeyframeData result = {};
	auto last                   = animator->keyframes.end();
	auto first                  = lowerBoundKeyframeIt( animator, t, group );
	if( first != last && ( *first )->group == group ) {
		auto a = *first;
		result = a->data;
		if( a->t < t && first + 1 != last ) {
			auto second = first + 1;
			while( second != last && ( *second )->group != group ) {
				++second;
			}
			if( second != last && ( *second )->group == group ) {
				auto b = *second;
				assert( b->t >= a->t );
				assert( a->data.type == b->data.type );

				auto duration = b->t - a->t;
				if( duration > 0 ) {
					auto adjustedT = ( t - a->t ) / duration;
					result         = lerp( adjustedT, a->data, b->data );
				} else {
					result = b->data;
				}
			}
		}
	}
	return result;
}

AnimatorNode getCurrentFrameNode( AnimatorState* animator, AnimatorNode* node )
{
	auto base   = *find_first_where( animator->baseNodes, entry->id == node->id );
	auto result = *base;

	auto group            = find_first_where( animator->groups, entry.id == node->group );
	GroupId translationId = group->id + 1;
	GroupId rotationId    = group->id + 2;
	GroupId scaleId       = group->id + 3;
	GroupId frameId       = group->id + 4;
	assert( width( group->children ) == 4 );

	auto translationKeyframe =
	    getInterpolatedKeyframe( animator, animator->currentFrame, translationId );
	auto rotationKeyframe = getInterpolatedKeyframe( animator, animator->currentFrame, rotationId );
	auto scaleKeyframe    = getInterpolatedKeyframe( animator, animator->currentFrame, scaleId );
	auto frameKeyframe    = getInterpolatedKeyframe( animator, animator->currentFrame, frameId );

	result.translation =
	    get_variant_or_default( translationKeyframe, translation, base->translation );
	result.rotation = get_variant_or_default( rotationKeyframe, rotation, base->rotation );
	result.scale    = get_variant_or_default( scaleKeyframe, scale, base->scale );
	result.frame    = get_variant_or_default( frameKeyframe, frame, base->frame );

	result.parent   = node->parent;
	result.selected = node->selected;
	result.group    = node->group;
	return result;
}
AnimatorNode toAbsoluteNode( const AnimatorNode* base, const AnimatorNode* rel )
{
	AnimatorNode result = *base;
	result.translation += rel->translation;
	result.rotation += rel->rotation;
	result.scale = multiplyComponents( result.scale, rel->scale );
	result.frame += rel->frame;
	return result;
}
AnimatorNode toRelativeNode( const AnimatorNode* base, const AnimatorNode* abs )
{
	AnimatorNode result = *base;
	result.translation  = abs->translation - base->translation;
	result.rotation     = abs->rotation - base->rotation;
	result.scale.x      = safeDivide( abs->scale.x, base->scale.x, 1 );
	result.scale.y      = safeDivide( abs->scale.y, base->scale.y, 1 );
	result.scale.z      = safeDivide( abs->scale.z, base->scale.z, 1 );
	result.frame        = abs->frame - base->frame;
	return result;
}

AnimatorNode* allocateNode( AnimatorState* animator )
{
	AnimatorNode* result = allocateStruct( &animator->nodeAllocator, AnimatorNode );
	if( result ) {
		*result       = {};
		result->voxel = -1;
	}
	return result;
}
void bindParent( AnimatorState* animator, AnimatorNode* node )
{
	node->parent = nullptr;
	if( node->parentId >= 0 ) {
		auto id = node->parentId;
		if( auto parent = find_first_where( animator->nodes, entry->id == id ) ) {
			node->parent = *parent;
			++node->parent->childrenCount;
		}
	}
	if( !node->parent ) {
		node->parentId = -1;
	}
}
AnimatorNode* allocateNode( AnimatorState* animator, const AnimatorNode& node )
{
	auto result = allocateNode( animator );
	if( result ) {
		*result    = node;
		result->id = animator->nodeIds++;
		bindParent( animator, result );
		if( result->name.size() <= 0 ) {
			auto idString      = toNumberString( result->id );
			result->name.resize( snprint( result->name.data(), result->name.capacity(),
			                              "Unnamed {}", StringView{idString} ) );
		}
	}
	return result;
}
AnimatorNode* addNewNode( AnimatorState* animator, const AnimatorNode& node )
{
	auto result = allocateNode( animator );
	if( result ) {
		*result       = node;
		result->id    = animator->nodeIds++;
		result->voxel = -1;
		if( result->scale.x == 0 ) {
			result->scale.x = 1;
		}
		if( result->scale.y == 0 ) {
			result->scale.y = 1;
		}
		if( result->scale.z == 0 ) {
			result->scale.z = 1;
		}
		bindParent( animator, result );
		if( result->name.size() <= 0 ) {
			auto idString      = toNumberString( result->id );
			result->name.resize( snprint( result->name.data(), result->name.capacity(),
			                              "Unnamed {}", StringView{idString} ) );
		}
		animator->nodes.push_back( result );
		auto groups =
		    addAnimatorGroups( animator, result->name, makeArrayView( animator->fieldNames ) );
		if( groups.size() ) {
			result->group = groups[0].id;
		} else {
			result->group = -1;
		}
	}
	return result;
}
AnimatorNode* addExistingNode( AnimatorState* animator, const AnimatorNode& node )
{
	auto result = allocateNode( animator );
	if( result ) {
		*result               = node;
		result->parent        = nullptr;
		result->childrenCount = 0;
		animator->nodes.push_back( result );
		auto groups =
		    addAnimatorGroups( animator, result->name, makeArrayView( animator->fieldNames ) );
		if( groups.size() ) {
			result->group = groups[0].id;
		} else {
			result->group = -1;
		}
	}
	return result;
}

void sortNodes( AnimatorState* animator )
{
	sort( animator->nodes.begin(), animator->nodes.end(),
	      []( const AnimatorNode* a, const AnimatorNode* b ) {
		      return a->childrenCount > b->childrenCount;
		  } );
}
void sortKeyframes( AnimatorState* animator )
{
	sort( animator->keyframes.begin(), animator->keyframes.end(),
	      []( const AnimatorKeyframe* a, const AnimatorKeyframe* b ) { return a->t < b->t; } );
}

AnimatorKeyframe* allocateKeyframe( AnimatorState* animator, const AnimatorKeyframe& key )
{
	auto result = allocateStruct( &animator->keyframesAllocator, AnimatorKeyframe );
	if( result ) {
		*result = key;
	}
	return result;
}
void addKeyframe( AnimatorState* animator, const AnimatorKeyframe& key )
{
	auto existing = find_first_where( animator->keyframes,
	                                  entry->group == key.group && floatEqSoft( entry->t, key.t ) );
	if( existing ) {
		**existing = key;
	} else if( animator->keyframes.remaining() ) {
		auto added = allocateStruct( &animator->keyframesAllocator, AnimatorKeyframe );
		*added     = key;
		animator->keyframes.push_back( added );
		insertion_sort_last_elem(
		    animator->keyframes.begin(), animator->keyframes.end(),
		    []( const AnimatorKeyframe* a, const AnimatorKeyframe* b ) { return a->t < b->t; } );
	}
}
// delete duplicate keyframes, keeping those that are selected
void deleteDuplicateKeyframes( AnimatorState* animator )
{
	if( !animator->currentAnimation ) {
		return;
	}
	FOR( selected : animator->selected ) {
		assert( selected->selected );
		auto range = equal_range(
		    animator->keyframes.begin(), animator->keyframes.end(), selected,
		    []( const AnimatorKeyframe* a, const AnimatorKeyframe* b ) { return a->t < b->t; } );
		auto group = selected->group;
		erase_if( animator->keyframes, range.first, range.second,
		          [animator, group]( AnimatorKeyframe* key ) {
			          if( !key->selected && key->group == group ) {
				          freeStruct( &animator->keyframesAllocator, key );
				          return true;
			          }
			          return false;
			      } );
	}
}

void openAnimation( AnimatorState* animator, AnimatorAnimation* animation )
{
	assert( !animator->currentAnimation );
	animator->currentAnimation = animation;

	animator->baseNodes.assign( animator->nodes );
	animator->nodes.clear();
	// clear marked flag
	FOR( node : animator->baseNodes ) {
		node->marked = false;
	}

	clearAnimatorGroups( animator );

	FOR( node : animation->nodes ) {
		auto baseIt = find_first_where( animator->baseNodes, entry->id == node.id );
		if( baseIt ) {
			auto base = *baseIt;
			assert( !base->marked );
			if( base->marked ) {
				continue;
			}
			base->marked = true;
			addExistingNode( animator, toAbsoluteNode( base, &node ) );
		}
	}
	FOR( base : animator->baseNodes ) {
		if( !base->marked ) {
			addExistingNode( animator, *base );
		}
	}

	// reassign parents
	FOR( node : animator->nodes ) {
		bindParent( animator, node );
	}
	sortNodes( animator );
	clearSelectedNodes( animator );

	populateVisibleGroups( animator );
}

void closeAnimation( StackAllocator* allocator, AnimatorState* animator )
{
	assert( animator->currentAnimation );

	// TODO: handle keyframes

	auto animation = animator->currentAnimation;
	if( animation->nodes.size() < animator->nodes.size() ) {
		animation->nodes = makeArray( allocator, AnimatorNode, animator->nodes.size() );
	}

	if( animation->nodes.size() == animator->nodes.size() ) {
		zip_for( animation->nodes, animator->nodes ) {
			auto baseIt = find_first_where( animator->baseNodes, entry->id == ( *second )->id );
			if( baseIt ) {
				auto base = *baseIt;
				*first    = toRelativeNode( base, ( *second ) );
			}
		}
	} else {
		LOG( ERROR, "out of memory for animation nodes" );
	}

	// free node memory
	FOR( node : animator->nodes ) {
		freeStruct( &animator->nodeAllocator, node );
	}

	clearAnimatorGroups( animator );
	animator->nodes.assign( animator->baseNodes );
	animator->currentAnimation = nullptr;
	clearSelectedNodes( animator );
}

void doKeyframesNames( AppData* app, GameInputs* inputs, float width, float height )
{
	using namespace ImGuiTexCoords;

	auto animator = &app->animatorState;
	auto renderer = &app->renderer;
	auto font     = ImGui->font;
	auto style    = &ImGui->style;

	auto rect         = imguiAddItem( width, height );
	ClippingRect clip = {renderer, rect};

	const auto padding      = style->innerPadding;
	const auto inner        = translate( rect, 0, -animator->scrollableRegion.scrollPos.y );
	const auto cellHeight   = PaddleHeight + padding * 2;
	const auto expandWidth  = ::width( style->rects[ExpandBox] );
	const auto expandHeight = ::height( style->rects[ExpandBox] );
	auto cell               = RectSetHeight( inner, cellHeight );
	cell.top += style->innerPadding;
	cell.bottom += style->innerPadding;
	cell.left += padding;

	renderer->color = 0xFF1D3E81;
	setTexture( renderer, 0, null );
	addRenderCommandSingleQuad( renderer, inner );

	bool repopulateVisibleGroups = false;
	renderer->color              = Color::White;
	FOR( visible : animator->visibleGroups ) {
		auto group    = find_first_where( animator->groups, entry.id == visible.group );
		auto textRect = RectSetLeft( cell, cell.left + expandWidth + padding * 2 );
		if( visible.group < 0 || visible.children ) {
			auto expandRect = cell;
			expandRect.top += padding;
			expandRect.bottom = expandRect.top + expandHeight;
			expandRect.right  = expandRect.left + expandWidth;
			setTexture( renderer, 0, null );
			renderer->color = Color::White;
			int32 typeIndex = ExpandBox;
			if( ( !group && animator->timelineRootExpanded ) || ( group && group->expanded ) ) {
				typeIndex = RetractBox;
			}
			auto texCoords = makeQuadTexCoords( style->texCoords[typeIndex] );
			setTexture( renderer, 0, style->atlas );
			addRenderCommandSingleQuad( renderer, expandRect, 0, texCoords );

			if( isPointInside( expandRect, inputs->mouse.position )
			    && isKeyPressed( inputs, KC_LButton ) ) {

				if( group ) {
					group->expanded = !group->expanded;
				} else {
					animator->timelineRootExpanded = !animator->timelineRootExpanded;
				}
				repopulateVisibleGroups = true;
			}
		}
		if( group ) {
			renderTextClipped( renderer, font, group->name, textRect );
		} else {
			renderTextClipped( renderer, font, "root", textRect );
		}
		cell = translate( cell, 0, cellHeight );
	}
	if( repopulateVisibleGroups ) {
		populateVisibleGroups( animator );
	}
}

void doKeyframesFrameNumbers( ImGuiHandle handle, AppData* app, GameInputs* inputs, float width )
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

	handle.shortIndex = 1;
	if( imguiHasCapture( handle ) || ( isPointInside( inner, inputs->mouse.position )
	                                   && isKeyPressed( inputs, KC_LButton ) ) ) {
		imguiFocus( handle );
		imguiCapture( handle );
		auto pos = ( inputs->mouse.position.x - inner.left + offset ) / PaddleWidth;
		if( pos < 0 ) {
			pos = 0;
		}
		auto oldFrame = exchange( animator->currentFrame, pos );
		if( !floatEqSoft( oldFrame, pos ) ) {
			// current frame changed
			FOR( node : animator->nodes ) {
				*node = getCurrentFrameNode( animator, node );
			}
		}
	}

	// render current frame indicator
	renderer->color = 0xFFFAB74D;
	addRenderCommandSingleQuad(
	    renderer, RectWH( inner.left + animator->currentFrame * PaddleWidth - offset, inner.top,
	                      PaddleWidth, height( inner ) ) );
	renderer->color = Color::White;

	// render frame numbers
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
	animator->keyframesMoved = true;
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
	deleteDuplicateKeyframes( animator );
}

void doKeyframesSelection( ImGuiHandle handle, AppData* app, GameInputs* inputs,
                           const ImGuiScrollableRegionResult* scrollable, bool clickedAny )
{
	auto animator = &app->animatorState;
	auto renderer = &app->renderer;
	auto width    = animator->scrollableRegion.dim.x;

	auto altDown = isKeyDown( inputs, KC_Alt );

	handle.shortIndex = 2;
	if( isPointInside( scrollable->inner, inputs->mouse.position ) || imguiHasCapture( handle ) ) {

		if( !animator->mouseSelecting && isKeyPressed( inputs, KC_LButton ) && !clickedAny ) {
			imguiFocus( handle );
			imguiCapture( handle );
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
						auto groupHeight = PaddleHeight + ImGui->style.innerPadding * 2;
						auto firstGroup  = (int32)floor( selection.top / groupHeight );
						auto lastGroupF  = ceil( selection.bottom / groupHeight );
						auto lastGroup   = (int32)lastGroupF;
						if( firstGroup < animator->visibleGroups.size() ) {
							firstGroup = clamp( firstGroup, 0, animator->visibleGroups.size() - 1 );
							lastGroup  = clamp( lastGroup, 0, animator->visibleGroups.size() );
							if( firstGroup == 0 ) {
								// top level group is every visible group
								FOR( visible : animator->visibleGroups ) {
									if( visible.group >= 0 ) {
										groups.push_back( visible.group );
									}
								}
							} else {
								for( auto i = firstGroup; i < lastGroup; ++i ) {
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
bool doKeyframesDisplay( ImGuiHandle handle, AppData* app, GameInputs* inputs, const GroupId group )
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
	auto parent     = find_first_where( animator->groups, entry.id == group );
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
			if( ( isRoot || isInRange( parent->children, frame->group ) )
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
		imguiFocus( handle );
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

void changeModelSpace( AnimatorNode* node, mat4arg model )
{
	assert( node );
	if( auto inv = inverse( model ) ) {
		auto base = transformVector3( node->base, {} );
		auto head = transformVector3( node->world, {} );

		auto localBase = transformVector3( inv.matrix, base );
		auto localHead = transformVector3( inv.matrix, head );

		auto delta          = localHead - localBase;
		node->translation   = localBase;
		auto toYawPitchRoll = [&]( vec3arg d, vec3arg oldRotation ) -> vec3 {
			vec3 result = oldRotation;
			auto xAxis = row( model, 0 ).xyz;
			auto yAxis = row( model, 1 ).xyz;
			auto zAxis = row( model, 2 ).xyz;
			if( !floatEqZero( d.x ) || !floatEqZero( d.y ) ) {
				result.z = angle( d - dot( d, zAxis ) * zAxis, {1, 0, 0}, -zAxis );
			}
			if( !floatEqZero( d.x ) || !floatEqZero( d.z ) ) {
				result.y = angle( d - dot( d, yAxis ) * yAxis, {-1, 0, 0}, -yAxis );
			}
			return result;
		};
		node->rotation = toYawPitchRoll( delta, node->rotation );
		assert( floatEqSoft( node->length, length( delta ) ) );
	} else {
		assert( 0 );
	}
}

void doKeyframesControl( AppData* app, GameInputs* inputs )
{
	auto animator = &app->animatorState;
	animator->keyframesMoved = false;

	auto layout = imguiBeginColumn( NamesWidth );
	imguiAddItem( NamesWidth, ImGui->style.innerPadding * 2 + stringHeight( ImGui->font ) );
	doKeyframesNames( app, inputs, NamesWidth, RegionHeight );

	const auto regionWidth = app->width - NamesWidth;
	imguiNextColumn( &layout, regionWidth );

	auto width                     = animator->duration + regionWidth;
	animator->scrollableRegion.dim = {width, RegionHeight};

	auto handle = imguiMakeHandle( &animator->duration, ImGuiControlType::Custom );

	doKeyframesFrameNumbers( handle, app, inputs, regionWidth );
	auto scrollable =
	    imguiBeginScrollableRegion( &animator->scrollableRegion, regionWidth, RegionHeight, true );
	bool clickedAny = false;
	FOR( visible : animator->visibleGroups ) {
		if( doKeyframesDisplay( handle, app, inputs, visible.group ) ) {
			clickedAny = true;
		}
	}
	auto maxFrame = max_element( animator->keyframes, compareKeyframes );
	if( maxFrame != animator->keyframes.end() ) {
		animator->duration = ( *maxFrame )->t;
	} else {
		animator->duration = 0;
	}
	doKeyframesSelection( handle, app, inputs, &scrollable, clickedAny );
	imguiEndScrollableRegion( &animator->scrollableRegion, &scrollable );

	if( animator->keyframesMoved ) {
		sortKeyframes( animator );
	}

	if( imguiHasMyChildFocus( handle ) && isKeyPressed( inputs, KC_Delete ) ) {
		// delete selected keyframes
		erase_if( animator->keyframes, [animator]( AnimatorKeyframe* key ) {
			if( key->selected ) {
				freeStruct( &animator->keyframesAllocator, key );
				return true;
			}
			return false;
		} );
		clearSelectedKeyframes( animator );
	}
}

void doAnimatorMenu( AppData* app, GameInputs* inputs, rectfarg rect )
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
	if( imguiBeginDropGroup( "Editor", &editor->expandedFlags, AnimatorEditor::EditorSettings ) ) {
		imguiSlider( "Scale", &editor->scale, AnimatorMinScale, AnimatorMaxScale );
		if( imguiButton( "Reset View" ) ) {
			editor->translation = {};
			editor->rotation    = AnimatorInitialRotation;
			editor->scale       = 1;
		}
		imguiEndDropGroup();
	}
	if( imguiBeginDropGroup( "Properties", &editor->expandedFlags, AnimatorEditor::Properties ) ) {
		if( auto selected = editor->clickedNode ) {
			auto node = *selected;
			AnimatorNode* base = nullptr;
			if( animator->currentAnimation ) {
				imguiSameLine( 2 );
				auto handle = imguiMakeHandle( animator );
				handle.shortIndex = 0;
				if( imguiPushButton( handle, "Absolute", !animator->showRelativeProperties ) ) {
					animator->showRelativeProperties = false;
				}
				handle.shortIndex = 1;
				if( imguiPushButton( handle, "Relative",
				                     animator->showRelativeProperties == true ) ) {
					animator->showRelativeProperties = true;
				}
				if( animator->showRelativeProperties ) {
					base = *find_first_where( animator->baseNodes, entry->id == node.id );
					node = toRelativeNode( base, selected );
				}
			}
			if( imguiEditbox( "Name", node.name.data(), &node.name.sz, node.name.capacity() ) ) {
				if( auto group = find_first_where( animator->groups, entry.id == node.group ) ) {
					group->name = node.name;
				}
			}
			imguiEditbox( "Length", &node.length );
			imguiEditbox( "Translation X", &node.translation.x );
			imguiEditbox( "Translation Y", &node.translation.y );
			imguiEditbox( "Translation Z", &node.translation.z );
			doRotation( "Rotation X", &node.rotation.x );
			doRotation( "Rotation Y", &node.rotation.y );
			doRotation( "Rotation Z", &node.rotation.z );
			imguiEditbox( "Scale X", &node.scale.x );
			imguiEditbox( "Scale Y", &node.scale.y );
			imguiEditbox( "Scale Z", &node.scale.z );
			{
				imguiSameLine( 2 );
				imguiText( "Voxel" );
				auto comboHandle = imguiMakeHandle( &node.voxel );
				int32 index      = node.voxel;
				if( imguiCombo( comboHandle, &index, animator->voxels.names ) ) {
					node.voxel = safe_truncate< int16 >( index );
				}

				if( node.voxel >= 0 ) {
					imguiSameLine( 2 );
				}
				imguiEditbox( "Frame", &node.frame );
				if( node.voxel >= 0 ) {
					auto frames = animator->voxels.voxels.animations[node.voxel].range;
					float val = (float)( node.frame );
					if( imguiSlider( imguiMakeHandle( &node.frame ), &val, 0,
					                 (float)width( frames ) - 1 ) ) {
						node.frame = (uint16)val;
					}
				}
			}
			{
				imguiSameLine( 2 );
				imguiText( "Parent" );
				auto comboHandle = imguiMakeHandle( &node.parentId );
				int32 index      = -1;
				if( node.parent ) {
					index = distance( animator->nodes.begin(),
					                  find_first_where( animator->nodes, entry == node.parent ) );
				}
				if( imguiCombo( comboHandle, (const void*)animator->nodes.data(),
				                (int32)sizeof( AnimatorNode* ), animator->nodes.size(), &index,
				                []( const void* entry ) -> StringView {
					                assert_alignment( entry, alignof( AnimatorNode** ) );
					                auto node = *(AnimatorNode**)entry;
					                return node->name;
					            },
				                true ) ) {
					if( index >= 0 ) {
						auto newParent = animator->nodes[index];
						auto isMyChild = []( AnimatorNode* parent, AnimatorNode* potentialChild ) {
							bounded_while( potentialChild&& potentialChild != parent, 100 ) {
								potentialChild = potentialChild->parent;
							}
							return potentialChild == parent;
						};
						if( newParent != selected && !isMyChild( selected, newParent ) ) {
							if( node.parent ) {
								node.parent->childrenCount -= node.childrenCount;
							}
							node.parent   = newParent;
							node.parentId = node.parent->id;

							changeModelSpace( &node, newParent->world );
							newParent->childrenCount += node.childrenCount;
						}
					} else {
						if( node.parent ) {
							node.parent->childrenCount -= node.childrenCount;
							node.parentId = -1;
							node.parent   = nullptr;
							changeModelSpace( &node, matrixIdentity() );
						}
					}
				}
			}

			if( animator->currentAnimation && animator->showRelativeProperties ) {
				assert( base );
				*selected = toAbsoluteNode( base, &node );
			} else {
				*selected = node;
			}
		}
	}

	{
		auto pushHandle = imguiMakeHandle( editor );
		auto pushButton = []( ImGuiHandle handle, uint8 index, StringView name,
		                      AnimatorMouseMode mode, AnimatorEditor* editor ) {
			handle.shortIndex = index;
			if( imguiPushButton( handle, name, editor->mouseMode == mode ) ) {
				editor->mouseMode = mode;
			}
		};
		pushButton( pushHandle, 0, "Select", AnimatorMouseMode::Select, editor );
		pushButton( pushHandle, 1, "Translate", AnimatorMouseMode::Translate, editor );
		pushButton( pushHandle, 2, "Rotate", AnimatorMouseMode::Rotate, editor );
	}

	if( imguiBeginDropGroup( "Plane", &editor->expandedFlags, AnimatorEditor::Plane ) ) {
		auto pushHandle = imguiMakeHandle( editor );
		auto pushButton = []( ImGuiHandle handle, uint8 index, StringView name,
		                      AnimatorMousePlane mode, AnimatorEditor* editor ) {
			handle.shortIndex = index;
			if( imguiPushButton( handle, name, editor->mousePlane == mode ) ) {
				editor->mousePlane = mode;
			}
		};
		pushButton( pushHandle, 30, "XY", AnimatorMousePlane::XY, editor );
		pushButton( pushHandle, 31, "YZ", AnimatorMousePlane::YZ, editor );
		pushButton( pushHandle, 32, "XZ", AnimatorMousePlane::XZ, editor );
		imguiEndDropGroup();
	}
	if( imguiBeginDropGroup( "Options", &editor->expandedFlags, AnimatorEditor::Options ) ) {
		switch( editor->mouseMode ) {
			case AnimatorMouseMode::Select: {
				break;
			}
			case AnimatorMouseMode::Translate: {
				auto pushHandle = imguiMakeHandle( editor );
				auto pushButton = []( ImGuiHandle handle, uint8 index, StringView name,
				                      AnimatorTranslateOptions mode, AnimatorEditor* editor ) {
					handle.shortIndex = index;
					if( imguiPushButton( handle, name, editor->translateOptions == mode ) ) {
						editor->translateOptions = mode;
					}
				};
				pushButton( pushHandle, 20, "World", AnimatorTranslateOptions::World, editor );
				pushButton( pushHandle, 21, "LocalAlong", AnimatorTranslateOptions::LocalAlong,
				            editor );
				pushButton( pushHandle, 22, "LocalPerpendicular",
				            AnimatorTranslateOptions::LocalPerpendicular, editor );
				pushButton( pushHandle, 23, "ParentAlong", AnimatorTranslateOptions::ParentAlong,
				            editor );
				pushButton( pushHandle, 24, "ParentPerpendicular",
				            AnimatorTranslateOptions::ParentPerpendicular, editor );
				break;
			}
			case AnimatorMouseMode::Rotate: {
				break;
			}
		}
		imguiEndDropGroup();
	}
	if( !animator->currentAnimation ) {
		if( imguiBeginDropGroup( "Animations", &editor->expandedFlags,
		                         AnimatorEditor::Animations ) ) {
			if( imguiButton( "New Animation" ) ) {
				if( animator->animations.remaining() ) {
					auto added = animator->animations.emplace_back();
					*added     = {};
					added->name.assign( "Unnamed" );
					added->id = -1;
				}
			}
			AnimatorAnimation* selected = nullptr;
			AnimatorAnimation* deleted = nullptr;
			FOR( animation : animator->animations ) {
				imguiSameLine( 3 );
				imguiText( animation.name );
				if( imguiButton( "Open" ) ) {
					selected = &animation;
				}
				if( imguiButton( "Delete" ) ) {
					deleted = &animation;
				}
			}
			if( selected ) {
				openAnimation( animator, selected );
			}
			if( deleted ) {
				// TODO: cleanup of resources
				// TODO: ask user whether to really delete
				animator->animations.erase( deleted );
			}
			imguiEndDropGroup();
		}
	} else {
		if( imguiBeginDropGroup( "Animations", &editor->expandedFlags,
		                         AnimatorEditor::Animations ) ) {
			auto name = &animator->currentAnimation->name;
			imguiEditbox( "Name", name->data(), &name->sz, name->capacity() );
			imguiButton( "Commit" );
			if( imguiButton( "Close" ) ) {
				// TODO: ask user whether to keep changes
				closeAnimation( &app->stackAllocator, animator );
			}
		}
	}
	if( animator->currentAnimation && editor->clickedNode
	    && imguiBeginDropGroup( "Keying", &editor->expandedFlags, AnimatorEditor::Keying ) ) {

		auto node    = editor->clickedNode;
		auto current = getCurrentFrameNode( animator, editor->clickedNode );

		auto compareVector = []( vec3arg a, vec3arg b ) {
			return floatEqSoft( a.x, b.x ) && floatEqSoft( a.y, b.y ) && floatEqSoft( a.z, b.z );
		};
		StringView translation = "Translation*";
		StringView rotation = "Rotation*";
		StringView scale = "Scale*";
		StringView frame = "Frame*";

		if( compareVector( current.translation, node->translation ) ) {
			translation.pop_back();
		}
		if( compareVector( current.rotation, node->rotation ) ) {
			rotation.pop_back();
		}
		if( compareVector( current.scale, node->scale ) ) {
			scale.pop_back();
		}
		if( current.frame == node->frame ) {
			frame.pop_back();
		}
		if( imguiButton( translation ) ) {
			AnimatorKeyframe key = {};
			key.t                = animator->currentFrame;
			key.group            = node->group + 1;
			set_variant( key.data, translation ) = node->translation;
			addKeyframe( animator, key );
		}
		if( imguiButton( rotation ) ) {
			AnimatorKeyframe key = {};
			key.t                = animator->currentFrame;
			key.group            = node->group + 2;
			set_variant( key.data, rotation ) = node->rotation;
			addKeyframe( animator, key );
		}
		if( imguiButton( scale ) ) {
			AnimatorKeyframe key = {};
			key.t                = animator->currentFrame;
			key.group            = node->group + 3;
			set_variant( key.data, scale ) = node->scale;
			addKeyframe( animator, key );
		}
		if( imguiButton( frame ) ) {
			AnimatorKeyframe key = {};
			key.t                = animator->currentFrame;
			key.group            = node->group + 4;
			set_variant( key.data, frame ) = node->frame;
			addKeyframe( animator, key );
		}

		imguiEndDropGroup();
	}
}

Ray3 pointToWorldSpaceRay( mat4arg invViewProj, vec2arg pos, float width, float height )
{
	Ray3 result;
	result.start = toWorldSpace( invViewProj, Vec3( pos, 0 ), width, height );
	auto end     = toWorldSpace( invViewProj, Vec3( pos, 1 ), width, height );
	result.dir   = end - result.start;
	return result;
}
vec3 rayIntersectionWithPlane( const Ray3& ray, vec3arg base, vec3arg planeNormal )
{
	vec3 result = {};
	if( auto hit = testRayVsPlane( ray.start, ray.dir, base, planeNormal ) ) {
		result = ray.start + ray.dir * hit.t;
		// calculating result like this sometimes loses some accuracy
		// we basically do result.z = base.z in case its the xy plane etc

		// project result onto plane
		result -= dot( result, planeNormal ) * planeNormal;
		// add plane normal component of base to result
		result += dot( base, planeNormal ) * planeNormal;
	}
	return result;
}

void deleteDeep( AnimatorState* animator, AnimatorNode* node );
void deleteChildren( AnimatorState* animator, AnimatorNode* parent )
{
	auto first = animator->nodes.begin();
	auto last = animator->nodes.end();
	for( auto it = first; it != last; ) {
		if( ( *it )->parent == parent ) {
			deleteDeep( animator, *it );
			first = animator->nodes.begin();
			last = animator->nodes.end();
			it = first;
			continue;
		}
		++it;
	}
}
void deleteDeep( AnimatorState* animator, AnimatorNode* node )
{
	if( animator->editor.clickedNode == node ) {
		animator->editor.clickedNode = nullptr;
	}
	deleteChildren( animator, node );
	unordered_remove( animator->nodes, node );
	freeStruct( &animator->nodeAllocator, node );
}

struct Plane {
	vec3 origin;
	vec3 normal;
	vec3 base;
	vec3 rotationNormal;
};
Plane getMouseModePlane( AnimatorState* animator, AnimatorNode* node )
{
	assert( animator );
	assert( node );

	Plane result = {};
	auto editor = &animator->editor;

	switch( editor->mousePlane ) {
		case AnimatorMousePlane::XY: {
			result.normal = {0, 0, 1};
			break;
		}
		case AnimatorMousePlane::YZ: {
			result.normal = {1, 0, 0};
			break;
		}
		case AnimatorMousePlane::XZ: {
			result.normal = {0, 1, 0};
			break;
		}
			InvalidDefaultCase;
	}
	result.base           = transformVector3( node->base, {} );
	result.origin         = transformVector3( node->world, {} );
	result.rotationNormal = result.normal;
	if( editor->mouseMode == AnimatorMouseMode::Rotate ) {
		switch( editor->mousePlane ) {
			case AnimatorMousePlane::XY: {
				result.normal = {0, 0, 1};
				if( node->parent ) {
					auto parentBase = transformVector3( node->parent->world, {} );
					result.normal   = safeNormalize(
					    transformVector3( node->parent->world, {0, 0, 1} ) - parentBase,
					    {0, 0, 1} );
				}
				break;
			}
			case AnimatorMousePlane::YZ: {
				result.normal = safeNormalize( result.origin - result.base, {1, 0, 0} );
				break;
			}
			case AnimatorMousePlane::XZ: {
				if( node->parent ) {
					auto parentBase = transformVector3( node->parent->world, {} );
					result.normal   = safeNormalize(
					    transformVector3( matrixRotationZ( node->rotation.z ) * node->parent->world,
					                      {0, 1, 0} )
					        - parentBase,
					    {0, 1, 0} );
				} else {
					result.normal = safeNormalize(
					    transformVector3( matrixRotationZ( node->rotation.z ), {0, 1, 0} ),
					    {0, 1, 0} );
				}
				break;
			}
			InvalidDefaultCase;
		}
	}
	return result;
}

vec3 getDestinationVectorFromScreenPos( AnimatorState* animator, AnimatorNode* node,
                                        const Plane& plane, mat4arg invViewProj, vec2arg pos,
                                        float width, float height )
{
	auto editor = &animator->editor;

	auto ray      = pointToWorldSpaceRay( invViewProj, pos, width, height );
	auto fromLine = []( const Ray3& ray, vec3arg bStart, vec3arg bDir ) {
		auto hit = shortestLineBetweenLines( ray.start, ray.dir, bStart, bDir );
		return bStart + bDir * hit.tB;
	};

	vec3 destination = plane.origin;
	switch( editor->mouseMode ) {
		case AnimatorMouseMode::Select: {
			break;
		}
		case AnimatorMouseMode::Translate: {
			switch( editor->translateOptions ) {
				case AnimatorTranslateOptions::World: {
					destination = rayIntersectionWithPlane( ray, plane.origin, plane.normal );
					break;
				}
				case AnimatorTranslateOptions::LocalAlong: {
					destination = fromLine( ray, plane.origin, plane.base - plane.origin );
					break;
				}
				case AnimatorTranslateOptions::LocalPerpendicular: {
					destination = fromLine( ray, plane.origin,
					                        cross( plane.origin - plane.base, plane.normal ) );
					break;
				}
				case AnimatorTranslateOptions::ParentAlong: {
					if( node->parent ) {
						auto parentBase = transformVector3( node->parent->base, {} );
						vec3 parentHead = transformVector3( node->parent->world, {} );
						destination     = fromLine( ray, plane.origin, parentBase - parentHead );
					} else {
						destination = fromLine( ray, plane.origin, plane.base - plane.origin );
					}
					break;
				}
				case AnimatorTranslateOptions::ParentPerpendicular: {
					if( node->parent ) {
						auto parentBase = transformVector3( node->parent->base, {} );
						vec3 parentHead = transformVector3( node->parent->world, {} );
						vec3 parentNormal =
						    transformVector3( node->parent->base, {0, 0, -1} ) - parentBase;

						destination = fromLine( ray, plane.origin, parentBase - parentHead );
						destination = fromLine( ray, plane.origin,
						                        cross( parentHead - parentBase, parentNormal ) );
					} else {
						destination = fromLine( ray, plane.origin,
						                        cross( plane.origin - plane.base, plane.normal ) );
					}
					break;
				}
			}
			break;
		}
		case AnimatorMouseMode::Rotate: {
			destination = rayIntersectionWithPlane( ray, plane.origin, plane.normal );
			break;
		}
	}
	return destination;
}

void doEditor( AppData* app, GameInputs* inputs, rectfarg rect )
{
	auto renderer = &app->renderer;
	auto animator = &app->animatorState;
	auto editor   = &animator->editor;
	auto handle   = imguiMakeHandle( editor, ImGuiControlType::None );

	// auto aspect = width( rect ) / height( rect );
	auto aspect = app->width / app->height;
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

	auto handleMouse = [&]( uint8 button, uint8 modifier,
	                        void ( *proc )( AnimatorEditor * editor, GameInputs * inputs ) ) {
		auto pressed      = isKeyPressed( inputs, button );
		auto modifierDown = ( modifier != 0 ) ? isKeyDown( inputs, modifier ) : ( true );
		if( ( isPointInside( rect, inputs->mouse.position ) && pressed && modifierDown )
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

	handleMouse( KC_LButton, KC_Space, []( AnimatorEditor* editor, GameInputs* inputs ) {
		auto delta = inputs->mouse.delta * 0.01f;
		editor->rotation.x -= delta.x;
		editor->rotation.y -= delta.y;
	} );
	handleMouse( KC_MButton, 0, []( AnimatorEditor* editor, GameInputs* inputs ) {
		auto delta = inputs->mouse.delta * 0.5f / editor->scale;
		editor->translation.x += delta.x;
		editor->translation.y -= delta.y;
	} );
	if( ( isPointInside( rect, inputs->mouse.position ) || imguiHasFocus( handle ) )
	    && !floatEqZero( inputs->mouse.wheel ) ) {

		editor->scale =
		    clamp( editor->scale + inputs->mouse.wheel * 0.1f, AnimatorMinScale, AnimatorMaxScale );
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

		// render origin indicator
		pushLine( stream, {0, -10, 0}, {0, 10, 0} );
	}

	sortNodes( animator );
	// update transforms
	FOR( node : animator->nodes ) {
		auto local = matrixScale( node->scale ) * matrixRotation( node->rotation )
		             * matrixTranslation( node->translation );
		if( node->parent ) {
			node->base = local * node->parent->world;
		} else {
			node->base = local;
		}
		node->world = matrixTranslation( node->length, 0, 0 ) * node->base;
	}

	// render voxels
	setRenderState( renderer, RenderStateType::DepthTest, true );
	setTexture( renderer, 0, animator->voxels.voxels.texture );
	auto stack = renderer->matrixStack;
	pushMatrix( stack );
	FOR( node : animator->nodes ) {
		if( node->voxel >= 0 ) {
			auto voxels = &animator->voxels.voxels;
			auto range  = voxels->animations[node->voxel].range;
			if( range ) {
				auto entry = &voxels->frames[range.min + ( node->frame % width( range ) )];
				currentMatrix( stack ) =
				    matrixTranslation( Vec3( -entry->offset.x, entry->offset.y, 0 ) ) * node->world;
				addRenderCommandMesh( renderer, entry->mesh );
			}
		}
	}
	popMatrix( stack );

	setRenderState( renderer, RenderStateType::DepthTest, false );
	// render skeleton
	setTexture( renderer, 0, null );
	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::White;
		FOR( node : animator->nodes ) {
			vec3 origin = {};
			if( node->parent ) {
				origin = transformVector3( node->parent->world, {} );
			}
			vec3 base     = transformVector3( node->base, {} );
			vec3 head     = transformVector3( node->world, {} );
			stream->color = Color::Blue;
			pushLine( stream, origin, base );
			stream->color = Color::White;
			pushLine( stream, base, head );
		}
	}

	// render controls
	auto viewProj    = renderer->view * projection;
	auto invViewProj = inverse( viewProj );

	setProjection( renderer, ProjectionType::Orthogonal );
	bool selectedAny = false;

	auto selectNode = [&]( AnimatorNode* node, vec2arg pos ) {
		imguiFocus( handle );
		if( isKeyDown( inputs, KC_Control ) ) {
			node->selected = !node->selected;
		} else {
			if( !node->selected ) {
				clearSelectedNodes( animator );
				node->selected = true;
			}
			editor->moving      = true;
			editor->clickedNode = node;
		}
	};

	auto lMousePressed = isKeyPressed( inputs, KC_LButton ) && !isKeyDown( inputs, KC_Space );

	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::White;
		FOR( node : animator->nodes ) {
			auto mat      = node->world * viewProj;
			auto head     = toScreenSpace( mat, {}, app->width, app->height );
			auto headRect = RectHalfSize( head, 5, 5 );
			if( isPointInside( headRect, inputs->mouse.position ) && lMousePressed
			    && imguiIsHover( handle ) ) {

				selectedAny = true;
				selectNode( node, head );
				editor->mouseOffset  = head - inputs->mouse.position;
				editor->clickedVoxel = false;
			}

			stream->color = ( node->selected ) ? ( Color::Red ) : ( Color::White );
			pushQuadOutline( stream, headRect );
		}
		if( !selectedAny && lMousePressed && imguiIsHover( handle ) ) {
			// no node was selected, try selecting voxels
			if( invViewProj ) {
				auto ray = pointToWorldSpaceRay( invViewProj.matrix, inputs->mouse.position,
				                                 app->width, app->height );
				ray.dir = safeNormalize( ray.dir );

				FOR( node : animator->nodes ) {
					if( node->voxel >= 0 ) {
						auto animation  = &animator->voxels.voxels.animations[node->voxel];
						auto range      = animation->range;
						auto frameIndex = range.min /* + ( node->frame % width( range ) )*/;
						auto info       = &animator->voxels.voxels.frameInfos[frameIndex];
						auto frame      = &animator->voxels.voxels.frames[frameIndex];
						auto mat = matrixTranslation( Vec3( -frame->offset.x, frame->offset.y, 0 ) )
						           * node->world;

						if( testRayVsObb( ray.start, ray.dir, info->bounds, mat ) ) {
							auto mat  = node->world * viewProj;
							auto head = toScreenSpace( mat, {}, app->width, app->height );
							selectNode( node, head );
							editor->mouseOffset = {};
							selectedAny         = true;

							auto plane              = getMouseModePlane( animator, node );
							editor->clickedPlanePos = getDestinationVectorFromScreenPos(
							    animator, node, plane, invViewProj.matrix, inputs->mouse.position,
							    app->width, app->height );
							editor->clickedRotation = node->rotation;
							editor->clickedVoxel    = true;
						}
					}
				}
			}
		}
	}
	if( isPointInside( rect, inputs->mouse.position ) && lMousePressed && imguiIsHover( handle )
	    && !selectedAny ) {

		clearSelectedNodes( animator );
		editor->moving = false;
	}
	if( isKeyUp( inputs, KC_LButton ) ) {
		editor->moving = false;
	}

	if( imguiHasFocus( handle ) && isKeyPressed( inputs, KC_Delete ) && editor->clickedNode ) {
		// delete node and all its children
		deleteDeep( animator, editor->clickedNode );
	}

	if( editor->clickedNode ) {
		auto node = editor->clickedNode;

		// get plane normal
		auto plane = getMouseModePlane( animator, node );

		// render reference plane for transformations/rotations
		if( editor->mouseMode == AnimatorMouseMode::Rotate
		    || ( editor->mouseMode == AnimatorMouseMode::Translate
		         && editor->translateOptions == AnimatorTranslateOptions::World ) ) {

			setProjection( renderer, ProjectionType::Perspective );
			const float size = 10;
			LINE_MESH_STREAM_BLOCK( stream, renderer ) {
				stream->color = Color::White;
				pushLine( stream, plane.origin - plane.normal * size,
				          plane.origin + plane.normal * size );
			}
			setRenderState( renderer, RenderStateType::BackCulling, false );

			auto mat      = matrixFromNormal( plane.normal );
			vec3 verts[4] = {plane.origin + transformVector3( mat, {-size, size, 0} ),
			                 plane.origin + transformVector3( mat, {size, size, 0} ),
			                 plane.origin + transformVector3( mat, {-size, -size, 0} ),
			                 plane.origin + transformVector3( mat, {size, -size, 0} )};
			renderer->color = 0x40FFFFFF;
			setTexture( renderer, 0, null );
			addRenderCommandSingleQuad( renderer, verts );

			setRenderState( renderer, RenderStateType::BackCulling, true );
			setProjection( renderer, ProjectionType::Orthogonal );
		}


		if( editor->moving && hasMagnitude( inputs->mouse.delta ) ) {
			if( invViewProj ) {
				auto mouse       = inputs->mouse.position + editor->mouseOffset;
				auto destination = getDestinationVectorFromScreenPos(
				    animator, node, plane, invViewProj.matrix, mouse, app->width, app->height );
				switch( editor->mouseMode ) {
					case AnimatorMouseMode::Select: {
						break;
					}
					case AnimatorMouseMode::Translate: {
						// destination is in world space, convert to local space
						mat4 mat = matrixIdentity();
						if( node->parent ) {
							if( auto invWorld = inverse( node->parent->world ) ) {
								mat = invWorld.matrix;
							}
						}

						vec3 delta = {};
						// transform world coordinates into local coordinates
						if( editor->clickedVoxel ) {
							auto clicked = transformVector3( mat, editor->clickedPlanePos );
							editor->clickedPlanePos = destination;
							destination             = transformVector3( mat, destination );
							delta                   = destination - clicked;
						} else {
							destination = transformVector3( mat, destination );
							auto head   = transformVector3( mat, plane.origin );
							delta       = destination - head;
						}
						node->translation += delta;

						break;
					}
					case AnimatorMouseMode::Rotate: {
						if( editor->mousePlane == AnimatorMousePlane::YZ ) {
							// there is no good reference vector for this rotation
							auto deltaIntersection = destination - plane.origin;
							auto other             = vec3{-plane.normal.z, 0, plane.normal.y};
							auto rotation = angle( deltaIntersection, other, plane.normal );
							node->rotation -=
							    dot( plane.rotationNormal, node->rotation ) * plane.rotationNormal;
							node->rotation -= plane.rotationNormal * rotation;
						} else if( !editor->clickedVoxel && node->length != 0 ) {
							auto deltaIntersection = destination - plane.base;
							auto deltaHead         = plane.origin - plane.base;

							// we want the two vectors to be on the same plane for better accuracy
							deltaIntersection -=
							    dot( deltaIntersection, plane.normal ) * plane.normal;
							deltaHead -= dot( deltaHead, plane.normal ) * plane.normal;

							// get angle of the two vectors guided by the plane normal
							auto deltaRotation =
							    angle( deltaIntersection, deltaHead, plane.normal );

							node->rotation = node->rotation - plane.rotationNormal * deltaRotation;
							FOR( element : node->rotation.elements ) {
								element = simplifyAngle( element );
							}
						} else if( editor->clickedVoxel ) {
							auto deltaIntersection = destination - plane.base;
							auto deltaHead         = editor->clickedPlanePos - plane.base;

							// we want the two vectors to be on the same plane for better accuracy
							deltaIntersection -=
							    dot( deltaIntersection, plane.normal ) * plane.normal;
							deltaHead -= dot( deltaHead, plane.normal ) * plane.normal;

							// get angle of the two vectors guided by the plane normal
							auto rotation = angle( deltaIntersection, deltaHead, plane.normal );

							node->rotation -=
							    dot( plane.rotationNormal, node->rotation ) * plane.rotationNormal;
							node->rotation -= plane.rotationNormal * rotation;
							node->rotation += dot( plane.rotationNormal, editor->clickedRotation ) * plane.rotationNormal;
							FOR( element : node->rotation.elements ) {
								element = simplifyAngle( element );
							}
						}
						break;
					}
						InvalidDefaultCase;
				}
			}
		}
	}

	setRenderState( renderer, RenderStateType::Scissor, false );

	if( isKeyPressed( inputs, KC_RButton ) && isPointInside( rect, inputs->mouse.position ) ) {
		editor->rightClickedPos = inputs->mouse.position;
		imguiShowContextMenu( editor->contextMenu, inputs->mouse.position );
	}

	renderer->color = Color::White;
	if( !animator->currentAnimation && imguiContextMenu( editor->contextMenu ) ) {
		if( imguiContextMenuEntry( "New Node" ) ) {
			AnimatorNode node = {};
			node.length = 10;
			if( auto parent = editor->clickedNode ) {
				node.parentId = parent->id;
			} else {
				node.parentId = -1;
			}
			node.voxel = -1;
			/*auto ray = pointToWorldSpaceRay( invViewProj.matrix, inputs->mouse.position,
			                                 app->width, app->height );
			auto position = rayIntersectionWithPlane( ray, {}, {0, 0, 1} );*/
			addNewNode( animator, node );
		}
		imguiEndContainer();
	} else {
		imguiGetContainer( editor->contextMenu )->setHidden( true );
	}
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

		animator->editor.contextMenu = imguiGenerateContainer( gui, {}, true );

		auto allocator           = &app->stackAllocator;
		const auto MaxNodes      = 10;
		const auto MaxKeyframes  = 50;
		const auto MaxAnimations = 20;
		animator->stringPool     = makeStringPool( allocator, 100 );
		animator->keyframes      = makeUArray( allocator, AnimatorKeyframe*, MaxNodes );
		animator->selected       = makeUArray( allocator, AnimatorKeyframe*, MaxNodes );
		animator->groups         = makeUArray( allocator, AnimatorGroup, MaxNodes * 5 );
		animator->visibleGroups  = makeUArray( allocator, AnimatorGroupDisplay, MaxNodes * 5 );
		animator->baseNodes      = makeUArray( allocator, AnimatorNode*, MaxNodes );
		animator->nodes          = makeUArray( allocator, AnimatorNode*, MaxNodes );
		animator->animations     = makeUArray( allocator, AnimatorAnimation, MaxAnimations );
		animator->nodeAllocator  = makeFixedSizeAllocator(
		    allocator, MaxNodes * 2, sizeof( AnimatorNode ), alignof( AnimatorNode ) );
		animator->keyframesAllocator = makeFixedSizeAllocator(
		    allocator, MaxKeyframes, sizeof( AnimatorKeyframe ), alignof( AnimatorKeyframe ) );

		animator->fieldNames[0] = pushString( &animator->stringPool, "Translation" );
		animator->fieldNames[1] = pushString( &animator->stringPool, "Rotation" );
		animator->fieldNames[2] = pushString( &animator->stringPool, "Scale" );
		animator->fieldNames[3] = pushString( &animator->stringPool, "Frame" );

		animator->editor.rotation = AnimatorInitialRotation;
		animator->editor.scale    = 1;
		animator->editor.expandedFlags |= AnimatorEditor::Properties;
		animator->timelineRootExpanded = true;

#if 1
		addNewNode( animator, {{}, {0, 0, HalfPi32}, {1, 1, 1}, 40, -1} );
		addNewNode( animator, {{}, {0, 0, HalfPi32}, {1, 1, 1}, 40, 0} );
		populateVisibleGroups( animator );

		if( loadVoxelCollection( allocator, "Data/voxels/hero.json", &animator->voxels.voxels ) ) {
			animator->voxels.names =
			    makeArray( allocator, StringView, animator->voxels.voxels.animations.size() );
			zip_for( animator->voxels.names, animator->voxels.voxels.animations ) {
				*first = second->name;
			}
		}

#endif

		animator->initialized = true;
	}

	renderer->color      = Color::White;
	renderer->clearColor = 0xFF1B2B34;

	setProjection( renderer, ProjectionType::Orthogonal );
	setRenderState( renderer, RenderStateType::DepthTest, false );

	rectf guiBounds = {0, 0, app->width, app->height};
	imguiBind( gui, renderer, font, inputs, app->stackAllocator.ptr, guiBounds );

	auto keyframesControlHeight = ( animator->currentAnimation ) ? ( RegionHeight ) : ( 0 );
	auto mainRowHeight  = app->height - keyframesControlHeight - 30;
	auto editorWidth    = app->width - PropertiesColumnWidth;
	auto layout         = imguiBeginColumn( PropertiesColumnWidth );

	doAnimatorMenu( app, inputs, RectSetHeight( imguiCurrentContainer()->rect, mainRowHeight ) );

	imguiNextColumn( &layout, editorWidth );
	auto editorRect = imguiAddItem( editorWidth, mainRowHeight );
	doEditor( app, inputs, editorRect );

	imguiEndColumn( &layout );

	if( animator->currentAnimation ) {
		doKeyframesControl( app, inputs );
	}

#if 1
	// debug
	debugPrintln( "Keyframes Count: {}", animator->keyframes.size() );
#endif

	imguiUpdate( dt );
	imguiFinalize();
}