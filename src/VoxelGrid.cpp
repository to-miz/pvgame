#define CELL_MAX_X 32
#define CELL_MAX_Y 32
#define CELL_MAX_Z 32
#define CELL_MAX_IN_PLANE ( CELL_MAX_X * CELL_MAX_Y )
#define CELL_MAX_COUNT ( CELL_MAX_X * CELL_MAX_Y * CELL_MAX_Z )

enum VoxelFaceValues : uint32 {
	VF_Front,
	VF_Left,
	VF_Back,
	VF_Right,
	VF_Top,
	VF_Bottom,

	VF_Count,
};
static const char* const VoxelFaceStrings[] = {
    "front", "left", "back", "right", "top", "bottom",
};

struct VoxelGridTextureMap {
	TextureId texture;
	struct Entry {
		QuadTexCoords texCoords;
	};
	Entry entries[VF_Count];
};

typedef uint32 VoxelCell;
enum VoxelCellFlags : VoxelCell {
	VoxelCellInner           = 0x8u,
	VoxelCellMask            = 0xFu,
	VoxelCellFaceTextureMask = 0x7u,
	VoxelCellBits            = 4,

	// only used by editor to mark a cell that should be removed when combining multiple grids
	VoxelToDelete = 0x80000000u,
};
#define SET_VOXEL_FACE( front, left, back, right, top, bottom )      \
	( ( ( (VoxelCell)front + 1 ) << ( VF_Front * VoxelCellBits ) )   \
	  | ( ( (VoxelCell)left + 1 ) << ( VF_Left * VoxelCellBits ) )   \
	  | ( ( (VoxelCell)back + 1 ) << ( VF_Back * VoxelCellBits ) )   \
	  | ( ( (VoxelCell)right + 1 ) << ( VF_Right * VoxelCellBits ) ) \
	  | ( ( (VoxelCell)top + 1 ) << ( VF_Top * VoxelCellBits ) )     \
	  | ( ( (VoxelCell)bottom + 1 ) << ( VF_Bottom * VoxelCellBits ) ) )

constexpr const VoxelCell EmptyCell    = {0};
constexpr const VoxelCell ToDeleteCell = {valueof( VoxelCellFlags::VoxelToDelete )};
constexpr const VoxelCell DefaultCell  = {SET_VOXEL_FACE( 0, 1, 2, 3, 4, 5 )};

struct VoxelGrid {
	VoxelCell data[CELL_MAX_COUNT];
	union {
		struct {
			int32 width;
			int32 height;
			int32 depth;
		};
		vec3i dim;
	};

	int32 size() { return width * height * depth; }
	int32 max_size() { return CELL_MAX_COUNT; }
	void clear() { zeroMemory( data, size() ); }
};

#define CELL_WIDTH 1.0f
#define CELL_HEIGHT 1.0f
#define CELL_DEPTH 1.0f

const vec3 VoxelCellSize = {CELL_WIDTH, CELL_HEIGHT, CELL_DEPTH};

inline vec2i getTexelPlaneByFace( int32 face )
{
	constexpr const vec2i texelPlaneByFace[] = {
	    {VectorComponent_X, VectorComponent_Y}, {VectorComponent_Z, VectorComponent_Y},
	    {VectorComponent_X, VectorComponent_Y}, {VectorComponent_Z, VectorComponent_Y},
	    {VectorComponent_X, VectorComponent_Z}, {VectorComponent_X, VectorComponent_Z},
	};
	assert( face >= 0 && face < countof( texelPlaneByFace ) );
	return texelPlaneByFace[face];
}

uint32 getVoxelFaceTexture( VoxelCell cell, VoxelFaceValues face )
{
	auto value        = valueof( face );
	auto textureIndex = ( ( cell >> ( value * VoxelCellBits ) ) & VoxelCellFaceTextureMask ) - 1;
	return textureIndex;
}
bool isVoxelFaceInner( VoxelCell cell, VoxelFaceValues face )
{
	auto value        = valueof( face );
	auto textureIndex = ( ( cell >> ( value * VoxelCellBits ) ) & VoxelCellInner );
	return textureIndex != 0;
}
VoxelCell setVoxelFaceTexture( VoxelCell cell, VoxelFaceValues face, int32 index )
{
	uint32 shiftAmount = ( valueof( face ) * VoxelCellBits );
	assert( shiftAmount < 32 );
	VoxelCell masked = ( ( VoxelCell )( ( index + 1 ) & VoxelCellFaceTextureMask ) );
	setFlagCond( masked, VoxelCellInner, isVoxelFaceInner( cell, face ) );
	masked <<= shiftAmount;
	return ( ( cell & ~( VoxelCellMask << shiftAmount ) ) | masked );
}
VoxelCell setVoxelFaceInner( VoxelCell cell, VoxelFaceValues face, bool inner )
{
	uint32 shiftAmount = ( valueof( face ) * VoxelCellBits );
	assert( shiftAmount < 32 );
	VoxelCell masked = ( cell >> shiftAmount ) & VoxelCellMask;
	setFlagCond( masked, VoxelCellInner, inner );
	masked <<= shiftAmount;
	return ( ( cell & ~( VoxelCellMask << shiftAmount ) ) | masked );
}

bool isPointInsideVoxelBounds( VoxelGrid* grid, int32 x, int32 y, int32 z )
{
	return ( x >= 0 && x < grid->width ) && ( y >= 0 && y < grid->height )
	       && ( z >= 0 && z < grid->depth );
}
bool isPointInsideVoxelBounds( VoxelGrid* grid, vec3iarg position )
{
	return ( position.x >= 0 && position.x < grid->width )
	       && ( position.y >= 0 && position.y < grid->height )
	       && ( position.z >= 0 && position.z < grid->depth );
}
VoxelCell& getCell( VoxelGrid* grid, int32 x, int32 y, int32 z )
{
	assert( isPointInsideVoxelBounds( grid, x, y, z ) );
	int32 index = x + y * grid->width + z * grid->width * grid->height;
	return grid->data[index];
}
VoxelCell& getCell( VoxelGrid* grid, vec3iarg position )
{
	assert( isPointInsideVoxelBounds( grid, position ) );
	int32 index = position.x + position.y * grid->width + position.z * grid->width * grid->height;
	return grid->data[index];
}

void generateMeshFromVoxelGrid( MeshStream* stream, VoxelGrid* grid, VoxelGridTextureMap* textures,
                                vec3arg cellSize )
{
	assert( isValid( stream ) );
	assert( grid );

	uint8 map[CELL_MAX_IN_PLANE];

	struct FindFirstQuadResult {
		bool found;
		vec3i position;
	};
	struct PlaneDescriptor {
		vec3 hAxis;
		vec3 vAxis;
		vec3 zAxis;
		vec3i faceNormal;
		vec3 origin;
		int8 hComponent;
		int8 vComponent;
		int8 zComponent;
		float hSize;
		float vSize;
		float zSize;
		int32 hCellCount;
		int32 vCellCount;
		int32 zCount;
		int8 frontOffset;
		Normal normal;
		bool isBackFacing;

		VoxelFaceValues face;
	};
	auto findFirstUnprocessedQuad = []( VoxelGrid* grid, const uint8* map, PlaneDescriptor* plane,
	                                    int32 layer ) -> FindFirstQuadResult {
		// assume we are searching on the xy plane
		for( int32 y = 0; y < plane->vCellCount; ++y ) {
			for( int32 x = 0; x < plane->hCellCount; ++x ) {
				auto mapIndex = x + y * plane->hCellCount;
				vec3i currentCell;
				currentCell.elements[plane->hComponent] = x;
				currentCell.elements[plane->vComponent] = y;
				currentCell.elements[plane->zComponent] = layer;
				vec3i frontCell                         = currentCell;
				frontCell.elements[plane->zComponent] += plane->frontOffset;

				if( getCell( grid, currentCell ) != EmptyCell && map[mapIndex] == 0 ) {
					// does the quad we are currently at need to be generated?
					if( !isPointInsideVoxelBounds( grid, frontCell )
					    || getCell( grid, frontCell ) == EmptyCell ) {
						FindFirstQuadResult result;
						result.found                                = true;
						result.position.elements[plane->hComponent] = x;
						result.position.elements[plane->vComponent] = y;
						result.position.elements[plane->zComponent] = layer;
						return result;
					}
				}
			}
		}
		return {false};
	};

	auto isGeneratingQuad = []( VoxelGrid* grid, PlaneDescriptor* plane, const uint8* map, int32 x,
	                            int32 y, int32 z, uint32 textureIndex ) {
		auto mapIndex = x + y * plane->hCellCount;
		vec3i currentCell;
		currentCell.elements[plane->hComponent] = x;
		currentCell.elements[plane->vComponent] = y;
		currentCell.elements[plane->zComponent] = z;
		vec3i frontCell                         = currentCell;
		frontCell.elements[plane->zComponent] += plane->frontOffset;

		return getCell( grid, currentCell ) != EmptyCell
		       && getVoxelFaceTexture( getCell( grid, currentCell ), plane->face ) == textureIndex
		       && map[mapIndex] == 0 && ( !isPointInsideVoxelBounds( grid, frontCell )
		                                  || getCell( grid, frontCell ) == EmptyCell );
	};

	auto processPlane = [findFirstUnprocessedQuad, isGeneratingQuad](
	    MeshStream* stream, VoxelGrid* grid, VoxelGridTextureMap* textures, PlaneDescriptor* plane,
	    uint8* map, size_t mapSize ) {

		for( int32 z = 0; z < plane->zCount; ++z ) {
			memset( map, 0, mapSize * sizeof( uint8 ) );
			for( ;; ) {
				auto first = findFirstUnprocessedQuad( grid, map, plane, z );
				if( first.found ) {
					auto position = swizzle( first.position, plane->hComponent, plane->vComponent,
					                         plane->zComponent );
					auto cell = getCell( grid, first.position );
					auto textureIndex = getVoxelFaceTexture( cell, plane->face );
					map[position.x + position.y * plane->hCellCount] = 1;

					vec3i posOffset = {};
					if( isVoxelFaceInner( cell, plane->face ) ) {
						auto next  = first.position + plane->faceNormal;
						if( isPointInsideVoxelBounds( grid, next ) ) {
							posOffset = plane->faceNormal;
						}
					}

					vec3 startVertex = plane->hAxis * ( plane->hSize * position.x )
					                   + plane->vAxis * ( plane->vSize * position.y )
					                   + plane->zAxis * ( plane->zSize * position.z )
					                   + plane->origin;
					assert( textureIndex < (uint32)countof( textures->entries ) );
					auto textureEntry = &textures->entries[textureIndex];
					auto texelPlane   = getTexelPlaneByFace( textureIndex );
					auto tw           = getAxisAlignedWidth( textureEntry->texCoords );
					auto th           = getAxisAlignedHeight( textureEntry->texCoords );
					auto texelWidth   = tw / grid->dim[texelPlane.x];
					auto texelHeight  = th / grid->dim[texelPlane.y];
					auto offsetedPos  = first.position + posOffset;
					auto tu           = offsetedPos[texelPlane.x] * texelWidth
					          + textureEntry->texCoords.elements[0].u;
					auto tv = offsetedPos[texelPlane.y] * texelHeight
					          + textureEntry->texCoords.elements[0].v;
					Vertex quad[4] = {
					    {startVertex, 0xFFFFFFFF, tu, tv, plane->normal},
					    {startVertex + plane->hAxis * plane->hSize, 0xFFFFFFFF, tu + texelWidth, tv,
					     plane->normal},
					    {startVertex + plane->vAxis * plane->vSize, 0xFFFFFFFF, tu,
					     tv + texelHeight, plane->normal},
					    {startVertex + plane->vAxis * plane->vSize + plane->hAxis * plane->hSize,
					     0xFFFFFFFF, tu + texelWidth, tv + texelHeight, plane->normal}};
					if( plane->face != textureIndex ) {
						// plane face doesn't match texture index
						// that means that the voxel face plane doesn't match the texelPlane
						// (ie the right face of the voxel taking the texture of the front face)
						// in that case we just repeat the quad over and over since the texture
						// doesn't depend on our current position in the plane

						// we end up outputting a single quad for each voxel that is like this
					} else {
						// texture coordinates depends on our current position in the plane
						// we greedily adjust the quad and texture coordinates as we move
						int32 y    = position.y;
						int32 xEnd = position.x + 1;
						for( int32 x = position.x + 1; x < plane->hCellCount; ++x ) {
							if( isGeneratingQuad( grid, plane, map, x, y, z, textureIndex ) ) {
								quad[1].position += plane->hAxis * plane->hSize;
								quad[1].texCoords.x += texelWidth;
								quad[3].position += plane->hAxis * plane->hSize;
								quad[3].texCoords.x += texelWidth;
								map[x + y * plane->hCellCount] = 1;
								++xEnd;
							} else {
								break;
							}
						}
						// we expanded the quad as much as we can in the x direction
						// now we expand in y
						++y;
						for( ; y < plane->vCellCount; ++y ) {
							bool generating = true;
							for( int32 x = position.x; x < xEnd; ++x ) {
								if( !isGeneratingQuad( grid, plane, map, x, y, z, textureIndex ) ) {
									generating = false;
									break;
								}
							}
							if( generating ) {
								for( int32 x = position.x; x < xEnd; ++x ) {
									map[x + y * plane->hCellCount] = 1;
								}
								quad[2].position += plane->vAxis * plane->vSize;
								quad[2].texCoords.y += texelHeight;
								quad[3].position += plane->vAxis * plane->vSize;
								quad[3].texCoords.y += texelHeight;
							} else {
								break;
							}
						}
					}

					if( plane->isBackFacing ) {
						// swap positions of 1 and 2 so that quad is ccw winded
						swap( quad[1], quad[2] );
					}
					pushQuad( stream, quad );
				} else {
					break;
				}
			}
		}
	};

	PlaneDescriptor plane;
	// front face
	plane.hAxis        = {1, 0, 0};
	plane.vAxis        = {0, -1, 0};
	plane.zAxis        = {0, 0, 1};
	plane.faceNormal   = {0, 0, 1};
	plane.hComponent   = VectorComponent_X;
	plane.vComponent   = VectorComponent_Y;
	plane.zComponent   = VectorComponent_Z;
	plane.hSize        = cellSize.x;
	plane.vSize        = cellSize.y;
	plane.zSize        = cellSize.y;
	plane.hCellCount   = grid->width;
	plane.vCellCount   = grid->height;
	plane.zCount       = grid->depth;
	plane.frontOffset  = -1;
	plane.origin       = {0, grid->height * cellSize.y, 0};
	plane.face         = VF_Front;
	plane.normal       = normal_neg_z_axis;
	plane.isBackFacing = false;

	stream->color = 0xFFFF0000;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );
	// back face
	plane.faceNormal   = {0, 0, -1};
	plane.frontOffset  = 1;
	plane.origin       = {0, grid->height * cellSize.y, cellSize.y};
	stream->color      = 0xFF00FF00;
	plane.face         = VF_Back;
	plane.normal       = normal_pos_z_axis;
	plane.isBackFacing = true;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );

	// right face
	plane.hAxis        = {0, 0, 1};
	plane.vAxis        = {0, -1, 0};
	plane.zAxis        = {1, 0, 0};
	plane.faceNormal   = {1, 0, 0};
	plane.hComponent   = VectorComponent_Z;
	plane.vComponent   = VectorComponent_Y;
	plane.zComponent   = VectorComponent_X;
	plane.hSize        = cellSize.y;
	plane.vSize        = cellSize.y;
	plane.zSize        = cellSize.x;
	plane.hCellCount   = grid->depth;
	plane.vCellCount   = grid->height;
	plane.zCount       = grid->width;
	plane.frontOffset  = 1;
	plane.origin       = {cellSize.x, grid->height * cellSize.y, 0};
	plane.face         = VF_Right;
	stream->color      = 0xFF0000FF;
	plane.normal       = normal_pos_x_axis;
	plane.isBackFacing = false;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );
	// left face
	plane.faceNormal   = {-1, 0, 0};
	plane.frontOffset  = -1;
	plane.origin       = {0, grid->height * cellSize.y, 0};
	stream->color      = 0xFFFF00FF;
	plane.face         = VF_Left;
	plane.normal       = normal_neg_x_axis;
	plane.isBackFacing = true;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );

	// top face
	plane.hAxis        = {1, 0, 0};
	plane.vAxis        = {0, 0, 1};
	plane.zAxis        = {0, -1, 0};
	plane.faceNormal   = {0, -1, 0};
	plane.hComponent   = VectorComponent_X;
	plane.vComponent   = VectorComponent_Z;
	plane.zComponent   = VectorComponent_Y;
	plane.hSize        = cellSize.x;
	plane.vSize        = cellSize.y;
	plane.zSize        = cellSize.y;
	plane.hCellCount   = grid->width;
	plane.vCellCount   = grid->depth;
	plane.zCount       = grid->height;
	plane.frontOffset  = -1;
	plane.face         = VF_Top;
	plane.origin       = {0, grid->height * cellSize.y, 0};
	stream->color      = 0xFFFFFF00;
	plane.normal       = normal_pos_y_axis;
	plane.isBackFacing = true;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );

	// bottom face
	plane.faceNormal   = {0, 1, 0};
	plane.frontOffset  = 1;
	plane.origin       = {0, grid->height * cellSize.y - cellSize.y, 0};
	stream->color      = 0xFF00FFFF;
	plane.face         = VF_Bottom;
	plane.normal       = normal_neg_y_axis;
	plane.isBackFacing = false;
	processPlane( stream, grid, textures, &plane, map, countof( map ) );
}

bool loadVoxelGridFromFile( PlatformServices* platform, StringView filename, VoxelGrid* grid )
{
	auto bytesToRead = sizeof( VoxelGrid );
	if( platform->readFileToBuffer( filename, grid, bytesToRead ) != bytesToRead ) {
		LOG( ERROR, "Failed to load voxel grid from file {}", filename );
		return false;
	}
	return true;
}
MeshId loadVoxelMeshFromFile( PlatformServices* platform, StackAllocator* allocator,
                              VoxelGridTextureMap* textures, StringView filename )
{
	// TODO: implement loading of texture maps too
	MeshId result = {};
	VoxelGrid grid;
	if( loadVoxelGridFromFile( platform, filename, &grid ) ) {
		TEMPORARY_MEMORY_BLOCK( allocator ) {
			auto meshStream = makeMeshStream( allocator, 10000, 40000, nullptr );
			generateMeshFromVoxelGrid( &meshStream, &grid, textures, VoxelCellSize );
			result = platform->uploadMesh( toMesh( &meshStream ) );
		}
	}
	return result;
}