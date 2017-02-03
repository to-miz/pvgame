struct DynamicVoxelCollection {
	DynamicVoxelCollection() = default;
	~DynamicVoxelCollection();
	DynamicVoxelCollection( const DynamicVoxelCollection& other );
	DynamicVoxelCollection( DynamicVoxelCollection&& other );
	DynamicVoxelCollection& operator=( const DynamicVoxelCollection& other );
	DynamicVoxelCollection& operator=( DynamicVoxelCollection&& other );

	void assign( const DynamicVoxelCollection& other );
	void assign( DynamicVoxelCollection&& other );
	void destroy();

	VoxelCollection voxels;

	Array< StringView > names = {};
	Array< VoxelGrid > grids = {};
	bool gridsLoaded          = false;

	void* memory      = nullptr;
	size_t memorySize = 0;

	inline explicit operator bool() const { return memory != nullptr; }
};
DynamicVoxelCollection loadDymaicVoxelCollection( StringView filename );