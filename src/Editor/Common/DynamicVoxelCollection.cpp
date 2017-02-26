DynamicVoxelCollection loadDynamicVoxelCollection( StringView filename )
{
	DynamicVoxelCollection result;
	result.memorySize = kilobytes( 20 );
	result.memory     = GlobalPlatformServices->allocate( result.memorySize, 1 );
	auto allocator    = makeStackAllocator( result.memory, result.memorySize );
	if( loadVoxelCollection( &allocator, filename, &result.voxels ) ) {
		result.names = makeArray( &allocator, StringView, result.voxels.animations.size() );
		zip_for( result.names, result.voxels.animations ) {
			*first = second->name;
		}
		auto inplace = GlobalPlatformServices->reallocateInPlace( result.memory, allocator.size,
		                                                          result.memorySize, 1 );
		assert( inplace == result.memory );
		if( inplace ) {
			result.memorySize = allocator.size;
		}
	} else {
		GlobalPlatformServices->deallocate( result.memory, result.memorySize, 1 );
		result.memory     = nullptr;
		result.memorySize = 0;
	}
	return result;
}
DynamicVoxelCollection loadDynamicVoxelCollectionWithoutMeshes( StringView filename )
{
	DynamicVoxelCollection result;
	result.memorySize = megabytes( 3 );
	result.memory     = GlobalPlatformServices->allocate( result.memorySize, 1 );
	auto allocator    = makeStackAllocator( result.memory, result.memorySize );
	if( loadVoxelCollectionTextureMapping( &allocator, filename, &result.voxels ) ) {
		result.grids = makeArray( &allocator, VoxelGrid, result.voxels.frames.size() );
		if( loadVoxelGridsFromFile( result.voxels.voxelsFilename, result.grids ) ) {
			result.gridsLoaded = true;
		}
		auto inplace = GlobalPlatformServices->reallocateInPlace( result.memory, allocator.size,
		                                                          result.memorySize, 1 );
		assert( inplace == result.memory );
		if( inplace ) {
			result.memorySize = allocator.size;
		}
	} else {
		GlobalPlatformServices->deallocate( result.memory, result.memorySize, 1 );
		result.memory     = nullptr;
		result.memorySize = 0;
	}
	return result;
}

DynamicVoxelCollection::~DynamicVoxelCollection() { destroy(); }
DynamicVoxelCollection::DynamicVoxelCollection( const DynamicVoxelCollection& other )
{
	assign( other );
}
DynamicVoxelCollection::DynamicVoxelCollection( DynamicVoxelCollection&& other )
{
	assign( std::move( other ) );
}
DynamicVoxelCollection& DynamicVoxelCollection::operator=( const DynamicVoxelCollection& other )
{
	if( this != &other ) {
		destroy();
		assign( other );
	}
	return *this;
}
DynamicVoxelCollection& DynamicVoxelCollection::operator=( DynamicVoxelCollection&& other )
{
	if( this != &other ) {
		destroy();
		assign( std::move( other ) );
	}
	return *this;
}
void DynamicVoxelCollection::assign( const DynamicVoxelCollection& other )
{
	assert( this != &other );
	assert( !memory );
	assert( !memorySize );

	if( other.memory && other.memorySize ) {
		constexpr const auto AlignmentPadding = 8;
		memorySize                            = other.memorySize + AlignmentPadding;
		memory                                = GlobalPlatformServices->allocate( memorySize, 1 );
		auto allocator                        = makeStackAllocator( memory, memorySize );

		copyVoxelCollection( &allocator, other.voxels, &voxels );
		names               = makeArray( &allocator, StringView, other.names.size() );
		grids               = makeArray( &allocator, VoxelGrid, other.grids.size() );
		gridsLoaded         = other.gridsLoaded;
		grids.assign( other.grids );
		auto index = 0;
		FOR( animation : voxels.animations ) {
			names[index++] = animation.name;
		}

		if( GlobalPlatformServices->reallocateInPlace( memory, allocator.size, memorySize, 1 ) ) {
			memorySize = allocator.size;
		}
	}
}
void DynamicVoxelCollection::assign( DynamicVoxelCollection&& other )
{
	assert( this != &other );
	assert( !memory );
	assert( !memorySize );

	voxels      = other.voxels;
	names       = exchange( other.names, {} );
	grids       = exchange( other.grids, {} );
	gridsLoaded = exchange( other.gridsLoaded, false );
	memory      = exchange( other.memory, nullptr );
	memorySize  = exchange( other.memorySize, 0 );
}
void DynamicVoxelCollection::destroy()
{
	assert( !memory || memorySize );
	if( memory && memorySize ) {
		destroyVoxelCollection( &voxels );
		GlobalPlatformServices->deallocate( memory, memorySize, 1 );

		memory     = nullptr;
		memorySize = 0;
	}
}