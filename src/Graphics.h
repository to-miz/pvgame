#pragma once

#ifndef _GRAPHICS_H_INCLUDED_
#define _GRAPHICS_H_INCLUDED_

enum class RenderCommandEntryType : int8 {
	Mesh,
	LineMesh,
	StaticMesh,
	SetTexture,
	SetShader,
	SetBlend,
	SetProjection,        // sets currently active projection type (perspective vs orthogonal)
	SetProjectionMatrix,  // specifies matrix of projection type
	SetScissorRect,
	SetRenderState,
	Jump,  // jumps are used to enable reordering of commands already in the stream
};
enum class RenderStateType {
	DepthTest,
	DepthWrite,
	Lighting,
	Scissor,
	BackFaceCulling,
	CullFrontFace,

	Count
};
enum class BlendType {

};
enum class ProjectionType { Perspective, Orthogonal };

// macro to define typesafe id types
#define GRAPHICS_MAKE_ID_ENTRY( name )                                           \
	struct name {                                                                \
		int32 id;                                                                \
		inline explicit operator bool() const { return id != 0; };               \
		friend bool operator==( name a, name b ) { return a.id == b.id; } \
		friend bool operator!=( name a, name b ) { return a.id != b.id; } \
	};

GRAPHICS_MAKE_ID_ENTRY( TextureId );
GRAPHICS_MAKE_ID_ENTRY( MeshId );
GRAPHICS_MAKE_ID_ENTRY( ShaderId );

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
	Color flashColor;
	float screenDepthOffset;
};
struct RenderCommandSetTexture {
	static const RenderCommandEntryType type = RenderCommandEntryType::SetTexture;

	int32 stage;
	TextureId id;
};
struct RenderCommandSetShader {
	static const RenderCommandEntryType type = RenderCommandEntryType::SetShader;

	ShaderId id;
};
struct RenderCommandSetBlend {
	BlendType type;
};
struct RenderCommandSetProjection {
	static const RenderCommandEntryType type = RenderCommandEntryType::SetProjection;

	ProjectionType projectionType;
};
struct RenderCommandSetProjectionMatrix {
	static const RenderCommandEntryType type = RenderCommandEntryType::SetProjectionMatrix;

	ProjectionType projectionType;
	mat4 matrix;
};
struct RenderCommandSetScissorRect {
	static const RenderCommandEntryType type = RenderCommandEntryType::SetScissorRect;

	recti scissor;
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
struct RenderCommandLineMesh {
	static const RenderCommandEntryType type = RenderCommandEntryType::LineMesh;

	Mesh mesh;
	int32 size;
};

#define MeshRenderOptionsEntries \
	Color color;                 \
	Color flashColor;            \
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

struct LineMeshStream : MeshStream {};

constexpr MeshRenderOptions defaultMeshRenderOptions() { return {0xFFFFFFFF, 1, 5}; }

// if the index buffer contains this value, it denotes that we want to start a new primitve, used
// with LineMesh, since it represents a line strip
constexpr uint16 MeshPrimitiveRestart = 0xFFFF;

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
MeshStream makeMeshStream( Mesh* mesh, MatrixStack* stack = nullptr,
                           MeshRenderOptions renderOptions = defaultMeshRenderOptions() )
{
	MeshStream result            = {};
	result.data.vertices         = mesh->vertices;
	result.data.verticesCapacity = mesh->verticesCount;
	result.data.indices          = mesh->indices;
	result.data.indicesCapacity  = mesh->indicesCount;
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
bool hasCapacity( MeshStream* stream, int32 verticesCount, int32 indicesCount )
{
	return ( stream->data.verticesCapacity - stream->data.verticesCount >= verticesCount )
	       && ( stream->data.indicesCapacity - stream->data.indicesCount >= indicesCount );
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
	PROFILE_FUNCTION();

	assert( isValid( stream ) );
	if( !hasCapacity( stream, 24, 36 ) ) {
		OutOfMemory();
		return;
	}

	auto vertices = stream->data.vertices + stream->data.verticesCount;
	auto indices  = stream->data.indices + stream->data.indicesCount;
	auto color    = stream->color;
	// front-face
	*( vertices++ ) = {box[0], color, 0, 0, normal_neg_z_axis};
	*( vertices++ ) = {box[1], color, 1, 0, normal_neg_z_axis};
	*( vertices++ ) = {box[2], color, 0, 1, normal_neg_z_axis};
	*( vertices++ ) = {box[3], color, 1, 1, normal_neg_z_axis};

	// top-face
	*( vertices++ ) = {box[4], color, 0, 0, normal_pos_y_axis};
	*( vertices++ ) = {box[5], color, 1, 0, normal_pos_y_axis};
	*( vertices++ ) = {box[0], color, 0, 1, normal_pos_y_axis};
	*( vertices++ ) = {box[1], color, 1, 1, normal_pos_y_axis};

	// bottom-face
	*( vertices++ ) = {box[6], color, 0, 0, normal_neg_y_axis};
	*( vertices++ ) = {box[7], color, 1, 0, normal_neg_y_axis};
	*( vertices++ ) = {box[2], color, 0, 1, normal_neg_y_axis};
	*( vertices++ ) = {box[3], color, 1, 1, normal_neg_y_axis};

	// right-face
	*( vertices++ ) = {box[1], color, 0, 0, normal_pos_x_axis};
	*( vertices++ ) = {box[5], color, 1, 0, normal_pos_x_axis};
	*( vertices++ ) = {box[3], color, 0, 1, normal_pos_x_axis};
	*( vertices++ ) = {box[7], color, 1, 1, normal_pos_x_axis};

	// left-face
	*( vertices++ ) = {box[4], color, 0, 0, normal_neg_x_axis};
	*( vertices++ ) = {box[0], color, 1, 0, normal_neg_x_axis};
	*( vertices++ ) = {box[6], color, 0, 1, normal_neg_x_axis};
	*( vertices++ ) = {box[2], color, 1, 1, normal_neg_x_axis};

	// back-face
	*( vertices++ ) = {box[5], color, 0, 0, normal_pos_z_axis};
	*( vertices++ ) = {box[4], color, 1, 0, normal_pos_z_axis};
	*( vertices++ ) = {box[7], color, 0, 1, normal_pos_z_axis};
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
	*( indices++ ) = currentVerticesCount + 10;
	*( indices++ ) = currentVerticesCount + 11;

	*( indices++ ) = currentVerticesCount + 8;
	*( indices++ ) = currentVerticesCount + 11;
	*( indices++ ) = currentVerticesCount + 9;

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
	PROFILE_FUNCTION();

	vec3 vertices[8] = {
	    {left, top, near}, {right, top, near}, {left, bottom, near}, {right, bottom, near},
	    {left, top, far},  {right, top, far},  {left, bottom, far},  {right, bottom, far},
	};
	pushBox( stream, vertices );
}
void pushAabb( MeshStream* stream, aabbarg aabb )
{
	PROFILE_FUNCTION();

	vec3 vertices[8] = {
	    {aabb.left, aabb.top, aabb.near},    {aabb.right, aabb.top, aabb.near},
	    {aabb.left, aabb.bottom, aabb.near}, {aabb.right, aabb.bottom, aabb.near},

	    {aabb.left, aabb.top, aabb.far},     {aabb.right, aabb.top, aabb.far},
	    {aabb.left, aabb.bottom, aabb.far},  {aabb.right, aabb.bottom, aabb.far},
	};
	pushBox( stream, vertices );
}
void pushAabbTransformed( MeshStream* stream, float left, float bottom, float near, float right,
                          float top, float far )
{
	PROFILE_FUNCTION();
	auto& m          = currentMatrix( stream->matrixStack );
	vec3 vertices[8] = {
	    transformVector( m, {left, top, near} ),    transformVector( m, {right, top, near} ),
	    transformVector( m, {left, bottom, near} ), transformVector( m, {right, bottom, near} ),
	    transformVector( m, {left, top, far} ),     transformVector( m, {right, top, far} ),
	    transformVector( m, {left, bottom, far} ),  transformVector( m, {right, bottom, far} ),
	};
	pushBox( stream, vertices );
}

void pushRay( MeshStream* stream, vec3arg start, vec3arg dirNormalized, float length )
{
	PROFILE_FUNCTION();

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
	PROFILE_FUNCTION();

	float length = 0;
	auto dir     = safeNormalize( end - start, &length );
	if( length != 0 ) {
		pushRay( stream, start, dir, length );
	}
}
void pushAabbOutline( MeshStream* stream, aabbarg box )
{
	PROFILE_FUNCTION();

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
	PROFILE_FUNCTION();

	auto pointSize = stream->pointSize;
	pushAabb( stream, point.x - pointSize, point.y - pointSize, point.z - pointSize,
	          point.x + pointSize, point.y + pointSize, point.z + pointSize );
}

void pushQuad( MeshStream* stream, vec3 quad[4],
               QuadTexCoordsArg texCoords = makeQuadTexCoordsDef() )
{
	PROFILE_FUNCTION();

	assert( isValid( stream ) );
	if( !hasCapacity( stream, 4, 6 ) ) {
		OutOfMemory();
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
	PROFILE_FUNCTION();

	assert( isValid( stream ) );
	if( !hasCapacity( stream, 4, 6 ) ) {
		OutOfMemory();
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
	PROFILE_FUNCTION();

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
	PROFILE_FUNCTION();

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
	PROFILE_FUNCTION();

	auto halfWidth = stream->lineWidth * 0.5f;
	pushQuad( stream, rectf{rect.left - halfWidth, rect.top, rect.left + halfWidth, rect.bottom},
	          z );
	pushQuad( stream, rectf{rect.right - halfWidth, rect.top, rect.right + halfWidth, rect.bottom},
	          z );
	pushQuad( stream, rectf{rect.left, rect.top + halfWidth, rect.right, rect.top - halfWidth}, z );
	pushQuad( stream,
	          rectf{rect.left, rect.bottom + halfWidth, rect.right, rect.bottom - halfWidth}, z );
}

uint16 pushLineStripVertexUnchecked( LineMeshStream* stream, float x, float y, float z = 0 )
{
	assert( isValid( stream ) );
	auto currentVerticesCount = safe_truncate< uint16 >( stream->data.verticesCount );
	stream->data.indices[stream->data.indicesCount++] = currentVerticesCount;
	stream->data.vertices[stream->data.verticesCount++] = {
	    {x, y, z}, stream->color, 0, 0, normal_neg_z_axis};
	return currentVerticesCount;
}
uint16 pushLineStripVertex( LineMeshStream* stream, float x, float y, float z = 0 )
{
	assert( isValid( stream ) );
	if( !hasCapacity( stream, 1, 1 ) ) {
		OutOfMemory();
		return 0;
	}
	auto currentVerticesCount = safe_truncate< uint16 >( stream->data.verticesCount );
	stream->data.indices[stream->data.indicesCount++] = currentVerticesCount;
	stream->data.vertices[stream->data.verticesCount++] = {
	    {x, y, z}, stream->color, 0, 0, normal_neg_z_axis};
	return currentVerticesCount;
}
uint16 pushLineStripVertex( LineMeshStream* stream, vec3arg v )
{
	return pushLineStripVertex( stream, v.x, v.y, v.z );
}
uint16 pushLineStripVertex( LineMeshStream* stream, vec2arg v, float z = 0 )
{
	return pushLineStripVertex( stream, v.x, v.y, z );
}
void pushEndLineStripUnchecked( LineMeshStream* stream )
{
	assert( isValid( stream ) );
	stream->data.indices[stream->data.indicesCount++] = MeshPrimitiveRestart;
}
void pushLineStripIndexUnchecked( LineMeshStream* stream, uint16 index )
{
	assert( isValid( stream ) );
	stream->data.indices[stream->data.indicesCount++] = index;
}
void pushEndLineStrip( LineMeshStream* stream )
{
	assert( isValid( stream ) );
	if( stream->data.indicesCapacity - stream->data.indicesCount < 1 ) {
		// TOOD: logging
		OutOfMemory();
		return;
	}
	stream->data.indices[stream->data.indicesCount++] = MeshPrimitiveRestart;
}
void pushLineStripIndex( LineMeshStream* stream, uint16 index )
{
	assert( isValid( stream ) );
	if( stream->data.indicesCapacity - stream->data.indicesCount < 1 ) {
		// TOOD: logging
		OutOfMemory();
		return;
	}
	stream->data.indices[stream->data.indicesCount++] = index;
}

void pushLine( LineMeshStream* stream, vec3arg start, vec3arg end )
{
	PROFILE_FUNCTION();

	if( !hasCapacity( stream, 2, 3 ) ) {
		OutOfMemory();
		return;
	}
	pushLineStripVertexUnchecked( stream, start.x, start.y, start.z );
	pushLineStripVertexUnchecked( stream, end.x, end.y, end.z );
	pushEndLineStripUnchecked( stream );
}
void pushLine2( LineMeshStream* stream, vec2arg start, vec2arg end )
{
	PROFILE_FUNCTION();

	if( !hasCapacity( stream, 2, 3 ) ) {
		OutOfMemory();
		return;
	}
	pushLineStripVertexUnchecked( stream, start.x, start.y, 0 );
	pushLineStripVertexUnchecked( stream, end.x, end.y, 0 );
	pushEndLineStripUnchecked( stream );
}
void pushAabbOutline( LineMeshStream* stream, aabbarg box )
{
	PROFILE_FUNCTION();

	if( !hasCapacity( stream, 8, 20 ) ) {
		OutOfMemory();
		return;
	}
	pushLineStripVertexUnchecked( stream, box.min.x, box.min.y, box.min.z );
	auto l0 = pushLineStripVertexUnchecked( stream, box.max.x, box.min.y, box.min.z );
	auto l1 = pushLineStripVertexUnchecked( stream, box.max.x, box.min.y, box.max.z );
	pushLineStripVertexUnchecked( stream, box.min.x, box.min.y, box.max.z );
	auto l2 = pushLineStripVertexUnchecked( stream, box.min.x, box.max.y, box.max.z );
	auto l3 = pushLineStripVertexUnchecked( stream, box.max.x, box.max.y, box.max.z );
	auto l4 = pushLineStripVertexUnchecked( stream, box.max.x, box.max.y, box.min.z );
	auto l5 = pushLineStripVertexUnchecked( stream, box.min.x, box.max.y, box.min.z );
	pushLineStripVertexUnchecked( stream, box.min.x, box.min.y, box.min.z );
	pushLineStripVertexUnchecked( stream, box.min.x, box.min.y, box.max.z );
	pushEndLineStrip( stream );

	pushLineStripIndexUnchecked( stream, l0 );
	pushLineStripIndexUnchecked( stream, l4 );
	pushEndLineStrip( stream );

	pushLineStripIndexUnchecked( stream, l1 );
	pushLineStripIndexUnchecked( stream, l3 );
	pushEndLineStrip( stream );

	pushLineStripIndexUnchecked( stream, l2 );
	pushLineStripIndexUnchecked( stream, l5 );
	pushEndLineStrip( stream );
}
void pushQuadOutline( LineMeshStream* stream, rectfarg rect, float z = 0 )
{
	PROFILE_FUNCTION();

	if( !hasCapacity( stream, 4, 6 ) ) {
		OutOfMemory();
		return;
	}
	auto start = pushLineStripVertexUnchecked( stream, rect.left, rect.top, z );
	pushLineStripVertexUnchecked( stream, rect.right, rect.top, z );
	pushLineStripVertexUnchecked( stream, rect.right, rect.bottom, z );
	pushLineStripVertexUnchecked( stream, rect.left, rect.bottom, z );
	pushLineStripIndexUnchecked( stream, start );
	pushEndLineStripUnchecked( stream );
}
void pushLines( LineMeshStream* stream, Array< vec3 > vertices )
{
	PROFILE_FUNCTION();

	if( !hasCapacity( stream, vertices.size(), vertices.size() + 1 ) ) {
		OutOfMemory();
		return;
	}
	FOR( entry : vertices ) {
		pushLineStripVertexUnchecked( stream, entry.x, entry.y, entry.z );
	}
	pushEndLineStripUnchecked( stream );
}
void pushLines( LineMeshStream* stream, Array< vec2 > vertices, float z = 0 )
{
	PROFILE_FUNCTION();

	if( !hasCapacity( stream, vertices.size(), vertices.size() + 1 ) ) {
		OutOfMemory();
		return;
	}
	FOR( entry : vertices ) {
		pushLineStripVertexUnchecked( stream, entry.x, entry.y, z );
	}
	pushEndLineStripUnchecked( stream );
}

struct RenderCommands {
	StackAllocator allocator;
	MatrixStack* matrixStack;
	mat4 view;
	float ambientStrength;
	Color lightColor;
	Color clearColor;
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
	result.clearColor      = Color::White;
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
	renderCommands->locked     = false;
	renderCommands->clearColor = Color::White;
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
RenderCommandLineMesh* getRenderCommandLineMesh( RenderCommandsStream* stream,
                                                 RenderCommandHeader* header )
{
	auto result = (RenderCommandLineMesh*)stream->ptr;
	assert_alignment( result, alignof( RenderCommandLineMesh ) );
	assert( header->next == sizeof( RenderCommandLineMesh ) );
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
		case RenderCommandEntryType::LineMesh: {
			getRenderCommandLineMesh( stream, header );
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

template< class T >
T* addRenderCommandMeshImpl( RenderCommands* renderCommands, int32 verticesCount,
                                         int32 indicesCount )
{
	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;

	allocateRenderCommandHeader( allocator, T );
	auto body = allocateStruct( allocator, T );

	auto startSize           = allocator->size;
	body->mesh.vertices      = allocateArray( allocator, Vertex, verticesCount );
	body->mesh.indices       = allocateArray( allocator, uint16, indicesCount );
	body->mesh.verticesCount = verticesCount;
	body->mesh.indicesCount  = indicesCount;
	auto endSize             = allocator->size;
	body->size               = safe_truncate< int32 >( endSize - startSize );

	return body;
}
RenderCommandMesh* addRenderCommandMesh( RenderCommands* renderCommands, int32 verticesCount,
                                         int32 indicesCount )
{
	return addRenderCommandMeshImpl< RenderCommandMesh >( renderCommands, verticesCount,
	                                                      indicesCount );
}
RenderCommandLineMesh* addRenderCommandLineMesh( RenderCommands* renderCommands,
                                                 int32 verticesCount, int32 indicesCount )
{
	return addRenderCommandMeshImpl< RenderCommandLineMesh >( renderCommands, verticesCount,
	                                                          indicesCount );
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
RenderCommandMesh* addRenderCommandMeshTransformed( RenderCommands* renderCommands,
                                                    const Mesh& mesh )
{
	PROFILE_FUNCTION();

	auto body = addRenderCommandMeshImpl< RenderCommandMesh >( renderCommands, mesh.verticesCount,
	                                                           mesh.indicesCount );
	auto& current = currentMatrix( renderCommands->matrixStack );
	auto vertices = body->mesh.vertices;
	for( auto i = 0, count = mesh.verticesCount; i < count; ++i ) {
		vertices[i]          = mesh.vertices[i];
		vertices[i].position = transformVector( current, mesh.vertices[i].position );
	}
	copy( body->mesh.indices, mesh.indices, body->mesh.indicesCount );
	return body;
}

RenderCommandStaticMesh* addRenderCommandMesh( RenderCommands* renderCommands, MeshId meshId )
{
	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;

	allocateRenderCommandHeader( allocator, RenderCommandStaticMesh );
	auto body               = allocateStruct( allocator, RenderCommandStaticMesh );
	*body                   = {};
	body->meshId            = meshId;
	body->matrix            = currentMatrix( renderCommands->matrixStack );
	body->flashColor        = renderCommands->flashColor;
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
LineMeshStream addRenderCommandLineMeshStream( RenderCommands* renderCommands, int32 verticesCount,
                                           int32 indicesCount )
{
	auto command = addRenderCommandLineMesh( renderCommands, verticesCount, indicesCount );
	LineMeshStream result;
	static_cast< MeshStream& >( result ) = makeMeshStream(
	    command->mesh.vertices, command->mesh.verticesCount, command->mesh.indices,
	    command->mesh.indicesCount, renderCommands->matrixStack, renderCommands->renderOptions );
	return result;
}

RenderCommandMesh* addRenderCommandSingleQuad( RenderCommands* renderCommands, rectfarg rect,
                                               float z = 0 )
{
	PROFILE_FUNCTION();

	auto color       = renderCommands->color;
	auto result      = addRenderCommandMesh( renderCommands, 4, 6 );
    result->mesh.vertices[0] = {{rect.left, rect.top, z}, color, 0, 0, normal_neg_z_axis};
    result->mesh.vertices[1] = {{rect.right, rect.top, z}, color, 1, 0, normal_neg_z_axis};
    result->mesh.vertices[2] = {{rect.left, rect.bottom, z}, color, 0, 1, normal_neg_z_axis};
    result->mesh.vertices[3] = {{rect.right, rect.bottom, z}, color, 1, 1, normal_neg_z_axis};

	result->mesh.indices[0] = 0;
	result->mesh.indices[1] = 1;
	result->mesh.indices[2] = 2;

	result->mesh.indices[3] = 2;
	result->mesh.indices[4] = 1;
	result->mesh.indices[5] = 3;
	return result;
}
RenderCommandMesh* addRenderCommandSingleQuad( RenderCommands* renderCommands, rectfarg rect,
                                               float z, QuadTexCoordsArg texCoords )
{
	PROFILE_FUNCTION();

	auto color       = renderCommands->color;
	auto result      = addRenderCommandMesh( renderCommands, 4, 6 );
	result->mesh.vertices[0] = {
	    {rect.left, rect.top, z}, color, texCoords.elements[0], normal_neg_z_axis};
	result->mesh.vertices[1] = {
	    {rect.right, rect.top, z}, color, texCoords.elements[1], normal_neg_z_axis};
	result->mesh.vertices[2] = {
	    {rect.left, rect.bottom, z}, color, texCoords.elements[2], normal_neg_z_axis};
	result->mesh.vertices[3] = {
	    {rect.right, rect.bottom, z}, color, texCoords.elements[3], normal_neg_z_axis};

	result->mesh.indices[0] = 0;
	result->mesh.indices[1] = 1;
	result->mesh.indices[2] = 2;

	result->mesh.indices[3] = 2;
	result->mesh.indices[4] = 1;
	result->mesh.indices[5] = 3;
	return result;
}
RenderCommandMesh* addRenderCommandSingleQuad( RenderCommands* renderCommands, vec3 quad[4],
                                               QuadTexCoordsArg texCoords = makeQuadTexCoordsDef() )
{
	PROFILE_FUNCTION();

	// TODO: calculate normal?
	auto color               = renderCommands->color;
	auto result              = addRenderCommandMesh( renderCommands, 4, 6 );
	result->mesh.vertices[0] = {quad[0], color, texCoords.elements[0], normal_neg_z_axis};
	result->mesh.vertices[1] = {quad[1], color, texCoords.elements[1], normal_neg_z_axis};
	result->mesh.vertices[2] = {quad[2], color, texCoords.elements[2], normal_neg_z_axis};
	result->mesh.vertices[3] = {quad[3], color, texCoords.elements[3], normal_neg_z_axis};

	result->mesh.indices[0] = 0;
	result->mesh.indices[1] = 1;
	result->mesh.indices[2] = 2;

	result->mesh.indices[3] = 2;
	result->mesh.indices[4] = 1;
	result->mesh.indices[5] = 3;
	return result;
}

template < class Stream, class Command >
struct MeshStreamingBlock {
	Stream stream;
	Command* meshCommand;
	inline explicit operator bool() const { return meshCommand != nullptr; }
};

template < class Stream, class Command >
MeshStreamingBlock< Stream, Command > beginMeshStreamingImpl( RenderCommands* renderCommands )
{
	PROFILE_FUNCTION();

	assert( isValid( renderCommands ) );
	assert( !renderCommands->locked );
	auto allocator = &renderCommands->allocator;

	allocateRenderCommandHeader( allocator, Command );
	auto body = allocateStruct( allocator, Command );

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
	MeshStreamingBlock< Stream, Command > result;
	static_cast< MeshStream& >( result.stream ) = makeMeshStream(
	    body->mesh.vertices, body->mesh.verticesCount, body->mesh.indices, body->mesh.indicesCount,
	    renderCommands->matrixStack, renderCommands->renderOptions );
	result.meshCommand = body;
	return result;
}
template < class Stream, class Command >
void endMeshStreamingImpl( RenderCommands* renderCommands,
                           MeshStreamingBlock< Stream, Command >* block )
{
	PROFILE_FUNCTION();

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
MeshStreamingBlock< MeshStream, RenderCommandMesh > beginMeshStreaming(
    RenderCommands* renderCommands )
{
	return beginMeshStreamingImpl< MeshStream, RenderCommandMesh >( renderCommands );
}
void endMeshStreaming( RenderCommands* renderCommands,
                       MeshStreamingBlock< MeshStream, RenderCommandMesh >* block )
{
	endMeshStreamingImpl( renderCommands, block );
}
MeshStreamingBlock< LineMeshStream, RenderCommandLineMesh > beginLineMeshStreaming(
    RenderCommands* renderCommands )
{
	return beginMeshStreamingImpl< LineMeshStream, RenderCommandLineMesh >( renderCommands );
}
void endLineMeshStreaming( RenderCommands* renderCommands,
                           MeshStreamingBlock< LineMeshStream, RenderCommandLineMesh >* block )
{
	endMeshStreamingImpl( renderCommands, block );
}

template < class Stream, class Command >
struct MeshStreamGuard {
	MeshStreamingBlock< Stream, Command > block;
	RenderCommands* renderCommands;

	MeshStreamGuard( RenderCommands* renderCommands )
	: block( beginMeshStreamingImpl< Stream, Command >( renderCommands ) ),
	  renderCommands( renderCommands )
	{
		assert( renderCommands && renderCommands->locked );
	}
	MeshStreamGuard( RenderCommands* renderCommands,
	                 const MeshStreamingBlock< Stream, Command >& block )
	: block( block ), renderCommands( renderCommands )
	{
		assert( renderCommands && renderCommands->locked );
	}
	~MeshStreamGuard() { endMeshStreamingImpl( renderCommands, &block ); }
};

#define MESH_STREAM_BLOCK( name, renderer )                                                   \
	if( auto _once = false ) {                                                                \
	} else                                                                                    \
		for( MeshStreamGuard< MeshStream, RenderCommandMesh > _guard( ( renderer ) ); !_once; \
		     _once = true )                                                                   \
			for( auto name = &_guard.block.stream; !_once; _once = true )

#define LINE_MESH_STREAM_BLOCK( name, renderer )                                              \
	if( auto _once = false ) {                                                                \
	} else                                                                                    \
		for( MeshStreamGuard< LineMeshStream, RenderCommandLineMesh > _guard( ( renderer ) ); \
		     !_once; _once = true )                                                           \
			for( auto name = &_guard.block.stream; !_once; _once = true )

void setTexture( RenderCommands* renderCommands, int32 textureStage, TextureId texture )
{
	PROFILE_FUNCTION();

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

void setShader( RenderCommands* renderCommands, ShaderId shader )
{
	PROFILE_FUNCTION();

	assert( isValid( renderCommands ) );

	auto allocator = &renderCommands->allocator;

	allocateRenderCommandHeader( allocator, RenderCommandSetShader );
	auto body = allocateStruct( allocator, RenderCommandSetShader );
	body->id  = shader;
}
void setShader( RenderCommands* renderCommands, null_t )
{
	setShader( renderCommands, ShaderId{} );
}

void setProjection( RenderCommands* renderCommands, ProjectionType type )
{
	PROFILE_FUNCTION();

	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;
	allocateRenderCommandHeader( allocator, RenderCommandSetProjection );
	auto body            = allocateStruct( allocator, RenderCommandSetProjection );
	body->projectionType = type;
}
void setProjectionMatrix( RenderCommands* renderCommands, ProjectionType type, mat4arg matrix )
{
	PROFILE_FUNCTION();

	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;
	allocateRenderCommandHeader( allocator, RenderCommandSetProjectionMatrix );
	auto body            = allocateStruct( allocator, RenderCommandSetProjectionMatrix );
	body->projectionType = type;
	body->matrix         = matrix;
}
void setScissorRect( RenderCommands* renderCommands, rectiarg rect )
{
	PROFILE_FUNCTION();
	assert( isValid( renderCommands ) );
	auto allocator = &renderCommands->allocator;
	allocateRenderCommandHeader( allocator, RenderCommandSetScissorRect );
	auto body     = allocateStruct( allocator, RenderCommandSetScissorRect );
	body->scissor = rect;
}
void setRenderState( RenderCommands* renderCommands, RenderStateType type, bool enabled )
{
	PROFILE_FUNCTION();

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
	PROFILE_FUNCTION();

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
	PROFILE_FUNCTION();

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
	PROFILE_FUNCTION();

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
void clip( RenderCommandsStream stream, rectfarg rect )
{
	PROFILE_FUNCTION();

	while( stream.size ) {
		auto header = getRenderCommandsHeader( &stream );
		switch( header->type ) {
			case RenderCommandEntryType::Mesh: {
				auto body = getRenderCommandMesh( &stream, header );
				clip( &body->mesh, rect );
				break;
			}
			default: {
				skipRenderCommandBody( &stream, header );
				break;
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
		::clip( stream, rect );
		clipped = true;
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
