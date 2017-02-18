struct VoxelCollection {
	struct Frame {
		MeshId mesh;
		vec2 offset;
	};

	struct FrameInfo {
		VoxelGridTextureMap textureMap;
		recti textureRegion[VF_Count];
		float frictionCoefficient;
		aabb bounds;
		// TODO: include more meta information like gun position
	};

	struct Animation {
		string name;
		rangeu16 range;
	};

	TextureId texture;
	Array< Frame > frames;
	Array< FrameInfo > frameInfos;
	Array< Animation > animations;
	string filename;        // filename of the voxel collection json
	string voxelsFilename;  // filename of the voxel grids file .raw
};

rangeu16 getAnimationRange( VoxelCollection* collection, StringView name )
{
	rangeu16 result;
	if( auto animation = find_first_where( collection->animations, entry.name == name ) ) {
		result = animation->range;
	} else {
		result = {};
	}
	return result;
}
Array< VoxelCollection::Frame > getAnimationFrames( VoxelCollection* collection, rangeu16 range )
{
	return makeRangeView( collection->frames, range );
}
Array< VoxelCollection::Frame > getAnimationFrames( VoxelCollection* collection, StringView name )
{
	return makeRangeView( collection->frames, getAnimationRange( collection, name ) );
}

bool loadVoxelCollectionTextureMapping( StackAllocator* allocator, StringView filename,
                                        VoxelCollection* out )
{
	assert( out );

	auto guard = StackAllocatorGuard( allocator );
	auto primary = allocator;
	auto scrap   = GlobalScrap;
	TEMPORARY_MEMORY_BLOCK( scrap ) {
		auto file = readFile( scrap, filename );
		auto doc  = makeJsonDocument( scrap, file );
		if( !doc || !doc.root.getObject() ) {
			return false;
		}
		auto root = doc.root.getObject();

		*out         = {};
		out->texture = GlobalPlatformServices->loadTexture( root["texture"].getString() );
		if( !out->texture ) {
			return false;
		}
		auto textureInfo = getTextureInfo( out->texture );
		auto itw         = 1.0f / textureInfo->width;
		auto ith         = 1.0f / textureInfo->height;

		auto mapping      = root["mapping"].getArray();
		int32 framesCount = 0;
		FOR( animationVal : mapping ) {
			auto animation = animationVal.getObject();
			framesCount += animation["frames"].getArray().size();
		}

		out->frames     = makeArray( primary, VoxelCollection::Frame, framesCount );
		out->frameInfos = makeArray( primary, VoxelCollection::FrameInfo, framesCount );
		out->animations = makeArray( primary, VoxelCollection::Animation, mapping.size() );

		int32 currentFrame = 0;
		for( int32 i = 0, count = mapping.size(); i < count; ++i ) {
			auto animation  = mapping[i].getObject();
			auto dest       = &out->animations[i];
			dest->name      = makeString( primary, animation["name"].getString() );
			dest->range.min = safe_truncate< uint16 >( currentFrame );

			FOR( frameVal : animation["frames"].getArray() ) {
				auto frame     = frameVal.getObject();
				auto destFrame = &out->frames[currentFrame];
				auto destInfo  = &out->frameInfos[currentFrame];
				++currentFrame;

				*destFrame                    = {};
				*destInfo                     = {};
				destInfo->frictionCoefficient = 1;

				for( auto face = 0; face < VF_Count; ++face ) {
					auto faceObject = frame[VoxelFaceStrings[face]].getObject();

					destInfo->textureMap.texture = out->texture;
					deserialize( faceObject["rect"], destInfo->textureRegion[face] );
					deserialize( faceObject["texCoords"],
					           destInfo->textureMap.entries[face].texCoords );
					destInfo->frictionCoefficient = faceObject["frictionCoefficient"].getFloat( 1 );
					FOR( vert : destInfo->textureMap.entries[face].texCoords.elements ) {
						vert.x *= itw;
						vert.y *= ith;
					}
				}
				deserialize( frame["offset"], destFrame->offset );
			}

			dest->range.max = safe_truncate< uint16 >( currentFrame );
		}
		out->voxelsFilename = makeString( primary, root["voxels"].getString() );
	}
	out->filename = makeString( primary, filename );
	guard.commit();
	return true;
}
bool loadVoxelGridsFromFile( StringView filename, Array< VoxelGrid > grids )
{
	auto bytesToRead = grids.size() * sizeof( VoxelGrid );
	if( !bytesToRead ) {
		return false;
	}
	if( GlobalPlatformServices->readFileToBuffer( filename, grids.data(), bytesToRead )
	    != bytesToRead ) {
		return false;
	}
	return true;
}
aabb getBoundsFromVoxelGrid( const VoxelGrid* grid )
{
	int32 left   = 10000;
	int32 bottom = 10000;
	int32 near   = 10000;
	int32 right  = -10000;
	int32 top    = -10000;
	int32 far    = -10000;

	for( auto z = 0, depth = grid->depth; z < depth; ++z ) {
		for( auto y = 0, height = grid->height; y < height; ++y ) {
			for( auto x = 0, width = grid->width; x < width; ++x ) {
				auto index = x + y * width + z * width * height;
				auto cell  = grid->data[index];
				if( cell != EmptyCell ) {
					left   = min( left, x );
					bottom = min( bottom, height - y );
					near   = min( near, z );
					right  = max( right, x + 1 );
					top    = max( top, height - y + 1 );
					far    = max( far, z + 1 );
				}
			}
		}
	}
	return {left * CELL_WIDTH,  bottom * CELL_HEIGHT, near * CELL_DEPTH,
	        right * CELL_WIDTH, top * CELL_HEIGHT,    far * CELL_DEPTH};
}
bool loadVoxelCollection( StackAllocator* allocator, StringView filename, VoxelCollection* out )
{
	if( !loadVoxelCollectionTextureMapping( allocator, filename, out ) ) {
		return false;
	}
	TEMPORARY_MEMORY_BLOCK( GlobalScrap ) {
		auto grids = makeArray( GlobalScrap, VoxelGrid, out->frames.size() );
		if( !loadVoxelGridsFromFile( out->voxelsFilename, grids ) ) {
			return false;
		}
		for( auto i = 0; i < grids.size(); ++i ) {
			auto grid  = &grids[i];
			auto frame = &out->frames[i];
			auto info  = &out->frameInfos[i];
			TEMPORARY_MEMORY_BLOCK( GlobalScrap ) {
				int32 vertices = (int32)getCapacityFor< Vertex >( GlobalScrap ) / 2;
				int32 indices  = ( vertices * sizeof( Vertex ) ) / sizeof( uint16 );
				auto stream    = makeMeshStream( GlobalScrap, vertices, indices, nullptr );
				generateMeshFromVoxelGrid( &stream, grid, &info->textureMap, VoxelCellSize );
				frame->mesh = GlobalPlatformServices->uploadMesh( toMesh( &stream ) );
				assert( frame->mesh );
				info->bounds = getBoundsFromVoxelGrid( grid );
			}
		}
	}
	return true;
}