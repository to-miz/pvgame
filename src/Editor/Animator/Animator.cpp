constexpr const float PaddleWidth            = 5;
constexpr const float PaddleHeight           = 10;
constexpr const float KeyframePrecision      = 1.0f / 5.0f;
constexpr const float PropertiesColumnWidth  = 200;
constexpr const float NamesWidth             = 100;
constexpr const float RegionHeight           = 200;
constexpr const float MenuHeight             = 18;
constexpr const int32 KeyablePropertiesCount = 5;

static const vec3 AnimatorInitialRotation = {0, -0.2f, 0};
const int16 AnimatorEventsId              = INT16_MAX - 1;
extern const GroupId AnimatorEventsGroup;

static const StringView EaseTypeNames[] = {
    "Lerp", "Step", "Smoothstep", "EaseOutBounce", "EaseOutElastic", "Curve",
};
static const StringView AnimatorAssetTypeNames[] = {
    "None", "VoxelCollection", "Collision", "Hitbox", "Hurtbox", "Deflect", "ParticleEmitter",
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
StringView to_string( AnimatorState* animator, AnimatorKeyframeData::Type type )
{
	type = (AnimatorKeyframeData::Type)( type - 1 );
	assert( type >= 0 && type < countof( animator->fieldNames ) );
	return animator->fieldNames[type];
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
	if( str.size() > 2 ) {
		switch( str[0] ) {
			case 'v':
			case 'V': {
				result = AnimatorAsset::type_collection;
				break;
			}
			case 'c':
			case 'C': {
				result = AnimatorAsset::type_collision;
				break;
			}
			case 'h':
			case 'H': {
				if( str[1] == 'i' || str[1] == 'I' ) {
					result = AnimatorAsset::type_hitbox;
				} else {
					result = AnimatorAsset::type_hurtbox;
				}
				break;
			}
			case 'd':
			case 'D': {
				result = AnimatorAsset::type_deflect;
				break;
			}
			case 'p':
			case 'P': {
				result = AnimatorAsset::type_emitter;
				break;
			}
		}
	}
	return result;
}

void writeValue( JsonWriter* writer, const AnimatorParticleEmitter& value )
{
	auto emitter = &value.emitter;
	writeStartObject( writer );
		writeProperty( writer, "count", emitter->count );
		writeProperty( writer, "maxAlive", emitter->maxAlive );
		writeProperty( writer, "velocity", emitter->velocity );
		writeProperty( writer, "textureId", valueof( emitter->textureId ) );
		writeProperty( writer, "flags", emitter->flags );
		writeProperty( writer, "interval", value.interval );
	writeEndObject( writer );
}
void deserialize( const JsonValue& value, AnimatorParticleEmitter& out )
{
	auto attr = value.getObject();
	deserialize( attr["count"], out.emitter.count );
	deserialize( attr["maxAlive"], out.emitter.maxAlive );
	deserialize( attr["velocity"], out.emitter.velocity );
	deserialize( attr["textureId"], out.emitter.textureId );
	deserialize( attr["flags"], out.emitter.flags );
	deserialize( attr["interval"], out.interval );
}

// AnimatorAsset
bool isHitboxType( AnimatorAsset::Type type )
{
	return type >= AnimatorAsset::type_collision && type <= AnimatorAsset::type_deflect;
}
// all three union fields point to the same location, but because of strict aliasing we can't
// read from a union field we didn't write to
rectf* getHitboxRect( AnimatorAsset* asset )
{
	assert( asset );
	assert( isHitboxType( asset->type ) );
	switch( asset->type ) {
		case AnimatorAsset::type_collision: {
			return &asset->collision;
			break;
		}
		case AnimatorAsset::type_hitbox: {
			return &asset->hitbox;
			break;
		}
		case AnimatorAsset::type_hurtbox: {
			return &asset->hurtbox;
			break;
		}
		case AnimatorAsset::type_deflect: {
			return &asset->deflect;
			break;
		}
	}
	return nullptr;
};
AnimatorAsset animatorHitboxAsset( AnimatorAsset::Type type, int16 id )
{
	AnimatorAsset result;
	result.id = id;
	result.setName( "Unnamed" );
	switch( type ) {
		case AnimatorAsset::type_collision: {
			result.type      = AnimatorAsset::type_collision;
			result.collision = {-5, -5, 5, 5};
			break;
		}
		case AnimatorAsset::type_hitbox: {
			result.type   = AnimatorAsset::type_hitbox;
			result.hitbox = {-5, -5, 5, 5};
			break;
		}
		case AnimatorAsset::type_hurtbox: {
			result.type    = AnimatorAsset::type_hurtbox;
			result.hurtbox = {-5, -5, 5, 5};
			break;
		}
		case AnimatorAsset::type_deflect: {
			result.type    = AnimatorAsset::type_deflect;
			result.deflect = {-5, -5, 5, 5};
			break;
		}
		InvalidDefaultCase;
	}
	return result;
}
AnimatorAsset animatorEmitterAsset( int16 id )
{
	AnimatorAsset result;
	result.id = id;
	result.setName( "Unnamed" );
	result.type    = AnimatorAsset::type_emitter;
	result.emitter = {};
	result.emitter.emitter = ParticleEmitters[0];
	result.emitter.interval = 30;
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
		entry->flags.selected = false;
	}
	animator->editor.clickedNode = nullptr;
}

int32 makeAnimatorGroup( int16 ownerId, AnimatorKeyframeData::Type type = {} )
{
	assert( type >= 0 && type < 256 );
	return ( int32 )( ( (uint32)ownerId << 8 ) | ( (uint32)type & 0xFF ) );
}
const GroupId AnimatorEventsGroup =
    makeAnimatorGroup( AnimatorEventsId, AnimatorKeyframeData::type_event );

AnimatorKeyframeData::Type getAnimatorGroupType( GroupId id )
{
	return ( AnimatorKeyframeData::Type )( (uint32)id & 0xFF );
}
bool isParentGroup( GroupId id )
{
	return id >= 0 && getAnimatorGroupType( id ) == AnimatorKeyframeData::type_none;
}
int16 getAnimatorKeyframeOwner( GroupId id ) { return ( int16 )( (uint32)id >> 8 ); }
bool isMyChild( GroupId parent, GroupId child )
{
	return getAnimatorKeyframeOwner( parent ) == getAnimatorKeyframeOwner( child );
}
GroupId getAnimatorParentGroup( GroupId id ) { return ( int32 )( (uint32)id & 0xFFFFFF00u ); }

void addAnimatorGroups( AnimatorState* animator, StringView parentName, int16 ownerId,
                        AnimatorAsset::Type type )
{
	Array< AnimatorGroup > groups = {};
	auto count                    = KeyablePropertiesCount + 1;
	if( type == AnimatorAsset::type_none ) {
		--count;
	}
	auto start = safe_truncate< int32 >( animator->groups.size() );
	auto end   = start + count;
	animator->groups.resize( end, {} );
	groups = makeRangeView( animator->groups, start, end );

	auto root      = &groups[0];
	root->name     = parentName;
	root->id       = makeAnimatorGroup( ownerId );
	root->expanded = false;

	for( int32 i = 1; i < AnimatorKeyframeData::type_frame; ++i ) {
		auto entry      = &groups[i];
		auto propertyType = (AnimatorKeyframeData::Type)i;
		entry->name       = to_string( animator, propertyType );
		entry->id         = makeAnimatorGroup( ownerId, propertyType );
		entry->expanded   = false;
	}
	if( type != AnimatorAsset::type_none ) {
		AnimatorKeyframeData::Type propertyType = {};
		switch( type ) {
			case AnimatorAsset::type_none: {
				break;
			}
			case AnimatorAsset::type_collection: {
				propertyType = AnimatorKeyframeData::type_frame;
				break;
			}
			case AnimatorAsset::type_collision:
			case AnimatorAsset::type_hitbox:
			case AnimatorAsset::type_hurtbox:
			case AnimatorAsset::type_deflect:
			case AnimatorAsset::type_emitter: {
				propertyType = AnimatorKeyframeData::type_active;
				break;
			}
			InvalidDefaultCase;
		}
		auto entry      = &groups[count - 1];
		entry->name     = to_string( animator, propertyType );
		entry->id       = makeAnimatorGroup( ownerId, propertyType );
		entry->expanded = false;
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
	animator->visibleGroups.push_back( {-1} );
	if( animator->flags.timelineRootExpanded ) {
		for( auto it = animator->groups.begin(), end = animator->groups.end(); it < end; ++it ) {
			animator->visibleGroups.push_back( {it->id} );
			if( !it->expanded && isParentGroup( it->id ) ) {
				// skip children
				auto cur = it->id;
				++it;
				while( it < end && isMyChild( cur, it->id ) ) {
					++it;
				}
				if( it == end ) {
					break;
				}
				if( !isMyChild( cur, it->id ) ) {
					--it;
				}
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
			result.type        = AnimatorKeyframeData::type_translation;
			result.translation = lerp( t, a.translation, b.translation );
			break;
		}
		case AnimatorKeyframeData::type_rotation: {
			result.type     = AnimatorKeyframeData::type_rotation;
			result.rotation = lerp( t, a.rotation, b.rotation );
			break;
		}
		case AnimatorKeyframeData::type_scale: {
			result.type  = AnimatorKeyframeData::type_scale;
			result.scale = lerp( t, a.scale, b.scale );
			break;
		}
		case AnimatorKeyframeData::type_flashColor: {
			result.type       = AnimatorKeyframeData::type_flashColor;
			result.flashColor = lerp( t, a.flashColor, b.flashColor );
			break;
		}
		case AnimatorKeyframeData::type_frame: {
			result.type  = AnimatorKeyframeData::type_frame;
			result.frame = auto_truncate( lerp( t, (float)a.frame, (float)b.frame ) );
			break;
		}
		case AnimatorKeyframeData::type_active: {
			result.type   = AnimatorKeyframeData::type_active;
			result.active = a.active;
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
		auto it = lower_bound( first, last, t,
		                       []( const auto& keyframe, float t ) { return keyframe->t < t; } );
		if( it == last ) {
			--it;
		}
		if( ( *it )->group != group && ( *it )->t <= t ) {
			while( ( *it )->t <= t ) {
				++it;
				if( it == last ) {
					return last;
				}
				if( ( *it )->group == group && ( *it )->t <= t ) {
					return it;
				}
			}
			return last;
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
	if( it != last && ( *it )->group == group ) {
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

AnimatorNode* findNodeById( AnimatorNodes& nodes, int16 id )
{
	if( auto it = find_first_where( nodes, entry->id == id ) ) {
		return it->get();
	}
	return nullptr;
}

AnimatorNode getCurrentFrameNode( AnimatorState* animator, AnimatorNode* node, float lastFrame = 0 )
{
	auto result = *node;

	auto baseGroup        = makeAnimatorGroup( node->id );
	GroupId translationId = baseGroup + AnimatorKeyframeData::type_translation;
	GroupId rotationId    = baseGroup + AnimatorKeyframeData::type_rotation;
	GroupId scaleId       = baseGroup + AnimatorKeyframeData::type_scale;
	GroupId flashColorId  = baseGroup + AnimatorKeyframeData::type_flashColor;
	GroupId frameId       = baseGroup + AnimatorKeyframeData::type_frame;
	GroupId activeId      = baseGroup + AnimatorKeyframeData::type_active;

	auto translationKeyframe =
	    getInterpolatedKeyframe( animator, animator->currentFrame, translationId );
	auto rotationKeyframe = getInterpolatedKeyframe( animator, animator->currentFrame, rotationId );
	auto scaleKeyframe    = getInterpolatedKeyframe( animator, animator->currentFrame, scaleId );
	auto flashColorKeyframe =
	    getInterpolatedKeyframe( animator, animator->currentFrame, flashColorId );

	result.translation =
	    get_variant_or_default( translationKeyframe, translation, node->translation );
	result.rotation   = get_variant_or_default( rotationKeyframe, rotation, node->rotation );
	result.scale      = get_variant_or_default( scaleKeyframe, scale, node->scale );
	result.flashColor = get_variant_or_default( flashColorKeyframe, flashColor, node->flashColor );
	switch( node->assetType ) {
		case AnimatorAsset::type_collection: {
			auto frameKeyframe =
			    getInterpolatedKeyframe( animator, animator->currentFrame, frameId );
			result.voxel.frame = get_variant_or_default( frameKeyframe, frame, node->voxel.frame );
			break;
		}
		case AnimatorAsset::type_collision:
		case AnimatorAsset::type_hitbox:
		case AnimatorAsset::type_hurtbox:
		case AnimatorAsset::type_deflect:
		case AnimatorAsset::type_emitter: {
			auto activeKeyframe =
			    getInterpolatedKeyframe( animator, animator->currentFrame, activeId );
			if( activeKeyframe.type == AnimatorKeyframeData::type_active ) {
				auto prevKey  = lowerBoundKeyframe( animator, animator->currentFrame, activeId );
				result.active = activeKeyframe.active;
				if( prevKey && prevKey->t >= lastFrame && prevKey->t < animator->currentFrame
				    && node->active && result.active ) {

					result.emitter.time = 0;
				}
				if( !result.active ) {
					result.emitter.time = 0;
				}
			} else {
				result.active = node->active;
			}
			break;
		}
	}

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

	result.voxel.animation = rel->voxel.animation;
	result.active          = rel->active;
	result.flashColor      = rel->flashColor;
	result.worldFlashColor = rel->worldFlashColor;
	result.emitter.time    = rel->emitter.time;
	result.parent          = rel->parent;
	result.assetId         = rel->assetId;
	result.asset           = rel->asset;
	result.assetType       = rel->assetType;
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

	result.voxel.animation = abs->voxel.animation;
	result.active          = abs->active;
	result.flashColor      = abs->flashColor;
	result.worldFlashColor = abs->worldFlashColor;
	result.emitter.time    = abs->emitter.time;
	result.parent          = abs->parent;
	result.assetId         = abs->assetId;
	result.asset           = abs->asset;
	result.assetType       = abs->assetType;
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
		case AnimatorKeyframeData::type_flashColor: {
			// TODO: what is the absolute value of color?
			break;
		}
		case AnimatorKeyframeData::type_frame: {
			break;
		}
		case AnimatorKeyframeData::type_active: {
			// TODO: what is the absolute value of active (a bool)?
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
		case AnimatorKeyframeData::type_flashColor: {
			// TODO: what is the relative value of color?
			break;
		}
		case AnimatorKeyframeData::type_frame: {
			break;
		}
		case AnimatorKeyframeData::type_active: {
			// TODO: what is the relative value of active (a bool)?
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
		if( auto parent = findNodeById( animator->nodes, node->parentId ) ) {
			node->parent = parent;

			int16 count = node->childrenCount + 1;
			auto current = node->parent;
			bounded_while( current, SkeletonMaxParents ) {
				current->childrenCount += count;
				current = current->parent;
			}
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
	addAnimatorGroups( animator, result->name, result->id, result->assetType );
	setUnsavedChanges( animator );
	return result;
}
AnimatorNode* addExistingNode( AnimatorState* animator, const AnimatorNode& node )
{
	animator->nodes.push_back( std::make_unique< AnimatorNode >( node ) );
	auto result           = animator->nodes.back().get();
	result->parent        = nullptr;
	result->childrenCount = 0;
	addAnimatorGroups( animator, result->name, result->id, result->assetType );
	return result;
}

AnimatorAsset* addNewAsset( AnimatorState* animator, AnimatorAsset::Type type,
                            StringView filename = {} )
{
	AnimatorAsset* result = nullptr;
	switch( type ) {
		case AnimatorAsset::type_collection: {
			auto asset = std::make_unique< AnimatorAsset >(
			    animatorLoadVoxelCollectionAsset( filename, animator->assetIds ) );
			if( asset && *asset ) {
				animator->assets.emplace_back( std::move( asset ) );
				++animator->assetIds;
				result = animator->assets.back().get();
			}
			break;
		}
		case AnimatorAsset::type_collision:
		case AnimatorAsset::type_hitbox:
		case AnimatorAsset::type_hurtbox:
		case AnimatorAsset::type_deflect: {
			auto asset = std::make_unique< AnimatorAsset >(
			    animatorHitboxAsset( type, animator->assetIds ) );
			if( asset && *asset ) {
				animator->assets.emplace_back( std::move( asset ) );
				++animator->assetIds;
				result = animator->assets.back().get();
			}
			break;
		}
		case AnimatorAsset::type_emitter: {
			auto asset =
			    std::make_unique< AnimatorAsset >( animatorEmitterAsset( animator->assetIds ) );
			if( asset && *asset ) {
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
	      []( const auto& a, const auto& b ) { return a->childrenCount > b->childrenCount; } );
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
	if( keyframe->data.type != AnimatorKeyframeData::type_event
	    && keyframe->data.easeType != type ) {

		if( keyframe->data.easeType == AnimatorKeyframeData::Curve ) {
			freeCurveData( animator, keyframe );
		} else if( type == AnimatorKeyframeData::Curve ) {
			auto index =
			    find_index_if( animator->curves, []( const auto& entry ) { return !entry.used; } );
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
void correctAsset( const AnimatorNode* base, AnimatorNode* node )
{
	if( base->assetId != node->assetId ) {
		node->assetId   = base->assetId;
		node->assetType = base->assetType;
	}
}
void refreshAsset( AnimatorState* animator, AnimatorNode* node )
{
	if( node->assetId >= 0 ) {
		if( auto it = find_first_where( animator->assets, entry->id == node->assetId ) ) {
			auto asset      = it->get();
			node->assetId   = asset->id;
			node->asset     = asset;
			node->assetType = asset->type;
		}
	}
	if( node->assetId < 0 || !node->asset || node->asset->id != node->assetId
	    || node->assetType != node->asset->type ) {

		node->assetId   = -1;
		node->assetType = AnimatorAsset::type_none;
		node->asset     = nullptr;
	}
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
		node->flags.marked = false;
	}

	clearAnimatorGroups( animator );

	FOR( node : animation->nodes ) {
		if( auto base = findNodeById( animator->baseNodes, node.id ) ) {
			assert( !base->flags.marked );
			if( base->flags.marked ) {
				continue;
			}
			base->flags.marked = true;
			// correct asset if base asset and node asset don't match
			correctAsset( base, &node );
			addExistingNode( animator, toAbsoluteNode( base, &node ) );
		}
	}
	FOR( base : animator->baseNodes ) {
		if( !base->flags.marked ) {
			addExistingNode( animator, *base );
		}
	}

	// reassign parents
	FOR( entry : animator->nodes ) {
		auto node = entry.get();
		bindParent( animator, node );
		refreshAsset( animator, node );
	}
	sortNodes( animator );
	clearSelectedNodes( animator );

	// events group
	animator->groups.push_back( {animator->fieldNames[6], AnimatorEventsGroup, 0} );

	populateVisibleGroups( animator );

	animator->keyframes.end();
	animator->keyframes.reserve( animation->keyframes.size() );
	FOR( keyframe : animation->keyframes ) {
		if( keyframe.data.type != AnimatorKeyframeData::type_event ) {
			auto base = findNodeById( animator->nodes, getAnimatorKeyframeOwner( keyframe.group ) );
			assert( base );
			animator->keyframes.push_back(
			    std::make_unique< AnimatorKeyframe >( toAbsoluteKeyframe( base, &keyframe ) ) );
		} else {
			animator->keyframes.push_back( std::make_unique< AnimatorKeyframe >( keyframe ) );
		}
	}
	sortKeyframes( animator );

	animator->curves = animation->curves;
}

void animatorSetCurrentFrame( AnimatorState* animator, float currentFrame, float lastFrame = 0 )
{
	animator->currentFrame = currentFrame;
	FOR( node : animator->nodes ) {
		*node = getCurrentFrameNode( animator, node.get(), lastFrame );
	}
}

void commitAnimation( AnimatorState* animator )
{
	assert( animator->currentAnimation );
	animatorSetCurrentFrame( animator, 0 );
	auto animation = animator->currentAnimation;
	animation->nodes.resize( animator->nodes.size() );

	zip_for( animation->nodes, animator->nodes ) {
		if( auto base = findNodeById( animator->baseNodes, ( *second )->id ) ) {
			*first = toRelativeNode( base, second->get() );
		}
	}

	animation->keyframes.resize( animator->keyframes.size() );
	zip_for( animation->keyframes, animator->keyframes ) {
		if( second->get()->data.type != AnimatorKeyframeData::type_event ) {
			auto base = find_first_where(
			    animator->nodes, entry->id == getAnimatorKeyframeOwner( second->get()->group ) );
			*first = toRelativeKeyframe( base->get(), second->get() );
		} else {
			*first = *second->get();
		}
	}
	animation->curves = animator->curves;
}
void closeAnimation( AnimatorState* animator, bool keep )
{
	assert( animator->currentAnimation );

	if( keep ) {
		commitAnimation( animator );
	}

	animator->keyframes.clear();
	clearAnimatorGroups( animator );
	FOR( entry : animator->baseNodes ) {
		auto base = entry.get();
		auto node = findNodeById( animator->nodes, base->id );
		assert( node );
		base->flags.visible      = node->flags.visible;
		base->flags.interactible = node->flags.interactible;
	}
	animator->nodes = std::move( animator->baseNodes );
	animator->baseNodes.clear();
	animator->currentAnimation = nullptr;
	animator->curves.clear();
	clearSelectedNodes( animator );
	clearSelectedKeyframes( animator );
}

void animatorMessageBox( AnimatorState* animator, StringView text, StringView title,
                         EditorCommon::MessageBoxAction* onYes,
                         EditorCommon::MessageBoxAction* onNo = nullptr )
{
	EditorCommon::showMessageBox( &animator->messageBox, text, title, onYes, onNo );
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
template < class T >
void animatorSaveNodes( JsonWriter* writer, Array< T > nodes, bool baseNodes,
                        const AnimatorNode* ( *get )(const T&))
{
	if( baseNodes ) {
		// write idents
		writePropertyName( writer, "idents" );
		writeStartArray( writer );
			FOR( entry : nodes ) {
				auto node = get( entry );
				writeStartObject( writer );
					writeProperty( writer, "id", node->id );
					writeProperty( writer, "name", StringView{node->name} );
				writeEndObject( writer );
			}
		writeEndArray( writer );
	}

	// write transforms
	writePropertyName( writer, "transforms" );
	writeStartArray( writer );
		FOR( entry : nodes ) {
			auto node = get( entry );
			writeStartObject( writer );
				writeProperty( writer, "id", node->id );
				writeProperty( writer, "translation", node->translation );
				writeProperty( writer, "rotation", node->rotation );
				writeProperty( writer, "scale", node->scale );
				if( baseNodes ) {
					writeProperty( writer, "length", node->length );
					writeProperty( writer, "parentId", NullableInt32{node->parentId} );
				}
			writeEndObject( writer );
		}
	writeEndArray( writer );

	// write voxels
	writePropertyName( writer, "voxels" );
	writeStartArray( writer );
		FOR( entry : nodes ) {
			auto node = get( entry );
			if( node->assetType == AnimatorAsset::type_collection ) {
				writeStartObject( writer );
					writeProperty( writer, "id", node->id );
					if( baseNodes ) {
						writeProperty( writer, "assetId", node->assetId );
					}
					writeProperty( writer, "animation", NullableInt32{node->voxel.animation} );
					writeProperty( writer, "frame", node->voxel.frame );
				writeEndObject( writer );
			}
		}
	writeEndArray( writer );

	// write emitters
	writePropertyName( writer, "emitters" );
	writeStartArray( writer );
		FOR( entry : nodes ) {
			auto node = get( entry );
			if( node->assetType == AnimatorAsset::type_emitter ) {
				writeStartObject( writer );
					writeProperty( writer, "id", node->id );
					if( baseNodes ) {
						writeProperty( writer, "assetId", node->assetId );
					}
					writeProperty( writer, "active", (bool)node->active );
				writeEndObject( writer );
			}
		}
	writeEndArray( writer );

	// write collision_bounds
	writePropertyName( writer, "collision_bounds" );
	writeStartArray( writer );
		FOR( entry : nodes ) {
			auto node = get( entry );
			if( node->assetType == AnimatorAsset::type_collision ) {
				writeStartObject( writer );
					writeProperty( writer, "id", node->id );
					if( baseNodes ) {
						writeProperty( writer, "assetId", node->assetId );
					}
					writeProperty( writer, "active", (bool)node->active );
				writeEndObject( writer );
			}
		}
	writeEndArray( writer );

	// write hitboxes
	writePropertyName( writer, "hitboxes" );
	writeStartArray( writer );
		FOR( entry : nodes ) {
			auto node = get( entry );
			if( node->assetType == AnimatorAsset::type_hitbox ) {
				writeStartObject( writer );
					writeProperty( writer, "id", node->id );
					if( baseNodes ) {
						writeProperty( writer, "assetId", node->assetId );
					}
					writeProperty( writer, "active", (bool)node->active );
				writeEndObject( writer );
			}
		}
	writeEndArray( writer );

	// write hurtboxes
	writePropertyName( writer, "hurtboxes" );
	writeStartArray( writer );
		FOR( entry : nodes ) {
			auto node = get( entry );
			if( node->assetType == AnimatorAsset::type_hurtbox ) {
				writeStartObject( writer );
					writeProperty( writer, "id", node->id );
					if( baseNodes ) {
						writeProperty( writer, "assetId", node->assetId );
					}
					writeProperty( writer, "active", (bool)node->active );
				writeEndObject( writer );
			}
		}
	writeEndArray( writer );

	// write deflect
	writePropertyName( writer, "deflect_bounds" );
	writeStartArray( writer );
		FOR( entry : nodes ) {
			auto node = get( entry );
			if( node->assetType == AnimatorAsset::type_deflect ) {
				writeStartObject( writer );
					writeProperty( writer, "id", node->id );
					if( baseNodes ) {
						writeProperty( writer, "assetId", node->assetId );
					}
					writeProperty( writer, "active", (bool)node->active );
				writeEndObject( writer );
			}
		}
	writeEndArray( writer );
}


void animatorSave( StackAllocator* allocator, AnimatorState* animator, StringView filename )
{
	auto current = animator->currentAnimation;
	if( current ) {
		closeAnimation( animator, true );
	}
	assert( !animator->currentAnimation );

	auto writeKeyframes = []( JsonWriter* writer, Array< AnimatorKeyframe > keyframes,
	                          Array< AnimatorCurveData > curves, StringView name, GroupId id ) {
		writePropertyName( writer, name );
		writeStartArray( writer );
		FOR( keyframe : keyframes ) {
			if( keyframe.group == id ) {
				writeStartObject( writer );
				writer->minimal = true;
				writeProperty( writer, "t", keyframe.t );
				writeProperty( writer, "easeType", to_string( keyframe.data.easeType ) );
				if( keyframe.data.easeType == AnimatorKeyframeData::Curve ) {
					auto curve = &curves[keyframe.data.curveIndex];
					writePropertyName( writer, "curve" );
					writeStartArray( writer );
					writeValue( writer, curve->curve0 );
					writeValue( writer, curve->curve1 );
					writeEndArray( writer );
				}
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
					case AnimatorKeyframeData::type_active: {
						writeProperty( writer, "value", keyframe.data.active );
						break;
					}
					case AnimatorKeyframeData::type_flashColor: {
						writeProperty( writer, "value", keyframe.data.flashColor );
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

	// make sure that every node has valid assets
	FOR( entry : animator->nodes ) {
		refreshAsset( animator, entry.get() );
	}

	TEMPORARY_MEMORY_BLOCK( allocator ) {
		auto jsonData  = beginVector( allocator, char );
		auto writerObj = makeJsonWriter( jsonData.data(), jsonData.capacity() );
		auto writer    = &writerObj;

		writeStartObject( writer );
		// write assets
		writePropertyName( writer, "assets" );
		writeStartObject( writer );
			// write voxel assets
			writePropertyName( writer, "voxels" );
			writeStartArray( writer );
			FOR( entry : animator->assets ) {
				auto asset = entry.get();
				if( asset->type == AnimatorAsset::type_collection ) {
					writeStartObject( writer );
					    writeProperty( writer, "id", asset->id );
					    writeProperty( writer, "name", asset->getName() );
					    writeProperty( writer, "filename", asset->collection->voxels.filename );
				    writeEndObject( writer );
				}
			}
			writeEndArray( writer );

			// write emitter assets
			writePropertyName( writer, "emitters" );
			writeStartArray( writer );
			FOR( entry : animator->assets ) {
				auto asset = entry.get();
				if( asset->type == AnimatorAsset::type_emitter ) {
					writeStartObject( writer );
						writeProperty( writer, "id", asset->id );
						writeProperty( writer, "name", asset->getName() );
				        writeProperty( writer, "emitter", asset->emitter );
			        writeEndObject( writer );
				}
			}
			writeEndArray( writer );

			// write collision assets
			writePropertyName( writer, "collision_bounds" );
			writeStartArray( writer );
			FOR( entry : animator->assets ) {
				auto asset = entry.get();
				if( asset->type == AnimatorAsset::type_collision ) {
					writeStartObject( writer );
						writeProperty( writer, "id", asset->id );
						writeProperty( writer, "name", asset->getName() );
				        writeProperty( writer, "bounds", asset->collision );
			        writeEndObject( writer );
				}
			}
			writeEndArray( writer );

			// write hitbox assets
			writePropertyName( writer, "hitboxes" );
			writeStartArray( writer );
			FOR( entry : animator->assets ) {
				auto asset = entry.get();
				if( asset->type == AnimatorAsset::type_hitbox ) {
					writeStartObject( writer );
						writeProperty( writer, "id", asset->id );
						writeProperty( writer, "name", asset->getName() );
				        writeProperty( writer, "bounds", asset->hitbox );
			        writeEndObject( writer );
				}
			}
			writeEndArray( writer );

			// write hurtbox assets
			writePropertyName( writer, "hurtboxes" );
			writeStartArray( writer );
			FOR( entry : animator->assets ) {
				auto asset = entry.get();
				if( asset->type == AnimatorAsset::type_hurtbox ) {
					writeStartObject( writer );
						writeProperty( writer, "id", asset->id );
						writeProperty( writer, "name", asset->getName() );
				        writeProperty( writer, "bounds", asset->hurtbox );
			        writeEndObject( writer );
				}
			}
			writeEndArray( writer );

			// write deflect assets
			writePropertyName( writer, "deflect_bounds" );
			writeStartArray( writer );
			FOR( entry : animator->assets ) {
				auto asset = entry.get();
				if( asset->type == AnimatorAsset::type_deflect ) {
					writeStartObject( writer );
						writeProperty( writer, "id", asset->id );
						writeProperty( writer, "name", asset->getName() );
				        writeProperty( writer, "bounds", asset->deflect );
			        writeEndObject( writer );
				}
			}
			writeEndArray( writer );
		writeEndObject( writer );

		// write nodes
		writePropertyName( writer, "nodes" );
		writeStartObject( writer );
		animatorSaveNodes< UniqueAnimatorNode >(
		    writer, makeArrayView( animator->nodes ), true,
		    []( const UniqueAnimatorNode& entry ) -> const AnimatorNode* { return entry.get(); } );
		writeEndObject( writer );

		// write animations
		writePropertyName( writer, "animations" );
		writeStartArray( writer );
			auto getter = []( const AnimatorNode& entry ) -> const AnimatorNode* { return &entry; };
			FOR( animation : animator->animations ) {
				// preprocess nodes and make sure that they are all valid
				// because we might have changed assets and deleted/added base nodes, we need to
				// make sure that everything is synced
				FOR( base : animator->nodes ) {
					base->flags.marked = false;
			    }
			    FOR( node : animation.nodes ) {
				    auto base = findNodeById( animator->nodes, node.id );
				    if( !base ) {
					    node.flags.marked = false;
					    continue;
				    }
				    base->flags.marked = true;
				    node.flags.marked = true;
				    correctAsset( base, &node );
			    }
			    erase_if( animation.nodes,
			              []( const auto& entry ) { return !entry.flags.marked; } );
			    FOR( entry : animator->nodes ) {
				    auto base = entry.get();
				    if( !base->flags.marked ) {
					    auto added = toRelativeNode( base, base );
					    animation.nodes.push_back( added );
				    }
			    }

			    writeStartObject( writer );
				    writeProperty( writer, "name", StringView{animation.name} );
			        animatorSaveNodes< AnimatorNode >( writer, makeArrayView( animation.nodes ),
			                                           false, getter );

			        writePropertyName( writer, "keyframes" );
			        writeStartArray( writer );
			        auto keyframes = makeArrayView( animation.keyframes );
			        auto curves    = makeArrayView( animation.curves );
			        FOR( node : animation.nodes ) {

				        writeStartObject( writer );
				        writeProperty( writer, "id", node.id );
				        auto group = makeAnimatorGroup( node.id );
				        writeKeyframes( writer, keyframes, curves, "translation",
				                        group + AnimatorKeyframeData::type_translation );

				        writeKeyframes( writer, keyframes, curves, "rotation",
				                        group + AnimatorKeyframeData::type_rotation );

				        writeKeyframes( writer, keyframes, curves, "scale",
				                        group + AnimatorKeyframeData::type_scale );

				        writeKeyframes( writer, keyframes, curves, "flashColor",
				                        group + AnimatorKeyframeData::type_flashColor );

				        switch( node.assetType ) {
				        	case AnimatorAsset::type_none: {
				        		break;
				        	}
					        case AnimatorAsset::type_collection: {
						        writeKeyframes( writer, keyframes, curves, "frame",
						                        group + AnimatorKeyframeData::type_frame );
						        break;
					        }
					        case AnimatorAsset::type_collision:
					        case AnimatorAsset::type_hitbox:
					        case AnimatorAsset::type_hurtbox:
					        case AnimatorAsset::type_deflect:
					        case AnimatorAsset::type_emitter: {
						        writeKeyframes( writer, keyframes, curves, "active",
						                        group + AnimatorKeyframeData::type_active );
						        break;
					        }
					        InvalidDefaultCase;
				        }
				        writeEndObject( writer );
			        }
			        writeEndArray( writer );

			        writePropertyName( writer, "events" );
			        writeStartArray( writer );
			        	FOR( keyframe : keyframes ) {
			        		if( keyframe.group == AnimatorEventsGroup ) {
			        			writeStartObject( writer );
								writer->minimal = true;
			        				writeProperty( writer, "t", keyframe.t );
					                writeProperty( writer, "value",
					                               to_string( keyframe.data.event ) );
				                writeEndObject( writer );
								writer->minimal = false;
			        		}
			        	}
			        writeEndArray( writer );

		        writeEndObject( writer );
			}
		writeEndArray( writer );

		writePropertyName( writer, "editor_meta" );
		writeStartObject( writer );
			writePropertyName( writer, "nodes" );
			writeStartArray( writer );
				FOR( entry : animator->nodes ) {
					auto node = entry.get();
					writeStartObject( writer );
						writeProperty( writer, "id", node->id );

						// flags to bitmask
						uint32 flags = 0;
						if( node->flags.visible ) flags |= BITFIELD( 2 );
						if( node->flags.interactible ) flags |= BITFIELD( 3 );
						writeProperty( writer, "flags", flags );
					writeEndObject( writer );
				}
			writeEndArray( writer );
		writeEndObject( writer );
		writeEndObject( writer );

		GlobalPlatformServices->writeBufferToFile( filename, writer->data(), writer->size() );
	}

	if( current ) {
		openAnimation( animator, current );
	}
}

struct translation_tag {};
struct rotation_tag {};
struct scale_tag {};
struct flashColor_tag {};
struct frame_tag {};
struct active_tag {};

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
Color& getValue( AnimatorKeyframeData& data, flashColor_tag )
{
	data.type = AnimatorKeyframeData::type_flashColor;
	return data.flashColor;
}
int8& getValue( AnimatorKeyframeData& data, frame_tag )
{
	data.type = AnimatorKeyframeData::type_frame;
	return data.frame;
}
bool8& getValue( AnimatorKeyframeData& data, active_tag )
{
	data.type = AnimatorKeyframeData::type_active;
	return data.active;
}

template < class Tag >
void loadKeyframes( JsonObjectArray array, GroupId group, Tag tag,
                    std::vector< AnimatorKeyframe >* outKeyframes,
                    std::vector< AnimatorCurveData >* outCurves )
{
	FOR( keyframe : array ) {
		outKeyframes->emplace_back( AnimatorKeyframe{} );
		auto added   = &outKeyframes->back();
		added->group = group;
		deserialize( keyframe["t"], added->t );
		auto easeType = keyframe["easeType"];
		if( jsonIsString( easeType ) ) {
			from_string( easeType.getString(), &added->data.easeType );
		} else {
			added->data.easeType = (AnimatorKeyframeData::EaseType)clamp(
			    easeType.getInt(), AnimatorKeyframeData::Lerp, AnimatorKeyframeData::Curve );
		}
		if( added->data.easeType == AnimatorKeyframeData::Curve ) {
			added->data.curveIndex = safe_truncate< int16 >( outCurves->size() );
			outCurves->emplace_back();
			auto curve     = &outCurves->back();
			auto curveData = keyframe["curve"].getArray();
			deserialize( curveData[0], curve->curve0 );
			deserialize( curveData[1], curve->curve1 );
			curve->used        = true;
			curve->differencer = makeEasingCurve( curve->curve0, curve->curve1 );
		}
		deserialize( keyframe["value"], getValue( added->data, tag ) );
	}
}

template <class T >
void animatorDeserializeNodes( T forEach )
{
	// transforms
	forEach( "transforms", []( JsonObject& attr, AnimatorNode* node, bool baseNode ) {
		deserialize( attr["translation"], node->translation );
		deserialize( attr["rotation"], node->rotation );
		deserialize( attr["scale"], node->scale );
		deserialize( attr["length"], node->length );
		if( baseNode ) {
			deserialize( attr["parentId"], node->parentId, -1 );
		}
	} );

	// voxels
	forEach( "voxels", []( JsonObject& attr, AnimatorNode* node, bool baseNode ) {
		if( baseNode ) {
			deserialize( attr["assetId"], node->assetId, -1 );
		}
		deserialize( attr["animation"], node->voxel.animation , -1);
		deserialize( attr["frame"], node->voxel.frame );
	} );

	// emitters
	forEach( "emitters", []( JsonObject& attr, AnimatorNode* node, bool baseNode ) {
		if( baseNode ) {
			deserialize( attr["assetId"], node->assetId, -1 );
		}
		deserialize( attr["active"], node->active );
	} );

	// collision_bounds
	forEach( "collision_bounds", []( JsonObject& attr, AnimatorNode* node, bool baseNode ) {
		if( baseNode ) {
			deserialize( attr["assetId"], node->assetId, -1 );
		}
		deserialize( attr["active"], node->active );
	} );

	// hitboxes
	forEach( "hitboxes", []( JsonObject& attr, AnimatorNode* node, bool baseNode ) {
		if( baseNode ) {
			deserialize( attr["assetId"], node->assetId, -1 );
		}
		deserialize( attr["active"], node->active );
	} );

	// hurtboxes
	forEach( "hurtboxes", []( JsonObject& attr, AnimatorNode* node, bool baseNode ) {
		if( baseNode ) {
			deserialize( attr["assetId"], node->assetId, -1 );
		}
		deserialize( attr["active"], node->active );
	} );

	// deflect_bounds
	forEach( "deflect_bounds", []( JsonObject& attr, AnimatorNode* node, bool baseNode ) {
		if( baseNode ) {
			deserialize( attr["assetId"], node->assetId, -1 );
		}
		deserialize( attr["active"], node->active );
	} );
}

bool animatorOpen( StackAllocator* allocator, AnimatorState* animator, StringView filename )
{
	TEMPORARY_MEMORY_BLOCK( allocator ) {
		auto file = readFile( allocator, filename );
		if( !file.size() ) {
			LOG( ERROR, "Unable to open file {}", filename );
			return false;
		}

		auto doc = makeJsonDocument( allocator, file );
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

		if( auto assets = root["assets"].getObject() ) {
			FOR( attr : assets["voxels"].getObjectArray() ) {
				auto voxelFile = attr["filename"].getString();
				if( auto added =
				        addNewAsset( animator, AnimatorAsset::type_collection, voxelFile ) ) {
					deserialize( attr["id"], added->id, -1 );
					added->setName( attr["name"].getString() );
				}
			}
			FOR( attr : assets["emitters"].getObjectArray() ) {
				if( auto added = addNewAsset( animator, AnimatorAsset::type_emitter ) ) {
					deserialize( attr["id"], added->id, -1 );
					added->setName( attr["name"].getString() );
					deserialize( attr["emitter"], added->emitter );
				}
			}
			FOR( attr : assets["collision_bounds"].getObjectArray() ) {
				if( auto added = addNewAsset( animator, AnimatorAsset::type_collision ) ) {
					deserialize( attr["id"], added->id, -1 );
					added->setName( attr["name"].getString() );
					deserialize( attr["bounds"], added->collision );
				}
			}
			FOR( attr : assets["hitboxes"].getObjectArray() ) {
				if( auto added = addNewAsset( animator, AnimatorAsset::type_hitbox ) ) {
					deserialize( attr["id"], added->id, -1 );
					added->setName( attr["name"].getString() );
					deserialize( attr["bounds"], added->hitbox );
				}
			}
			FOR( attr : assets["hurtboxes"].getObjectArray() ) {
				if( auto added = addNewAsset( animator, AnimatorAsset::type_hurtbox ) ) {
					deserialize( attr["id"], added->id, -1 );
					added->setName( attr["name"].getString() );
					deserialize( attr["bounds"], added->hurtbox );
				}
			}
			FOR( attr : assets["deflect_bounds"].getObjectArray() ) {
				if( auto added = addNewAsset( animator, AnimatorAsset::type_deflect ) ) {
					deserialize( attr["id"], added->id, -1 );
					added->setName( attr["name"].getString() );
					deserialize( attr["bounds"], added->deflect );
				}
			}
		}

		if( auto nodes = root["nodes"].getObject() ) {
			// idents
			FOR( attr : nodes["idents"].getObjectArray() ) {
				auto addedNode = std::make_unique< AnimatorNode >();
				auto node      = addedNode.get();
				deserialize( attr["id"], node->id, -1 );
				node->name = attr["name"].getString();
				animator->nodes.push_back( std::move( addedNode ) );
			}

			// nodes
			bool success = true;
			auto for_each_node = [&]( StringView arrayName,
			                          void ( *op )( JsonObject&, AnimatorNode*, bool ) ) {
				FOR( attr : nodes[arrayName].getObjectArray() ) {
					int16 id = (int16)attr["id"].getInt( -1 );
					if( auto node = findNodeById( animator->nodes, id ) ) {
						op( attr, node, true );
					} else {
						LOG( ERROR, "{}: No node found with id {}", filename, id );
						success = false;
					}
				}
			};
			animatorDeserializeNodes( for_each_node );

			if( !success ) {
				return false;
			}
		}
		FOR( node : animator->nodes ) {
			auto parentId = node->parentId;
			if( !bindParent( animator, node.get() ) ) {
				LOG( ERROR, "{}: parent {} not found", parentId );
				return false;
			}
		}
		sortNodes( animator );

		// animations
		FOR( animation : root["animations"].getObjectArray() ) {
			animator->animations.emplace_back();
			auto added = &animator->animations.back();
			added->name = animation["name"].getString();

			// nodes
			added->nodes.resize( animator->nodes.size() );
			zip_for( added->nodes, animator->nodes ) {
				first->id = second->get()->id;
			}

			bool success = true;
			auto for_each_node = [&]( StringView arrayName,
			                          void ( *op )( JsonObject&, AnimatorNode*, bool ) ) {
				FOR( attr : animation[arrayName].getObjectArray() ) {
					int16 id = (int16)attr["id"].getInt( -1 );
					if( auto node = find_first_where( added->nodes, entry.id == id ) ) {
						op( attr, node, false );
					} else {
						LOG( ERROR, "{}: No node found with id {}", filename, id );
						success = false;
					}
				}
			};
			animatorDeserializeNodes( for_each_node );

			if( !success ) {
				return false;
			}

			// keyframes
			FOR( keyframes : animation["keyframes"].getObjectArray() ) {
				int16 id = (int16)keyframes["id"].getInt( -1 );
				if( auto node = find_first_where( added->nodes, entry.id == id ) ) {
					auto group = makeAnimatorGroup( node->id );
					loadKeyframes( keyframes["translation"].getObjectArray(),
					               group + AnimatorKeyframeData::type_translation,
					               translation_tag{}, &added->keyframes, &added->curves );
					loadKeyframes( keyframes["rotation"].getObjectArray(),
					               group + AnimatorKeyframeData::type_rotation, rotation_tag{},
					               &added->keyframes, &added->curves );
					loadKeyframes( keyframes["scale"].getObjectArray(),
					               group + AnimatorKeyframeData::type_scale, scale_tag{},
					               &added->keyframes, &added->curves );
					loadKeyframes( keyframes["flashColor"].getObjectArray(),
					               group + AnimatorKeyframeData::type_flashColor, flashColor_tag{},
					               &added->keyframes, &added->curves );
					loadKeyframes( keyframes["frame"].getObjectArray(),
					               group + AnimatorKeyframeData::type_frame, frame_tag{},
					               &added->keyframes, &added->curves );
					loadKeyframes( keyframes["active"].getObjectArray(),
					               group + AnimatorKeyframeData::type_active, active_tag{},
					               &added->keyframes, &added->curves );
				} else {
					LOG( ERROR, "{}: No node found with id {}", filename, id );
					return false;
				}
			}

			// events
			FOR( event : animation["events"].getObjectArray() ) {
				added->keyframes.emplace_back( AnimatorKeyframe{} );
				auto keyframe   = &added->keyframes.back();
				keyframe->group = AnimatorEventsGroup;
				deserialize( event["t"], keyframe->t );
				set_variant( keyframe->data, event ) =
				    auto_from_string( event["value"].getString() );
			}
		}

		// editor meta data
		if( auto meta = root["editor_meta"].getObject() ) {
			FOR( attr : meta["nodes"].getObjectArray() ) {
				int16 id = (int16)attr["id"].getInt( -1 );
				if( auto node = findNodeById( animator->nodes, id ) ) {
					auto flags               = attr["flags"].getUInt();
					node->flags.visible      = ( flags & BITFIELD( 2 ) ) != 0;
					node->flags.interactible = ( flags & BITFIELD( 3 ) ) != 0;
				}
			}
		}
	}

	TEMPORARY_MEMORY_BLOCK( allocator ) {
		// check whether ids are unique
		int16 maxAssetId = 0;
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
			maxAssetId = max( maxAssetId, asset->id );
		}
		animator->assetIds = maxAssetId + 1;

		int16 maxNodeId = 0;
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
			maxNodeId = max( maxNodeId, node->id );
		}
		animator->nodeIds = maxNodeId + 1;

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
				if( auto baseIndex = find_index( baseIds, node->id ) ) {
					auto base      = animator->nodes[baseIndex.get()].get();
					entry.parentId = base->parentId;
					entry.assetId  = base->assetId;
				} else {
					LOG( ERROR, "{}: id {} not found as base", node->id );
					return false;
				}
				if( entry.assetId >= 0 ) {
					auto index = find_index( assetIds, entry.assetId );
					if( !index ) {
						LOG( ERROR, "{}: asset {} not found", filename, entry.assetId );
						return false;
					}
					entry.asset     = animator->assets[index.get()].get();
					entry.assetType = entry.asset->type;
				}
			}
            // we don't need to check keyframe ids, since they are derived from node ids
			// if node ids are valid, so are keyframe ids

#if GAME_DEBUG
			// check whether all keyframes point to valid nodes
			FOR( keyframe : animation.keyframes ) {
				if( keyframe.data.type != AnimatorKeyframeData::type_event ) {
					auto id = getAnimatorKeyframeOwner( keyframe.group );
					if( !find_first_where( ids, entry == id ) ) {
						LOG( ERROR, "{}: Invalid keyframe, no parent", filename );
						return false;
					}
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
bool animatorMenuOpen( AppData* app )
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
		if( !animatorMenuOpen( app ) ) {
			animator->flags.unsavedChanges = true;
		}
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
	return animator->filename.size() != 0;
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
		if( visible.group < 0 || isParentGroup( visible.group ) ) {
			auto expandRect = cell;
			expandRect.top += padding;
			expandRect.bottom = expandRect.top + expandHeight;
			expandRect.right  = expandRect.left + expandWidth;
			setTexture( renderer, 0, null );
			renderer->color = Color::White;
			int32 typeIndex = ExpandBox;
			if( ( !group && animator->flags.timelineRootExpanded )
			    || ( group && group->expanded ) ) {

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
		if( !isKeyDown( inputs, KC_Alt ) ) {
			pos = round( pos );
		} else {
			pos = fmod( pos, KeyframePrecision );
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
									groups.push_back( visible.group );
									if( isParentGroup( visible.group ) ) {
										append_unique( groups, visible.group + 1 );
										append_unique( groups, visible.group + 2 );
										append_unique( groups, visible.group + 3 );
										append_unique( groups, visible.group + 4 );
										append_unique( groups, visible.group + 5 );
									}
								}
							} else {
								for( auto i = firstGroup; i < lastGroup; ++i ) {
									auto visible = &animator->visibleGroups[i];
									append_unique( groups, visible->group );
									if( isParentGroup( visible->group ) ) {
										append_unique( groups, visible->group + 1 );
										append_unique( groups, visible->group + 2 );
										append_unique( groups, visible->group + 3 );
										append_unique( groups, visible->group + 4 );
										append_unique( groups, visible->group + 5 );
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
	auto isParent   = ( parent && isParentGroup( parent->id ) );
	MESH_STREAM_BLOCK( stream, renderer ) {
		Color colors[] = {Color::White, Color::Red};
		FOR( entry : animator->keyframes ) {
			auto frame = entry.get();
			if( group >= 0 && !isParent && frame->group != group ) {
				continue;
			}
			if( isParent && !isMyChild( parent->id, frame->group ) ) {
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
			if( ( isRoot || isMyChild( parent->id, frame->group ) )
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

	auto doEditSlider = [&]( StringView text, float* val, float min, float max ) {
		imguiSameLine( 2 );
		if( imguiEditbox( text, val ) ) {
			setUnsavedChanges( animator );
		}
		if( imguiSlider( val, min, max ) ) {
			setUnsavedChanges( animator );
		}
	};
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
		auto listboxHandle = imguiMakeHandle( &editor->assetsScrollPos, ImGuiControlType::Listbox );
		imguiListboxIntrusive( listboxHandle, &editor->assetsScrollPos, {assets, getter}, 200,
		                       200 );
		if( imguiHasFocus( listboxHandle ) && isKeyPressed( inputs, KC_RButton ) ) {
			imguiShowContextMenu( animator->assetContextMenu, inputs->mouse.position );
		}

		auto firstIt = find_first_where( animator->assets, entry->item.selected );
		auto first   = ( firstIt ) ? ( firstIt->get() ) : ( nullptr );
		if( imguiBeginDropGroup( "Add/Remove", &editor->expandedFlags,
		                         AnimatorEditor::AssetsAdd ) ) {
			imguiSameLine( 2 );
			if( imguiButton( "Voxel Collection", imguiRatio() ) ) {
				auto filename = getOpenFilename( JsonFilter, nullptr, false );
				if( filename.size() ) {
					addNewAsset( animator, AnimatorAsset::type_collection, filename );
				}
			}
			if( imguiButton( "Texture Map", imguiRatio() ) ) {
			}
			imguiSameLine( 2 );
			if( imguiButton( "Collision Bounds", imguiRatio() ) ) {
				addNewAsset( animator, AnimatorAsset::type_collision );
			}
			if( imguiButton( "Hitbox", imguiRatio() ) ) {
				addNewAsset( animator, AnimatorAsset::type_hitbox );
			}
			imguiSameLine( 2 );
			if( imguiButton( "Hurtbox", imguiRatio() ) ) {
				addNewAsset( animator, AnimatorAsset::type_hurtbox );
			}
			if( imguiButton( "Deflect Bounds", imguiRatio() ) ) {
				addNewAsset( animator, AnimatorAsset::type_deflect );
			}
			imguiSameLine( 1 );
			if( imguiButton( "Particle Emitter", imguiRatio() ) ) {
				addNewAsset( animator, AnimatorAsset::type_emitter );
			}
			if( imguiButton( "Remove Selected", imguiRatio() ) ) {
				if( first ) {
					animatorMessageBox(
					    animator, "Selected assets will be deleted.", "Delete Assets",
					    []( AppData* app ) { deleteSelectedAssets( &app->animatorState ); } );
				}
			}
			imguiEndDropGroup();
		}
		if( imguiBeginDropGroup( "Asset Properties", &editor->expandedFlags,
		                         AnimatorEditor::AssetsProperties ) ) {
			if( first ) {
				if( imguiEditbox( "Name", first->name, &first->nameLength,
				                  countof( first->name ) ) ) {
					first->setName( {first->name, first->nameLength} );
				}
				switch( first->type ) {
					case AnimatorAsset::type_collection: {
						break;
					}
					case AnimatorAsset::type_collision:
					case AnimatorAsset::type_hitbox:
					case AnimatorAsset::type_hurtbox: {
						auto hitbox = getHitboxRect( first );
						imguiEditbox( "Bounds", hitbox );
						break;
					}
					case AnimatorAsset::type_emitter: {
						auto emitter = &first->emitter.emitter;
						imguiEditbox( "Count", &emitter->count );
						imguiEditbox( "Max Alive", &emitter->maxAlive );
						imguiEditbox( "Velocity", &emitter->velocity );
						imguiEditbox( "Interval", &first->emitter.interval );
						break;
					}
				}
			}
			imguiEndDropGroup();
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
		imguiSameLine( 2 );
		auto infoRect = imguiAddItem( 24, 200 );
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
			clearSelectedNodes( animator );
			if( index < 0 || index > ::size( animator->nodes ) ) {
				editor->clickedNode = nullptr;
			} else {
				editor->clickedNode = animator->nodes[index].get();
				editor->clickedNode->flags.selected = true;
			}
		}
		// info rect
		{
			auto clip = ClippingRect( renderer, infoRect );
			setTexture( renderer, 0, null );
			MESH_STREAM_BLOCK( stream, renderer ) {
				auto style    = &ImGui->style;
				stream->color = style->listboxBg;
				pushQuad( stream, infoRect );

				stream->color = Color::White;
				auto buttonRect =
				    RectCentered( center( RectWH( infoRect.leftTop + vec2{style->innerPadding}, 8,
				                                  style->listboxItemHeight ) ),
				                  8.0f, 8.0f );
				buttonRect = translate( buttonRect, 0, -editor->nodesListboxScroll );
				for( auto i = 0, count = ::size( animator->nodes ); i <= count; ++i ) {
					auto node = ( i > 0 ) ? animator->nodes[i - 1].get() : nullptr;

					auto visibleButton      = buttonRect;
					auto interactibleButton = translate( visibleButton, 11, 0 );
					if( ( i == 0 && editor->viewFlags.visibleNodes )
					    || ( i > 0 && node->flags.visible ) ) {

						pushQuad( stream, visibleButton );
					}
					if( ( i == 0 && editor->viewFlags.interactibleNodes )
					    || ( i > 0 && node->flags.interactible ) ) {

						pushQuad( stream, interactibleButton );
					}
					buttonRect = translate( buttonRect, 0, style->listboxItemHeight );

					if( isPointInside( visibleButton, inputs->mouse.position )
					    && isKeyPressed( inputs, KC_LButton ) ) {

						if( i == 0 ) {
							// all button
							editor->viewFlags.visibleNodes = !editor->viewFlags.visibleNodes;
						} else {
							// single node
							auto index = i - 1;
							auto node = animator->nodes[index].get();
							node->flags.visible = !node->flags.visible;
						}
					}
					if( isPointInside( interactibleButton, inputs->mouse.position )
					    && isKeyPressed( inputs, KC_LButton ) ) {

						if( i == 0 ) {
							// all button
							editor->viewFlags.interactibleNodes =
							    !editor->viewFlags.interactibleNodes;
						} else {
							// single node
							auto index = i - 1;
							auto node = animator->nodes[index].get();
							node->flags.interactible = !node->flags.interactible;
						}
					}
				}
			}}
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
					base = findNodeById( animator->baseNodes, node.id );
					node = toRelativeNode( base, selected );
				}
			}
			if( imguiEditbox( "Name", node.name.data(), &node.name.sz, node.name.capacity() ) ) {
				if( auto group = find_first_where( animator->groups,
				                                   entry.id == makeAnimatorGroup( node.id ) ) ) {
					group->name = node.name;
				}
			}
			if( imguiEditbox( "Flash Color", &node.flashColor ) ) {
				setUnsavedChanges( animator );
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
			doEditSlider( "Scale X", &node.scale.x, -2, 2 );
			doEditSlider( "Scale Y", &node.scale.y, -2, 2 );
			doEditSlider( "Scale Z", &node.scale.z, -2, 2 );
			{
				imguiSameLine( 2 );
				imguiText( "Parent" );
				auto comboHandle = imguiMakeHandle( &selected->parentId );
				int32 index      = -1;
				if( node.parent ) {
					auto pos = find_if( animator->nodes, [node]( const auto& entry ) {
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
			// can change asset only in bone mode
			if( !animator->currentAnimation ) {
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
				if( imguiCombo( comboHandle, (const void*)animator->assets.data(),
				                (int32)sizeof( UniqueAnimatorAsset* ), ::size( animator->assets ),
				                &index,
				                []( const void* entry ) -> StringView {
					                assert_alignment( entry, alignof( UniqueAnimatorAsset* ) );
					                auto asset = ( (UniqueAnimatorAsset*)entry )->get();
					                return asset->item.text;
					            },
				                true ) ) {
					// TODO: make sure that nodes in animations also now use this asset
					setUnsavedChanges( animator );
					if( index >= 0 ) {
						auto asset           = animator->assets[index].get();
						node.asset           = asset;
						node.assetId         = asset->id;
						node.assetType       = asset->type;
						node.voxel.animation = -1;
						node.voxel.frame     = 0;
					} else {
						node.asset     = nullptr;
						node.assetId   = -1;
						node.assetType = {};
					}
				}
			}
			switch( node.assetType ) {
				case AnimatorAsset::type_collection: {
					auto collection = get_variant( *node.asset, collection );
					imguiSameLine( 2 );
					imguiText( "Voxel" );
					auto comboHandle = imguiMakeHandle( &node.voxel );
					int32 index      = node.voxel.animation;
					if( imguiCombo( comboHandle, &index, collection->names, true ) ) {
						node.voxel.animation = auto_truncate( index );
						setUnsavedChanges( animator );
					}

					if( node.voxel.animation >= 0 ) {
						imguiSameLine( 2 );
					}
					if( imguiEditbox( "Frame", &node.voxel.frame ) ) {
						setUnsavedChanges( animator );
					}
					if( node.voxel.animation >= 0 ) {
						auto frames = collection->voxels.animations[node.voxel.animation].range;
						float val = (float)( node.voxel.frame );
						if( imguiSlider( imguiMakeHandle( &node.voxel.frame ), &val, 0,
						                 (float)width( frames ) ) ) {
							node.voxel.frame = min( safe_truncate< int8 >( val ),
							                        safe_truncate< int8 >( width( frames ) - 1 ) );
							setUnsavedChanges( animator );
						}
					}
					break;
				}
				case AnimatorAsset::type_collision:
				case AnimatorAsset::type_hitbox:
				case AnimatorAsset::type_hurtbox:
				case AnimatorAsset::type_deflect:
				case AnimatorAsset::type_emitter: {
					node.active = imguiCheckbox( "Active", (bool)node.active );
					break;
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
			if( imguiPushButton( handle, name, editor->mouseMode == mode, imguiRatio() ) ) {
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
			if( imguiPushButton( handle, name, editor->mousePlane == mode, imguiRatio() ) ) {
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
				animator->toDeleteData.toDelete = deleted;
				animatorMessageBox(
				    animator, "Animation will be deleted", "Delete Animation", []( AppData* app ) {
					    auto animator = &app->animatorState;
					    animator->animations.erase( animator->toDeleteData.toDelete );
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
	if( animator->currentAnimation
	    && imguiBeginDropGroup( "Keying", &editor->expandedFlags, AnimatorEditor::Keying ) ) {

		auto node    = editor->clickedNode;

		auto compareVector = []( vec3arg a, vec3arg b ) {
			return floatEqSoft( a.x, b.x ) && floatEqSoft( a.y, b.y ) && floatEqSoft( a.z, b.z );
		};
		StringView translation = "Translation*";
		StringView rotation    = "Rotation*";
		StringView scale       = "Scale*";
		StringView frame       = "Frame*";
		StringView active      = "Active*";
		StringView flashColor  = "Flash Color*";

		if( node ) {
			auto current = getCurrentFrameNode( animator, editor->clickedNode );
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
			if( current.active == node->active ) {
				active.pop_back();
			}
			if( current.flashColor == node->flashColor ) {
				flashColor.pop_back();
			}
		}
		imguiSameLine( 2 );
		if( imguiButton( translation ) ) {
			if( node ) {
				AnimatorKeyframe key = {};
				key.t                = animator->currentFrame;
				key.group = makeAnimatorGroup( node->id, AnimatorKeyframeData::type_translation );
				set_variant( key.data, translation ) = node->translation;
				addKeyframe( animator, key );
			}
		}
		if( imguiButton( rotation ) ) {
			if( node ) {
				AnimatorKeyframe key = {};
				key.t                = animator->currentFrame;
				key.group = makeAnimatorGroup( node->id, AnimatorKeyframeData::type_rotation );
				set_variant( key.data, rotation ) = node->rotation;
				addKeyframe( animator, key );
			}
		}
		imguiSameLine( 2 );
		if( imguiButton( scale ) ) {
			if( node ) {
				AnimatorKeyframe key = {};
				key.t                = animator->currentFrame;
				key.group            = makeAnimatorGroup( node->id, AnimatorKeyframeData::type_scale );
				set_variant( key.data, scale ) = node->scale;
				addKeyframe( animator, key );
			}
		}
		if( imguiButton( flashColor ) ) {
			if( node ) {
				AnimatorKeyframe key = {};
				key.t                = animator->currentFrame;
				key.group = makeAnimatorGroup( node->id, AnimatorKeyframeData::type_flashColor );
				set_variant( key.data, flashColor ) = node->flashColor;
				addKeyframe( animator, key );
			}
		}
		imguiSameLine( 2 );
		if( imguiButton( frame ) ) {
			if( node && node->assetType == AnimatorAsset::type_collection ) {
				AnimatorKeyframe key = {};
				key.t                = animator->currentFrame;
				key.group = makeAnimatorGroup( node->id, AnimatorKeyframeData::type_frame );
				set_variant( key.data, frame ) = node->voxel.frame;
				addKeyframe( animator, key );
			}
		}
		if( imguiButton( active ) ) {
			if( node && ( isHitboxType( node->assetType )
			              || node->assetType == AnimatorAsset::type_emitter ) ) {
				AnimatorKeyframe key = {};
				key.t                = animator->currentFrame;
				key.group = makeAnimatorGroup( node->id, AnimatorKeyframeData::type_active );
				set_variant( key.data, active ) = (bool)node->active;
				addKeyframe( animator, key );
			}
		}
		if( imguiButton( "Event Attack" ) ) {
			AnimatorKeyframe key = {};
			key.t                = animator->currentFrame;
			key.group            = AnimatorEventsGroup;
			set_variant( key.data, event ) = SkeletonEventType::Attack;
			addKeyframe( animator, key );
		}

		imguiEndDropGroup();
	}
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
					destination = intersectionRayVsPlane( ray, plane.origin, plane.normal );
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
			destination = intersectionRayVsPlane( ray, plane.origin, plane.normal );
			break;
		}
	}
	return destination;
}

struct AnimatorProjection {
	mat4 viewProj;
	MatrixInverseResult invViewProj;
};
AnimatorProjection animatorSetProjection( AppData* app, GameInputs* inputs, rectfarg rect )
{
	AnimatorProjection result;

	auto animator = &app->animatorState;
	auto editor   = &animator->editor;
	auto renderer = &app->renderer;
	auto handle   = imguiMakeHandle( editor, ImGuiControlType::None );
	if( editor->viewType == AnimatorEditorViewType::Perspective ) {
		auto projection = matrixPerspectiveFovProjection( rect, app->width, app->height,
		                                                  degreesToRadians( 65 ), -1, 1 );
		setProjectionMatrix( renderer, ProjectionType::Perspective, projection );
		setProjection( renderer, ProjectionType::Perspective );

		auto mat = processEditorViewGui( &editor->view, inputs, rect );
		if( animator->flags.mirror ) {
			mat = matrixScale( -1, 1, 1 ) * mat;
		}
		renderer->view  = mat;
		result.viewProj = renderer->view * projection;
	} else {
		auto orthogonal = matrixOrthogonalProjection( 0, 0, app->width, app->height,
		                                               -editor->view.z, editor->view.z );
		// translate origin first, then apply orthogonal matrix
		auto c          = center( rect );
		auto offset     = matrixTranslation( c.x, -c.y, -editor->view.z ) * matrixScale( 1, -1, 1 );
		auto projection = offset * orthogonal;
		setProjectionMatrix( renderer, ProjectionType::Perspective, projection );
		setProjection( renderer, ProjectionType::Perspective );
		setRenderState( renderer, RenderStateType::BackFaceCulling, false );
		setRenderState( renderer, RenderStateType::CullFrontFace, false );

		editor->view.scale += 5;
		auto mat = processEditorViewGui( &editor->view, inputs, rect );
		editor->view.scale -= 5;
		if( animator->flags.mirror ) {
			mat = matrixScale( -1, 1, 1 ) * mat;
		}
		renderer->view  = mat;
		result.viewProj = renderer->view * projection;
	}

	result.invViewProj = inverse( result.viewProj );
	return result;
}

void animatorUpdateAndRenderNodes( RenderCommands* renderer, AnimatorState* animator )
{
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
			node->base            = local * node->parent->world;
			node->worldFlashColor = node->parent->worldFlashColor.bits | node->flashColor.bits;
		} else {
			node->base            = local;
			node->worldFlashColor = node->flashColor;
		}
		node->world = matrixTranslation( node->length, 0, 0 ) * node->base;
	}

	setRenderState( renderer, RenderStateType::DepthTest, true );
	setRenderState( renderer, RenderStateType::BackFaceCulling, false );
	auto stack = renderer->matrixStack;
	pushMatrix( stack );
	FOR( entry : animator->nodes ) {
		auto node = entry.get();
		// render voxels
		if( node->assetType == AnimatorAsset::type_collection && node->voxel.animation >= 0 ) {
			auto collection = get_variant( *node->asset, collection );
			setTexture( renderer, 0, collection->voxels.texture );
			auto voxels = &collection->voxels;
			auto range  = voxels->animations[node->voxel.animation].range;
			if( range ) {
				auto entry = &voxels->frames[range.min + ( node->voxel.frame % width( range ) )];
				currentMatrix( stack ) =
				    matrixTranslation( Vec3( -entry->offset.x, entry->offset.y, 0 ) ) * node->world;
			    renderer->flashColor = node->worldFlashColor;
				addRenderCommandMesh( renderer, entry->mesh );
			}
		}
	}
    renderer->flashColor = 0;
	setRenderState( renderer, RenderStateType::BackFaceCulling, true );
	popMatrix( stack );

	// render particles
	renderParticles( renderer, &animator->particleSystem );

	setRenderState( renderer, RenderStateType::DepthTest, false );
	if( animator->editor.viewFlags.visibleNodes ) {
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
	}
}
rectf animatorControlRect( vec2arg pos ) { return RectHalfSize( pos, 5, 5 ); }

void doNodeEditor( AppData* app, GameInputs* inputs, rectfarg rect )
{
	auto renderer = &app->renderer;
	auto animator = &app->animatorState;
	auto editor   = &animator->editor;
	auto handle   = imguiMakeHandle( editor, ImGuiControlType::None );

	auto projection = animatorSetProjection( app, inputs, rect );
	setScissorRect( renderer, Rect< int32 >( rect ) );
	setRenderState( renderer, RenderStateType::Scissor, true );

	animatorUpdateAndRenderNodes( renderer, animator );
	if( !( editor->viewFlags.visibleNodes ) ) {
		setProjection( renderer, ProjectionType::Orthogonal );
		setRenderState( renderer, RenderStateType::Scissor, false );
		return;
	}

	// render controls
	auto& viewProj    = projection.viewProj;
	auto& invViewProj = projection.invViewProj;

	setProjection( renderer, ProjectionType::Orthogonal );
	bool selectedAny = false;

	auto selectNode = [&]( AnimatorNode* node, vec2arg pos ) {
		imguiFocus( handle );
		if( isKeyDown( inputs, KC_Control ) ) {
			node->flags.selected = !node->flags.selected;
		} else {
			if( !node->flags.selected ) {
				clearSelectedNodes( animator );
				node->flags.selected = true;
			}
			editor->moving      = true;
			editor->clickedNode = node;
		}
	};

	auto lMousePressed = isKeyPressed( inputs, KC_LButton ) && !isKeyDown( inputs, KC_Space );

	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::White;
		FOR( entry : animator->nodes ) {
			auto node     = entry.get();
			if( node->flags.visible ) {
				auto mat      = node->world * viewProj;
				auto head     = toScreenSpace( mat, {}, app->width, app->height );
				auto headRect = animatorControlRect( head );
				if( node->flags.interactible && editor->viewFlags.interactibleNodes
				    && isPointInside( headRect, inputs->mouse.position ) && lMousePressed
				    && imguiIsHover( handle ) ) {

					selectedAny = true;
					selectNode( node, head );
					editor->mouseOffset  = head - inputs->mouse.position;
					editor->clickedVoxel = false;
				}

				stream->color = ( node->flags.selected ) ? ( Color::Red ) : ( Color::White );
				pushQuadOutline( stream, headRect );
			}
		}
		if( editor->viewFlags.visibleNodes && editor->viewFlags.interactibleNodes && !selectedAny
		    && lMousePressed && imguiIsHover( handle ) ) {
			// no node was selected, try selecting voxels
			if( invViewProj ) {
				auto ray = pointToWorldSpaceRay( invViewProj.matrix, inputs->mouse.position,
				                                 app->width, app->height );
				ray.dir = safeNormalize( ray.dir );

				FOR( entry : animator->nodes ) {
					auto node = entry.get();
					if( !node->selectable() ) {
						continue;
					}
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

		if( editor->moving && hasMagnitude( inputs->mouse.delta ) && invViewProj ) {
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
			auto position = intersectionRayVsPlane( ray, {}, {0, 0, 1} );*/
			addNewNode( animator, node );
		}
		imguiEndContainer();
	} else {
		imguiGetContainer( editor->contextMenu )->setHidden( true );
	}
}
void doHitboxEditor( AppData* app, GameInputs* inputs, rectfarg rect )
{
	auto renderer = &app->renderer;
	auto animator = &app->animatorState;
	auto editor   = &animator->editor;
	auto handle   = imguiMakeHandle( editor, ImGuiControlType::None );

	auto projection = animatorSetProjection( app, inputs, rect );
	setScissorRect( renderer, Rect< int32 >( rect ) );
	setRenderState( renderer, RenderStateType::Scissor, true );

	animatorUpdateAndRenderNodes( renderer, animator );

	// render hitboxes
	auto& viewProj = projection.viewProj;

	const Color HitboxColors[4] = {0x800000FF, 0x80FF0000, 0x8000FF00, 0x80634040};
	setRenderState( renderer, RenderStateType::BackFaceCulling, false );
	setTexture( renderer, 0, null );
	MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = 0x800000FF;
		FOR( nodeEntry : animator->nodes ) {
			auto node = nodeEntry.get();
			if( isHitboxType( node->assetType ) ) {
				auto head       = transformVector3( node->world, {} );
				auto hitbox     = getHitboxRect( node->asset );
				auto abs        = translate( *hitbox, head.xy );
				auto quad       = makeQuad( abs, head.z );
				auto colorIndex = node->assetType - AnimatorAsset::type_collision;
				assert( colorIndex >= 0 && colorIndex < countof( HitboxColors ) );
				stream->color = HitboxColors[colorIndex];
				pushQuad( stream, quad.elements );
			}
		}
	}

	// render controls
	auto& invViewProj = projection.invViewProj;
	auto view         = &editor->hitboxView;

	setRenderState( renderer, RenderStateType::BackFaceCulling, true );
	setProjection( renderer, ProjectionType::Orthogonal );

	struct HitboxControl {
		int8 x;
		int8 y;
	};

	// same ordering as AnimatorEditorHitboxFeature
	static const HitboxControl controls[8] = {
	    {RectComponent_Left, RectComponent_Top},     {-1, RectComponent_Top},
	    {RectComponent_Right, RectComponent_Top},    {RectComponent_Right, -1},
	    {RectComponent_Right, RectComponent_Bottom}, {-1, RectComponent_Bottom},
	    {RectComponent_Left, RectComponent_Bottom},  {RectComponent_Left, -1},
	};

	auto lMousePressed =
	    isKeyPressed( inputs, KC_LButton ) && isPointInside( rect, inputs->mouse.position );
	auto selected = false;
	LINE_MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::White;
		FOR( nodeEntry : animator->nodes ) {
			auto node = nodeEntry.get();
			if( isHitboxType( node->assetType ) ) {
				auto& hitbox = *getHitboxRect( node->asset );

				for( auto i = 0, count = countof( controls ); i < count; ++i ) {
					auto control = controls[i];
					vec2 pos;
					if( control.x < 0 ) {
						pos.x = ( hitbox.left + hitbox.right ) * 0.5f;
						pos.y = hitbox.elements[control.y];
					} else if( control.y < 0 ) {
						pos.x = hitbox.elements[control.x];
						pos.y = ( hitbox.top + hitbox.bottom ) * 0.5f;
					} else {
						pos = {hitbox.elements[control.x], hitbox.elements[control.y]};
					}
					auto head = transformVector3( node->world, {} );
					pos = toScreenSpace( viewProj, Vec3( pos, 0 ) + head, app->width, app->height );
					auto controlRect = animatorControlRect( pos );

					if( isPointInside( controlRect, inputs->mouse.position )
					    && lMousePressed ) {

						view->selectedFeature = ( AnimatorEditorHitboxFeature )( i + 1 );
						editor->mouseOffset   = inputs->mouse.position - pos;
						editor->clickedNode   = node;
						editor->moving = true;
						selected = true;
					}
					if( editor->clickedNode == node && valueof( view->selectedFeature ) == i + 1 ) {
						stream->color = Color::Red;
					} else {
						stream->color = Color::White;
					}
					pushQuadOutline( stream, controlRect );
				}
			}
		}
	}
	if( !selected && lMousePressed && invViewProj ) {
		// try selecting a hitbox itself
		FOR( nodeEntry : animator->nodes ) {
			auto node = nodeEntry.get();
			if( isHitboxType( node->assetType ) ) {
				auto& hitbox = *getHitboxRect( node->asset );

				auto head  = transformVector3( node->world, {} );
				auto mouse = inputs->mouse.position;
				auto ray =
				    pointToWorldSpaceRay( invViewProj.matrix, mouse, app->width, app->height );
				auto intersection = intersectionRayVsPlane( ray, head, {0, 0, 1} ) - head;
				if( isPointInside( correct( hitbox ), intersection.xy ) ) {
					view->selectedFeature = AnimatorEditorHitboxFeature::None;
					editor->clickedNode   = node;
					editor->mouseOffset   = {};
					editor->moving        = true;
					editor->clickedPlanePos = intersection;
					selected              = true;
				}
			}
		}
	}

	if( lMousePressed && !selected ) {
		editor->moving        = false;
		editor->clickedNode   = nullptr;
		view->selectedFeature = AnimatorEditorHitboxFeature::None;
	}
	if( isKeyReleased( inputs, KC_LButton ) ) {
		editor->moving = false;
	}

	if( editor->moving && editor->clickedNode && hasMagnitude( inputs->mouse.delta )
	    && invViewProj ) {

		auto node  = editor->clickedNode;
		auto head  = transformVector3( node->world, {} );
		auto mouse = inputs->mouse.position - editor->mouseOffset;
		auto ray   = pointToWorldSpaceRay( invViewProj.matrix, mouse, app->width, app->height );
		auto destination = intersectionRayVsPlane( ray, head, {0, 0, 1} ) - head;

		if( view->selectedFeature != AnimatorEditorHitboxFeature::None ) {
			switch( editor->mouseMode ) {
				case AnimatorMouseMode::Select: {
					break;
				}
				case AnimatorMouseMode::Translate: {
					if( !isKeyDown( inputs, KC_Alt ) ) {
						destination = round( destination );
					}
					assert( node->asset );
					auto& hitbox = *getHitboxRect( node->asset );
					auto index = valueof( view->selectedFeature ) - 1;
					assert( index >= 0 && index < 8 );
					auto control = controls[index];
					if( control.x >= 0 ) {
						hitbox.elements[control.x] = destination.x;
					}
					if( control.y >= 0 ) {
						hitbox.elements[control.y] = destination.y;
					}

					auto swapped = false;
					if( hitbox.left > hitbox.right ) {
						swap( hitbox.left, hitbox.right );
						swapped = true;
						if( control.x >= 0 ) {
							control.x = ( control.x + 2 ) % 4;
						}
					}
					if( hitbox.top < hitbox.bottom ) {
						swap( hitbox.top, hitbox.bottom );
						swapped = true;
						if( control.y >= 0 ) {
							control.y = ( control.y + 2 ) % 4;
						}
					}
					if( swapped ) {
						if( auto it = find_index_if( controls, [control]( HitboxControl entry ) {
							    return entry.x == control.x&& entry.y == control.y;
							} ) ) {
							view->selectedFeature =
							    ( AnimatorEditorHitboxFeature )( it.get() + 1 );
						}
					}
					break;
				}
			}
		} else {
			switch( editor->mouseMode ) {
				case AnimatorMouseMode::Select: {
					break;
				}
				case AnimatorMouseMode::Translate: {
					assert( node->asset );
					auto hitbox = getHitboxRect( node->asset );
					*hitbox     = translate( *hitbox, destination.xy - editor->clickedPlanePos.xy );
					editor->clickedPlanePos.xy = destination.xy;
					break;
				}
			}
		}
	}

	setRenderState( renderer, RenderStateType::Scissor, false );
}

void doEditor( AppData* app, GameInputs* inputs, rectfarg rect )
{
	auto animator = &app->animatorState;
	auto editor   = &animator->editor;
	switch( editor->editType ) {
		case AnimatorEditorEditType::Node: {
			doNodeEditor( app, inputs, rect );
			break;
		}
		case AnimatorEditorEditType::Hitbox: {
			doHitboxEditor( app, inputs, rect );
			break;
		}
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
		imguiLoadDefaultStyle( gui, &app->platform, font );

		animator->controlIcons = app->platform.loadTexture( "Data/Images/animator_controls.png" );

		animator->editor.contextMenu = imguiGenerateContainer( gui, {}, ImGuiVisibility::Hidden );
		animator->fileMenu           = imguiGenerateContainer( gui, {}, ImGuiVisibility::Hidden );
		animator->viewMenu           = imguiGenerateContainer( gui, {}, ImGuiVisibility::Hidden );
		animator->editMenu           = imguiGenerateContainer( gui, {}, ImGuiVisibility::Hidden );
		animator->assetContextMenu   = imguiGenerateContainer( gui, {}, ImGuiVisibility::Hidden );
		animator->messageBox.container =
		    imguiGenerateContainer( gui, {0, 0, 300, 10}, ImGuiVisibility::Hidden );

		auto allocator                   = &app->stackAllocator;
		const auto MaxNodes              = 10;
		const auto MaxKeyframes          = 50;
		const auto MaxAnimations         = 20;
		animator->stringPool             = makeStringPool( allocator, 100 );
		animator->particleSystem         = makeParticleSystem( allocator, 200 );
		animator->particleSystem.texture = app->platform.loadTexture( "Data/Images/dust.png" );

		animator->fieldNames[0] = pushString( &animator->stringPool, "Translation" );
		animator->fieldNames[1] = pushString( &animator->stringPool, "Rotation" );
		animator->fieldNames[2] = pushString( &animator->stringPool, "Scale" );
		animator->fieldNames[3] = pushString( &animator->stringPool, "Flash Color" );
		animator->fieldNames[4] = pushString( &animator->stringPool, "Frame" );
		animator->fieldNames[5] = pushString( &animator->stringPool, "Active" );
		animator->fieldNames[6] = pushString( &animator->stringPool, "Events" );

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

		addNewAsset( animator, AnimatorAsset::type_collection, "Data/voxels/hero.json" );
		animator->flags.unsavedChanges     = false;
		animator->flags.uncommittedChanges = false;
#endif

		animator->initialized = true;
	}

	renderer->color      = Color::White;
	renderer->clearColor = 0xFF1B2B34;

	setProjection( renderer, ProjectionType::Orthogonal );
	setRenderState( renderer, RenderStateType::DepthTest, false );

	auto t = dt * GameConstants::DeltaToFrameTime;
	processParticles( &animator->particleSystem, t );

	if( animator->currentAnimation && animator->flags.playing ) {
		auto lastFrame = animator->currentFrame;
		animator->currentFrame += t;
		if( animator->currentFrame > animator->duration ) {
			animator->currentFrame = animator->duration;
		}
		animatorSetCurrentFrame( animator, animator->currentFrame, lastFrame );
		FOR( entry : animator->nodes ) {
			auto node = entry.get();
			if( node->assetType == AnimatorAsset::type_emitter && node->active ) {
				auto emitter = &get_variant( *node->asset, emitter );
				node->emitter.time -= t;
				if( node->emitter.time <= 0.0f ) {
					if( emitter->interval > 0 ) {
						node->emitter.time += emitter->interval;
					} else {
						node->emitter.time = FLOAT32_MAX;
					}
					auto pos = transformVector3( node->world, {} );
					pos.y    = -pos.y;
					emitParticles( &animator->particleSystem, pos, emitter->emitter );
				}
			}
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
		auto itemHandle = imguiMakeHandle( &animator->fileMenu );
		imguiMenuItem( itemHandle, "File", animator->fileMenu );
		imguiMenuItem( itemHandle, "Edit", animator->editMenu );
		imguiMenuItem( itemHandle, "View", animator->viewMenu );
		imguiMenuItem( itemHandle, "Preferences" );
		imguiMenuItem( itemHandle, "Help" );
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

	if( imguiContextMenu( animator->viewMenu ) ) {
		auto editor = &animator->editor;
		imguiRadiobox( "Perspective", &editor->viewType, AnimatorEditorViewType::Perspective );
		imguiRadiobox( "Orthogonal", &editor->viewType, AnimatorEditorViewType::Orthogonal );
		editor->viewFlags.visibleNodes =
		    imguiCheckbox( "Draw Nodes", editor->viewFlags.visibleNodes != 0 );
		imguiEndContainer();
	}

	if( imguiContextMenu( animator->editMenu ) ) {
		auto editor = &animator->editor;
		imguiRadiobox( "Node Mode", &editor->editType, AnimatorEditorEditType::Node );
		imguiRadiobox( "Hitbox Mode", &editor->editType, AnimatorEditorEditType::Hitbox );
		imguiEndContainer();
	}

	if( imguiContextMenu( animator->assetContextMenu ) ) {
		auto anyAssetSelected =
		    any_of( animator->assets, []( const auto& entry ) { return entry->item.selected; } );
		imguiContextMenuEntry( "Copy", anyAssetSelected );
		imguiContextMenuEntry( "Paste", false );
		imguiContextMenuEntry( "Delete", anyAssetSelected );
		imguiEndContainer();
	}

	handleMessageBox( app, inputs, animator->messageBox );

	imguiUpdate( dt );
	imguiFinalize();

#if 1
	// debug
	debugPrintln( "AnimatorKeyframes Count: {}", animator->keyframes.size() );

	FOR( entry : animator->assets ) {
		auto asset = entry.get();
		debugPrintln( "Asset {}: {}", asset->item.text, asset->id );
	}
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
	nameLength     = copyToString( str, name, countof( name ) );
	itemTextLength = snprint( itemText, countof( itemText ), "{} - {}", str, to_string( type ) );
	item.text      = {itemText, itemTextLength};
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
		case type_collision: {
			type      = type_collision;
			collision = other.collision;
			break;
		}
		case type_hitbox: {
			type   = type_hitbox;
			hitbox = other.hitbox;
			break;
		}
		case type_hurtbox: {
			type    = type_hurtbox;
			hurtbox = other.hurtbox;
			break;
		}
		case type_deflect: {
			type    = type_deflect;
			deflect = other.deflect;
			break;
		}
		case type_emitter: {
			type    = type_emitter;
			emitter = other.emitter;
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
		case type_collision: {
			type      = type_collision;
			collision = other.collision;
			break;
		}
		case type_hitbox: {
			type   = type_hitbox;
			hitbox = other.hitbox;
			break;
		}
		case type_hurtbox: {
			type    = type_hurtbox;
			hurtbox = other.hurtbox;
			break;
		}
		case type_deflect: {
			type    = type_deflect;
			deflect = other.deflect;
			break;
		}
		case type_emitter: {
			type    = type_emitter;
			emitter = other.emitter;
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
		case type_none:
		case type_collision:
		case type_hitbox:
		case type_hurtbox:
		case type_deflect:
		case type_emitter: {
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