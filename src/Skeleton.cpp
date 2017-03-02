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
                Array< SkeletonEmitterState >* emitters, Array< SkeletonHitboxState >* hitboxes,
                bool base )
{
	auto transformsAttr        = attr["transforms"].getObjectArray();
	const auto transformsCount = transformsAttr.size();
	auto readTransforms        = [base]( const JsonObject& attr, SkeletonTransform* dest ) {
		deserialize( attr["id"], dest->id, -1 );
		deserialize( attr["translation"], dest->translation );
		deserialize( attr["rotation"], dest->rotation );
		deserialize( attr["scale"], dest->scale );
		deserialize( attr["flashColor"], dest->flashColor, {} );
		if( base ) {
			deserialize( attr["length"], dest->length );
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

	auto collisionsArray = attr["collision_bounds"].getObjectArray();
	auto hitboxesArray   = attr["hitboxes"].getObjectArray();
	auto hurtboxesArray  = attr["hurtboxes"].getObjectArray();
	auto deflectsArray   = attr["deflect_bounds"].getObjectArray();
	*hitboxes            = makeArray( allocator, SkeletonHitboxState,
	                       collisionsArray.size() + hitboxesArray.size() + hurtboxesArray.size()
	                           + deflectsArray.size() );

	auto readHitbox = [base]( const JsonObject& attr, SkeletonHitboxState* dest ) {
		*dest = {};
		deserialize( attr["id"], dest->id, -1 );
		dest->index = -1;
		if( base ) {
			deserialize( attr["assetId"], dest->assetIndex, -1 );
		}
		deserialize( attr["active"], dest->active );
	};
	auto currentHitbox = 0;
	FOR( entry : collisionsArray ) {
		auto dest = &hitboxes->at( currentHitbox++ );
		readHitbox( entry, dest );
		dest->type = SkeletonHitboxState::Collision;
	}
	FOR( entry : hitboxesArray ) {
		auto dest = &hitboxes->at( currentHitbox++ );
		readHitbox( entry, dest );
		dest->type = SkeletonHitboxState::Hitbox;
	}
	FOR( entry : hurtboxesArray ) {
		auto dest = &hitboxes->at( currentHitbox++ );
		readHitbox( entry, dest );
		dest->type = SkeletonHitboxState::Hurtbox;
	}
	FOR( entry : deflectsArray ) {
		auto dest = &hitboxes->at( currentHitbox++ );
		readHitbox( entry, dest );
		dest->type = SkeletonHitboxState::Deflect;
	}
}

template < class T >
bool hasSameType( AnimatorAsset::Type type, const T& value )
{
	return type == T::AssetType;
}
bool hasSameType( AnimatorAsset::Type type, const SkeletonHitboxState& value )
{
	return type == value.type + AnimatorAsset::type_collision;
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
		    || !hasSameType( assetIds[assetIdentIndex].type, visuals ) ) {
			return {nodeId, false};
		}
		visuals.assetIndex = assetIds[assetIdentIndex].assetIndex;
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
	// allocator can't be GlobalScrap because this function uses GlobalScrap itself and out will be
	// wiped as a result at the end
	assert( allocator != GlobalScrap );

#define ABORT_ERROR( str, ... )                            \
	do {                                                   \
		LOG( ERROR, "{}: " str, filename, ##__VA_ARGS__ ); \
		__debugbreak();                                    \
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
			auto deflects          = assets["deflect_bounds"].getObjectArray();
			const auto assetsCount = voxels.size() + emitters.size() + collision_bounds.size()
			                         + hitboxes.size() + hurtboxes.size() + deflects.size();

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
			// hitboxes
			// hurtboxes
			// deflect_bounds

			auto deserializeHitboxes = []( const JsonObjectArray& array, Array< rectf > hitboxes,
			                               int32 currentHitbox, UArray< SkeletonAssetId >* idents,
			                               AnimatorAsset::Type type ) {
				for( auto i = 0, count = array.size(); i < count; ++i ) {
					auto attr  = array[i];
					auto dest  = &hitboxes[currentHitbox];
					auto ident = idents->emplace_back();
					deserialize( attr["id"], ident->id, -1 );
					ident->nameLength =
					    (int16)copyToString( attr["name"].getString(), ident->name );
					ident->assetIndex = auto_truncate( currentHitbox );
					ident->type       = type;

					deserialize( attr["bounds"], *dest );
					++currentHitbox;
				}
				return currentHitbox;
			};
			const auto hitboxesCount =
			    collision_bounds.size() + hitboxes.size() + hurtboxes.size() + deflects.size();
			out->hitboxes            = makeArray( allocator, rectf, hitboxesCount );
			auto currentHitbox       = 0;
			currentHitbox = deserializeHitboxes( collision_bounds, out->hitboxes, currentHitbox,
			                                     &assetIds, AnimatorAsset::type_collision );
			currentHitbox = deserializeHitboxes( hitboxes, out->hitboxes, currentHitbox, &assetIds,
			                                     AnimatorAsset::type_hitbox );
			currentHitbox = deserializeHitboxes( hurtboxes, out->hitboxes, currentHitbox, &assetIds,
			                                     AnimatorAsset::type_hurtbox );
			currentHitbox = deserializeHitboxes( deflects, out->hitboxes, currentHitbox, &assetIds,
			                                     AnimatorAsset::type_deflect );

			assert( assetIds.size() == assetsCount );
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
				           &out->baseHitboxes, true );

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

				// hitboxes
				bindIds = bindIdsToIndices( out->baseHitboxes, nodeIds, out->assetIds );
				if( !bindIds.second ) {
					ABORT_ERROR( "Invalid collision_bound/hitbox/hurtbox/deflect_bounds entry {}",
					             bindIds.first );
				}
				FOR( entry : out->baseHitboxes ) {
					entry.relative = out->hitboxes[entry.assetIndex];
				}
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
				           &animation->emitters, &animation->hitboxes, false );
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
				if( !applyPermutationByIds( animation->hitboxes, out->baseHitboxes ) ) {
					ABORT_ERROR( "Invalid hitboxes in animation \"{}\"",
					             StringView{ident->name, ident->nameLength} );
				}
				// copy base node values to relative transforms
				for( auto i = 0, count = animation->transforms.size(); i < count; ++i ) {
					auto dest    = &animation->transforms[i];
					auto src     = &out->baseNodes[i];
					dest->length = src->length;
					dest->parent = src->parent;
				}
				// copy assetIndex and index from bas
				auto copyFromBase = []( auto& entries, const auto& bases ) {
					assert( entries.size() == bases.size() );
					for( auto i = 0, count = entries.size(); i < count; ++i ) {
						auto dest = &entries[i];
						auto base = &bases[i];
						assert( dest->id == base->id );
						dest->assetIndex = base->assetIndex;
						dest->index      = base->index;
					}
				};
				copyFromBase( animation->visuals, out->baseVisuals );
				copyFromBase( animation->emitters, out->baseEmitters );
				for( auto i = 0, count = animation->hitboxes.size(); i < count; ++i ) {
					auto dest = &animation->hitboxes[i];
					auto base = &out->baseHitboxes[i];
					assert( dest->id == base->id );
					dest->relative   = base->relative;
					dest->assetIndex = base->assetIndex;
					dest->index      = base->index;
				}

				auto compare = []( const auto& a, const auto& b ) { return a.t < b.t; };
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

						// flashColor
						auto flashColor       = keyframesAttr["flashColor"].getObjectArray();
						const auto flashColorCount = flashColor.size();
						dest->flashColor =
						    makeArray( allocator, SkeletonKeyframe< Color >, flashColorCount );
						for( auto j = 0; j < flashColorCount; ++j ) {
							deserializeKeyframe( flashColor[j], &dest->flashColor[j] );
						}
						sort( dest->flashColor.begin(), dest->flashColor.end(), compare );

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
						float durations[6] = {};
						if( dest->translation.size() ) {
							durations[0] = dest->translation.back().t;
						}
						if( dest->rotation.size() ) {
							durations[1] = dest->rotation.back().t;
						}
						if( dest->scale.size() ) {
							durations[2] = dest->scale.back().t;
						}
						if( dest->flashColor.size() ) {
							durations[3] = dest->flashColor.back().t;
						}
						if( dest->frame.size() ) {
							durations[4] = dest->frame.back().t;
						}
						if( dest->active.size() ) {
							durations[5] = dest->active.back().t;
						}
						auto currentDuration = *max_element( begin( durations ), end( durations ) );
						animation->duration  = max( currentDuration, animation->duration );
						dest->hasKeyframes   = dest->translation.size() || dest->rotation.size()
						                     || dest->scale.size() || dest->flashColor.size()
						                     || dest->frame.size() || dest->active.size();
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

				// events
				auto events = attr["events"].getObjectArray();
				const auto eventsCount = events.size();
				animation->events = makeArray( allocator, SkeletonEvent, eventsCount );
				for( auto i = 0; i < eventsCount; ++i ) {
					auto event = events[i];
					auto dest  = &animation->events[i];

					deserialize( event["t"], dest->t );
					dest->type = auto_from_string( event["value"].getString() );
				}
				if( animation->events.size() ) {
					sort( animation->events.begin(), animation->events.end(), compare );
					auto eventsDuration = animation->events.back().t;
					animation->duration = max( animation->duration, eventsDuration );
				}
			}

		} else {
			ABORT_ERROR( "No animations defined" );
		}

		for( auto i = 0, count = out->nodeIds.size(); i < count; ++i ) {
			out->nodeIds[i].index = auto_truncate( i );
		}

		// remove trivial nodes
		// TODO: implement removal of trivial nodes even if animations are present
		if( !out->animations.size() ) {
			auto isTrivial = []( const SkeletonTransform& node ) {
				return node.parent >= 0 && !hasMagnitude( node.translation )
				       && !hasMagnitude( node.rotation ) && floatEq( node.scale.x, 1 )
				       && floatEq( node.scale.y, 1 ) && floatEq( node.scale.z, 1 )
				       && node.flashColor.bits == 0 && floatEqZero( node.length );
			};
			bool trivialFound = false;
			const auto count  = out->baseNodes.size();
			do {
				trivialFound = false;
				for( auto i = 0; i < count; ++i ) {
					auto& node = out->baseNodes[i];
					if( isTrivial( node ) ) {
						FOR( asset : out->baseVisuals ) {
							if( asset.index == i ) {
								asset.index  = node.parent;
								trivialFound = true;
							}
						}
						FOR( asset : out->baseEmitters ) {
							if( asset.index == i ) {
								asset.index  = node.parent;
								trivialFound = true;
							}
						}
						FOR( asset : out->baseHitboxes ) {
							if( asset.index == i ) {
								asset.index  = node.parent;
								trivialFound = true;
							}
						}
						FOR( id : out->nodeIds ) {
							if( id.index == i ) {
								id.index = node.parent;
							}
						}
						node.id = -1;
					}
				}
			} while( trivialFound );

			auto fixIndices = [out]( int16 index ) {
				FOR( asset : out->baseVisuals ) {
					if( asset.index > index ) {
						--asset.index;
					}
				}
				FOR( asset : out->baseEmitters ) {
					if( asset.index > index ) {
						--asset.index;
					}
				}
				FOR( asset : out->baseHitboxes ) {
					if( asset.index > index ) {
						--asset.index;
					}
				}
			};
			auto view = makeInitializedArrayView( out->baseNodes.data(), out->baseNodes.size() );
			erase_if( view, [&]( const auto& entry ) {
				if( entry.id < 0 ) {
					fixIndices( (int16)indexof( view, entry ) );
					return true;
				}
				return false;
			} );
			out->baseNodes = makeArrayView( view );
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
	result.id                 = -1;
	auto prevSize             = allocator->size;
	if( loadSkeletonDefinitionImpl( allocator, filename, &result ) ) {
		result.id          = id;
		result.sizeInBytes = auto_truncate( allocator->size - prevSize );

		LOG( INFORMATION, "{}: Success loading skeleton definition", filename );
	} else {
		LOG( ERROR, "{}: Failed to load skeleton definition", filename );
	}
	return result;
}

Skeleton* makeSkeleton( const SkeletonDefinition& definition )
{
	assert( definition );

	auto allocator = DynamicStackAllocator{safe_truncate< size_t >(
	    definition.sizeInBytes + sizeof( Skeleton ) + alignof( Skeleton ) )};

	Skeleton* result                = allocateStruct( &allocator, Skeleton );
	*result                         = {};
	result->rootTransform.transform = matrixIdentity();
	const auto nodesCount           = definition.baseNodes.size();
	result->transforms              = makeArray( &allocator, SkeletonTransform, nodesCount );
	result->transforms.assign( definition.baseNodes );

	result->worldTransforms = makeArray( &allocator, SkeletonWorldTransform, nodesCount );

	result->visuals = makeArray( &allocator, SkeletonVoxelVisuals, definition.baseVisuals.size() );
	result->visuals.assign( definition.baseVisuals );

	result->emitters =
	    makeArray( &allocator, SkeletonEmitterState, definition.baseEmitters.size() );
	result->emitters.assign( definition.baseEmitters );

	result->hitboxes = makeArray( &allocator, SkeletonHitboxState, definition.baseHitboxes.size() );
	result->hitboxes.assign( definition.baseHitboxes );

	const auto voxelsCount = definition.voxels.size();
	result->voxels         = makeArray( &allocator, const VoxelCollection*, voxelsCount );
	for( auto i = 0; i < voxelsCount; ++i ) {
		result->voxels[i] = &definition.voxels[i];
	}

	if( definition.animations.size() ) {
		auto makeAnimations = [](
		    StackAllocator* allocator, UArray< SkeletonAnimationState >& animations,
		    Array< Array< SkeletonKeyframesState > >& statesPool, int32 nodesCount ) {
			const int32 MaxAnimationStates = 4;
			animations = makeUArray( allocator, SkeletonAnimationState, MaxAnimationStates );
			statesPool =
			    makeArray( allocator, Array< SkeletonKeyframesState >, MaxAnimationStates );
			FOR( states : statesPool ) {
				states = makeArray( allocator, SkeletonKeyframesState, nodesCount );
			}
		};
		makeAnimations( &allocator, result->animations, result->keyframeStatesPool, nodesCount );
		makeAnimations( &allocator, result->prevAnimations, result->prevKeyframeStatesPool,
		                nodesCount );
	}
	result->definition   = &definition;
	result->definitionId = definition.id;
	result->dirty        = true;

	result->base = allocator.ptr;
	allocator.fitToSize();
	allocator.releaseOwnership();

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
		auto result           = skeleton->animations.size();
		auto state            = skeleton->animations.emplace_back();
		*state                = {};
		state->animationIndex = auto_truncate( animationIndex );
		state->flags          = SkeletonAnimationState::Playing;
		setFlagCond( state->flags, SkeletonAnimationState::Repeating, repeating );
		state->prevFrame      = 0;
		state->currentFrame   = 0;
		state->keyframeStates = skeleton->keyframeStatesPool[result];
		fill( state->keyframeStates.data(), {-1, -1, -1, -1, -1}, state->keyframeStates.size() );
		state->prevEvent = -1;
		state->event     = -1;
		skeleton->dirty  = true;
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
bool isAnimationFinished( Skeleton* skeleton, int32 animationIndex )
{
	assert( skeleton );
	assert( skeleton->definition );
	auto state     = &skeleton->animations[animationIndex];
	auto animation = &skeleton->definition->animations[state->animationIndex];
	return state->currentFrame >= animation->duration;
}

Ring< const SkeletonEvent > getAnimationEvents( Skeleton* skeleton, int32 animationIndex )
{
	assert( skeleton );
	assert( skeleton->definition );
	auto state           = &skeleton->animations[animationIndex];
	const auto animation = &skeleton->definition->animations[state->animationIndex];

	auto prevFrame    = state->prevFrame;
	auto currentFrame = state->currentFrame;

	bool full  = false;
	auto first = state->prevEvent;
	auto last  = state->event + 1;
	if( first < 0 ) {
		full  = false;
		first = 0;
	}
	if( last < 0 ) {
		last = 0;
	}

	auto result = makeRingView< const SkeletonEvent >(
	    animation->events.data(), animation->events.size(), first, last, full );
	while( !result.empty() && result.front().t < prevFrame ) {
		result.pop_front();
	}
	while( !result.empty() && result.back().t >= currentFrame ) {
		result.pop_back();
	}
	return result;
}

void update( Skeleton* skeleton, ParticleSystem* particleSystem, float dt )
{
	assert( skeleton );
	assert( skeleton->transforms.size() == skeleton->worldTransforms.size() );
	assert( skeleton->definition );

	// dt >= 0 means we are calculating a future bone position
	// dt < 0 means we are interpolating between previous bone position and current (used when
	// rendering, rendering is behind and catches up). Negative, because dt is relative to current
	// frame time
	auto animationStates = ( dt >= 0 ) ? ( skeleton->animations ) : ( skeleton->prevAnimations );
	auto definition      = skeleton->definition;
	if( dt < 0 ) {
		dt = 1 + dt;
	}
	if( animationStates.size() ) {
		// apply animations base values (only toplevel animation)
		auto animations     = definition->animations;
		auto transforms     = skeleton->transforms;
		auto visuals        = skeleton->visuals;
		auto emitters       = skeleton->emitters;
		auto hitboxes       = skeleton->hitboxes;

		const auto transformsCount = transforms.size();
		const auto visualsCount    = visuals.size();
		const auto emittersCount   = emitters.size();
		const auto hitboxesCount   = hitboxes.size();

		// TODO: actually apply all animation base values, not just the toplevel one
		const auto toplevel = &animations[animationStates[0].animationIndex];
		transforms.assign( toplevel->transforms );
		visuals.assign( toplevel->visuals );
		// emitters can't be assigned easily because timer isn't allowed to be reset here
		for( auto i = 0; i < emittersCount; ++i ) {
			emitters[i].active = toplevel->emitters[i].active;
		}
		hitboxes.assign( toplevel->hitboxes );

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
		auto processColor = [findCurrentKeyframe, interpolateKeyframeData](
		    Array< BezierForwardDifferencerData > curves, float currentFrame, const auto& keyframes,
		    int16* index, auto* data ) {
			typeof( keyframes )::value_type::value_type def = {};
			*index = findCurrentKeyframe( currentFrame, keyframes, *index );
			*data  = interpolateKeyframeData( curves, currentFrame, keyframes, *index, def );
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

		if( dt > 0 ) {
			FOR( state : animationStates ) {
				if( !( state.flags & SkeletonAnimationState::Playing ) ) {
					continue;
				}
				auto animation  = &animations[state.animationIndex];
				auto keyframes  = animation->keyframes;
				auto curves     = animation->curves;
				state.prevFrame = state.currentFrame;
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
							*keyframeState = {-1, -1, -1, -1, -1};
						}
						processKeyframe( curves, currentFrame, currentKeyframes->translation,
						                 &keyframeState->translation, &transform->translation );
						processKeyframe( curves, currentFrame, currentKeyframes->rotation,
						                 &keyframeState->rotation, &transform->rotation );
						processScale( curves, currentFrame, currentKeyframes->scale,
						              &keyframeState->scale, &transform->scale );
						processColor( curves, currentFrame, currentKeyframes->flashColor,
						              &keyframeState->flashColor, &transform->flashColor );
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
						auto activated   = ( !wasActive && entry->active );
						auto reactivated = prev != keyframeState->custom && wasActive && entry->active;
						if( activated || reactivated || reset ) {
							// set time to 0 to emit particles immediately
							entry->time = 0;
						}
					}
				}
				for( auto i = 0; i < hitboxesCount; ++i ) {
					auto entry            = &hitboxes[i];
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
						auto activated   = ( !wasActive && entry->active );
						auto reactivated = prev != keyframeState->custom && wasActive && entry->active;
						if( activated || reactivated || reset ) {
							// TODO: reactivation of hitboxes
						}
					}
				}

				// events
				state.prevEvent = state.event;
				if( reset ) {
					state.event = -1;
				}
				state.event = findCurrentKeyframe( currentFrame, animation->events, state.event );
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
		auto rootTransform   = skeleton->rootTransform;
		if( skeleton->mirrored ) {
			rootTransform.transform = matrixScale( -1, 1, 1 ) * rootTransform.transform;
		}

		auto count = skeleton->transforms.size();
		for( auto i = 0; i < count; ++i ) {
			auto transform = &transforms[i];
			auto world     = &worldTransforms[i];

			auto local = matrixTranslation( transform->length, 0, 0 )
			             * matrixScale( transform->scale ) * matrixRotation( transform->rotation )
			             * matrixTranslation( transform->translation );

			assert( transform->parent < i );
			if( transform->parent >= 0 ) {
				auto parent       = &worldTransforms[transform->parent];
				world->transform  = local * parent->transform;
				world->flashColor = transform->flashColor.bits | parent->flashColor;
			} else {
				world->transform  = local * rootTransform.transform;
				world->flashColor = transform->flashColor.bits | rootTransform.flashColor.bits;
			}
		}

		// emit particles
		if( particleSystem ) {
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

						auto pos = transformVector3( worldTransforms[state.index].transform, {} );
						pos.y    = -pos.y;
						emitParticles( particleSystem, pos, emitter->emitter );
					}
				}
			}
		}

		// calculate hitbox origins
		FOR( entry : skeleton->hitboxes ) {
			entry.origin = transformVector3( worldTransforms[entry.index].transform, {} );
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
			auto world      = &worldTransforms[visual.index];
			auto collection = voxels[visual.assetIndex];
			setTexture( renderer, 0, collection->texture );
			auto range = collection->animations[visual.animation].range;
			if( range ) {
				auto entry = &collection->frames[range.min + ( visual.frame % width( range ) )];
				currentMatrix( stack ) =
				    matrixTranslation( Vec3( -entry->offset.x, entry->offset.y, 0 ) )
				    * world->transform;
				renderer->flashColor = world->flashColor;
				addRenderCommandMesh( renderer, entry->mesh );
			}
		}
	}
	renderer->flashColor = 0;
	popMatrix( stack );
	setRenderState( renderer, RenderStateType::BackFaceCulling, true );
}

void advanceSkeletons( SkeletonSystem* system, float dt )
{
	FOR( skeleton : system->skeletons ) {
		skeleton->prevAnimations.assign( skeleton->animations );
		for( auto i = 0, count = skeleton->animations.size(); i < count; ++i ) {
			auto prev            = &skeleton->prevAnimations[i];
			auto cur             = &skeleton->animations[i];
			prev->keyframeStates = skeleton->prevKeyframeStatesPool[i];
			prev->keyframeStates.assign( cur->keyframeStates );
		}
		update( skeleton, nullptr, dt );
	}
}
void processSkeletonSystem( SkeletonSystem* system, ParticleSystem* particleSystem, float dt )
{
	assert( system );
	assert( particleSystem );

	FOR( skeleton : system->skeletons ) {
		update( skeleton, particleSystem, dt );
	}
}

NullableInt32 getHitboxIndex( SkeletonDefinition* definition, StringView name,
                              SkeletonHitboxState::Type type )
{
	assert( definition );
	const auto count = definition->baseHitboxes.size();
	FOR( id : definition->nodeIds ) {
		if( id.getName() == name ) {
			for( auto i = 0; i < count; ++i ) {
				auto hitbox = &definition->baseHitboxes[i];
				if( hitbox->index == id.index && hitbox->type == type ) {
					return {i};
				}
			}
		}
	}
	return NullableInt32::makeNull();
}
struct LoadSkeletonDefinitionNamesOutputPair {
	Array< const StringView > names;
	Array< int8 > indices;
};
struct LoadSkeletonDefinitionIndicesOutput {
	LoadSkeletonDefinitionNamesOutputPair animations;
	LoadSkeletonDefinitionNamesOutputPair nodes;
	LoadSkeletonDefinitionNamesOutputPair collisions;
	LoadSkeletonDefinitionNamesOutputPair hitboxes;
	LoadSkeletonDefinitionNamesOutputPair hurtboxes;
	LoadSkeletonDefinitionNamesOutputPair deflects;

	EntitySkeletonTraits traits;
};
SkeletonDefinition loadSkeletonDefinitionAndIndices( StackAllocator* allocator, StringView filename,
                                                     LoadSkeletonDefinitionIndicesOutput& out )
{
	assert( isValid( allocator ) );
	assert( out.animations.names.size() == out.animations.indices.size() );
	assert( out.nodes.names.size() == out.nodes.indices.size() );
	assert( out.collisions.names.size() == out.collisions.indices.size() );
	assert( out.hitboxes.names.size() == out.hitboxes.indices.size() );
	assert( out.hurtboxes.names.size() == out.hurtboxes.indices.size() );
	assert( out.deflects.names.size() == out.deflects.indices.size() );

	SkeletonDefinition result = loadSkeletonDefinition( allocator, filename, 0 );
	if( result ) {
		for( auto i = 0, count = out.animations.indices.size(); i < count; ++i ) {
			auto name  = out.animations.names[i];
			auto index = find_index_if( result.animationIds, [name]( const auto& entry ) {
				return entry.getName() == name;
			} );
			out.animations.indices[i] = auto_truncate( index.value );
		}
		for( auto i = 0, count = out.nodes.indices.size(); i < count; ++i ) {
			auto name   = out.nodes.names[i];
			int8 index = -1;
			auto id     = find_if( result.nodeIds,
			                   [name]( const auto& entry ) { return entry.getName() == name; } );
			if( id != result.animationIds.end() ) {
				index = auto_truncate( id->index );
			}
			out.nodes.indices[i] = index;
		}
		for( auto i = 0, count = out.collisions.indices.size(); i < count; ++i ) {
			auto name  = out.collisions.names[i];
			auto index = getHitboxIndex( &result, name, SkeletonHitboxState::Collision );
			out.collisions.indices[i] = auto_truncate( index.value );
		}
		for( auto i = 0, count = out.hitboxes.indices.size(); i < count; ++i ) {
			auto name               = out.hitboxes.names[i];
			auto index              = getHitboxIndex( &result, name, SkeletonHitboxState::Hitbox );
			out.hitboxes.indices[i] = auto_truncate( index.value );
		}
		for( auto i = 0, count = out.hurtboxes.indices.size(); i < count; ++i ) {
			auto name  = out.hurtboxes.names[i];
			auto index = getHitboxIndex( &result, name, SkeletonHitboxState::Hurtbox );
			out.hurtboxes.indices[i] = auto_truncate( index.value );
		}
		for( auto i = 0, count = out.deflects.indices.size(); i < count; ++i ) {
			auto name               = out.deflects.names[i];
			auto index              = getHitboxIndex( &result, name, SkeletonHitboxState::Deflect );
			out.deflects.indices[i] = auto_truncate( index.value );
		}

		auto traits      = &out.traits;
		auto copyIndices = [filename]( Array< const int8 > ids, Array< int8 > dest ) {
			auto count = ids.size();
			if( count > dest.size() ) {
				LOG( ERROR, "{}: exceeds max hitbox count of {}, truncating", filename,
				     dest.size() );
				count = dest.size();
			}
			if( count ) {
				copy( dest.data(), ids.data(), count );
			}
			return safe_truncate< int8 >( count );
		};
		static_assert(
		    countof( EntitySkeletonTraits::hitboxesCounts ) == SkeletonHitboxState::Count,
		    "hitboxesCount has wrong size" );
		traits->hitboxesCounts[SkeletonHitboxState::Collision] =
		    copyIndices( out.collisions.indices, makeArrayView( traits->collisionIdsData ) );
		traits->hitboxesCounts[SkeletonHitboxState::Hitbox] =
		    copyIndices( out.hitboxes.indices, makeArrayView( traits->hitboxIdsData ) );
		traits->hitboxesCounts[SkeletonHitboxState::Hurtbox] =
		    copyIndices( out.hurtboxes.indices, makeArrayView( traits->hurtboxIdsData ) );
		traits->hitboxesCounts[SkeletonHitboxState::Deflect] =
		    copyIndices( out.deflects.indices, makeArrayView( traits->deflectIdsData ) );
	}

	return result;
}
template < class T >
bool loadTypedSkeletonDefinitionAndTraits( SkeletonSystem* system, StringView filename,
                                           T* typedDefinition, EntitySkeletonTraits* traits )
{
	if( system->definitions.remaining() ) {
		auto definition  = system->definitions.emplace_back();
		*typedDefinition = {};
		*traits          = {};

		LoadSkeletonDefinitionIndicesOutput inout = {
		    {T::AnimationNames, typedDefinition->getAnimationIds()},
		    {T::NodeNames, typedDefinition->getNodeIds()},
		    {T::CollisionNames, typedDefinition->getCollisionIds()},
		    {T::HitboxNames, typedDefinition->getHitboxIds()},
		    {T::HurtboxNames, typedDefinition->getHurtboxIds()},
		    {T::DeflectNames, typedDefinition->getDeflectIds()},
		};
		auto allocator = DynamicStackAllocator{megabytes( 1 )};
		*definition    = loadSkeletonDefinitionAndIndices( &allocator, filename, inout );
		if( *definition ) {
			typedDefinition->definition = definition;
			*traits                     = inout.traits;
			traits->definition          = definition;
			allocator.fitToSize();
			allocator.releaseOwnership();
			return true;
		}
		system->definitions.pop_back();
	}
	return false;
}

SkeletonSystem makeSkeletonSystem()
{
	SkeletonSystem result = {};
	const int32 SkeletonCount   = 20;
	const int32 DefinitionCount = 5;
	result.skeletons =
	    makeUninitializedArrayView( allocate< Skeleton* >( SkeletonCount ), SkeletonCount );
	result.definitions = makeUninitializedArrayView(
	    allocate< SkeletonDefinition >( DefinitionCount ), DefinitionCount );

	loadTypedSkeletonDefinitionAndTraits( &result,
	                                      getEntitySkeletonDefinitionFilename( Entity::type_hero ),
	                                      &result.hero, &result.skeletonTraits[Entity::type_hero] );
	loadTypedSkeletonDefinitionAndTraits(
	    &result, getEntitySkeletonDefinitionFilename( Entity::type_wheels ), &result.wheels,
	    &result.skeletonTraits[Entity::type_wheels] );

	return result;
}

Skeleton* addSkeleton( SkeletonSystem* system, const SkeletonDefinition& definition )
{
	auto findEmptySkeleton = []( SkeletonSystem* system ) -> Skeleton** {};
	if( !system->skeletons.remaining() ) {
		const auto newSize = system->skeletons.capacity() * 2;
		system->skeletons  = makeInitializedArrayView(
		    reallocate( system->skeletons.data(), newSize, system->skeletons.capacity() ),
		    system->skeletons.size(), newSize );
	}
	if( system->skeletons.remaining() ) {
		auto entry = system->skeletons.emplace_back();
		*entry     = makeSkeleton( definition );
		return *entry;
	}
	return nullptr;
}

void deleteSkeleton( SkeletonSystem* system, Skeleton* skeleton )
{
	assert( system );
	assert( skeleton );

	assert( GlobalPlatformServices );
	auto base = skeleton->base;
	*skeleton = {};
	GlobalPlatformServices->free( base );
	unordered_remove( system->skeletons, skeleton );
}

pair< rectf, bool > getHitboxRelative( Skeleton* skeleton, int32 index )
{
	assert( skeleton );
	assert( !skeleton->dirty );

	auto rootOrigin = transformVector3( skeleton->rootTransform.transform, {} );
	auto hitbox     = &skeleton->hitboxes[index];
	auto origin     = hitbox->origin.xy - rootOrigin.xy;
	if( skeleton->mirrored ) {
		origin = -origin;
	}
	pair< rectf, bool > result;
	result.first        = translate( hitbox->relative, origin );
	result.first.top    = -result.first.top;
	result.first.bottom = -result.first.bottom;

	result.second = (bool)hitbox->active;
	return result;
}
pair< rectf, bool > getHitboxAbsolute( Skeleton* skeleton, int32 index )
{
	assert( skeleton );
	assert( !skeleton->dirty );

	auto hitbox = &skeleton->hitboxes[index];
	pair< rectf, bool > result;
	result.first        = translate( hitbox->relative, hitbox->origin.xy );
	result.first.top    = -result.first.top;
	result.first.bottom = -result.first.bottom;

	result.second       = (bool)hitbox->active;
	return result;
}

void setTransform( Skeleton* skeleton, mat4arg transform )
{
	skeleton->rootTransform.transform = transform;
	skeleton->dirty                   = true;
}
void setMirrored( Skeleton* skeleton, bool mirrored )
{
	skeleton->dirty    = skeleton->mirrored != mirrored;
	skeleton->mirrored = mirrored;
}

NullableInt32 getNodeIndex( Skeleton* skeleton, StringView name )
{
	assert( skeleton );
	assert( skeleton->definition );
	auto definition = skeleton->definition;
	return find_index_if( definition->nodeIds,
	                      [name]( const auto& entry ) { return entry.getName() == name; } );
}
vec3 getNode( Skeleton* skeleton, int32 index )
{
	return transformVector3( skeleton->worldTransforms[index].transform, {} );
}

Entity* addEntity( EntitySystem* entitySystem, SkeletonSystem* skeletonSystem, EntityHandle handle,
                   Entity::Type type, vec2arg position /*= {}*/ )
{
	assert( entitySystem );
	assert( handle );
	Entity* result = nullptr;
	if( entitySystem->entries.remaining() ) {
		auto traits         = getEntityTraits( type );

		if( traits->flags.dynamic ) {
			result  = entitySystem->entries.emplace_back();
			*result = {};
		} else {
			result = entitySystem->entries.insert(
			    entitySystem->entries.begin() + entitySystem->entriesCount, 1, {} );
			++entitySystem->entriesCount;
		}
		result->position                = position;
		result->type                    = type;
		result->handle                  = handle;
		result->flags.dynamic           = traits->flags.dynamic;
		result->gravityModifier         = traits->init.gravityModifier;
		result->bounceModifier          = traits->init.bounceModifier;
		result->airFrictionCoeffictient = traits->init.airFrictionCoeffictient;
		result->team                    = traits->team;

		auto skeletonTraits = getSkeletonTraits( skeletonSystem, type );
		if( skeletonTraits && skeletonTraits->definition && *skeletonTraits->definition ) {
			result->skeleton = addSkeleton( skeletonSystem, *skeletonTraits->definition );
			assert( result->skeleton );
			update( result->skeleton, nullptr, 0 );
		}

		switch( type ) {
			case Entity::type_hero: {
				result->hero.stats                 = traits->stats;
				result->hero.currentAnimationIndex = -1;
				result->hero.currentAnimation      = -1;
				break;
			}
			case Entity::type_wheels: {
				result->wheels.stats            = traits->stats;
				result->wheels.currentAnimation = -1;
				break;
			}
		}
	}
	return result;
}