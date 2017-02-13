bool loadVoxelCollection( StackAllocator* allocator, StringView filename, VoxelCollection* out );
void from_string( StringView str, AnimatorKeyframeData::EaseType* out );
void deserialize( const JsonValue& value, AnimatorParticleEmitter& out );

template < class T, class Op >
void readValues( StackAllocator* allocator, const JsonObjectArray& array, Array< T >* out, Op op )
{
	const auto count = array.size();
	*out             = makeArray( allocator, T, count );
	for( auto i = 0; i < count; ++i ) {
		op( array[i], &( ( *out )[i] ) );
	}
}

void readNodes( StackAllocator* allocator, const JsonObject& attr,
                Array< SkeletonTransform >* transforms, Array< SkeletonVoxelVisuals >* visuals,
                Array< SkeletonEmitterState >* emitters, bool base )
{
	auto transformsAttr        = attr["transforms"].getObjectArray();
	const auto transformsCount = transformsAttr.size();
	auto readTransforms        = [base]( const JsonObject& attr, SkeletonTransform* dest ) {
		deserialize( attr["id"], dest->id, -1 );
		deserialize( attr["translation"], dest->translation );
		deserialize( attr["rotation"], dest->rotation );
		deserialize( attr["scale"], dest->scale );
		deserialize( attr["length"], dest->length );
		if( base ) {
			deserialize( attr["parentId"], dest->parent, -1 );
		}
	};
	readValues( allocator, transformsAttr, transforms, readTransforms );

	auto readVoxels = [base]( const JsonObject& attr, SkeletonVoxelVisuals* dest ) {
		deserialize( attr["id"], dest->id, -1 );
		dest->index = -1;
		if( base ) {
			deserialize( attr["assetId"], dest->assetIndex, -1 );
		}
		deserialize( attr["animation"], dest->animation, -1 );
		deserialize( attr["frame"], dest->frame );
	};
	readValues( allocator, attr["voxels"].getObjectArray(), visuals, readVoxels );

	auto readEmitters = [base]( const JsonObject& attr, SkeletonEmitterState* dest ) {
		deserialize( attr["id"], dest->id, -1 );
		dest->index = -1;
		if( base ) {
			deserialize( attr["assetId"], dest->assetIndex, -1 );
		}
		deserialize( attr["active"], dest->active );
		dest->time = 0;
	};
	readValues( allocator, attr["emitters"].getObjectArray(), emitters, readEmitters );
}

template < class T >
pair< int16, bool > bindIdsToIndices( Array< T > visualsArray, Array< int16 > ids,
                                      Array< SkeletonAssetId > assetIds )
{
	FOR( visuals : visualsArray ) {
		auto nodeId = visuals.id;
		// convert id to index
		visuals.index = auto_truncate( find_index( ids, nodeId ).value );
		// assetIndex is initialized with voxel id
		auto assetId         = visuals.assetIndex;
		auto assetIdentIndex = find_index_if( assetIds, [assetId]( const SkeletonAssetId& id ) {
			                       return id.id == assetId;
			                   } ).value;

		if( visuals.index < 0 || assetIdentIndex < 0 || assetIds[assetIdentIndex].assetIndex < 0
		    || assetIds[assetIdentIndex].type != Array< T >::value_type::AssetType ) {
			return {nodeId, false};
		}
		visuals.assetIndex               = assetIds[assetIdentIndex].assetIndex;
	}
	return {-1, true};
}

template < class T >
bool applyPermutation( Array< T > array, Array< int16 > permutation )
{
	if( array.size() != permutation.size() ) {
		return false;
	}

	TEMPORARY_MEMORY_BLOCK( GlobalScrap ) {
		const auto count = array.size();
		auto oldValues = makeArray( GlobalScrap, int16, count );
		iota_n( oldValues.begin(), count, (int16)0 );
		for( auto i = 0; i < count; ++i ) {
			auto perm    = permutation[i];
			if( i != oldValues[perm] ) {
				swap( array[i], array[oldValues[perm]] );
				swap( oldValues[i], oldValues[perm] );
			}
		}
	}
	return true;
}
bool sortTransforms( Array< SkeletonTransform > transforms )
{
	auto scrap = GlobalScrap;
	const auto transformsCount = transforms.size();
	TEMPORARY_MEMORY_BLOCK( scrap ) {
		// init children counts
		struct ChildrenCount {
			int16 id;
			int16 index;
			int16 parentId;
			int16 parent;
			int16 childrenCount;
		};
		auto childrenCounts = makeArray( scrap, ChildrenCount, transformsCount );
		for( auto i = 0; i < transformsCount; ++i ) {
			auto entry           = &childrenCounts[i];
			entry->id            = transforms[i].id;
			entry->index         = auto_truncate( i );
			entry->parentId      = transforms[i].parent;
			entry->parent        = -1;
			entry->childrenCount = 0;
		}

		auto bindParents = [&](){
			FOR( entry : childrenCounts ) {
				auto parentId = entry.parentId;
				if( parentId >= 0 ) {
					entry.parent = auto_truncate(
					    find_index_if( childrenCounts, [parentId]( ChildrenCount entry ) {
						    return entry.id == parentId;
						} ).value );
					if( entry.parent < 0 ) {
						return false;
					}
				}
			}
			return true;
		};
		if( !bindParents() ) {
			return false;
		}

		// calculate children counts
		FOR( entry : childrenCounts ) {
			if( entry.parent >= 0 ) {
				int16 add       = entry.childrenCount + 1;
				auto parent     = entry.parent;
				auto iterations = 0;
				for( ; iterations < SkeletonMaxParents && parent >= 0; ++iterations ) {
					childrenCounts[parent].childrenCount += add;
					parent = childrenCounts[parent].parent;
				}
				if( iterations >= SkeletonMaxParents ) {
					return false;
				}
			}
		}
		// sort by children counts
		sort( childrenCounts.begin(), childrenCounts.end(), []( ChildrenCount a, ChildrenCount b ) {
			return a.childrenCount > b.childrenCount;
		} );
		if( !bindParents() ) {
			return false;
		}

		auto permutation = makeArray( scrap, int16, transformsCount );
		// output permutation
		for( auto i = 0; i < transformsCount; ++i ) {
			permutation[i] = childrenCounts[i].index;
		}
		applyPermutation( transforms, permutation );

		// set parent indices of transforms
		for( auto i = 0; i < transformsCount; ++i ) {
			transforms[i].parent = childrenCounts[i].parent;
		}
	}
	return true;
}

template<class T >
bool applyPermutationByIds( Array< T > array, Array< int16 > ids )
{
	if( array.size() != ids.size() ) {
		return false;
	}

	TEMPORARY_MEMORY_BLOCK( GlobalScrap ) {
		const auto count = array.size();
		auto permutation = makeArray( GlobalScrap, int16, count );
		for( auto i = 0; i < count; ++i ) {
			auto id        = ids[i];
			permutation[i] = auto_truncate(
			    find_index_if( array, [id]( const T& entry ) { return entry.id == id; } ).value );
			if( permutation[i] < 0 ) {
				return false;
			}
		}

		applyPermutation( array, permutation );
	}
	return true;
}
template < class T >
bool applyPermutationByIds( Array< T > array, Array< T > base )
{
	if( array.size() != base.size() ) {
		return false;
	}

	TEMPORARY_MEMORY_BLOCK( GlobalScrap ) {
		const auto count = array.size();
		auto ids         = makeArray( GlobalScrap, int16, count );
		for( auto i = 0; i < count; ++i ) {
			ids[i] = base[i].id;
		}

		applyPermutationByIds( array, ids );
	}
	return true;
}

bool loadSkeletonDefinitionImpl( StackAllocator* allocator, StringView filename,
                                 SkeletonDefinition* out )
{
#define ABORT_ERROR( str, ... )                            \
	do {                                                   \
		LOG( ERROR, "{}: " str, filename, ##__VA_ARGS__ ); \
		/*__debugbreak();*/                                    \
		return false;                                      \
	} while( false )

	auto guard = StackAllocatorGuard( allocator );

	auto scrap = GlobalScrap;
	TEMPORARY_MEMORY_BLOCK( scrap ) {
		auto file = readFile( scrap, filename );
		auto doc = makeJsonDocument( scrap, file );
		if( !doc ) {
			ABORT_ERROR( "Invalid json" );
		}
		auto root = doc.root.getObject();
		if( !root ) {
			ABORT_ERROR( "Invalid json, root is not object" );
		}

		// assets
		if( auto assets = root["assets"].getObject() ) {
			auto voxels            = assets["voxels"].getObjectArray();
			auto emitters          = assets["emitters"].getObjectArray();
			auto collision_bounds  = assets["collision_bounds"].getObjectArray();
			auto hitboxes          = assets["hitboxes"].getObjectArray();
			auto hurtboxes         = assets["hurtboxes"].getObjectArray();
			const auto assetsCount = voxels.size() + emitters.size() + collision_bounds.size()
			                         + hitboxes.size() + hurtboxes.size();

			auto assetIds = makeUArray( allocator, SkeletonAssetId, assetsCount );

			// voxels
			const auto voxelsCount = voxels.size();
			out->voxels            = makeArray( allocator, VoxelCollection, voxelsCount );
			for( auto i = 0; i < voxelsCount; ++i ) {
				auto attr           = voxels[i];
				auto dest           = &out->voxels[i];
				auto ident          = assetIds.emplace_back();
				auto voxelsFilename = attr["filename"].getString();
				if( !loadVoxelCollection( allocator, voxelsFilename, dest ) ) {
					ABORT_ERROR( "Failed to load voxel collection {}", voxelsFilename );
				}
				deserialize( attr["id"], ident->id, -1 );
				ident->nameLength = (int16)copyToString( attr["name"].getString(), ident->name );
				ident->assetIndex = auto_truncate( i );
				ident->type       = AnimatorAsset::type_collection;
			}

			// emitters
			const auto emittersCount = emitters.size();
			out->emitters = makeArray( allocator, AnimatorParticleEmitter, emittersCount );
			for( auto i = 0; i < emittersCount; ++i ) {
				auto attr  = emitters[i];
				auto dest  = &out->emitters[i];
				auto ident = assetIds.emplace_back();
				deserialize( attr["id"], ident->id, -1 );
				ident->nameLength = (int16)copyToString( attr["name"].getString(), ident->name );
				ident->assetIndex = auto_truncate( i );
				ident->type       = AnimatorAsset::type_emitter;

				deserialize( attr["emitter"], *dest );
			}

			// collision_bounds
			// TODO: implement

			// hitboxes
			// TODO: implement

			// hurtboxes
			// TODO: implement

			// assert( assetIds.size() == assetsCount );
			out->assetIds = makeArrayView( assetIds );
		} else {
			ABORT_ERROR( "No assets defined" );
		}

		// nodes
		auto nodes   = root["nodes"].getObject();
		auto nodeIds = makeArray( scrap, int16, nodes["idents"].getArray().size() );
		fill( nodeIds.data(), (int16)-1, nodeIds.size() );
		if( nodes ) {
			TEMPORARY_MEMORY_BLOCK( scrap ) {
				// idents
				auto idents            = nodes["idents"].getObjectArray();
				const auto identsCount = idents.size();
				auto readIdents        = []( const JsonObject& attr, SkeletonId* dest ) {
					deserialize( attr["id"], dest->id, -1 );
					dest->nameLength = (int16)copyToString( attr["name"].getString(), dest->name );
				};
				readValues( allocator, idents, &out->nodeIds, readIdents );

				readNodes( allocator, nodes, &out->baseNodes, &out->baseVisuals, &out->baseEmitters,
				           true );

				if( !sortTransforms( out->baseNodes ) ) {
					ABORT_ERROR( "Failed to form graph from parent/children nodes" );
				}

				const auto transformsCount = out->baseNodes.size();
				// fill nodeIds
				if( nodeIds.size() != transformsCount ) {
					ABORT_ERROR( "Invalid transforms count" );
				}
				for( auto i = 0; i < transformsCount; ++i ) {
					nodeIds[i] = out->baseNodes[i].id;
					if( nodeIds[i] < 0 ) {
						ABORT_ERROR( "Invalid transform id -1" );
					}
				}

				if( !applyPermutationByIds( out->nodeIds, nodeIds ) ) {
					ABORT_ERROR( "Invalid idents" );
				}

				// voxels
				auto bindIds = bindIdsToIndices( out->baseVisuals, nodeIds, out->assetIds );
				if( !bindIds.second ) {
					ABORT_ERROR( "Invalid voxels entry {}", bindIds.first );
				}

				// emitters
				bindIds = bindIdsToIndices( out->baseEmitters, nodeIds, out->assetIds );
				if( !bindIds.second ) {
					ABORT_ERROR( "Invalid emitter entry {}", bindIds.first );
				}

				// collision_bounds
				// TODO: implement

				// hitboxes
				// TODO: implement

				// hurtboxes
				// TODO: implement
			}
		} else {
			ABORT_ERROR( "No nodes defined" );
		}

		// animations
		if( auto animations = root["animations"].getObjectArray() ) {
			const auto animationsCount = animations.size();
			out->animations            = makeArray( allocator, SkeletonAnimation, animationsCount );
			out->animationIds          = makeArray( allocator, SkeletonId, animationsCount );
			for( auto i = 0; i < animationsCount; ++i ) {
				auto attr      = animations[i];
				auto animation = &out->animations[i];
				auto ident     = &out->animationIds[i];
				*animation     = {};
				// name
				ident->id = auto_truncate( i );
				ident->nameLength =
				    auto_truncate( copyToString( attr["name"].getString(), ident->name ) );

				readNodes( allocator, attr, &animation->transforms, &animation->visuals,
				           &animation->emitters, false );
				if( !applyPermutationByIds( animation->transforms, nodeIds ) ) {
					ABORT_ERROR( "Invalid transforms in animation \"{}\"",
					             StringView{ident->name, ident->nameLength} );
				}
				if( !applyPermutationByIds( animation->visuals, out->baseVisuals ) ) {
					ABORT_ERROR( "Invalid visuals in animation \"{}\"",
					             StringView{ident->name, ident->nameLength} );
				}
				if( !applyPermutationByIds( animation->emitters, out->baseEmitters ) ) {
					ABORT_ERROR( "Invalid emitters in animation \"{}\"",
					             StringView{ident->name, ident->nameLength} );
				}
				// copy base node values to relative transforms
				for( auto i = 0, count = animation->transforms.size(); i < count; ++i ) {
					animation->transforms[i].parent = out->baseNodes[i].parent;
				}
				for( auto i = 0, count = animation->visuals.size(); i < count; ++i ) {
					auto visuals = &animation->visuals[i];
					auto base    = &out->baseVisuals[i];
					assert( visuals->id == base->id );
					visuals->assetIndex = base->assetIndex;
					visuals->index      = base->index;
				}
				for( auto i = 0, count = animation->emitters.size(); i < count; ++i ) {
					auto emitter = &animation->emitters[i];
					auto base    = &out->baseEmitters[i];
					assert( emitter->id == base->id );
					emitter->assetIndex = base->assetIndex;
					emitter->index      = base->index;
				}

				// TODO: implement
				// collision_bounds
				// hitboxes
				// hurtboxes

				// keyframes
				TEMPORARY_MEMORY_BLOCK( scrap ) {
					auto curves = beginVector( scrap, BezierForwardDifferencerData );

					bool success             = true;
					auto deserializeKeyframe = [&]( const JsonObject& attr, auto* out ) {
						deserialize( attr["t"], out->t );
						AnimatorKeyframeData::EaseType easeType = {};
						from_string( attr["easeType"].getString(), &easeType );
						out->easeType   = (SkeletonEaseType)valueof( easeType );
						out->curveIndex = -1;
						if( out->easeType == SkeletonEaseType::Curve ) {
							auto curve = attr["curve"].getArray();
							vec2 c0    = {};
							vec2 c1    = {};
							deserialize( curve[0], c0 );
							deserialize( curve[1], c1 );
							if( curves.remaining() ) {
								out->curveIndex        = auto_truncate( curves.size() );
								*curves.emplace_back() = makeEasingCurve( c0, c1 );
							} else {
								success = false;
							}
						}
						deserialize( attr["value"], out->data );
					};
					auto compare = []( const auto& a, const auto& b ) { return a.t < b.t; };

					auto keyframes = attr["keyframes"].getObjectArray();
					const auto keyframesCount = keyframes.size();
					if( keyframesCount != animation->transforms.size() ) {
						ABORT_ERROR( "Invalid keyframes amount" );
					}
					animation->keyframes =
					    makeArray( allocator, SkeleonKeyframes, keyframesCount );
					for( auto i = 0; i < keyframesCount; ++i ) {
						auto keyframesAttr = keyframes[i];
						auto dest = &animation->keyframes[i];
						*dest = {};

						deserialize( keyframesAttr["id"], dest->id, -1 );

						// translation
						auto translation            = keyframesAttr["translation"].getObjectArray();
						const auto translationCount = translation.size();
						dest->translation =
						    makeArray( allocator, SkeletonKeyframe< vec3 >, translationCount );
						for( auto j = 0; j < translationCount; ++j ) {
							deserializeKeyframe( translation[j], &dest->translation[j] );
						}
						sort( dest->translation.begin(), dest->translation.end(), compare );

						// rotation
						auto rotation            = keyframesAttr["rotation"].getObjectArray();
						const auto rotationCount = rotation.size();
						dest->rotation =
						    makeArray( allocator, SkeletonKeyframe< vec3 >, rotationCount );
						for( auto j = 0; j < rotationCount; ++j ) {
							deserializeKeyframe( rotation[j], &dest->rotation[j] );
						}
						sort( dest->rotation.begin(), dest->rotation.end(), compare );

						// scale
						auto scale            = keyframesAttr["scale"].getObjectArray();
						const auto scaleCount = scale.size();
						dest->scale =
						    makeArray( allocator, SkeletonKeyframe< vec3 >, scaleCount );
						for( auto j = 0; j < scaleCount; ++j ) {
							deserializeKeyframe( scale[j], &dest->scale[j] );
						}
						sort( dest->scale.begin(), dest->scale.end(), compare );

						// frame
						auto frame            = keyframesAttr["frame"].getObjectArray();
						const auto frameCount = frame.size();
						dest->frame = makeArray( allocator, SkeletonKeyframe< int8 >, frameCount );
						for( auto j = 0; j < frameCount; ++j ) {
							deserializeKeyframe( frame[j], &dest->frame[j] );
						}
						sort( dest->frame.begin(), dest->frame.end(), compare );

						// active
						auto active            = keyframesAttr["active"].getObjectArray();
						const auto activeCount = active.size();
						dest->active =
						    makeArray( allocator, SkeletonKeyframe< bool8 >, activeCount );
						for( auto j = 0; j < activeCount; ++j ) {
							deserializeKeyframe( active[j], &dest->active[j] );
						}
						sort( dest->active.begin(), dest->active.end(), compare );

						// calculate duration
						float durations[5] = {};
						if( dest->translation.size() ) {
							durations[0] = dest->translation.back().t;
						}
						if( dest->rotation.size() ) {
							durations[1] = dest->rotation.back().t;
						}
						if( dest->scale.size() ) {
							durations[2] = dest->scale.back().t;
						}
						if( dest->frame.size() ) {
							durations[3] = dest->frame.back().t;
						}
						if( dest->active.size() ) {
							durations[4] = dest->active.back().t;
						}
						auto currentDuration = *max_element( begin( durations ), end( durations ) );
						animation->duration  = max( currentDuration, animation->duration );
						dest->hasKeyframes  = dest->translation.size() || dest->rotation.size()
						                     || dest->scale.size() || dest->frame.size()
						                     || dest->active.size();
					}
					if( !success ) {
						ABORT_ERROR( "Out of memory" );
					}
					animation->curves =
					    makeArray( allocator, BezierForwardDifferencerData, curves.size() );
					animation->curves.assign( makeArrayView( curves ) );
				}
				if( !applyPermutationByIds( animation->keyframes, nodeIds ) ) {
					ABORT_ERROR( "Invalid keyframes" );
				}
			}
		} else {
			ABORT_ERROR( "No animations defined" );
		}
	}
#undef ABORT_ERROR
	guard.commit();
	return true;
}

SkeletonDefinition loadSkeletonDefinition( StackAllocator* allocator, StringView filename,
                                           int16 id )
{
	assert( id >= 0 );
	SkeletonDefinition result = {};
	result.id = -1;
	if( loadSkeletonDefinitionImpl( allocator, filename, &result ) ) {
		result.id = id;

		LOG( INFORMATION, "{}: Success loading skeleton definition", filename );
	}
	return result;
}

Skeleton makeSkeleton( StackAllocator* allocator, const SkeletonDefinition& definition )
{
	assert( definition );
	Skeleton result       = {};
	result.rootTransform  = matrixIdentity();
	const auto nodesCount = definition.baseNodes.size();
	result.transforms     = makeArray( allocator, SkeletonTransform, nodesCount );
	result.transforms.assign( definition.baseNodes );

	result.worldTransforms = makeArray( allocator, mat4, nodesCount );

	result.visuals = makeArray( allocator, SkeletonVoxelVisuals, definition.baseVisuals.size() );
	result.visuals.assign( definition.baseVisuals );

	result.emitters = makeArray( allocator, SkeletonEmitterState, definition.baseEmitters.size() );
	result.emitters.assign( definition.baseEmitters );

	const auto voxelsCount = definition.voxels.size();
	result.voxels          = makeArray( allocator, const VoxelCollection*, voxelsCount );
	for( auto i = 0; i < voxelsCount; ++i ) {
		result.voxels[i] = &definition.voxels[i];
	}

	const int32 MaxAnimationStates = 4;
	result.animations   = makeUArray( allocator, SkeletonAnimationState, MaxAnimationStates );
	result.keyframeStatesPool =
	    makeArray( allocator, Array< SkeletonKeyframesState >, MaxAnimationStates );
	FOR( states : result.keyframeStatesPool ) {
		states = makeArray( allocator, SkeletonKeyframesState, nodesCount );
	}
	result.definition   = &definition;
	result.definitionId = definition.id;
	result.dirty        = true;
	return result;
}

int32 playAnimation( Skeleton* skeleton, int32 animationIndex, bool repeating = false )
{
	assert( skeleton );
	assert( animationIndex >= 0 );
	auto existing = find_index_if( skeleton->animations, [animationIndex]( const auto& entry ) {
		return entry.animationIndex == animationIndex;
	} );
	if( existing ) {
		auto state = &skeleton->animations[existing.get()];
		setFlag( state->flags, SkeletonAnimationState::Playing );
		return existing.get();
	}
	if( skeleton->animations.remaining() ) {
		auto result     = skeleton->animations.size();
		auto definition = skeleton->definition;
		assert( definition );
		auto state            = skeleton->animations.emplace_back();
		*state                = {};
		state->animationIndex = auto_truncate( animationIndex );
		state->flags          = SkeletonAnimationState::Playing;
		setFlagCond( state->flags, SkeletonAnimationState::Repeating, repeating );
		state->currentFrame   = 0;
		state->keyframeStates = skeleton->keyframeStatesPool[result];
		fill( state->keyframeStates.data(), {-1, -1, -1, -1}, state->keyframeStates.size() );
		skeleton->dirty = true;
		return result;
	}
	return -1;
}
int32 playAnimation( Skeleton* skeleton, StringView name, bool repeating = false )
{
	assert( skeleton );
	auto definition = skeleton->definition;
	assert( definition );
	auto animation = find_index_if( definition->animationIds, [name]( const auto& entry ) {
		return StringView{entry.name, entry.nameLength} == name;
	} );
	if( animation ) {
		return playAnimation( skeleton, animation.get(), repeating );
	}
	return -1;
}
void stopAnimations( Skeleton* skeleton )
{
	skeleton->animations.clear();
	skeleton->dirty = true;
}

void update( Skeleton* skeleton, ParticleSystem* particleSystem, float dt )
{
	assert( skeleton );
	assert( skeleton->transforms.size() == skeleton->worldTransforms.size() );
	assert( skeleton->definition );

	auto animationStates = skeleton->animations;
	auto definition      = skeleton->definition;
	if( animationStates.size() ) {
		// apply animations base values (only toplevel animation)
		auto animations     = definition->animations;
		const auto toplevel = &animations[animationStates[0].animationIndex];
		auto transforms     = skeleton->transforms;
		auto visuals        = skeleton->visuals;
		auto emitters       = skeleton->emitters;

		const auto transformsCount = transforms.size();
		const auto visualsCount    = visuals.size();
		const auto emittersCount   = emitters.size();

		transforms.assign( toplevel->transforms );
		visuals.assign( toplevel->visuals );
		// emitters can't be assigned easily because timer isn't allowed to be reset here
		for( auto i = 0; i < emittersCount; ++i ) {
			emitters[i].active = toplevel->emitters[i].active;
		}

		auto findCurrentKeyframe = []( float currentFrame, const auto& keyframes, int16 index ) {
			while( index + 1 < keyframes.size() ) {
				if( keyframes[index + 1].t <= currentFrame ) {
					++index;
				} else {
					break;
				}
			}
			return index;
		};
		auto ease = []( Array< BezierForwardDifferencerData > curves, const auto& keyframe,
		                float t ) {
			switch( keyframe.easeType ) {
				case SkeletonEaseType::Lerp: {
					return t;
				}
				case SkeletonEaseType::Step: {
					return 0.0f;
				}
				case SkeletonEaseType::Smoothstep: {
					return smoothstep( t );
				}
				case SkeletonEaseType::EaseOutBounce: {
					return easeOutBounce( t );
				}
				case SkeletonEaseType::EaseOutElastic: {
					return easeOutElastic( t );
				}
				case SkeletonEaseType::Curve: {
					auto curve = &curves[keyframe.curveIndex];
					return evaluateBezierForwardDifferencerFromX( curve, t );
				}
					InvalidDefaultCase;
			}
			return t;
		};
		auto interpolateKeyframeData = [ease]( Array< BezierForwardDifferencerData > curves,
		                                       float currentFrame, const auto& keyframes,
		                                       int16 index, auto def ) {
			typeof( keyframes )::value_type::value_type result = def;
			if( index >= 0 && index < keyframes.size() ) {
				auto current = &keyframes[index];
				if( index + 1 < keyframes.size() ) {
					auto next        = &keyframes[index + 1];
					auto invDuration = 1.0f / ( next->t - current->t );
					auto delta       = ( currentFrame - current->t ) * invDuration;
					auto t           = ease( curves, *current, delta );
					result           = lerp( t, current->data, next->data );
				} else {
					result = current->data;
				}
			}
			return result;
		};
		auto processKeyframe = [findCurrentKeyframe, interpolateKeyframeData](
		    Array< BezierForwardDifferencerData > curves, float currentFrame, const auto& keyframes,
		    int16* index, auto* data ) {
			typeof( keyframes )::value_type::value_type def = {};
			*index = findCurrentKeyframe( currentFrame, keyframes, *index );
			*data += interpolateKeyframeData( curves, currentFrame, keyframes, *index, def );
		};
		auto processScale = [findCurrentKeyframe, interpolateKeyframeData](
		    Array< BezierForwardDifferencerData > curves, float currentFrame, const auto& keyframes,
		    int16* index, auto* data ) {
			typeof( keyframes )::value_type::value_type def = {1, 1, 1};
			*index = findCurrentKeyframe( currentFrame, keyframes, *index );
			*data  = multiplyComponents(
			    interpolateKeyframeData( curves, currentFrame, keyframes, *index, def ), *data );
		};
		auto processCustom = [findCurrentKeyframe]( Array< BezierForwardDifferencerData > curves,
		                                            float currentFrame, const auto& keyframes,
		                                            int16* index, auto* data ) {
			typeof( keyframes )::value_type::value_type def = {};
			*index = findCurrentKeyframe( currentFrame, keyframes, *index );
			if( *index >= 0 ) {
				*data = keyframes[*index].data;
			} else {
				*data = def;
			}
		};

		FOR( state : animationStates ) {
			if( !( state.flags & SkeletonAnimationState::Playing ) ) {
				continue;
			}
			auto animation = &animations[state.animationIndex];
			auto keyframes = animation->keyframes;
			auto curves    = animation->curves;
			state.currentFrame += dt;
			bool reset = false;
			if( state.currentFrame > animation->duration ) {
				if( state.flags & SkeletonAnimationState::Repeating ) {
					state.currentFrame -= animation->duration;
					reset = true;
				} else {
					clearFlag( state.flags, SkeletonAnimationState::Playing );
				}
			}
			auto currentFrame = state.currentFrame;
			// apply keyframe animations
			for( auto i = 0; i < transformsCount; ++i ) {
				auto currentKeyframes = &keyframes[i];
				if( currentKeyframes->hasKeyframes ) {
					auto transform     = &transforms[i];
					auto keyframeState = &state.keyframeStates[i];
					if( reset ) {
						*keyframeState = {-1, -1, -1, -1};
					}
					processKeyframe( curves, currentFrame, currentKeyframes->translation,
					                 &keyframeState->translation, &transform->translation );
					processKeyframe( curves, currentFrame, currentKeyframes->rotation,
					                 &keyframeState->rotation, &transform->rotation );
					processScale( curves, currentFrame, currentKeyframes->scale,
					              &keyframeState->scale, &transform->scale );
				}
			}
			for( auto i = 0; i < visualsCount; ++i ) {
				auto entry            = &visuals[i];
				auto currentKeyframes = &keyframes[entry->index];
				if( currentKeyframes->hasKeyframes ) {
					auto keyframeState = &state.keyframeStates[entry->index];
					processCustom( curves, currentFrame, currentKeyframes->frame,
					               &keyframeState->custom, &entry->frame );
				}
			}
			for( auto i = 0; i < emittersCount; ++i ) {
				auto entry            = &emitters[i];
				auto currentKeyframes = &keyframes[entry->index];
				if( currentKeyframes->hasKeyframes ) {
					auto keyframeState = &state.keyframeStates[entry->index];
					bool8 wasActive    = {false};
					auto prev          = keyframeState->custom;
					if( prev >= 0 && currentKeyframes->active.size() ) {
						wasActive = currentKeyframes->active[prev].data;
					}
					processCustom( curves, currentFrame, currentKeyframes->active,
					               &keyframeState->custom, &entry->active );
					if( ( prev != keyframeState->custom && wasActive && entry->active ) || reset ) {
						// force emitting of particles, because emitter was reactivated or reset
						entry->time = 0;
					}
				}
			}
		}

		// apply base values
		{
			// transforms
			auto baseTransforms = definition->baseNodes;
			assert( transformsCount == baseTransforms.size() );
			for( auto i = 0; i < transformsCount; ++i ) {
				auto current    = &transforms[i];
				const auto base = &baseTransforms[i];

				current->translation += base->translation;
				current->rotation += base->rotation;
				current->scale = multiplyComponents( current->scale, base->scale );
			}
		}
		{
			// visuals
			auto baseVisuals = definition->baseVisuals;
			assert( visualsCount == baseVisuals.size() );
			for( auto i = 0; i < visualsCount; ++i ) {
				auto current    = &visuals[i];
				const auto base = &baseVisuals[i];

				current->frame += base->frame;
			}
		}

		skeleton->dirty = true;
	}

	// calculate world transforms
	if( skeleton->dirty ) {
		skeleton->dirty      = false;
		auto transforms      = skeleton->transforms;
		auto worldTransforms = skeleton->worldTransforms;
		auto& rootTransform  = skeleton->rootTransform;

		auto count = skeleton->transforms.size();
		for( auto i = 0; i < count; ++i ) {
			auto transform = &transforms[i];
			auto world     = &worldTransforms[i];

			auto local = matrixTranslation( transform->length, 0, 0 )
			             * matrixScale( transform->scale ) * matrixRotation( transform->rotation )
			             * matrixTranslation( transform->translation );

			assert( transform->parent < i );
			if( transform->parent >= 0 ) {
				*world = local * worldTransforms[transform->parent];
			} else {
				*world = local * rootTransform;
			}
		}

		// emit particles
		FOR( state : skeleton->emitters ) {
			if( state.active ) {
				state.time -= dt;
				if( state.time <= 0 ) {
					// emit
					const auto emitter = &definition->emitters[state.assetIndex];
					if( emitter->interval > 0 ) {
						state.time += emitter->interval;
					} else {
						state.time = FLOAT32_MAX;
					}

					auto pos = transformVector3( worldTransforms[state.index], {} );
					emitParticles( particleSystem, pos, emitter->emitter );
				}
			}
		}
	}
}

void render( RenderCommands* renderer, const Skeleton* skeleton )
{
	assert( renderer );
	assert( skeleton );

#if GAME_RENDER_SKELETON_NODES
	// debug
	setTexture( renderer, 0, null );
	MESH_STREAM_BLOCK( stream, renderer ) {
		stream->color = Color::White;
		FOR( world : skeleton->worldTransforms ) {
			auto pos = transformVector3( world, {} );
			pushAabb( stream, AabbHalfSize( pos, 1, 1, 1 ) );
		}
	}
#endif

	auto worldTransforms = skeleton->worldTransforms;
	auto voxels          = skeleton->voxels;

	setRenderState( renderer, RenderStateType::BackFaceCulling, false );
	auto stack = renderer->matrixStack;
	pushMatrix( stack );
	FOR( visual : skeleton->visuals ) {
		if( visual.animation >= 0 ) {
			auto& world     = worldTransforms[visual.index];
			auto collection = voxels[visual.assetIndex];
			setTexture( renderer, 0, collection->texture );
			auto range = collection->animations[visual.animation].range;
			if( range ) {
				auto entry = &collection->frames[range.min + ( visual.frame % width( range ) )];
				currentMatrix( stack ) =
				    matrixTranslation( Vec3( -entry->offset.x, entry->offset.y, 0 ) ) * world;
				addRenderCommandMesh( renderer, entry->mesh );
			}
		}
	}
	popMatrix( stack );
	setRenderState( renderer, RenderStateType::BackFaceCulling, true );
}

void processSkeletonSystem( SkeletonSystem* system, ParticleSystem* particleSystem, float dt )
{
	assert( system );
	assert( particleSystem );

	FOR( skeleton : system->skeletons ) {
		update( &skeleton, particleSystem, dt );
	}
}

SkeletonDefinition loadSkeletonDefinitionAndAnimationIndices(
    StackAllocator* allocator, StringView filename, Array< const StringView > animationNames,
    Array< int8 > outAnimationIndices )
{
	assert( isValid( allocator ) );
	assert( animationNames.size() == outAnimationIndices.size() );

	SkeletonDefinition result = loadSkeletonDefinition( allocator, filename, 0 );

	for( auto i = 0, count = outAnimationIndices.size(); i < count; ++i ) {
		auto name = animationNames[i];
		outAnimationIndices[i] =
		    auto_truncate( find_index_if( result.animationIds, [name]( const auto& entry ) {
			                   return StringView{entry.name, entry.nameLength} == name;
			               } ).value );
	}

	return result;
}

SkeletonSystem makeSkeletonSystem( StackAllocator* allocator, int32 maxSkeletons,
                                   int32 maxDefinitions )
{
	SkeletonSystem result = {};
	result.skeletons      = makeUArray( allocator, Skeleton, maxSkeletons );
	result.definitions    = makeUArray( allocator, SkeletonDefinition, maxDefinitions );

	result.hero.definition  = result.definitions.emplace_back();
	*result.hero.definition = loadSkeletonDefinitionAndAnimationIndices(
	    allocator, "Data/voxels/hero_animations.json", makeArrayView( HeroAnimationNames ),
	    makeArrayView( (int8*)&result.hero.ids, sizeof( result.hero.ids ) / sizeof( int8 ) ) );
	return result;
}

Skeleton* addSkeleton( StackAllocator* allocator, SkeletonSystem* system,
                       const SkeletonDefinition& definition )
{
	Skeleton* result = nullptr;
	if( system->skeletons.remaining() ) {
		result  = system->skeletons.emplace_back();
		*result = makeSkeleton( allocator, definition );
	}
	return result;
}