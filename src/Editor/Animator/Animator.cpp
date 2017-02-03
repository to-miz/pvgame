constexpr const float PaddleWidth           = 5;
constexpr const float PaddleHeight          = 10;
constexpr const float KeyframePrecision     = 1.0f / 5.0f;
constexpr const float PropertiesColumnWidth = 200;
constexpr const float NamesWidth            = 100;
constexpr const float RegionHeight          = 200;
constexpr const float MenuHeight            = 18;

static const vec3 AnimatorInitialRotation = {0, -0.2f, 0};


static const StringView EaseTypeNames[] = {
	"Lerp",
	"Step",
	"Smoothstep",
	"EaseOutBounce",
	"EaseOutElastic",
	"Curve",
};
static const StringView AnimatorAssetTypeNames[] = {
	"None",
	"VoxelCollection",
};
StringView to_string( AnimatorKeyframeData::EaseType type )
{
	assert( type >= 0 && type < countof( EaseTypeNames ) );
	return EaseTypeNames[type];
}
StringView to_string( AnimatorAsset::Type type )
{
	assert( type >= 0 && type < countof( AnimatorAssetTypeNames ) );
	return AnimatorAssetTypeNames[type];
}
void from_string( StringView str, AnimatorKeyframeData::EaseType* out )
{
	assert( out );
	// accepts gibberish strings too, but that doesn't matter too much
	// alternatively could just loop over EaseTypeNames and compare
	*out = {};
	if( str.size() ) {
		switch( str[0] ) {
			case 'l':
			case 'L': {
				*out = AnimatorKeyframeData::Lerp;
				break;
			}
			case 's':
			case 'S': {
				*out = AnimatorKeyframeData::Step;
				break;
			}
			case 'e':
			case 'E': {
				if( str.size() >= 8 ) {
					switch( str[7] ) {
						case 'b':
						case 'B': {
							*out = AnimatorKeyframeData::EaseOutBounce;
							break;
						}
						case 'e':
						case 'E': {
							*out = AnimatorKeyframeData::EaseOutElastic;
							break;
						}
					}
				}
				break;
			}
			case 'c':
			case 'C': {
				*out = AnimatorKeyframeData::Curve;
				break;
			}
		}
	}
}
template <>
AnimatorAsset::Type convert_to< AnimatorAsset::Type >( StringView str,
                                                       AnimatorAsset::Type def )
{
	AnimatorAsset::Type result = def;
	if( str.size() ) {
		switch( str[0] ) {
			case 'v':
			case 'V': {
				result = AnimatorAsset::type_collection;
				break;
			}
		}
	}
	return result;
}

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

void setUnsavedChanges( AnimatorState* animator )
{
	animator->flags.unsavedChanges    = true;
	animator->flags.uncommittedChanges = true;
}

bool compareKeyframes( const AnimatorKeyframes::value_type& a,
                       const AnimatorKeyframes::value_type& b )
{
	return a->t < b->t;
}
void clearSelectedKeyframes( AnimatorState* animator )
{
	FOR( frame : animator->keyframes ) {
		frame->selected = false;
	}
	animator->selected.clear();
	animator->flags.selectionRectValid = false;
	animator->clickedGroup             = -1;
}
void clearSelectedNodes( AnimatorState* animator )
{
	FOR( entry : animator->nodes ) {
		entry->selected = false;
	}
	animator->editor.clickedNode = nullptr;
}

int32 makeAnimatorGroup( int16 ownerId, int32 index )
{
	assert( index >= 0 && index < 256 );
	return (int32)( (uint32)ownerId << 8 | (uint32)index );
}
int16 getAnimatorKeyframeOwner( GroupId id )
{
	return (int16)( (uint32)id >> 8 );
}
GroupId getAnimatorParentGroup( GroupId id )
{
	return (int32)( (uint32)id & 0xFFFFFF00u );
}
void addAnimatorGroups( AnimatorState* animator, StringView parentName,
                        Array< const StringView > childNames, int16 ownerId )
{
	Array< AnimatorGroup > groups = {};
	auto count                    = childNames.size() + 1;
	auto start                    = safe_truncate< int32 >( animator->groups.size() );
	auto end                      = start + count;
	animator->groups.resize( end, {} );
	groups = makeRangeView( animator->groups, start, end );
	for( auto i = 0; i < count; ++i ) {
		auto entry      = &groups[i];
		entry->name     = ( i == 0 ) ? parentName : childNames[i - 1];
		entry->id       = makeAnimatorGroup( ownerId, i );
		entry->expanded = false;
	}
	if( count > 1 ) {
		groups[0].children = {groups[1].id, groups[1].id + childNames.size()};
	} else {
		groups[0].children = {};
	}
}
void clearAnimatorGroups( AnimatorState* animator )
{
	animator->visibleGroups.clear();
	animator->groups.clear();
}

void populateVisibleGroups( AnimatorState* animator )
{
	animator->visibleGroups.clear();
	animator->visibleGroups.push_back( {-1, -1} );
	if( animator->flags.timelineRootExpanded ) {
		for( auto it = animator->groups.begin(), end = animator->groups.end(); it < end; ++it ) {
			animator->visibleGroups.push_back( {it->id, it->children} );
			if( !it->expanded && it->children ) {
				it += width( it->children );
			}
		}
	}
}

float animatorKeyframeEase( Array< AnimatorCurveData > curves, AnimatorKeyframeData::EaseType type,
                            int16 curveIndex, float t )
{
	switch( type ) {
		case AnimatorKeyframeData::Lerp: {
			return t;
		}
		case AnimatorKeyframeData::Step: {
			return 0;
		}
		case AnimatorKeyframeData::Smoothstep: {
			return smoothstep( t );
		}
		case AnimatorKeyframeData::EaseOutBounce: {
			return easeOutBounce( t );
		}
		case AnimatorKeyframeData::EaseOutElastic: {
			return easeOutElastic( t );
		}
		case AnimatorKeyframeData::Curve: {
			assert( curveIndex >= 0 );
			auto curve = &curves[curveIndex];
			assert( curve->used );
			return evaluateBezierForwardDifferencerFromX( &curve->differencer, t );
		}
		InvalidDefaultCase;
	}
	return 0;
}
AnimatorKeyframeData lerp( Array< AnimatorCurveData > curves, float t,
                           const AnimatorKeyframeData& a, const AnimatorKeyframeData& b )
{
	AnimatorKeyframeData result = {};
	assert( a.type == b.type );
	t = animatorKeyframeEase( curves, a.easeType, a.curveIndex, t );
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
AnimatorKeyframes::iterator lowerBoundKeyframeIt( AnimatorState* animator, float t, GroupId group )
{
	auto first = animator->keyframes.begin();
	auto last  = animator->keyframes.end();
	if( first != last ) {
		auto it = lower_bound( first, last, t, []( const AnimatorKeyframes::value_type& keyframe,
		                                           float t ) { return keyframe->t < t; } );
		if( it == last ) {
			--it;
		}
		if( ( *it )->group == group && ( *it )->t <= t ) {
			return it;
		}
		if( it != first ) {
			--it;
			while( it != first && ( *it )->group != group ) {
				--it;
			}
			if( it == first && ( *it )->group != group ) {
				it = last;
			}
		} else {
			it = last;
		}
		return it;
	}
	return last;
}
AnimatorKeyframe* lowerBoundKeyframe( AnimatorState* animator, float t, GroupId group )
{
	auto last = animator->keyframes.end();
	auto it   = lowerBoundKeyframeIt( animator, t, group );
	if( it != last ) {
		return it->get();
	}
	return nullptr;
}
AnimatorKeyframeData getInterpolatedKeyframe( AnimatorState* animator, float t, GroupId group )
{
	AnimatorKeyframeData result = {};
	auto last                   = animator->keyframes.end();
	auto first                  = lowerBoundKeyframeIt( animator, t, group );
	if( first != last && ( *first )->group == group ) {
		auto a = first->get();
		result = a->data;
		if( a->t < t && first + 1 != last ) {
			auto second = first + 1;
			while( second != last && ( *second )->group != group ) {
				++second;
			}
			if( second != last && ( *second )->group == group ) {
				auto b = second->get();
				assert( b->t >= a->t );
				assert( a->data.type == b->data.type );

				auto duration = b->t - a->t;
				if( duration > 0 ) {
					auto adjustedT = ( t - a->t ) / duration;
					result = lerp( makeArrayView( animator->curves ), adjustedT, a->data, b->data );
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
	auto base   = find_first_where( animator->baseNodes, entry->id == node->id )->get();
	auto result = *base;

	auto baseGroup = makeAnimatorGroup( node->id, 0 );
	assert_init( auto group = find_first_where( animator->groups, entry.id == baseGroup ),
	             width( group->children ) == 4 );
	GroupId translationId = baseGroup + 1;
	GroupId rotationId    = baseGroup + 2;
	GroupId scaleId       = baseGroup + 3;
	GroupId frameId       = baseGroup + 4;

	auto translationKeyframe =
	    getInterpolatedKeyframe( animator, animator->currentFrame, translationId );
	auto rotationKeyframe = getInterpolatedKeyframe( animator, animator->currentFrame, rotationId );
	auto scaleKeyframe    = getInterpolatedKeyframe( animator, animator->currentFrame, scaleId );

	result.translation =
	    get_variant_or_default( translationKeyframe, translation, base->translation );
	result.rotation = get_variant_or_default( rotationKeyframe, rotation, base->rotation );
	result.scale    = get_variant_or_default( scaleKeyframe, scale, base->scale );
	if( base->assetType == AnimatorAsset::type_collection ) {
		auto frameKeyframe = getInterpolatedKeyframe( animator, animator->currentFrame, frameId );
		result.voxel.frame = get_variant_or_default( frameKeyframe, frame, base->voxel.frame );
	}

	result.parent   = node->parent;
	result.selected = node->selected;
	return result;
}
AnimatorNode toAbsoluteNode( const AnimatorNode* base, const AnimatorNode* rel )
{
	auto result = *base;
	result.translation += rel->translation;
	result.rotation += rel->rotation;
	result.scale = multiplyComponents( result.scale, rel->scale );
	if( base->assetType == AnimatorAsset::type_collection ) {
		result.voxel.frame += rel->voxel.frame;
	}

	result.parent   = rel->parent;
	return result;
}
AnimatorNode toRelativeNode( const AnimatorNode* base, const AnimatorNode* abs )
{
	auto result        = *base;
	result.translation = abs->translation - base->translation;
	result.rotation    = abs->rotation - base->rotation;
	result.scale.x     = safeDivide( abs->scale.x, base->scale.x, 1 );
	result.scale.y     = safeDivide( abs->scale.y, base->scale.y, 1 );
	result.scale.z     = safeDivide( abs->scale.z, base->scale.z, 1 );
	if( base->assetType == AnimatorAsset::type_collection ) {
		result.voxel.frame = abs->voxel.frame - base->voxel.frame;
	}

	result.parent   = abs->parent;
	return result;
}
AnimatorKeyframe toAbsoluteKeyframe( const AnimatorNode* base, const AnimatorKeyframe* keyframe )
{
	auto result = *keyframe;
	switch( result.data.type ) {
		case AnimatorKeyframeData::type_translation: {
			result.data.translation += base->translation;
			break;
		}
		case AnimatorKeyframeData::type_rotation: {
			result.data.rotation += base->rotation;
			break;
		}
		case AnimatorKeyframeData::type_scale: {
			result.data.scale = multiplyComponents( base->scale, result.data.scale );
			break;
		}
		case AnimatorKeyframeData::type_frame: {
			result.data.frame += base->voxel.frame;
			break;
		}
		InvalidDefaultCase;
	}
	return result;
}
AnimatorKeyframe toRelativeKeyframe( const AnimatorNode* base, const AnimatorKeyframe* keyframe )
{
	auto result = *keyframe;
	switch( result.data.type ) {
		case AnimatorKeyframeData::type_translation: {
			result.data.translation -= base->translation;
			break;
		}
		case AnimatorKeyframeData::type_rotation: {
			result.data.rotation -= base->rotation;
			break;
		}
		case AnimatorKeyframeData::type_scale: {
			result.data.scale.x = safeDivide( result.data.scale.x, base->scale.x, 1 );
			result.data.scale.y = safeDivide( result.data.scale.y, base->scale.y, 1 );
			result.data.scale.z = safeDivide( result.data.scale.z, base->scale.z, 1 );
			break;
		}
		case AnimatorKeyframeData::type_frame: {
			result.data.frame -= base->voxel.frame;
			break;
		}
		InvalidDefaultCase;
	}
	return result;
}

bool bindParent( AnimatorState* animator, AnimatorNode* node )
{
	bool result = true;
	node->parent = nullptr;
	if( node->parentId >= 0 ) {
		auto id = node->parentId;
		if( auto parent = find_first_where( animator->nodes, entry->id == id ) ) {
			node->parent = parent->get();
			node->parent->childrenCount += node->childrenCount + 1;
		} else {
			result = false;
		}
	}
	if( !node->parent ) {
		node->parentId = -1;
	}
	return result;
}
AnimatorNode* addNewNode( AnimatorState* animator, const AnimatorNode& node )
{
	animator->nodes.push_back( std::make_unique< AnimatorNode >( node ) );
	auto result = animator->nodes.back().get();
	result->id  = animator->nodeIds++;
	bindParent( animator, result );
	if( result->name.size() <= 0 ) {
		auto idString = toNumberString( result->id );
		result->name.resize( snprint( result->name.data(), result->name.capacity(), "Unnamed {}",
		                              StringView{idString} ) );
	}
	addAnimatorGroups( animator, result->name, makeArrayView( animator->fieldNames ), result->id );
	setUnsavedChanges( animator );
	return result;
}
AnimatorNode* addExistingNode( AnimatorState* animator, const AnimatorNode& node )
{
	animator->nodes.push_back( std::make_unique< AnimatorNode >( node ) );
	auto result           = animator->nodes.back().get();
	result->parent        = nullptr;
	result->childrenCount = 0;
	addAnimatorGroups( animator, result->name, makeArrayView( animator->fieldNames ), result->id );
	return result;
}

AnimatorAsset* addNewAsset( AnimatorState* animator, StringView filename, AnimatorAsset::Type type )
{
	AnimatorAsset* result = nullptr;
	switch( type ) {
		case AnimatorAsset::type_collection: {
			auto asset = std::make_unique< AnimatorAsset >(
			    animatorLoadVoxelCollectionAsset( filename, animator->assetIds ) );
			if( asset ) {
				animator->assets.emplace_back( std::move( asset ) );
				++animator->assetIds;
				result = animator->assets.back().get();
			}
			break;
		}
	}
	return result;
}

void sortNodes( AnimatorState* animator )
{
	sort( animator->nodes.begin(), animator->nodes.end(),
	      []( const UniqueAnimatorNode& a, const UniqueAnimatorNode& b ) {
		      return a->childrenCount > b->childrenCount;
		  } );
}
void sortKeyframes( AnimatorState* animator )
{
	sort( animator->keyframes.begin(), animator->keyframes.end(), compareKeyframes );
}

void freeCurveData( AnimatorState* animator, AnimatorKeyframe* keyframe )
{
	if( keyframe->data.curveIndex == ::size( animator->curves ) - 1 ) {
		animator->curves.pop_back();
	} else {
		auto curve  = &animator->curves[keyframe->data.curveIndex];
		curve->used = false;
	}
	keyframe->data.curveIndex = -1;
}
void setKeyframeEaseType( AnimatorState* animator, AnimatorKeyframe* keyframe,
                          AnimatorKeyframeData::EaseType type )
{
	if( keyframe->data.easeType != type ) {
		if( keyframe->data.easeType == AnimatorKeyframeData::Curve ) {
			freeCurveData( animator, keyframe );
		} else if( type == AnimatorKeyframeData::Curve ) {
			auto index = find_index_if(
			    animator->curves, []( const AnimatorCurveData& entry ) { return !entry.used; } );
			if( !index ) {
				index = ::size( animator->curves );
				animator->curves.emplace_back();
			}
			auto curve = &animator->curves[index.get()];
			curve->used = true;
			curve->curve0 = {1, 0};
			curve->curve1 = {0, 1};
			curve->differencer = makeEasingCurve( curve->curve0, curve->curve1 );
			keyframe->data.curveIndex = safe_truncate< int16 >( index.get() );
		}
		keyframe->data.easeType = type;
	}
}
void addKeyframe( AnimatorState* animator, const AnimatorKeyframe& key )
{
	auto existing = find_first_where( animator->keyframes,
	                                  entry->group == key.group && floatEqSoft( entry->t, key.t ) );
	if( existing ) {
		**existing = key;
	} else {
		animator->keyframes.push_back( std::make_unique< AnimatorKeyframe >( key ) );
		insertion_sort_last_elem( animator->keyframes.begin(), animator->keyframes.end(),
		                          compareKeyframes );
	}
	setUnsavedChanges( animator );
}
// delete duplicate keyframes, keeping those that are selected
// need functor because equal_range passes in arguments both ways, so we can't use a simple
// lambda if the types of the two arguments don't match
struct DeleteDuplicateKeyframesFunctor {
	bool operator()( const AnimatorKeyframes::value_type& a, const AnimatorKeyframe* b ) const
	{
		return a->t < b->t;
	}
	bool operator()( const AnimatorKeyframe* a, const AnimatorKeyframes::value_type& b ) const
	{
		return a->t < b->t;
	}
};

void onDeleteKeyframe( AnimatorState* animator, AnimatorKeyframe* keyframe )
{
	if( keyframe->data.easeType == AnimatorKeyframeData::Curve ) {
		freeCurveData( animator, keyframe );
	}
}
void deleteDuplicateKeyframes( AnimatorState* animator )
{
	if( !animator->currentAnimation ) {
		return;
	}
	FOR( selected : animator->selected ) {
		assert( selected->selected );
		auto range = equal_range( animator->keyframes.begin(), animator->keyframes.end(), selected,
		                          DeleteDuplicateKeyframesFunctor() );
		auto group = selected->group;
		erase_if( animator->keyframes, range.first, range.second,
		          [animator, group]( const AnimatorKeyframes::value_type& key ) {
			          if( !key->selected && key->group == group ) {
				          onDeleteKeyframe( animator, key.get() );
				          return true;
			          }
			          return false;
			      } );
	}
	setUnsavedChanges( animator );
}
void deleteSelectedKeyframes( AnimatorState* animator )
{
	erase_if( animator->keyframes, [animator]( const AnimatorKeyframes::value_type& key ) {
		if( key->selected ) {
			onDeleteKeyframe( animator, key.get() );
			return true;
		}
		return false;
	} );
	clearSelectedKeyframes( animator );
	setUnsavedChanges( animator );
}
void deleteSelectedAssets( AnimatorState* animator )
{
	erase_if( animator->assets, [animator]( const UniqueAnimatorAsset& entry ) {
		auto asset = entry.get();
		if( asset->item.selected ) {
			FOR( it : animator->nodes ) {
				auto node = it.get();
				if( node->asset == asset ) {
					node->assetId   = -1;
					node->assetType = AnimatorAsset::type_none;
					node->asset     = nullptr;
					// TODO: delete keyframes
				}
			}
			FOR( it : animator->baseNodes ) {
				auto node = it.get();
				if( node->asset == asset ) {
					node->assetId   = -1;
					node->assetType = AnimatorAsset::type_none;
					node->asset   = nullptr;
				}
			}
			return true;
		}
		return false;
	} );
	setUnsavedChanges( animator );
}

void openAnimation( AnimatorState* animator, AnimatorAnimation* animation )
{
	assert( !animator->currentAnimation );
	animator->currentAnimation         = animation;
	animator->flags.uncommittedChanges = false;

	animator->baseNodes = std::move( animator->nodes );
	animator->nodes.clear();
	// clear marked flag
	FOR( node : animator->baseNodes ) {
		node->marked = false;
	}

	clearAnimatorGroups( animator );

	FOR( node : animation->nodes ) {
		auto baseIt = find_first_where( animator->baseNodes, entry->id == node.id );
		if( baseIt ) {
			auto base = baseIt->get();
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
		bindParent( animator, node.get() );
	}
	sortNodes( animator );
	clearSelectedNodes( animator );

	populateVisibleGroups( animator );

	animator->keyframes.end();
	animator->keyframes.reserve( animation->keyframes.size() );
	FOR( keyframe : animation->keyframes ) {
		auto base = find_first_where( animator->baseNodes,
		                              entry->id == getAnimatorKeyframeOwner( keyframe.group ) );
		assert( base );
		animator->keyframes.push_back(
		    std::make_unique< AnimatorKeyframe >( toAbsoluteKeyframe( base->get(), &keyframe ) ) );
	}
}

void commitAnimation( AnimatorState* animator )
{
	assert( animator->currentAnimation );
	auto animation = animator->currentAnimation;
	animation->nodes.resize( animator->nodes.size() );

	zip_for( animation->nodes, animator->nodes ) {
		auto base = find_first_where( animator->baseNodes, entry->id == ( *second )->id );
		if( base ) {
			*first = toRelativeNode( base->get(), second->get() );
		}
	}

	animation->keyframes.resize( animator->keyframes.size() );
	zip_for( animation->keyframes, animator->keyframes ) {
		auto base = find_first_where(
		    animator->baseNodes, entry->id == getAnimatorKeyframeOwner( second->get()->group ) );
		*first = toRelativeKeyframe( base->get(), second->get() );
	}
}
void closeAnimation( AnimatorState* animator, bool keep )
{
	assert( animator->currentAnimation );

	if( keep ) {
		commitAnimation( animator );
	}

	animator->keyframes.clear();
	clearAnimatorGroups( animator );
	animator->nodes = std::move( animator->baseNodes );
	animator->baseNodes.clear();
	animator->currentAnimation = nullptr;
	clearSelectedNodes( animator );
	clearSelectedKeyframes( animator );
}

void animatorMessageBox( AnimatorState* animator, StringView text, StringView title,
                         AnimatorMessageBoxAction* onYes, AnimatorMessageBoxAction* onNo = nullptr )
{
	animator->messageBox.text         = text;
	animator->messageBox.title        = title;
	animator->messageBox.action.onYes = onYes;
	animator->messageBox.action.onNo  = onNo;
	animator->messageBox.type = ( onYes && onNo ) ? ( AnimatorState::MessageBox::YesNoCancel )
	                                              : ( AnimatorState::MessageBox::OkCancel );
	imguiShowModal( animator->messageBox.container );
}

void animatorClear( AnimatorState* animator )
{
	animator->assets.clear();
	animator->keyframes.clear();
	animator->selected.clear();
	animator->groups.clear();
	animator->visibleGroups.clear();
	animator->baseNodes.clear();
	animator->nodes.clear();
	animator->animations.clear();
	animator->filename.clear();
	animator->currentAnimation         = nullptr;
	animator->duration                 = 0;
	animator->currentFrame             = 0;
	animator->flags.unsavedChanges     = false;
	animator->flags.uncommittedChanges = false;
	animator->nodeIds                  = 0;
	animator->assetIds                 = 0;
	animator->messageBox.action        = {};

	clearSelectedKeyframes( animator );
	clearSelectedNodes( animator );
	clearAnimatorGroups( animator );
}
void animatorSave( StackAllocator* allocator, AnimatorState* animator, StringView filename )
{
	auto writeNode = []( JsonWriter* writer, AnimatorNode* node ) {
		writeStartObject( writer );
			writeProperty( writer, "translation", node->translation );
			writeProperty( writer, "rotation", node->rotation );
			writeProperty( writer, "scale", node->scale );
			writeProperty( writer, "length", node->length );
			writeProperty( writer, "id", node->id );
			writeProperty( writer, "parentId", NullableInt32{node->parentId} );
			writeProperty( writer, "assetId", NullableInt32{node->assetId} );
			switch( node->assetType ) {
				case AnimatorAsset::type_none: {
					break;
				}
				case AnimatorAsset::type_collection: {
					writePropertyName( writer, "voxel" );
					writeStartObject( writer );
						writeProperty( writer, "animation", NullableInt32{node->voxel.animation} );
						writeProperty( writer, "frame", node->voxel.frame );
					writeEndObject( writer );
					break;
				}
				InvalidDefaultCase;
			}
			writeProperty( writer, "name", StringView{node->name} );
		writeEndObject( writer );
	};

	auto writeNodeKeyableProperties = []( JsonWriter* writer, AnimatorNode* node ) {
		writeProperty( writer, "translation", node->translation );
		writeProperty( writer, "rotation", node->rotation );
		writeProperty( writer, "scale", node->scale );
		writeProperty( writer, "id", node->id );
		switch( node->assetType ) {
			case AnimatorAsset::type_none: {
				break;
			}
			case AnimatorAsset::type_collection: {
				writeProperty( writer, "frame", node->voxel.frame );
				break;
			}
			InvalidDefaultCase;
		}
	};

	auto writeKeyframes = []( JsonWriter* writer, Array< AnimatorKeyframe > keyframes,
	                          StringView name, GroupId id ) {
		writePropertyName( writer, name );
		writeStartArray( writer );
			FOR( keyframe : keyframes ) {
				if( keyframe.group == id ) {
					writeStartObject( writer );
					writer->minimal = true;
						writeProperty( writer, "t", keyframe.t );
						writeProperty( writer, "easeType", to_string( keyframe.data.easeType ) );
						switch( keyframe.data.type ) {
							case AnimatorKeyframeData::type_translation: {
								writeProperty( writer, "value", keyframe.data.translation );
								break;
							}
							case AnimatorKeyframeData::type_rotation: {
								writeProperty( writer, "value", keyframe.data.rotation );
								break;
							}
							case AnimatorKeyframeData::type_scale: {
								writeProperty( writer, "value", keyframe.data.scale );
								break;
							}
							case AnimatorKeyframeData::type_frame: {
								writeProperty( writer, "value", keyframe.data.frame );
								break;
							}
							InvalidDefaultCase;
						}
					writeEndObject( writer );
					writer->minimal = false;
				}
			}
		writeEndArray( writer );
	};

	auto writeAsset = []( JsonWriter* writer, const AnimatorAsset* asset ) {
		writeStartObject( writer );
			writeProperty( writer, "type", to_string( asset->type ) );
			switch( asset->type ) {
				case AnimatorAsset::type_collection: {
					assert( asset->collection );
					writeProperty( writer, "filename", asset->collection->voxels.filename );
					break;
				}
				InvalidDefaultCase;
			}
			writeProperty( writer, "id", asset->id );
			writeProperty( writer, "name", StringView{asset->name, asset->nameLength} );
		writeEndObject( writer );
	};

	AnimatorAnimation* current = animator->currentAnimation;
	if( animator->currentAnimation ) {
		closeAnimation( animator, true );
	}

	TEMPORARY_MEMORY_BLOCK( allocator ) {
		auto bufferSize = (int32)getCapacityFor< char >( allocator );
		auto buffer     = allocateArray( allocator, char, bufferSize );
		auto writerObj  = makeJsonWriter( buffer, bufferSize );
		auto writer     = &writerObj;

		auto nodes = ( animator->currentAnimation ) ? &animator->baseNodes : &animator->nodes;

		writeStartObject( writer );
			writePropertyName( writer, "assets" );
			writeStartArray( writer );
				FOR( asset : animator->assets ) {
					writeAsset( writer, asset.get() );
				}
			writeEndArray( writer );

			writePropertyName( writer, "nodes" );
			writeStartArray( writer );
			FOR( node : *nodes ) {
				writeNode( writer, node.get() );
			}
			writeEndArray( writer );

			writePropertyName( writer, "animations" );
			writeStartArray( writer );
			FOR( animation : animator->animations ) {
				auto keyframes = makeArrayView( animation.keyframes );
				writeStartObject( writer );
					writePropertyName( writer, "nodes" );
					writeStartArray( writer );
					FOR( node : animation.nodes ) {
						writeStartObject( writer );
							writeNodeKeyableProperties( writer, &node );
							auto group = makeAnimatorGroup( node.id, 0 );
							writePropertyName( writer, "keyframes" );
							writeStartObject( writer );
								writeKeyframes( writer, keyframes, "translation", group + 1 );
								writeKeyframes( writer, keyframes, "rotation", group + 2 );
								writeKeyframes( writer, keyframes, "scale", group + 3 );
								switch( node.assetType ) {
									case AnimatorAsset::type_none: {
										break;
									}
									case AnimatorAsset::type_collection: {
										writeKeyframes( writer, keyframes, "frame", group + 4 );
										break;
									}
									InvalidDefaultCase;
								}
							writeEndObject( writer );
						writeEndObject( writer );
					}
					writeEndArray( writer );

					writeProperty( writer, "name", StringView{animation.name} );
				writeEndObject( writer );
			}
			writeEndArray( writer );
		writeEndObject( writer );

		GlobalPlatformServices->writeBufferToFile( filename, writer->data(), writer->size() );
		animator->flags.unsavedChanges     = false;
		animator->flags.uncommittedChanges = false;
	}

	if( current ) {
		openAnimation( animator, current );
	}
}

struct translation_tag {};
struct rotation_tag {};
struct scale_tag {};
struct frame_tag {};

vec3& getValue( AnimatorKeyframeData& data, translation_tag )
{
	data.type = AnimatorKeyframeData::type_translation;
	return data.translation;
}
vec3& getValue( AnimatorKeyframeData& data, rotation_tag )
{
	data.type = AnimatorKeyframeData::type_rotation;
	return data.rotation;
}
vec3& getValue( AnimatorKeyframeData& data, scale_tag )
{
	data.type = AnimatorKeyframeData::type_scale;
	return data.scale;
}
int16& getValue( AnimatorKeyframeData& data, frame_tag )
{
	data.type = AnimatorKeyframeData::type_frame;
	return data.frame;
}

template < class Tag >
void loadKeyframes( JsonObjectArray array, GroupId group, Tag tag,
                    std::vector< AnimatorKeyframe >* out )
{
	FOR( keyframe : array ) {
		out->emplace_back( AnimatorKeyframe{} );
		auto added   = &out->back();
		added->group = group;
		serialize( keyframe["t"], added->t );
		auto easeType = keyframe["easeType"];
		if( jsonIsString( easeType ) ) {
			from_string( easeType.getString(), &added->data.easeType );
		} else {
			added->data.easeType = (AnimatorKeyframeData::EaseType)clamp(
			    easeType.getInt(), AnimatorKeyframeData::Lerp, AnimatorKeyframeData::Curve );
		}
		serialize( keyframe["value"], getValue( added->data, tag ) );
	}
}

bool animatorOpen( StackAllocator* allocator, AnimatorState* animator, StringView filename )
{
	TEMPORARY_MEMORY_BLOCK( allocator ) {
		auto bufferSize = (int32)getCapacityFor< char >( allocator ) / 2;
		auto buffer     = allocateArray( allocator, char, bufferSize );
		auto fileSize =
		    (int32)GlobalPlatformServices->readFileToBuffer( filename, buffer, bufferSize );
		if( !fileSize ) {
			LOG( ERROR, "Unable to open file {}", filename );
			return false;
		}

		auto jsonDocSize                       = remaining( allocator );
		JsonStackAllocatorStruct jsonAllocator = {allocateArray( allocator, char, jsonDocSize ), 0,
		                                          jsonDocSize};
		auto doc = jsonMakeDocument( &jsonAllocator, buffer, fileSize, JSON_READER_REASONABLE );
		if( !doc || !doc.root.getObject() ) {
			if( !doc ) {
				LOG( ERROR, "{}: Json error {}", filename, jsonGetErrorString( doc.errorType ) );
			} else {
				LOG( ERROR, "{}: Json root not object", filename );
			}
			return false;
		}

		animatorClear( animator );
		auto root = doc.root.getObject();

		FOR( asset : root["assets"].getObjectArray() ) {
			auto type = convert_to< AnimatorAsset::Type >( asset["type"].getString() );
			if( auto added = addNewAsset( animator, asset["filename"].getString(), type ) ) {
				serialize( asset["id"], added->id, -1 );
				added->setName( asset["name"].getString() );
			}
		}

		FOR( node : root["nodes"].getObjectArray() ) {
			auto addedContainer = std::make_unique< AnimatorNode >();
			auto added          = addedContainer.get();
			serialize( node["translation"], added->translation );
			serialize( node["rotation"], added->rotation );
			serialize( node["scale"], added->scale );
			serialize( node["length"], added->length );
			serialize( node["id"], added->id, -1 );
			serialize( node["parentId"], added->parentId, -1 );
			serialize( node["assetId"], added->assetId, -1 );
			auto voxel = node["voxel"].getObject();
			serialize( voxel["animation"], added->voxel.animation, -1 );
			serialize( voxel["frame"], added->voxel.frame );
			added->name = node["name"].getString();
			animator->nodes.push_back( std::move( addedContainer ) );
		}
		FOR( node : animator->nodes ) {
			auto parentId = node->parentId;
			if( !bindParent( animator, node.get() ) ) {
				LOG( ERROR, "{}: parent {} not found", parentId );
				return false;
			}
		}
		sortNodes( animator );

		FOR( animation : root["animations"].getObjectArray() ) {
			animator->animations.emplace_back();
			auto added = &animator->animations.back();
			added->name = animation["name"].getString();
			auto nodes = animation["nodes"].getObjectArray();
			added->nodes.reserve( nodes.size() );
			FOR( node : nodes ) {
				added->nodes.emplace_back();
				auto addedNode = &added->nodes.back();
				serialize( node["id"], addedNode->id, -1 );
				serialize( node["translation"], addedNode->translation );
				serialize( node["rotation"], addedNode->rotation );
				serialize( node["scale"], addedNode->scale );
				serialize( node["frame"], addedNode->voxel.frame );

				auto keyframes = node["keyframes"].getObject();
				auto group = makeAnimatorGroup( addedNode->id, 0 );
				loadKeyframes( keyframes["translation"].getObjectArray(), group + 1,
				               translation_tag{}, &added->keyframes );
				loadKeyframes( keyframes["rotation"].getObjectArray(), group + 2, rotation_tag{},
				               &added->keyframes );
				loadKeyframes( keyframes["scale"].getObjectArray(), group + 3, scale_tag{},
				               &added->keyframes );
				loadKeyframes( keyframes["frame"].getObjectArray(), group + 4, frame_tag{},
				               &added->keyframes );
			}
		}
	}

	TEMPORARY_MEMORY_BLOCK( allocator ) {
		// check whether ids are unique
		int16 maxId = 0;
		auto cap = (int32)getCapacityFor< int16 >( allocator ) / 3;
		auto assetIds = makeUArray( allocator, int16, cap );
		FOR( entry : animator->assets ) {
			auto asset = entry.get();
			if( asset->id < 0 ) {
				LOG( ERROR, "{}: Invalid id {}", filename, asset->id );
				return false;
			}
			if( !append_unique( assetIds, asset->id ) ) {
				LOG( ERROR, "{}: Asset id {} not unique", filename, asset->id );
				return false;
			}
		}

		auto baseIds = makeUArray( allocator, int16, cap );
		FOR( entry : animator->nodes ) {
			auto node = entry.get();
			if( node->id < 0 ) {
				LOG( ERROR, "{}: Invalid id {}", filename, node->id );
				return false;
			}
			if( !append_unique( baseIds, node->id ) ) {
				LOG( ERROR, "{}: Node id {} not unique", filename, node->id );
				return false;
			}
			if( node->assetId >= 0 ) {
				auto index = find_index( assetIds, node->assetId );
				if( !index ) {
					LOG( ERROR, "{}: asset {} not found", filename, node->assetId );
					return false;
				}
				node->asset     = animator->assets[index.get()].get();
				node->assetType = node->asset->type;
			}
		}
		animator->nodeIds = maxId;

		auto ids = makeUArray( allocator, int16, cap );
		FOR( animation : animator->animations ) {
			ids.clear();
			FOR( entry : animation.nodes ) {
				auto node = &entry;
				if( node->id < 0 ) {
					LOG( ERROR, "{}: Invalid id {}", filename, node->id );
					return false;
				}
				if( !append_unique( ids, node->id ) ) {
					LOG( ERROR, "{}: Node id {} not unique", filename, node->id );
					return false;
				}
				if( !find_first_where( baseIds, entry == node->id ) ) {
					LOG( ERROR, "{}: id {} not found as base", node->id );
					return false;
				}
				if( maxId < node->id ) {
					maxId = node->id;
				}
			}
			// we don't need to check keyframe ids, since they are derived from node ids
			// if node ids are valid, so are keyframe ids

#if GAME_DEBUG
			// check whether all keyframes point to valid nodes
			FOR( keyframe : animation.keyframes ) {
				auto id = getAnimatorKeyframeOwner( keyframe.group );
				if( !find_first_where( ids, entry == id ) ) {
					LOG( ERROR, "{}: Invalid keyframe, no parent", filename );
					return false;
				}
			}
#endif
		}
	}
	animator->filename = filename;
	return true;
}

FilenameString animatorGetSaveFilename()
{
	auto result = getSaveFilename( JsonFilter, nullptr );
	if( result.size() && !equalsIgnoreCase( getFilenameExtension( result ), ".json" ) ) {
		result.append( ".json" );
	}
	return result;
}

bool animatorMenuSave( AppData* app )
{
	auto animator = &app->animatorState;
	if( !animator->filename.size() ) {
		animator->filename = animatorGetSaveFilename();
	}
	if( animator->filename.size() ) {
		animatorSave( &app->stackAllocator, animator, animator->filename );
	}
	return animator->filename.size() != 0;
}
bool animatorMenuSaveAs( AppData* app )
{
	auto animator = &app->animatorState;
	auto filename = animatorGetSaveFilename();
	if( filename.size() ) {
		animator->filename.assign( filename );
		animatorSave( &app->stackAllocator, animator, animator->filename );
	}
	return filename.size() != 0;
}
void animatorMenuOpen( AppData* app )
{
	auto animator = &app->animatorState;

	auto saveAndOpen = []( AppData* app ) {
		if( animatorMenuSave( app ) ) {
			animatorMenuOpen( app );
		}
	};
	auto open = []( AppData* app ) {
		auto animator = &app->animatorState;
		animator->flags.unsavedChanges = false;
		animatorMenuOpen( app );
	};

	if( animator->flags.unsavedChanges ) {
		animatorMessageBox( animator, "Save changes?", "Unsaved changes", saveAndOpen, open );
	} else {
		auto filename = getOpenFilename( JsonFilter, nullptr, false );
		if( filename.size() ) {
			if( !animatorOpen( &app->stackAllocator, animator, filename ) ) {
				animatorClear( animator );
			}
		}
	}
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
	addRenderCommandSingleQuad( renderer, rect );

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
			if( ( !group && animator->flags.timelineRootExpanded ) || ( group && group->expanded ) ) {
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
					animator->flags.timelineRootExpanded = !animator->flags.timelineRootExpanded;
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
	auto leftButton   = isKeyPressed( inputs, KC_LButton );
	if( imguiHasCapture( handle )
	    || ( isPointInside( inner, inputs->mouse.position ) && leftButton ) ) {
		imguiFocus( handle );
		imguiCapture( handle );
		auto pos = ( inputs->mouse.position.x - inner.left + offset ) / PaddleWidth;
		if( pos < 0 ) {
			pos = 0;
		}
		auto oldFrame = exchange( animator->currentFrame, pos );
		if( !floatEqSoft( oldFrame, pos ) && !leftButton ) {
			// current frame changed and mouse moved
			FOR( node : animator->nodes ) {
				*node = getCurrentFrameNode( animator, node.get() );
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
	if( animator->flags.selectionRectValid ) {
		animator->selectionA.x += movement;
		animator->selectionB.x += movement;
	}
	animator->flags.keyframesMoved = true;
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
	setUnsavedChanges( animator );
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

		if( !animator->flags.mouseSelecting && isKeyPressed( inputs, KC_LButton ) && !clickedAny ) {
			imguiFocus( handle );
			imguiCapture( handle );
			auto absSelection = getAbsSelection( animator, scrollable );
			if( animator->flags.selectionRectValid
			    && isPointInside( absSelection, inputs->mouse.position ) ) {
				// moving selection around
				animator->flags.moveSelection = true;
				animator->mouseStart =
				    ( inputs->mouse.position - absSelection.leftTop ) / PaddleWidth;
			} else {
				animator->flags.mouseSelecting     = true;
				animator->flags.selectionRectValid = false;
				animator->selectionA         = inputs->mouse.position - scrollable->inner.leftTop
				                       - animator->scrollableRegion.scrollPos;
				animator->selectionA.x /= PaddleWidth;
				clearSelectedKeyframes( animator );
			}
		}

		if( animator->flags.mouseSelecting ) {
			if( !isKeyDown( inputs, KC_LButton ) ) {
				animator->flags.mouseSelecting = false;

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
						if( firstGroup < ::size( animator->visibleGroups ) ) {
							auto visibleGroupsCount = ::size( animator->visibleGroups );
							firstGroup = clamp( firstGroup, 0, visibleGroupsCount - 1 );
							lastGroup  = clamp( lastGroup, 0, visibleGroupsCount );
							if( firstGroup == 0 ) {
								// top level group is every visible group
								FOR( visible : animator->visibleGroups ) {
									if( visible.group >= 0 ) {
										groups.push_back( visible.group );
										auto children = visible.children;
										for( auto i = children.min; i < children.max; ++i ) {
											groups.push_back( i );
										}
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
					FOR( entry : animator->keyframes ) {
						auto frame = entry.get();
						if( frame->t + 1 >= selection.left && frame->t < selection.right
						    && exists( groups, frame->group ) ) {

							frame->selected = true;
							animator->selected.push_back( frame );
							animator->flags.selectionRectValid = true;
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
			if( animator->flags.moveSelection ) {
				settleSelected( animator, altDown );
			}
			animator->flags.moveSelection = false;
		}
		if( animator->flags.selectionRectValid && animator->flags.moveSelection
		    && animator->selected.size() ) {
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
			animator->flags.moveSelection = false;
		}
	}
	if( animator->flags.mouseSelecting || animator->flags.selectionRectValid ) {
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
	if( animator->flags.selectionRectValid
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
		FOR( entry : animator->keyframes ) {
			auto frame = entry.get();
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
				animator->flags.moving = true;
			}
			if( custom.changed() ) {
				animator->flags.moving = true;
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
		FOR( entry : animator->keyframes ) {
			auto frame = entry.get();
			if( ( isRoot || isInRange( parent->children, frame->group ) )
			    && floatEqSoft( frame->t, clickedT ) ) {

				frame->selected = true;
				append_unique( animator->selected, frame );
			}
		}
	}
	if( animator->flags.moving ) {
		moveSelected( animator, movement );
	}
	if( !clickedAny && animator->flags.moving && animator->selected.size()
	    && !isKeyDown( inputs, KC_LButton ) ) {

		animator->flags.moving = false;
		settleSelected( animator, altDown );
		imguiFocus( handle );
	}
	if( !clickedAny && !isKeyDown( inputs, KC_LButton ) ) {
		animator->flags.moving = false;
	}
	gui->processInputs = true;
	if( animator->selected.empty() ) {
		animator->flags.selectionRectValid = false;
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

void doKeyframesControl( AppData* app, GameInputs* inputs, float dt )
{
	auto animator = &app->animatorState;
	animator->flags.keyframesMoved = false;

	auto layout              = imguiBeginColumn( NamesWidth );
	const auto numbersHeight = ImGui->style.innerPadding * 2 + stringHeight( ImGui->font );
	const auto regionHeight  = RegionHeight - numbersHeight;

	static const float ControlWidths[] = {
	    11,  // resume
	    14,  // play
	    5,   // pause
	    11,  // stop
	    10,  // repeat
	    10,  // mirror
	};
	float icw                             = 1.0f / 66;
	float ich                             = 1.0f / 11;
	static const rectf ControlTexCoords[] = {
		scale( RectWH( 0, 0, 11, 11 ), icw, ich ),
		scale( RectWH( 12, 0, 14, 11 ), icw, ich ),
		scale( RectWH( 27, 0, 5, 11 ), icw, ich ),
		scale( RectWH( 33, 0, 11, 11 ), icw, ich ),
		scale( RectWH( 45, 0, 10, 11 ), icw, ich ),
		scale( RectWH( 56, 0, 10, 11 ), icw, ich ),
	};
	imguiSameLine( 6 );
	imguiAddItem( 2, 11 );
	auto buttonHandle = imguiMakeHandle( app, ImGuiControlType::Custom );
	auto playIndex    = ( animator->flags.playing ) ? ( 2 ) : ( 0 );
	if( imguiIconButton( buttonHandle, ControlWidths[0], numbersHeight, animator->controlIcons,
	                     makeQuadTexCoords( ControlTexCoords[playIndex] ), ControlWidths[playIndex],
	                     11 ) ) {
		animator->flags.playing = !animator->flags.playing;
	}
	if( imguiIconButton( buttonHandle, ControlWidths[1], numbersHeight, animator->controlIcons,
	                     makeQuadTexCoords( ControlTexCoords[1] ), ControlWidths[1], 11 ) ) {
		animator->flags.playing = true;
		animator->currentFrame  = 0;
	}
	if( imguiIconButton( buttonHandle, ControlWidths[3], numbersHeight, animator->controlIcons,
	                     makeQuadTexCoords( ControlTexCoords[3] ), ControlWidths[3], 11 ) ) {
		animator->flags.playing = false;
	}
	auto renderer   = ImGui->renderer;
	renderer->color = ( animator->flags.repeat ) ? ( Color::Red ) : ( Color::White );
	if( imguiIconButton( buttonHandle, ControlWidths[4], numbersHeight, animator->controlIcons,
	                     makeQuadTexCoords( ControlTexCoords[4] ), ControlWidths[4], 11 ) ) {
		animator->flags.repeat = !animator->flags.repeat;
	}
	renderer->color = ( animator->flags.mirror ) ? ( Color::Red ) : ( Color::White );
	if( imguiIconButton( buttonHandle, ControlWidths[5], numbersHeight, animator->controlIcons,
	                     makeQuadTexCoords( ControlTexCoords[5] ), ControlWidths[5], 11 ) ) {
		animator->flags.mirror = !animator->flags.mirror;
	}
	renderer->color = Color::White;

	doKeyframesNames( app, inputs, NamesWidth, regionHeight );

	const float curveControlWidth = regionHeight;
	const auto regionWidth        = app->width - NamesWidth - curveControlWidth;

	auto width = animator->duration + regionWidth;
	imguiNextColumn( &layout, regionWidth );
	animator->scrollableRegion.dim = {width, regionHeight};

	auto handle = imguiMakeHandle( &animator->duration, ImGuiControlType::Custom );

	doKeyframesFrameNumbers( handle, app, inputs, regionWidth );
	auto scrollable =
	    imguiBeginScrollableRegion( &animator->scrollableRegion, regionWidth, regionHeight, true );
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

	if( animator->flags.keyframesMoved ) {
		sortKeyframes( animator );
	}

	if( imguiHasMyChildFocus( handle ) && isKeyPressed( inputs, KC_Delete ) ) {
		// delete selected keyframes
		deleteSelectedKeyframes( animator );
	}

	// curve control
	imguiNextColumn( &layout, curveControlWidth );

	if( animator->selected.size() ) {
		auto first = animator->selected[0];
		auto comboHandle = imguiMakeHandle( &first->data.easeType );
		int32 index = (int32)first->data.easeType;
		if( imguiCombo( comboHandle, &index, makeArrayView( EaseTypeNames ), false ) ) {
			FOR( entry : animator->selected ) {
				setKeyframeEaseType( animator, entry, (AnimatorKeyframeData::EaseType)index );
			}
		}

		auto container = imguiCurrentContainer();
		auto curveControlHeight = app->height - container->rect.bottom - 10;
		if( first->data.easeType == AnimatorKeyframeData::Curve ) {
			curveControlHeight -= 24;
		}
		auto curveControlRect = imguiAddItem( curveControlWidth - 10, curveControlHeight );
		curveControlRect = imguiInnerRect( curveControlRect );
		setTexture( renderer, 0, null );
		if( first->data.easeType == AnimatorKeyframeData::Step ) {
			LINE_MESH_STREAM_BLOCK( stream, renderer ) {
				stream->color = Color::White;
				const auto count = 3;
				if( !hasCapacity( stream, count, count + 1 ) ) {
					break;
				}
				pushLineStripVertexUnchecked( stream, curveControlRect.left,
				                              curveControlRect.bottom );
				pushLineStripVertexUnchecked( stream, curveControlRect.right,
				                              curveControlRect.bottom );
				pushLineStripVertexUnchecked( stream, curveControlRect.right,
				                              curveControlRect.top );
				pushEndLineStripUnchecked( stream );
			}
		} else {
			dt *= GameConstants::DeltaToFrameTime;
			rectf curveRect   = curveControlRect;
			rectf curveDomain = {0, 1.1f, 1, -0.1f};
			if( first->data.easeType == AnimatorKeyframeData::Curve ) {
				auto curve         = &animator->curves[first->data.curveIndex];
				curveDomain.top    = max( curve->curve0.y, curve->curve1.y, 1.0f ) + 0.1f;
				curveDomain.bottom = min( curve->curve0.y, curve->curve1.y, 0.0f ) - 0.1f;
				auto h             = height( curveControlRect ) / -::height( curveDomain );
				curveRect.top    = curveControlRect.bottom + h * ( curveDomain.bottom - 1 );
				curveRect.bottom = curveControlRect.bottom + h * curveDomain.bottom;
			} else if( first->data.easeType == AnimatorKeyframeData::EaseOutElastic ) {
				curveRect.top += ::height( curveRect ) * 0.25f;
			}
			LINE_MESH_STREAM_BLOCK( stream, renderer ) {
				stream->color                = Color::White;
				pushQuadOutline( stream, curveRect );

				static const int32 Counts[6] = {2, 0, 20, 40, 40, 20};
				assert( first->data.easeType >= 0 && first->data.easeType < 6 );
				const auto count = Counts[first->data.easeType];
				if( !hasCapacity( stream, count, count + 1 ) ) {
					break;
				}
				auto curves = makeArrayView( animator->curves );
				auto w      = ::width( curveRect );
				auto h      = ::height( curveRect );
				for( auto i = 0; i < count; ++i ) {
					auto t = i * ( 1.0f / ( count - 1 ) );
					auto d = animatorKeyframeEase( curves, first->data.easeType,
					                               first->data.curveIndex, t );

					auto x = t * w + curveRect.left;
					auto y = ( 1 - d ) * h + curveRect.top;
					pushLineStripVertexUnchecked( stream, x, y );
				}
				pushEndLineStripUnchecked( stream );

				if( first->data.easeType == AnimatorKeyframeData::Curve ) {
					auto curve = &animator->curves[first->data.curveIndex];
					auto c0    = curve->curve0;
					c0.x       = c0.x * w + curveRect.left;
					c0.y       = ( 1 - c0.y ) * h + curveRect.top;
					auto c1    = curve->curve1;
					c1.x       = c1.x * w + curveRect.left;
					c1.y       = ( 1 - c1.y ) * h + curveRect.top;
					pushLine2( stream, {curveRect.left, curveRect.bottom}, c0 );
					pushLine2( stream, {curveRect.right, curveRect.top}, c1 );
				}
			}
			if( first->data.easeType == AnimatorKeyframeData::Curve ) {
				auto curve   = &animator->curves[first->data.curveIndex];
				auto changed = false;
				auto c0              = curve->curve0.y;
				auto c1              = curve->curve1.y;
				const float maxDelta = 0.01f * dt;
				if( imguiPoint( &curve->curve0, curveDomain, curveControlRect ) ) {
					if( curve->curve0.y < curveDomain.bottom + 0.1f && curve->curve0.y < c0
					    && curve->curve0.y - c0 < maxDelta ) {
						curve->curve0.y = c0 - maxDelta;
					} else if( curve->curve0.y > curveDomain.top - 0.1f && curve->curve0.y > c0
					           && curve->curve0.y - c0 > maxDelta ) {
						curve->curve0.y = c0 + maxDelta;
					}
					changed = true;
				}
				if( imguiPoint( &curve->curve1, curveDomain, curveControlRect ) ) {
					if( curve->curve1.y < curveDomain.bottom + 0.1f && curve->curve1.y < c1
					    && curve->curve1.y - c1 < maxDelta ) {
						curve->curve1.y = c1 - maxDelta;
					} else if( curve->curve1.y > curveDomain.top - 0.1f && curve->curve1.y > c1
					           && curve->curve1.y - c1 > maxDelta ) {
						curve->curve1.y = c1 + maxDelta;
					}
					changed = true;
				}
				imguiSameLine( 2 );
				auto format = defaultPrintFormat();
				format.precision = 3;
				if( imguiEditbox( &curve->curve0, format ) ) {
					curve->curve0.x = clamp( curve->curve0.x );
					changed = true;
				}
				if( imguiEditbox( &curve->curve1, format ) ) {
					curve->curve1.x = clamp( curve->curve1.x );
					changed = true;
				}
				if( changed ) {
					curve->differencer = makeEasingCurve( curve->curve0, curve->curve1 );
				}
			}
		}
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

	auto doRotation = [&]( StringView text, float* val ) {
		imguiSameLine( 2 );
		auto handle = imguiMakeHandle( val, ImGuiControlType::None );
		float adjusted = radiansToDegrees( *val );
		if( imguiEditbox( handle, text, &adjusted ) ) {
			*val = degreesToRadians( adjusted );
			setUnsavedChanges( animator );
		}
		if( imguiSlider( handle, &adjusted, -180, 180 ) ) {
			*val = degreesToRadians( adjusted );
			setUnsavedChanges( animator );
		}
	};

	renderer->color = Color::White;
	if( imguiBeginDropGroup( "Editor", &editor->expandedFlags, AnimatorEditor::EditorSettings ) ) {
		imguiSlider( "Scale", &editor->view.scale, EditorViewMinScale, EditorViewMaxScale );
		if( imguiButton( "Reset View" ) ) {
			editor->view.translation = {};
			editor->view.rotation    = AnimatorInitialRotation;
			editor->view.scale       = 1;
		}
		imguiEndDropGroup();
	}
	if( imguiBeginDropGroup( "Assets", &editor->expandedFlags, AnimatorEditor::Assets ) ) {
		auto assets = makeArrayView( animator->assets );
		auto getter = []( UniqueAnimatorAsset& asset ) -> ImGuiListboxItem& { return asset->item; };
		imguiListboxIntrusive( &editor->assetsScrollPos, {assets, getter}, 200, 200 );
		auto firstIt = find_first_where( animator->assets, entry->item.selected );
		auto first   = ( firstIt ) ? ( firstIt->get() ) : ( nullptr );
		if( first ) {
			if( imguiEditbox( "Name", first->name, &first->nameLength, countof( first->name ) ) ) {
				first->setName( {first->name, first->nameLength} );
			}
		}
		imguiSameLine( 1 );
		if( imguiButton( "Add Voxel Collection", imguiRelative() ) ) {
			auto filename = getOpenFilename( JsonFilter, nullptr, false );
			if( filename.size() ) {
				addNewAsset( animator, filename, AnimatorAsset::type_collection );
			}
		}
		if( imguiButton( "Add Texture Map", imguiRelative() ) ) {

		}
		if( imguiButton( "Remove Selected", imguiRelative() ) ) {
			if( first ) {
				animatorMessageBox(
				    animator, "Selected assets will be deleted.", "Delete Assets",
				    []( AppData* app ) { deleteSelectedAssets( &app->animatorState ); } );
			}
		}
		imguiEndDropGroup();
	}
	if( imguiBeginDropGroup( "Nodes", &editor->expandedFlags, AnimatorEditor::Nodes ) ) {
		auto handle = imguiMakeHandle( &editor->nodesListboxScroll );
		int32 index = -1;
		if( auto clicked = editor->clickedNode ) {
			index = find_index_if( animator->nodes, [clicked]( const UniqueAnimatorNode& entry ) {
				        return entry.get() == clicked;
				    } ).value;
		}
		auto listRect = imguiAddItem( 200, 200 );
		if( imguiListboxSingleSelect(
		        handle, &editor->nodesListboxScroll, (const void*)animator->nodes.data(),
		        (int32)sizeof( UniqueAnimatorNode* ), ::size( animator->nodes ), &index, listRect,
		        []( const void* entry ) -> StringView {
			        assert_alignment( entry, alignof( UniqueAnimatorNode* ) );
			        auto node = (UniqueAnimatorNode*)entry;
			        return node->get()->name;
			    },
		        true ) ) {
			if( index < 0 || index > ::size( animator->nodes ) ) {
				editor->clickedNode = nullptr;
			} else {
				editor->clickedNode = animator->nodes[index].get();
			}
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
				if( imguiPushButton( handle, "Absolute",
				                     !animator->flags.showRelativeProperties ) ) {
					animator->flags.showRelativeProperties = false;
				}
				handle.shortIndex = 1;
				if( imguiPushButton( handle, "Relative",
				                     animator->flags.showRelativeProperties == true ) ) {
					animator->flags.showRelativeProperties = true;
				}
				if( animator->flags.showRelativeProperties ) {
					base = find_first_where( animator->baseNodes, entry->id == node.id )->get();
					node = toRelativeNode( base, selected );
				}
			}
			if( imguiEditbox( "Name", node.name.data(), &node.name.sz, node.name.capacity() ) ) {
				if( auto group = find_first_where( animator->groups,
				                                   entry.id == makeAnimatorGroup( node.id, 0 ) ) ) {
					group->name = node.name;
				}
			}
			if( imguiEditbox( "Length", &node.length ) ) {
				setUnsavedChanges( animator );
			}
			if( imguiEditbox( "Translation X", &node.translation.x ) ) {
				setUnsavedChanges( animator );
			}
			if( imguiEditbox( "Translation Y", &node.translation.y ) ) {
				setUnsavedChanges( animator );
			}
			if( imguiEditbox( "Translation Z", &node.translation.z ) ) {
				setUnsavedChanges( animator );
			}
			doRotation( "Rotation X", &node.rotation.x );
			doRotation( "Rotation Y", &node.rotation.y );
			doRotation( "Rotation Z", &node.rotation.z );
			if( imguiEditbox( "Scale X", &node.scale.x ) ) {
				setUnsavedChanges( animator );
			}
			if( imguiEditbox( "Scale Y", &node.scale.y ) ) {
				setUnsavedChanges( animator );
			}
			if( imguiEditbox( "Scale Z", &node.scale.z ) ) {
				setUnsavedChanges( animator );
			}
			{
				imguiSameLine( 2 );
				imguiText( "Parent" );
				auto comboHandle = imguiMakeHandle( &selected->parentId );
				int32 index      = -1;
				if( node.parent ) {
					auto pos = find_if( animator->nodes,
					                    [node]( const UniqueAnimatorNode& entry ) {
						                    return entry.get() == node.parent;
						                } );
					index = ::distance( animator->nodes.begin(), pos );
				}
				if( imguiCombo(
				        comboHandle, (const void*)animator->nodes.data(),
				        (int32)sizeof( UniqueAnimatorNode* ), (int32)animator->nodes.size(), &index,
				        []( const void* entry ) -> StringView {
					        assert_alignment( entry, alignof( UniqueAnimatorNode* ) );
					        auto node = (UniqueAnimatorNode*)entry;
					        return node->get()->name;
					    },
				        true ) ) {
					setUnsavedChanges( animator );
					if( index >= 0 ) {
						auto newParent = animator->nodes[index].get();
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
			{
				imguiSameLine( 2 );
				imguiText( "Asset" );
				auto comboHandle = imguiMakeHandle( &selected->assetType );
				int32 index      = -1;
				if( node.asset ) {
					index =
					    find_index_if( animator->assets, [node](
					                                         const UniqueAnimatorAsset& entry ) {
						    return entry.get() == node.asset;
						} ).value;
				}
				if( imguiCombo(
				        comboHandle, (const void*)animator->assets.data(),
				        (int32)sizeof( UniqueAnimatorAsset* ), ::size( animator->assets ), &index,
				        []( const void* entry ) -> StringView {
					        assert_alignment( entry, alignof( UniqueAnimatorAsset* ) );
					        auto asset = ( (UniqueAnimatorAsset*)entry )->get();
					        return {asset->name, asset->nameLength};
					    },
				        true ) ) {
					setUnsavedChanges( animator );
					if( index >= 0 ) {
						auto asset           = animator->assets[index].get();
						node.asset           = asset;
						node.assetType       = asset->type;
						node.voxel.animation = -1;
						node.voxel.frame     = 0;
					} else {
						node.asset     = nullptr;
						node.assetType = {};
					}
				}
			}
			if( node.assetType == AnimatorAsset::type_collection ) {
				auto collection = get_variant( *node.asset, collection );
				imguiSameLine( 2 );
				imguiText( "Voxel" );
				auto comboHandle = imguiMakeHandle( &node.voxel );
				int32 index      = node.voxel.animation;
				if( imguiCombo( comboHandle, &index, collection->names, true ) ) {
					node.voxel.animation = safe_truncate< int16 >( index );
					setUnsavedChanges( animator );
				}

				if( node.voxel.animation >= 0 ) {
					imguiSameLine( 2 );
				}
				if( imguiEditbox( "Frame", &node.voxel.frame ) ) {
					setUnsavedChanges( animator );
				}
				if( node.assetType == AnimatorAsset::type_collection
				    && node.voxel.animation >= 0 ) {

					auto frames = collection->voxels.animations[node.voxel.animation].range;
					float val = (float)( node.voxel.frame );
					if( imguiSlider( imguiMakeHandle( &node.voxel.frame ), &val, 0,
					                 (float)width( frames ) - 1 ) ) {
						node.voxel.frame = (uint16)val;
						setUnsavedChanges( animator );
					}
				}
			}

			if( animator->currentAnimation && animator->flags.showRelativeProperties ) {
				assert( base );
				*selected = toAbsoluteNode( base, &node );
			} else {
				*selected = node;
			}
		}
		imguiEndDropGroup();
	}

	{
		imguiSameLine( 1 );
		auto pushHandle = imguiMakeHandle( editor );
		auto pushButton = []( ImGuiHandle handle, uint8 index, StringView name,
		                      AnimatorMouseMode mode, AnimatorEditor* editor ) {
			handle.shortIndex = index;
			if( imguiPushButton( handle, name, editor->mouseMode == mode, imguiRelative() ) ) {
				editor->mouseMode = mode;
			}
		};
		pushButton( pushHandle, 0, "Select", AnimatorMouseMode::Select, editor );
		pushButton( pushHandle, 1, "Translate", AnimatorMouseMode::Translate, editor );
		pushButton( pushHandle, 2, "Rotate", AnimatorMouseMode::Rotate, editor );
	}

	if( imguiBeginDropGroup( "Plane", &editor->expandedFlags, AnimatorEditor::Plane ) ) {
		imguiSameLine( 1 );
		auto pushHandle = imguiMakeHandle( editor );
		auto pushButton = []( ImGuiHandle handle, uint8 index, StringView name,
		                      AnimatorMousePlane mode, AnimatorEditor* editor ) {
			handle.shortIndex = index;
			if( imguiPushButton( handle, name, editor->mousePlane == mode, imguiRelative() ) ) {
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
				animator->animations.emplace_back();
				auto added = &animator->animations.back();
				added->name.assign( "Unnamed" );
			}
			auto end      = animator->animations.end();
			auto selected = end;
			auto deleted  = end;
			for( auto it = animator->animations.begin(); it != end; ++it ) {
				imguiSameLine( 3 );
				imguiText( it->name );
				if( imguiButton( "Open" ) ) {
					selected = it;
				}
				if( imguiButton( "Delete" ) ) {
					deleted = it;
				}
			}
			if( selected != end ) {
				openAnimation( animator, &*selected );
			}
			if( deleted != end ) {
				animator->messageBox.action.data.toDelete = deleted;
				animatorMessageBox(
				    animator, "Animation will be deleted", "Delete Animation", []( AppData* app ) {
				    	auto animator = &app->animatorState;
					    animator->animations.erase( animator->messageBox.action.data.toDelete );
					} );
			}
			imguiEndDropGroup();
		}
	} else {
		if( imguiBeginDropGroup( "Animations", &editor->expandedFlags,
		                         AnimatorEditor::Animations ) ) {
			auto name = &animator->currentAnimation->name;
			imguiEditbox( "Name", name->data(), &name->sz, name->capacity() );
			if( imguiButton( "Commit" ) ) {
				commitAnimation( animator );
			}
			if( imguiButton( "Close" ) ) {
				if( animator->flags.uncommittedChanges ) {
					animatorMessageBox(
					    animator, "Keep changes?", "Uncommitted Changes",
					    []( AppData* app ) { closeAnimation( &app->animatorState, true ); },
					    []( AppData* app ) { closeAnimation( &app->animatorState, false ); } );
				} else {
					closeAnimation( animator, false );
				}
			}
			imguiEndDropGroup();
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
		if( current.voxel.frame == node->voxel.frame ) {
			frame.pop_back();
		}
		if( imguiButton( translation ) ) {
			AnimatorKeyframe key = {};
			key.t                = animator->currentFrame;
			key.group            = makeAnimatorGroup( node->id, 1 );
			set_variant( key.data, translation ) = node->translation;
			addKeyframe( animator, key );
		}
		if( imguiButton( rotation ) ) {
			AnimatorKeyframe key = {};
			key.t                = animator->currentFrame;
			key.group            = makeAnimatorGroup( node->id, 2 );
			set_variant( key.data, rotation ) = node->rotation;
			addKeyframe( animator, key );
		}
		if( imguiButton( scale ) ) {
			AnimatorKeyframe key = {};
			key.t                = animator->currentFrame;
			key.group            = makeAnimatorGroup( node->id, 3 );
			set_variant( key.data, scale ) = node->scale;
			addKeyframe( animator, key );
		}
		if( imguiButton( frame ) ) {
			AnimatorKeyframe key = {};
			key.t                = animator->currentFrame;
			key.group            = makeAnimatorGroup( node->id, 4 );
			set_variant( key.data, frame ) = node->voxel.frame;
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

void deleteDeep( AnimatorState* animator, AnimatorNodes::iterator node );
void deleteChildren( AnimatorState* animator, AnimatorNode* parent )
{
	auto first = animator->nodes.begin();
	auto last = animator->nodes.end();
	for( auto it = first; it != last; ) {
		if( ( *it )->parent == parent ) {
			deleteDeep( animator, it );
			first = animator->nodes.begin();
			last = animator->nodes.end();
			it = first;
			continue;
		}
		++it;
	}
}
void deleteDeep( AnimatorState* animator, AnimatorNodes::iterator node )
{
	if( animator->editor.clickedNode == node->get() ) {
		animator->editor.clickedNode = nullptr;
	}
	deleteChildren( animator, node->get() );
	unordered_erase( animator->nodes, node );
}
void deleteDeep( AnimatorState* animator, AnimatorNode* node )
{
	setUnsavedChanges( animator );
	auto it = find_if( animator->nodes, [node]( const UniqueAnimatorNode& entry ) {
		return entry.get() == node;
	} );
	deleteDeep( animator, it );
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

	auto mat = processEditorViewGui( &editor->view, inputs, rect );
	if( animator->flags.mirror ) {
		mat = matrixScale( -1, 1, 1 ) * mat;
	}
	renderer->view = mat;

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
	auto stack = renderer->matrixStack;
	pushMatrix( stack );
	FOR( entry : animator->nodes ) {
		auto node = entry.get();
		if( node->assetType == AnimatorAsset::type_collection && node->voxel.animation >= 0 ) {
			auto collection = get_variant( *node->asset, collection );
			setTexture( renderer, 0, collection->voxels.texture );
			auto voxels = &collection->voxels;
			auto range  = voxels->animations[node->voxel.animation].range;
			if( range ) {
				auto entry = &voxels->frames[range.min + ( node->voxel.frame % width( range ) )];
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
				selectNode( node.get(), head );
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

				FOR( entry : animator->nodes ) {
					auto node = entry.get();
					if( node->assetType == AnimatorAsset::type_collection
					    && node->voxel.animation >= 0 ) {

						auto collection = get_variant( *node->asset, collection );
						auto animation =
						    &collection->voxels.animations[node->voxel.animation];
						auto range      = animation->range;
						auto frameIndex = range.min + ( node->voxel.frame % width( range ) );
						auto info       = &collection->voxels.frameInfos[frameIndex];
						auto frame      = &collection->voxels.frames[frameIndex];
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
			setRenderState( renderer, RenderStateType::BackFaceCulling, false );

			auto mat      = matrixFromNormal( plane.normal );
			vec3 verts[4] = {plane.origin + transformVector3( mat, {-size, size, 0} ),
			                 plane.origin + transformVector3( mat, {size, size, 0} ),
			                 plane.origin + transformVector3( mat, {-size, -size, 0} ),
			                 plane.origin + transformVector3( mat, {size, -size, 0} )};
			renderer->color = 0x40FFFFFF;
			setTexture( renderer, 0, null );
			addRenderCommandSingleQuad( renderer, verts );

			setRenderState( renderer, RenderStateType::BackFaceCulling, true );
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
							node->rotation += dot( plane.rotationNormal, editor->clickedRotation )
							                  * plane.rotationNormal;
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
			node.voxel.animation = -1;
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

	inputs->disableEscapeForQuickExit = true;
	imguiBind( gui );
	if( !animator->initialized ) {
		*gui = defaultImmediateModeGui();
		imguiLoadDefaultStyle( gui, &app->platform );

		animator->controlIcons = app->platform.loadTexture( "Data/Images/animator_controls.png" );

		animator->editor.contextMenu   = imguiGenerateContainer( gui, {}, true );
		animator->fileMenu             = imguiGenerateContainer( gui, {}, true );
		animator->messageBox.container = imguiGenerateContainer( gui, {0, 0, 300, 10}, true );

		auto allocator           = &app->stackAllocator;
		const auto MaxNodes      = 10;
		const auto MaxKeyframes  = 50;
		const auto MaxAnimations = 20;
		animator->stringPool     = makeStringPool( allocator, 100 );

		animator->fieldNames[0] = pushString( &animator->stringPool, "Translation" );
		animator->fieldNames[1] = pushString( &animator->stringPool, "Rotation" );
		animator->fieldNames[2] = pushString( &animator->stringPool, "Scale" );
		animator->fieldNames[3] = pushString( &animator->stringPool, "Frame" );

		animator->editor.view.rotation = AnimatorInitialRotation;
		animator->editor.view.scale    = 1;
		animator->editor.view.z        = 200;
		animator->editor.expandedFlags |= AnimatorEditor::Properties;
		animator->flags.timelineRootExpanded = true;

#if 1
		AnimatorNode node;
		node.rotation = {0, 0, HalfPi32};
		node.length = 40;
		addNewNode( animator, node );
		node.parentId = 0;
		addNewNode( animator, node );
		populateVisibleGroups( animator );

		addNewAsset( animator, "Data/voxels/hero.json", AnimatorAsset::type_collection );
#endif

		animator->initialized = true;
	}

	renderer->color      = Color::White;
	renderer->clearColor = 0xFF1B2B34;

	setProjection( renderer, ProjectionType::Orthogonal );
	setRenderState( renderer, RenderStateType::DepthTest, false );

	if( animator->currentAnimation && animator->flags.playing ) {
		animator->currentFrame += dt * ( 60.0f / 1000.0f );
		if( animator->currentFrame > animator->duration ) {
			animator->currentFrame = animator->duration;
		}
		FOR( node : animator->nodes ) {
			*node = getCurrentFrameNode( animator, node.get() );
		}
		if( animator->currentFrame >= animator->duration ) {
			if( animator->flags.repeat ) {
				animator->currentFrame = 0;
			} else {
				animator->flags.playing = false;
			}
		}
	}

	rectf guiBounds = {0, 0, app->width, app->height};
	imguiBind( gui, renderer, font, inputs, app->stackAllocator.ptr, guiBounds );

	auto keyframesControlHeight =
	    ( animator->currentAnimation ) ? ( RegionHeight + MenuHeight ) : ( 0 );
	auto mainRowHeight = app->height - keyframesControlHeight - ImGui->style.innerPadding * 2;
	auto editorWidth   = app->width - PropertiesColumnWidth;
	auto layout        = imguiBeginColumn( app->width );

	if( imguiMenu( true ) ) {
		imguiMenuItem( "File", animator->fileMenu );
		imguiMenuItem( "Edit" );
		imguiMenuItem( "View" );
		imguiMenuItem( "Preferences" );
		imguiMenuItem( "Help" );
	}

	imguiEndColumn( &layout );
	imguiNextColumn( &layout, PropertiesColumnWidth );

	auto container = imguiCurrentContainer();
	auto menuRect =
	    RectSetHeight( translate( container->rect, container->addPosition ), mainRowHeight );
	doAnimatorMenu( app, inputs, menuRect );

	imguiNextColumn( &layout, editorWidth );
	auto editorRect = imguiAddItem( editorWidth, mainRowHeight );
	doEditor( app, inputs, editorRect );

	imguiEndColumn( &layout );

	if( animator->currentAnimation ) {
		doKeyframesControl( app, inputs, dt );
	}

	if( isHotkeyPressed( inputs, KC_O, KC_Control ) ) {
		animatorMenuOpen( app );
	}
	if( isHotkeyPressed( inputs, KC_S, KC_Control ) ) {
		animatorMenuSave( app );
	}
	if( isHotkeyPressed( inputs, KC_S, KC_Control, KC_Shift ) ) {
		animatorMenuSaveAs( app );
	}

	if( imguiContextMenu( animator->fileMenu ) ) {
		if( imguiContextMenuEntry( "New" ) ) {
			auto saveAndClear = []( AppData* app ) {
				if( animatorMenuSave( app ) ) {
					animatorClear( &app->animatorState );
				}
			};
			auto clear = []( AppData* app ) { animatorClear( &app->animatorState ); };
			if( animator->flags.unsavedChanges ) {
				animatorMessageBox( animator, "Save changes?", "Unsaved changes", saveAndClear,
				                    clear );
			} else {
				clear( app );
			}
		}
		if( imguiContextMenuEntry( "Open" ) ) {
			animatorMenuOpen( app );
		}

		if( imguiContextMenuEntry( "Save" ) ) {
			animatorMenuSave( app );
		}
		if( imguiContextMenuEntry( "Save As" ) ) {
			animatorMenuSaveAs( app );
		}
		imguiEndContainer();
	}

	if( imguiDialog( animator->messageBox.title, animator->messageBox.container ) ) {
		auto messageBox = &animator->messageBox;

		if( isKeyPressed( inputs, KC_Return ) ) {
			if( messageBox->action.onYes ) {
				messageBox->action.onYes( app );
			}
			imguiClose( messageBox->container );
		} else if( isKeyPressed( inputs, KC_Escape ) ) {
			imguiClose( messageBox->container );
		} else {
			imguiText( messageBox->text );
			switch( messageBox->type ) {
				case AnimatorState::MessageBox::OkCancel: {
					imguiSameLine( 2 );
					if( imguiButton( "Ok" ) ) {
						if( messageBox->action.onYes ) {
							messageBox->action.onYes( app );
						}
						imguiClose( messageBox->container );
					}
					if( imguiButton( "Cancel" ) ) {
						imguiClose( messageBox->container );
					}
					break;
				}
				case AnimatorState::MessageBox::YesNoCancel: {
					imguiSameLine( 3 );
					if( imguiButton( "Yes" ) ) {
						if( messageBox->action.onYes ) {
							messageBox->action.onYes( app );
						}
						imguiClose( messageBox->container );
					}
					if( imguiButton( "No" ) ) {
						if( messageBox->action.onNo ) {
							messageBox->action.onNo( app );
						}
						imguiClose( messageBox->container );
					}
					if( imguiButton( "Cancel" ) ) {
						imguiClose( messageBox->container );
					}
					break;
				}
			}
		}
	}

	imguiUpdate( dt );
	imguiFinalize();

#if 1
	// debug
	debugPrintln( "AnimatorKeyframes Count: {}", animator->keyframes.size() );
#endif
}

// AnimatorAsset
AnimatorAsset animatorLoadVoxelCollectionAsset( StringView filename, int16 id )
{
	AnimatorAsset result;
	result.type            = AnimatorAsset::type_collection;
	result.collection = new DynamicVoxelCollection( loadDynamicVoxelCollection( filename ) );
	if( !result.collection || !result.collection->memory ) {
		if( result.collection ) {
			delete result.collection;
			result.collection = nullptr;
		}
		result.type = AnimatorAsset::type_none;
	} else {
		result.id = id;
		result.setName( getFilenameWithoutExtension( filename ) );
	}
	return result;
}

void AnimatorAsset::setName( StringView str )
{
	nameLength = copyToString( str, name, countof( name ) );
	item.text = {name, nameLength};
}

AnimatorAsset::~AnimatorAsset() { destroy(); }
AnimatorAsset::AnimatorAsset( const AnimatorAsset& other )
{
	assign( other );
}
AnimatorAsset::AnimatorAsset( AnimatorAsset&& other ) {
	assign( std::move( other ) );
}
AnimatorAsset& AnimatorAsset::operator=( const AnimatorAsset& other )
{
	if( this != &other ) {
		destroy();
		assign( other );
	}
	return *this;
}
AnimatorAsset& AnimatorAsset::operator=( AnimatorAsset&& other )
{
	if( this != &other ) {
		destroy();
		assign( std::move( other ) );
	}
	return *this;
}
void AnimatorAsset::assign( const AnimatorAsset& other )
{
	assert( type == type_none );
	switch( other.type ) {
		case type_none: {
			break;
		}
		case type_collection: {
			type       = type_collection;
			collection = new DynamicVoxelCollection( *other.collection );
			break;
		}
		InvalidDefaultCase;
	}
	setName( {other.name, other.nameLength} );
	id = other.id;
}
void AnimatorAsset::assign( AnimatorAsset&& other )
{
	assert( type == type_none );
	switch( exchange( other.type, type_none ) ) {
		case type_none: {
			break;
		}
		case type_collection: {
			type       = type_collection;
			collection = exchange( other.collection, nullptr );
			break;
		}
		InvalidDefaultCase;
	}
	setName( {other.name, other.nameLength} );
	id = exchange( other.id, -1 );
}
void AnimatorAsset::destroy()
{
	switch( type ) {
		case type_none: {
			break;
		}
		case type_collection: {
			delete collection;
			collection = nullptr;
			break;
		}
		InvalidDefaultCase;
	}
	type = type_none;
}