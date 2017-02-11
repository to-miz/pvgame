SkeletonDefinition loadSkeletonDefinition( StackAllocator* allocator, StringView filename,
                                           int16 id )
{
	return {};
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
		if( visual.animation >= 0 ) {
			auto& world     = worldTransforms[visual.index];
			auto collection = voxels[visual.voxelIndex];
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
}
