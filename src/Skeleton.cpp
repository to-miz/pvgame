SkeletonDefinition loadSkeletonDefinition( StackAllocator* allocator, StringView filename,
                                           int16 id )
{
	SkeletonDefinition result = {};
	result.id                 = -1;
	auto guard = StackAllocatorGuard( allocator );
	bool success = true;

	struct Node {
		int16 parentId;
		int16 parent;
		int16 childrenCount;
		int16 assetId;
		int16 id;
	};
	struct VoxelAsset {
		int16 index;
		StringView filename;
	};

	auto scrap                = GlobalScrap;
	TEMPORARY_MEMORY_BLOCK( scrap ) {
		auto file = readFile( scrap, filename );
		auto doc  = makeJsonDocument( scrap, file );
		if( !doc ) {
			LOG( ERROR, "{}: Failed to load skeleton definition", filename );
			success = false;
			break;
		}
		auto root = doc.root.getObject();
		if( !root ) {
			LOG( ERROR, "{}: Root of json not an object", filename );
			success = false;
			break;
		}

		if( auto assets = root["assets"].getObjectArray() ) {
			const auto count = assets.size();
			result.assetIds  = makeArray( allocator, SkeletonAssetId, count );
			auto infos       = makeUArray( scrap, VoxelAsset, count );
			for( auto i = 0; i < count; ++i ) {
				auto attr    = assets[i];
				auto assetId = &result.assetIds[i];

				assetId->type = auto_from_string( attr["type"].getString() );
				if( assetId->type == AnimatorAsset::type_collection ) {
					auto info      = infos.emplace_back();
					info->index    = auto_truncate( i );
					info->filename = attr["filename"].getString();
				}
				deserialize( attr["id"], assetId->id, -1 );
				assetId->nameLength = auto_truncate( copyToString(
				    attr["name"].getString(), assetId->name, countof( assetId->name ) ) );
			}
		}

		if( auto nodes = root["nodes"].getObjectArray() ) {
			const auto count = nodes.size();
			result.baseNodes = makeArray( allocator, SkeletonTransform, count );
			result.nodeIds   = makeArray( allocator, SkeletonId, count );
			auto baseVisuals = makeUArray( allocator, SkeletonVisuals, count );

			TEMPORARY_MEMORY_BLOCK( scrap ) {
				auto graph = makeArray( scrap, Node, count );
				for( auto i = 0; i < count; ++i ) {
					auto attr      = nodes[i];
					auto current   = &result.baseNodes[i];
					auto ident     = &result.nodeIds[i];
					auto graphNode = &graph[i];
					*graphNode = {};

					deserialize( attr["translation"], current->translation );
					deserialize( attr["rotation"], current->rotation );
					deserialize( attr["scale"], current->scale );
					deserialize( attr["id"], current->id );
					deserialize( attr["length"], current->length );
					current->parent = -1;

					graphNode->parent = -1;
					deserialize( attr["parentId"], graphNode->parentId, -1 );
					deserialize( attr["assetId"], graphNode->assetId, -1 );

					ident->id         = current->id;
					graphNode->id     = current->id;
					ident->nameLength = auto_truncate( copyToString(
					    attr["name"].getString(), ident->name, countof( ident->name ) ) );

					if( graphNode->assetId >= 0 ) {
						auto assetId = graphNode->assetId;
						if( auto info = find_first_where( result.assetIds, entry.id == assetId ) ) {
							if( info->type == AnimatorAsset::type_collection ) {
								auto visuals        = baseVisuals.emplace_back();
								visuals->type       = SkeletonVisualType::Voxel;
								visuals->voxelIndex = -1;
								visuals->index      = auto_truncate( i );
								auto voxel          = attr["voxel"].getObject();
								deserialize( voxel["animation"], visuals->voxel.animation );
								deserialize( voxel["frame"], visuals->voxel.frame );
							}
						}
					}
				}

				fitToSize( allocator, &baseVisuals );
				result.baseVisuals = makeArrayView( baseVisuals );

				// bind parents / build graph
				if( graph.size() >= SkeletonMaxParents ) {
					LOG( ERROR, "{}: Node count can't exceed {}", filename, SkeletonMaxParents );
					success = false;
					break;
				}
				FOR( entry : graph ) {
					if( entry.parentId >= 0 ) {
						auto parentId = entry.parentId;
						entry.parent =
						    auto_truncate( find_index_if( graph, [parentId]( const auto& entry ) {
							                   return entry.id == parentId;
							               } ).value );
						if( entry.parent < 0 ) {
							LOG( ERROR, "{}: Parent not found of node {}", filename, entry.id );
							success = false;
							break;
						}

						int16 childrenCount = entry.childrenCount + 1;
						auto parent         = &graph[entry.parent];
						bounded_while( parent, SkeletonMaxParents ) {
							parent->childrenCount += childrenCount;
							parent = &graph[parent->parent];
						}
					}
				}
			}

			// count visuals
		} else {
			LOG( ERROR, "{}: No nodes defined", filename );
			success = false;
			break;
		}

		if( success ) {
			LOG( INFORMATION, "{}: Loaded skeleton definition", filename );
			result.id = id;
		}
	}

	if( result && success ) {
		guard.commit();
	} else {
		result.id = -1;
	}
	return result;
}

void update( Skeleton* skeleton )
{
	assert( skeleton );
	assert( skeleton->transforms.size() == skeleton->worldTransforms.size() );

	// calculate world transforms
	if( skeleton->dirty ) {
		skeleton->dirty = false;
		auto transforms      = skeleton->transforms;
		auto worldTransforms = skeleton->worldTransforms;

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
				*world = local;
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

	auto stack = renderer->matrixStack;
	pushMatrix( stack );
	FOR( visual : skeleton->visuals ) {
		switch( visual.type ) {
			case SkeletonVisualType::Voxel: {
				if( visual.voxel.animation >= 0 ) {
					auto& world     = worldTransforms[visual.index];
					auto collection = voxels[visual.voxelIndex];
					setTexture( renderer, 0, collection->texture );
					auto range = collection->animations[visual.voxel.animation].range;
					if( range ) {
						auto entry =
						    &collection
						         ->frames[range.min + ( visual.voxel.frame % width( range ) )];
						currentMatrix( stack ) =
						    matrixTranslation( Vec3( -entry->offset.x, entry->offset.y, 0 ) )
						    * world;
						addRenderCommandMesh( renderer, entry->mesh );
					}
				}
				break;
			}
			InvalidDefaultCase;
		}
	}
	popMatrix( stack );
}
