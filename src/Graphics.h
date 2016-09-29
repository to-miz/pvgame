#pragma once

#ifndef _GRAPHICS_H_INCLUDED_
#define _GRAPHICS_H_INCLUDED_

enum class RenderCommandEntryType : int8 {
	Mesh,
	StaticMesh,
	SetTexture,
	SetBlend,
	SetProjection,
	SetRenderState,
	Jump, // jumps are used to enable reordering of commands already in the stream
};
enum class RenderStateType {
	DepthTest,
	Lighting,

	Count
};
enum class BlendType {

};
enum class ProjectionType { Perspective, Orthogonal };

struct TextureId {
	int32 id;
	inline explicit operator bool() const { return id != 0; };
};
inline bool operator==( TextureId a, TextureId b ) { return a.id == b.id; }
inline bool operator!=( TextureId a, TextureId b ) { return a.id != b.id; }

struct MeshId {
	int32 id;
	inline explicit operator bool() const { return id != 0; };
};
inline bool operator==( MeshId a, MeshId b ) { return a.id == b.id; }
inline bool operator!=( MeshId a, MeshId b ) { return a.id != b.id; }

struct Vertex {
	vec3 position;
	Color color;
	vec2 texCoords;
	Normal normal;
};
struct Mesh {
	Vertex* vertices;
	uint16* indices;
	int32 verticesCount;
	int32 indicesCount;
};
// Mesh makeMesh(  );

// header is only 4 bytes
struct RenderCommandHeader {
	RenderCommandEntryType type;
	uint8 offset;
	uint16 next;
};
struct RenderCommandMesh {
	static const RenderCommandEntryType type = RenderCommandEntryType::Mesh;

	Mesh mesh;
	int32 size;
};
struct RenderCommandStaticMesh {
	static const RenderCommandEntryType type = RenderCommandEntryType::StaticMesh;

	MeshId meshId;
	mat4 matrix;
	float screenDepthOffset;
};
struct RenderCommandSetTexture {
	static const RenderCommandEntryType type = RenderCommandEntryType::SetTexture;

	int32 stage;
	TextureId id;
};
struct RenderCommandSetBlend {
	BlendType type;
};
struct RenderCommandSetProjection {
	static const RenderCommandEntryType type = RenderCommandEntryType::SetProjection;

	ProjectionType projectionType;
};
struct RenderCommandSetRenderState {
	static const RenderCommandEntryType type = RenderCommandEntryType::SetRenderState;

	RenderStateType renderStateType;
	bool enabled;
};
struct RenderCommandJump {
	static const RenderCommandEntryType type = RenderCommandEntryType::Jump;
	RenderCommandJump* next;        // used to sort the linked list
	RenderCommandJump* nextInLine;  // next jump forward in stream
	char* jumpDestination;          // jump destination
	char* jumpDestinationOriginal;  // original jump destination before sorting, needed to turn a
	                                // sorted linked list into an in line jump list
	void* userData;                 // can be used to store sorting criteria
};

#define MeshRenderOptionsEntries \
	Color color;                 \
	float lineWidth;             \
	float pointSize
struct MeshRenderOptions {
	MeshRenderOptionsEntries;
};
#define MeshRenderOptionsUnion           \
	union {                              \
		struct {                         \
			MeshRenderOptionsEntries;    \
		};                               \
		MeshRenderOptions renderOptions; \
	}
struct MeshStream {
	struct {
		Vertex* vertices;
		uint16* indices;
		int32 verticesCount;
		int32 verticesCapacity;
		int32 indicesCount;
		int32 indicesCapacity;
	} data;
	MatrixStack* matrixStack;

	MeshRenderOptionsUnion;
};

constexpr MeshRenderOptions defaultMeshRenderOptions() { return {0xFFFFFFFF, 1, 5}; }

MeshStream makeMeshStream( StackAllocator* allocator, int32 maxVertices, int32 maxIndices,
                           MatrixStack* stack,
                           MeshRenderOptions renderOptions = defaultMeshRenderOptions() )
{
	MeshStream result            = {};
	result.data.vertices         = allocateArray( allocator, Vertex, maxVertices );
	result.data.verticesCapacity = maxVertices;
	result.data.indices          = allocateArray( allocator, uint16, maxIndices );
	result.data.indicesCapacity  = maxIndices;
	result.matrixStack           = stack;
	result.renderOptions         = renderOptions;
	return result;
}
MeshStream makeMeshStream( Vertex* vertices, int32 maxVertices, uint16* indices, int32 maxIndices,
                           MatrixStack* stack,
                           MeshRenderOptions renderOptions = defaultMeshRenderOptions() )
{
	MeshStream result            = {};
	result.data.vertices         = vertices;
	result.data.verticesCapacity = maxVertices;
	result.data.indices          = indices;
	result.data.indicesCapacity  = maxIndices;
	result.matrixStack           = stack;
	result.renderOptions         = renderOptions;
	return result;
}

bool isValid( MeshStream* stream )
{
	assert( stream );
	assert( stream->data.vertices );
	assert( stream->data.indices );
	assert( stream->data.verticesCapacity );
	assert( stream->data.indicesCapacity );
	return stream && stream->data.vertices && stream->data.indices && stream->data.verticesCapacity
	       && stream->data.indicesCapacity;
}
Mesh toMesh( MeshStream* stream )
{
	assert( isValid( stream ) );
	Mesh result;
	result.vertices      = stream->data.vertices;
	result.indices       = stream->data.indices;
	result.verticesCount = stream->data.verticesCount;
	result.indicesCount  = stream->data.indicesCount;
	return result;
}

void clear( MeshStream* stream )
{
	assert( isValid( stream ) );
	stream->data.verticesCount = 0;
	stream->data.indicesCount  = 0;
}

void pushBox( MeshStream* stream, vec3 box[8] )
{
	assert( isValid( stream ) );
	if( ( stream->data.verticesCapacity - stream->data.verticesCount < 24 )
	    || ( stream->data.indicesCapacity - stream->data.indicesCount < 36 ) ) {
		// TOOD: logging
		// OutOfMemory();
		return;
	}

	auto vertices = stream->data.vertices + stream->data.verticesCount;
	auto indices  = stream->data.indices + stream->data.indicesCount;
	auto color    = stream->color;
	// front-face
	*( vertices++ ) = {box[0], color, 0, 0, normal_neg_z_axis};
	*( vertices++ ) = {box[1], color, 0, 1, normal_neg_z_axis};
	*( vertices++ ) = {box[2], color, 1, 0, normal_neg_z_axis};
	*( vertices++ ) = {box[3], color, 1, 1, normal_neg_z_axis};

	// top-face
	*( vertices++ ) = {box[4], color, 0, 0, normal_pos_y_axis};
	*( vertices++ ) = {box[5], color, 0, 1, normal_pos_y_axis};
	*( vertices++ ) = {box[0], color, 1, 0, normal_pos_y_axis};
	*( vertices++ ) = {box[1], color, 1, 1, normal_pos_y_axis};

	// bottom-face
	*( vertices++ ) = {box[6], color, 0, 0, normal_neg_y_axis};
	*( vertices++ ) = {box[7], color, 0, 1, normal_neg_y_axis};
	*( vertices++ ) = {box[2], color, 1, 0, normal_neg_y_axis};
	*( vertices++ ) = {box[3], color, 1, 1, normal_neg_y_axis};

	// right-face
	*( vertices++ ) = {box[1], color, 0, 0, normal_pos_x_axis};
	*( vertices++ ) = {box[5], color, 0, 1, normal_pos_x_axis};
	*( vertices++ ) = {box[3], color, 1, 0, normal_pos_x_axis};
	*( vertices++ ) = {box[7], color, 1, 1, normal_pos_x_axis};

	// left-face
	*( vertices++ ) = {box[4], color, 0, 0, normal_neg_x_axis};
	*( vertices++ ) = {box[0], color, 0, 1, normal_neg_x_axis};
	*( vertices++ ) = {box[6], color, 1, 0, normal_neg_x_axis};
	*( vertices++ ) = {box[2], color, 1, 1, normal_neg_x_axis};

	// back-face
	*( vertices++ ) = {box[5], color, 0, 0, normal_pos_z_axis};
	*( vertices++ ) = {box[4], color, 0, 1, normal_pos_z_axis};
	*( vertices++ ) = {box[7], color, 1, 0, normal_pos_z_axis};
	*( vertices++ ) = {box[6], color, 1, 1, normal_pos_z_axis};

	auto currentVerticesCount = safe_truncate< uint16 >( stream->data.verticesCount );

	// front-face
	*( indices++ ) = currentVerticesCount + 0;
	*( indices++ ) = currentVerticesCount + 1;
	*( indices++ ) = currentVerticesCount + 2;

	*( indices++ ) = currentVerticesCount + 2;
	*( indices++ ) = currentVerticesCount + 1;
	*( indices++ ) = currentVerticesCount + 3;

	// top-face
	*( indices++ ) = currentVerticesCount + 4;
	*( indices++ ) = currentVerticesCount + 5;
	*( indices++ ) = currentVerticesCount + 6;

	*( indices++ ) = currentVerticesCount + 6;
	*( indices++ ) = currentVerticesCount + 5;
	*( indices++ ) = currentVerticesCount + 7;

	// bottom-face
	*( indices++ ) = currentVerticesCount + 8;
	*( indices++ ) = currentVerticesCount + 9;
	*( indices++ ) = currentVerticesCount + 10;

	*( indices++ ) = currentVerticesCount + 10;
	*( indices++ ) = currentVerticesCount + 9;
	*( indices++ ) = currentVerticesCount + 11;

	// right-face
	*( indices++ ) = currentVerticesCount + 12;
	*( indices++ ) = currentVerticesCount + 13;
	*( indices++ ) = currentVerticesCount + 14;

	*( indices++ ) = currentVerticesCount + 14;
	*( indices++ ) = currentVerticesCount + 13;
	*( indices++ ) = currentVerticesCount + 15;

	// left-face
	*( indices++ ) = currentVerticesCount + 16;
	*( indices++ ) = currentVerticesCount + 17;
	*( indices++ ) = currentVerticesCount + 18;

	*( indices++ ) = currentVerticesCount + 18;
	*( indices++ ) = currentVerticesCount + 17;
	*( indices++ ) = currentVerticesCount + 19;

	// back-face
	*( indices++ ) = currentVerticesCount + 20;
	*( indices++ ) = currentVerticesCount + 21;
	*( indices++ ) = currentVerticesCount + 22;

	*( indices++ ) = currentVerticesCount + 22;
	*( indices++ ) = currentVerticesCount + 21;
	*( indices++ ) = currentVerticesCount + 23;

	stream->data.verticesCount += 24;
	stream->data.indicesCount += 36;
}

void pushAabb( MeshStream* stream, float left, float bottom, float near, float right, float top,
               float far )
{
	vec3 vertices[8] = {
	    {left, top, near}, {right, top, near}, {left, bottom, near}, {right, bottom, near},
	    {left, top, far},  {right, top, far},  {left, bottom, far},  {right, bottom, far},
	};
	pushBox( stream, vertices );
}
void pushAabb( MeshStream* stream, aabbarg aabb )
{
	vec3 vertices[8] = {
	    {aabb.left, aabb.top, aabb.near},    {aabb.right, aabb.top, aabb.near},
	    {aabb.left, aabb.bottom, aabb.near}, {aabb.right, aabb.bottom, aabb.near},

	    {aabb.left, aabb.top, aabb.far},     {aabb.right, aabb.top, aabb.far},
	    {aabb.left, aabb.bottom, aabb.far},  {aabb.right, aabb.bottom, aabb.far},
	};
	pushBox( stream, vertices );
}

void pushRay( MeshStream* stream, vec3arg start, vec3arg dirNormalized, float length )
{
	vec3 hNormal = safeNormalize( vec3{dirNormalized.z, 0, -dirNormalized.x}, vec3{1, 0, 0} )
	               * stream->lineWidth;
	vec3 vNormal = safeNormalize( vec3{0, dirNormalized.z, -dirNormalized.y}, vec3{0, 1, 0} )
	               * stream->lineWidth;
	vec3 end = start + dirNormalized * length;

	vec3 vertices[] = {
	    {start - vNormal}, {start - hNormal}, {start + vNormal}, {start + hNormal},
	    {end - vNormal},   {end - hNormal},   {end + vNormal},   {end + hNormal},
	};
	pushBox( stream, vertices );
}
void pushLine( MeshStream* stream, vec3arg start, vec3arg end )
{
	float length = 0;
	auto dir     = safeNormalize( end - start, &length );
	if( length != 0 ) {
		pushRay( stream, start, dir, length );
	}
}
void pushAabbOutline( MeshStream* stream, aabbarg box )
{
	pushLine( stream, {box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z} );
	pushLine( stream, {box.min.x, box.min.y, box.min.z}, {box.min.x, box.max.y, box.min.z} );
	pushLine( stream, {box.min.x, box.min.y, box.min.z}, {box.min.x, box.min.y, box.max.z} );

	pushLine( stream, {box.max.x, box.min.y, box.min.z}, {box.max.x, box.max.y, box.min.z} );
	pushLine( stream, {box.max.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.max.z} );

	pushLine( stream, {box.max.x, box.max.y, box.min.z}, {box.min.x, box.max.y, box.min.z} );
	pushLine( stream, {box.max.x, box.max.y, box.min.z}, {box.max.x, box.max.y, box.max.z} );

	pushLine( stream, {box.max.x, box.max.y, box.max.z}, {box.max.x, box.min.y, box.max.z} );
	pushLine( stream, {box.max.x, box.max.y, box.max.z}, {box.min.x, box.max.y, box.max.z} );

	pushLine( stream, {box.min.x, box.max.y, box.max.z}, {box.min.x, box.min.y, box.max.z} );
	pushLine( stream, {box.min.x, box.max.y, box.max.z}, {box.min.x, box.max.y, box.min.z} );

	pushLine( stream, {box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z} );
}
void pushPoint( MeshStream* stream, vec3arg point )
{
	auto pointSize = stream->pointSize;
	pushAabb( stream, point.x - pointSize, point.y - pointSize, point.z - pointSize,
	          point.x + pointSize, point.y + pointSize, point.z + pointSize );
}

void pushQuad( MeshStream* stream, vec3 quad[4],
               QuadTexCoordsArg texCoords = makeQuadTexCoordsDef() )
{
	assert( isValid( stream ) );
	if( ( stream->data.verticesCapacity - stream->data.verticesCount < 4 )
	    || ( stream->data.indicesCapacity - stream->data.indicesCount < 6 ) ) {
		// TOOD: logging
		// OutOfMemory();
		return;
	}

	auto vertices   = stream->data.vertices + stream->data.verticesCount;
	auto indices    = stream->data.indices + stream->data.indicesCount;
	auto color      = stream->color;
	*( vertices++ ) = {quad[0], color, texCoords.elements[0]};
	*( vertices++ ) = {quad[1], color, texCoords.elements[1]};
	*( vertices++ ) = {quad[2], color, texCoords.elements[2]};
	*( vertices++ ) = {quad[3], color, texCoords.elements[3]};

	auto currentVerticesCount = safe_truncate< uint16 >( stream->data.verticesCount );

	*( indices++ ) = currentVerticesCount + 0;
	*( indices++ ) = currentVerticesCount + 1;
	*( indices++ ) = currentVerticesCount + 2;

	*( indices++ ) = currentVerticesCount + 2;
	*( indices++ ) = currentVerticesCount + 1;
	*( indices++ ) = currentVerticesCount + 3;

	stream->data.verticesCount += 4;
	stream->data.indicesCount += 6;
}
void pushQuad( MeshStream* stream, Vertex vertices[4] )
{
	assert( isValid( stream ) );
	if( ( stream->data.verticesCapacity - stream->data.verticesCount < 4 )
	    || ( stream->data.indicesCapacity - stream->data.indicesCount < 6 ) ) {
		// TOOD: logging
		// OutOfMemory();
		return;
	}

	copy( stream->data.vertices + stream->data.verticesCount, vertices, 4 );
	auto indices    = stream->data.indices + stream->data.indicesCount;
	auto currentVerticesCount = safe_truncate< uint16 >( stream->data.verticesCount );

	*( indices++ ) = currentVerticesCount + 0;
	*( indices++ ) = currentVerticesCount + 1;
	*( indices++ ) = currentVerticesCount + 2;

	*( indices++ ) = currentVerticesCount + 2;
	*( indices++ ) = currentVerticesCount + 1;
	*( indices++ ) = currentVerticesCount + 3;

	stream->data.verticesCount += 4;
	stream->data.indicesCount += 6;
}
void pushQuad( MeshStream* stream, rectfarg rect, float z = 0 )
{
	auto color         = stream->color;
	Vertex vertices[4] = {
	    {rect.left, rect.top, z, color, 0, 0, normal_neg_z_axis},
	    {rect.right, rect.top, z, color, 1, 0, normal_neg_z_axis},
	    {rect.left, rect.bottom, z, color, 0, 1, normal_neg_z_axis},
	    {rect.right, rect.bottom, z, color, 1, 1, normal_neg_z_axis},
	};
	pushQuad( stream, vertices );
}
void pushQuad( MeshStream* stream, rectfarg rect, float z, QuadTexCoordsArg texCoords )
{
	auto color         = stream->color;
	Vertex vertices[4] = {
	    {rect.left, rect.top, z, color, texCoords.elements[0], normal_neg_z_axis},
	    {rect.right, rect.top, z, color, texCoords.elements[1], normal_neg_z_axis},
	    {rect.left, rect.bottom, z, color, texCoords.elements[2], normal_neg_z_axis},
	    {rect.right, rect.bottom, z, color, texCoords.elements[3], normal_neg_z_axis},
	};
	pushQuad( stream, vertices );
}
void pushQuadOutline( MeshStream* stream, rectfarg rect, float z = 0 )
{
	auto halfWidth = stream->lineWidth * 0.5f;
	pushQuad( stream, rectf{rect.left - halfWidth, rect.top, rect.left + halfWidth, rect.bottom},
	          z );
	pushQuad( stream, rectf{rect.right - halfWidth, rect.top, rect.right + halfWidth, rect.bottom},
	          z );
	pushQuad( stream, rectf{rect.left, rect.top + halfWidth, rect.right, rect.top - halfWidth}, z );
	pushQuad( stream,
	          rectf{rect.left, rect.bottom + halfWidth, rect.right, rect.bottom - halfWidth}, z );
}

struct RenderCommands {
	StackAllocator allocator;
	MatrixStack* matrixStack;
	mat4 view;
	float ambientStrength;
	Color lightColor;
	vec3 lightPosition;

	bool locked;
	bool wireframe;
	MeshRenderOptionsUnion;
};

RenderCommands makeRenderCommands( StackAllocator* allocator, size_t capacity, MatrixStack* stack )
{
	assert( isValid( allocator ) );
	RenderCommands result  = {};
	result.allocator       = makeStackAllocator( allocator, capacity );
	result.matrixStack     = stack;
	result.view            = matrixIdentity();
	result.renderOptions   = defaultMeshRenderOptions();
	result.ambientStrength = 1;
	return result;
}

bool isValid( RenderCommands* renderCommands )
{
	return renderCommands && isValid( &renderCommands->allocator ) && !renderCommands->locked;
}
char* back( RenderCommands* renderCommands ) { return back( &renderCommands->allocator ); }
void clear( RenderCommands* renderCommands )
{
	assert( isValid( renderCommands ) );
	clear( &renderCommands->allocator );
	renderCommands->locked = false;
}

struct RenderCommandsStream {
	char* ptr;
	size_t size;
};
RenderCommandsStream getRenderCommandsStream( RenderCommands* renderCommands )
{
	return {renderCommands->allocator.ptr, renderCommands->allocator.size};
}
RenderCommandHeader* getRenderCommandsHeader( RenderCommandsStream* stream )
{
	auto header = (RenderCommandHeader*)stream->ptr;
	assert_alignment( header, alignof( RenderCommandHeader ) );
	stream->ptr += sizeof( RenderCommandHeader ) + header->offset;
	stream->size -= sizeof( RenderCommandHeader ) + header->offset;
	return header;
}
void* getRenderCommandBodyImpl( RenderCommandsStream* stream, RenderCommandHeader* header,
                                uint16 size, uint32 alignment )
{
	auto result = (void*)stream->ptr;
	assert_alignment( result, alignment );
	assert( header->next == size );
	stream->ptr += header->next;
	stream->size -= header->next;
	return result;
}

#define getRenderCommandBody( stream, header, type ) \
	(type*)getRenderCommandBodyImpl( ( stream ), ( header ), sizeof( type ), alignof( type ) )

RenderCommandMesh* getRenderCommandMesh( RenderCommandsStream* stream, RenderCommandHeader* header )
{
	auto result = (RenderCommandMesh*)stream->ptr;
	assert_alignment( result, alignof( RenderCommandMesh ) );
	assert( header->next == sizeof( RenderCommandMesh ) );
	stream->ptr += header->next + result->size;
	stream->size -= header->next + result->size;
	return result;
}
void skipRenderCommandBody( RenderCommandsStream* stream, RenderCommandHeader* header )
{
	switch( header->type ) {
		case RenderCommandEntryType::Mesh: {
			getRenderCommandMesh( stream, header );
			break;
		}
		default: {
			stream->ptr += header->next;
			stream->size -= header->next;
			break;
		}
	}
}

char* nextRenderCommandHeaderLocation( RenderCommands* renderCommands )
{
	return back( &renderCommands->allocator )
	       + getAlignmentOffset( &renderCommands->allocator, alignof( RenderCommandHeader ) );
}

RenderCommandHeader* allocateRenderCommandHeaderImpl( StackAllocator* allocator,
                                                      RenderCommandEntryType type, uint16 size,
                                                      uint32 alignment )
{
	auto header    = allocateStruct( allocator, RenderCommandHeader );
	header->type   = type;
	header->offset = safe_truncate< uint8 >( getAlignmentOffset( allocator, alignment ) );
	auto alignmentOffsetOfHeader =
	    getAlignmentOffset( allocator->ptr + size, alignof( RenderCommandHeader ) );
	header->next = size + safe_truncate< uint16 >( alignmentOffsetOfHeader );
	return header;
}
#define allocateRenderCommandHeader( allocator, _type ) \
	allocateRenderCommandHeaderImpl( ( allocator ), _type::type, sizeof( _type ), alignof( _type ) )

RenderCommandMesh* addRenderCommandMesh( RenderCommands* renderCommands, int32 verticesCount,
                                         int32 indicesCount )
{
	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;

	allocateRenderCommandHeader( allocator, RenderCommandMesh );
	auto body = allocateStruct( allocator, RenderCommandMesh );

	auto startSize           = allocator->size;
	body->mesh.vertices      = allocateArray( allocator, Vertex, verticesCount );
	body->mesh.indices       = allocateArray( allocator, uint16, indicesCount );
	body->mesh.verticesCount = verticesCount;
	body->mesh.indicesCount  = indicesCount;
	auto endSize             = allocator->size;
	body->size               = safe_truncate< int32 >( endSize - startSize );

	return body;
}

void addRenderCommandMesh( RenderCommands* renderCommands, const Mesh& mesh )
{
	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;

	allocateRenderCommandHeader( allocator, RenderCommandMesh );
	auto body  = allocateStruct( allocator, RenderCommandMesh );
	body->mesh = mesh;
	body->size = 0;
}

RenderCommandStaticMesh* addRenderCommandMesh( RenderCommands* renderCommands, MeshId meshId )
{
	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;

	allocateRenderCommandHeader( allocator, RenderCommandStaticMesh );
	auto body    = allocateStruct( allocator, RenderCommandStaticMesh );
	*body        = {};
	body->meshId = meshId;
	body->matrix = currentMatrix( renderCommands->matrixStack );
	return body;
}

MeshStream addRenderCommandMeshStream( RenderCommands* renderCommands, int32 verticesCount,
                                       int32 indicesCount )
{
	auto command = addRenderCommandMesh( renderCommands, verticesCount, indicesCount );
	return makeMeshStream( command->mesh.vertices, command->mesh.verticesCount,
	                       command->mesh.indices, command->mesh.indicesCount,
	                       renderCommands->matrixStack, renderCommands->renderOptions );
}

struct MeshStreamingBlock {
	MeshStream stream;
	RenderCommandMesh* meshCommand;
	inline explicit operator bool() const { return meshCommand != nullptr; }
};

MeshStreamingBlock beginMeshStreaming( RenderCommands* renderCommands )
{
	assert( isValid( renderCommands ) );
	assert( !renderCommands->locked );
	auto allocator = &renderCommands->allocator;

	allocateRenderCommandHeader( allocator, RenderCommandMesh );
	auto body = allocateStruct( allocator, RenderCommandMesh );

	auto remainingBytes = remaining( allocator ) - alignof( Vertex ) - alignof( uint16 );
	auto verticesCount  = safe_truncate< int32 >( ( remainingBytes / 2 ) / sizeof( Vertex ) );
	auto indicesCount   = safe_truncate< int32 >( ( remainingBytes / 2 ) / sizeof( uint16 ) );

	auto startSize           = allocator->size;
	body->mesh.vertices      = allocateArray( allocator, Vertex, verticesCount );
	body->mesh.indices       = allocateArray( allocator, uint16, indicesCount );
	body->mesh.verticesCount = verticesCount;
	body->mesh.indicesCount  = indicesCount;
	auto endSize             = allocator->size;
	body->size               = safe_truncate< int32 >( endSize - startSize );

	renderCommands->locked = true;
	return {makeMeshStream( body->mesh.vertices, body->mesh.verticesCount, body->mesh.indices,
	                        body->mesh.indicesCount, renderCommands->matrixStack,
	                        renderCommands->renderOptions ),
	        body};
}
void endMeshStreaming( RenderCommands* renderCommands, MeshStreamingBlock* block )
{
	assert( renderCommands && renderCommands->locked );
	auto stream               = &block->stream;
	auto meshCommand          = block->meshCommand;
	meshCommand->mesh.indices = fitToSizeArrays(
	    &renderCommands->allocator, stream->data.vertices, stream->data.verticesCount,
	    stream->data.verticesCapacity, stream->data.indices, stream->data.indicesCount,
	    stream->data.indicesCapacity );
	auto memoryStart           = (char*)stream->data.vertices;
	auto memoryEnd             = (char*)( meshCommand->mesh.indices + stream->data.indicesCount );
	meshCommand->mesh.vertices = stream->data.vertices;
	meshCommand->mesh.verticesCount = stream->data.verticesCount;
	meshCommand->mesh.indicesCount  = stream->data.indicesCount;
	meshCommand->size               = safe_truncate< int32 >( memoryEnd - memoryStart );
	renderCommands->locked          = false;
}

struct MeshStreamGuard {
	MeshStreamingBlock block;
	RenderCommands* renderCommands;

	MeshStreamGuard( RenderCommands* renderCommands )
	: block( beginMeshStreaming( renderCommands ) )
	{
		assert( renderCommands && renderCommands->locked );
	}
	MeshStreamGuard( RenderCommands* renderCommands, MeshStreamingBlock block )
	: block( block ), renderCommands( renderCommands )
	{
		assert( renderCommands && renderCommands->locked );
	}
	~MeshStreamGuard() { endMeshStreaming( renderCommands, &block ); }
};

MeshStreamGuard addRenderCommandMeshStreamImpl( RenderCommands* renderCommands )
{
	return MeshStreamGuard( renderCommands, beginMeshStreaming( renderCommands ) );
}

#define MESH_STREAM_BLOCK( name, renderer )                                                       \
	if( auto _once = false ) {                                                                    \
	} else                                                                                        \
		for( auto _guard = addRenderCommandMeshStreamImpl( ( renderer ) ); !_once; _once = true ) \
			for( auto name = &_guard.block.stream; !_once; _once = true )

void setTexture( RenderCommands* renderCommands, int32 textureStage, TextureId texture )
{
	assert( isValid( renderCommands ) );

	auto allocator = &renderCommands->allocator;

	allocateRenderCommandHeader( allocator, RenderCommandSetTexture );
	auto body   = allocateStruct( allocator, RenderCommandSetTexture );
	body->stage = textureStage;
	body->id    = texture;
}

void setTexture( RenderCommands* renderCommands, int32 textureStage, null_t )
{
	setTexture( renderCommands, textureStage, TextureId{} );
}

void setProjection( RenderCommands* renderCommands, ProjectionType type )
{
	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;
	allocateRenderCommandHeader( allocator, RenderCommandSetProjection );
	auto body            = allocateStruct( allocator, RenderCommandSetProjection );
	body->projectionType = type;
}
void setRenderState( RenderCommands* renderCommands, RenderStateType type, bool enabled )
{
	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;
	allocateRenderCommandHeader( allocator, RenderCommandSetRenderState );
	auto body            = allocateStruct( allocator, RenderCommandSetRenderState );
	body->renderStateType = type;
	body->enabled = enabled;
}

RenderCommandJump* addRenderCommandJump( RenderCommands* renderCommands, RenderCommandJump* prev,
                                         void* userData )
{
	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;
	allocateRenderCommandHeader( allocator, RenderCommandJump );
	auto body             = allocateStruct( allocator, RenderCommandJump );
	body->next            = nullptr;
	body->nextInLine      = nullptr;
	body->jumpDestination = nextRenderCommandHeaderLocation( renderCommands );
	body->jumpDestinationOriginal = body->jumpDestination;
	body->userData        = userData;
	if( prev ) {
		assert( prev->next == nullptr );
		prev->next       = body;
		prev->nextInLine = body;
	}
	return body;
}

typedef bool RenderCommandJumpCompareType( const RenderCommandJump& a, const RenderCommandJump& b );
void sortRenderCommandJumps( RenderCommands* renderCommands, RenderCommandJump* first,
                             RenderCommandJump* last, RenderCommandJumpCompareType* cmp )
{
	// the block of memory to be sorted always needs to begin and end with a jump, so we assert that
	// there actually is a jump at the end of the memory region to be sorted
	assert( back( &renderCommands->allocator ) == (char*)last + sizeof( RenderCommandJump ) );
	auto result = merge_sort( first, cmp );
	auto sorted = result.head;
	while( first ) {
		assert( sorted );
		first->jumpDestination = sorted->jumpDestinationOriginal;
		first = sorted->nextInLine;
		sorted = sorted->next;
	}
	assert( !first && !sorted );
}

// simplified clipping, will result in artifacts if input isn't a mesh composed of axis aligned
// quads on the xy plane
void clip( Mesh* mesh, rectfarg rect )
{
	assert( mesh );
	assert( isValid( rect ) );

	// treat mesh as a series of quads
	auto vertices = mesh->vertices;
	for( intmax i = 0, count = mesh->verticesCount / 4; i < count; ++i, vertices += 4 ) {
		auto width = vertices[1].position.x - vertices[0].position.x;
		auto height = vertices[2].position.y - vertices[0].position.y;

		auto ltrt = vertices[1].texCoords - vertices[0].texCoords;
		auto ltlb = vertices[2].texCoords - vertices[0].texCoords;

		auto entry = vertices;
		for( intmax j = 0; j < 4; ++j, ++entry ) {
			if( entry->position.x < rect.left ) {
				float t = ( rect.left - entry->position.x ) / width;
				entry->texCoords += t * ltrt;
				entry->position.x = rect.left;
			}
			if( entry->position.x > rect.right ) {
				float t = ( entry->position.x - rect.right ) / width;
				entry->texCoords -= t * ltrt;
				entry->position.x = rect.right;
			}
			if( entry->position.y < rect.top ) {
				float t = ( rect.top - entry->position.y ) / height;
				entry->texCoords += t * ltlb;
				entry->position.y = rect.top;
			}
			if( entry->position.y > rect.bottom ) {
				float t = ( entry->position.y - rect.bottom ) / height;
				entry->texCoords -= t * ltlb;
				entry->position.y = rect.bottom;
			}
		}
	}
}
struct ClippingRect {
	RenderCommands* renderer;
	char* start;
	rectf rect = {};
	bool clipped = false;

	inline ClippingRect( RenderCommands* renderer, rectfarg rect )
	: renderer( renderer ),
	  start( back( renderer ) ),
	  rect( rect )
	{
		auto& matrix = currentMatrix( renderer->matrixStack );
		this->rect.leftTop = transformVector( matrix, rect.leftTop );
		this->rect.rightBottom = transformVector( matrix, rect.rightBottom );
	}
	inline ~ClippingRect()
	{
		if( !clipped ) {
			clip();
		}
	}
	inline void dismiss() { clipped = true; }
	void clip()
	{
		assert( renderer );
		assert( start );
		assert( !clipped );
		auto stream = RenderCommandsStream{start, ( size_t )( back( renderer ) - start )};
		while( stream.size ) {
			auto header = getRenderCommandsHeader( &stream );
			switch( header->type ) {
				case RenderCommandEntryType::Mesh: {
					auto body = getRenderCommandMesh( &stream, header );
					::clip( &body->mesh, rect );
					break;
				}
				default: {
					skipRenderCommandBody( &stream, header );
					break;
				}
			}
		}
	}
};

// renderCommands state guard

struct RenderCommandsStateGuard {
	RenderCommands* renderer;
	MeshRenderOptions renderOptions;

	RenderCommandsStateGuard( RenderCommands* renderer )
	: renderer( renderer ), renderOptions( renderer->renderOptions )
	{
	}
	~RenderCommandsStateGuard() { renderer->renderOptions = renderOptions; }
};

#define RENDER_COMMANDS_STATE_BLOCK( renderer ) \
	if( auto _once = false ) {                  \
	} else                                      \
		for( auto _guard = RenderCommandsStateGuard( ( renderer ) ); !_once; _once = true )

#endif  // _GRAPHICS_H_INCLUDED_
