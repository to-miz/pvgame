#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_TYPE_RGBA_ARB                 0x202B

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define ERROR_INVALID_VERSION_ARB               0x2095
#define ERROR_INVALID_PROFILE_ARB               0x2096

/*GL 1.2*/
#define GL_UNSIGNED_BYTE_3_3_2            0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_INT_8_8_8_8           0x8035
#define GL_UNSIGNED_INT_10_10_10_2        0x8036
#define GL_TEXTURE_BINDING_3D             0x806A
#define GL_PACK_SKIP_IMAGES               0x806B
#define GL_PACK_IMAGE_HEIGHT              0x806C
#define GL_UNPACK_SKIP_IMAGES             0x806D
#define GL_UNPACK_IMAGE_HEIGHT            0x806E
#define GL_TEXTURE_3D                     0x806F
#define GL_PROXY_TEXTURE_3D               0x8070
#define GL_TEXTURE_DEPTH                  0x8071
#define GL_TEXTURE_WRAP_R                 0x8072
#define GL_MAX_3D_TEXTURE_SIZE            0x8073
#define GL_UNSIGNED_BYTE_2_3_3_REV        0x8362
#define GL_UNSIGNED_SHORT_5_6_5           0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV       0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV     0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV     0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#define GL_BGR                            0x80E0
#define GL_BGRA                           0x80E1
#define GL_MAX_ELEMENTS_VERTICES          0x80E8
#define GL_MAX_ELEMENTS_INDICES           0x80E9
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE_MIN_LOD                0x813A
#define GL_TEXTURE_MAX_LOD                0x813B
#define GL_TEXTURE_BASE_LEVEL             0x813C
#define GL_TEXTURE_MAX_LEVEL              0x813D
#define GL_SMOOTH_POINT_SIZE_RANGE        0x0B12
#define GL_SMOOTH_POINT_SIZE_GRANULARITY  0x0B13
#define GL_SMOOTH_LINE_WIDTH_RANGE        0x0B22
#define GL_SMOOTH_LINE_WIDTH_GRANULARITY  0x0B23
#define GL_ALIASED_LINE_WIDTH_RANGE       0x846E

/*GL 3.0*/
#define GL_RGBA_INTEGER                   0x8D99

/* GL 3.1 */
#define GL_PRIMITIVE_RESTART              0x8F9D

/**/
#define GL_CLAMP_TO_BORDER                0x812D
#define GL_TEXTURE0                       0x84C0

#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_DELETE_STATUS                  0x8B80
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

#define GL_BUFFER_SIZE                    0x8764
#define GL_BUFFER_USAGE                   0x8765
#define GL_QUERY_COUNTER_BITS             0x8864
#define GL_CURRENT_QUERY                  0x8865
#define GL_QUERY_RESULT                   0x8866
#define GL_QUERY_RESULT_AVAILABLE         0x8867
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_ARRAY_BUFFER_BINDING           0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING   0x8895
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA
#define GL_BUFFER_ACCESS                  0x88BB
#define GL_BUFFER_MAPPED                  0x88BC
#define GL_BUFFER_MAP_POINTER             0x88BD
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_SAMPLES_PASSED                 0x8914
#define GL_SRC1_ALPHA                     0x8589

#define GL_MAP_READ_BIT                   0x0001
#define GL_MAP_WRITE_BIT                  0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT       0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT         0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT         0x0020

/* GL 3.3 */
#define GL_INT_2_10_10_10_REV             0x8D9F

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

typedef void APIENTRY gl_attach_shader( GLuint program, GLuint shader );
typedef void APIENTRY gl_compile_shader( GLuint shader );
typedef GLuint APIENTRY gl_create_shader( GLenum type );
typedef void APIENTRY gl_delete_shader( GLuint shader );
typedef void APIENTRY gl_detach_shader( GLuint program, GLuint shader );

typedef GLuint APIENTRY gl_create_program( void );
typedef void APIENTRY gl_delete_program( GLuint program );
typedef void APIENTRY gl_link_program( GLuint program );
typedef void APIENTRY gl_use_program( GLuint program );
typedef void APIENTRY gl_validate_program( GLuint program );

typedef void APIENTRY gl_gen_buffers( GLsizei n, GLuint* buffers );
typedef void APIENTRY gl_bind_buffer( GLenum target, GLuint buffer );
typedef void APIENTRY gl_buffer_data( GLenum target, GLsizeiptr size, const void* data,
                                      GLenum usage );
typedef void APIENTRY gl_vertex_attrib_pointer( GLuint index, GLint size, GLenum type,
                                                GLboolean normalized, GLsizei stride,
                                                const void* pointer );
typedef void APIENTRY gl_enable_vertex_attrib_array( GLuint index );
typedef void APIENTRY gl_shader_source( GLuint shader, GLsizei count, const GLchar* const* string,
                                        const GLint* length );
typedef void APIENTRY gl_draw_arrays_indirect( GLenum mode, const void* indirect );
typedef void APIENTRY gl_draw_arrays_instanced( GLenum mode, GLint first, GLsizei count,
                                                GLsizei instancecount );
typedef void APIENTRY gl_draw_arrays( GLenum mode, GLint first, GLsizei count );
typedef void APIENTRY gl_get_shader_iv( GLuint shader, GLenum pname, GLint* params );
typedef void APIENTRY gl_bind_attrib_location( GLuint program, GLuint index, const GLchar* name );
typedef void APIENTRY gl_get_program_iv( GLuint program, GLenum pname, GLint* params );

typedef void APIENTRY gl_uniform1f( GLint location, GLfloat v0 );
typedef void APIENTRY gl_uniform2f( GLint location, GLfloat v0, GLfloat v1 );
typedef void APIENTRY gl_uniform3f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
typedef void APIENTRY gl_uniform4f( GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
                                    GLfloat v3 );
typedef void APIENTRY gl_uniform1i( GLint location, GLint v0 );
typedef void APIENTRY gl_uniform2i( GLint location, GLint v0, GLint v1 );
typedef void APIENTRY gl_uniform3i( GLint location, GLint v0, GLint v1, GLint v2 );
typedef void APIENTRY gl_uniform4i( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
typedef void APIENTRY gl_uniform1fv( GLint location, GLsizei count, const GLfloat* value );
typedef void APIENTRY gl_uniform2fv( GLint location, GLsizei count, const GLfloat* value );
typedef void APIENTRY gl_uniform3fv( GLint location, GLsizei count, const GLfloat* value );
typedef void APIENTRY gl_uniform4fv( GLint location, GLsizei count, const GLfloat* value );
typedef void APIENTRY gl_uniform1iv( GLint location, GLsizei count, const GLint* value );
typedef void APIENTRY gl_uniform2iv( GLint location, GLsizei count, const GLint* value );
typedef void APIENTRY gl_uniform3iv( GLint location, GLsizei count, const GLint* value );
typedef void APIENTRY gl_uniform4iv( GLint location, GLsizei count, const GLint* value );
typedef void APIENTRY gl_uniform_matrix2fv( GLint location, GLsizei count, GLboolean transpose,
                                            const GLfloat* value );
typedef void APIENTRY gl_uniform_matrix3fv( GLint location, GLsizei count, GLboolean transpose,
                                            const GLfloat* value );
typedef void APIENTRY gl_uniform_matrix4fv( GLint location, GLsizei count, GLboolean transpose,
                                            const GLfloat* value );
typedef GLint APIENTRY gl_get_uniform_location( GLuint program, const GLchar* name );

gl_attach_shader* glAttachShader       = nullptr;
gl_compile_shader* glCompileShader     = nullptr;
gl_create_shader* glCreateShader       = nullptr;
gl_delete_shader* glDeleteShader       = nullptr;
gl_detach_shader* glDetachShader       = nullptr;
gl_create_program* glCreateProgram     = nullptr;
gl_delete_program* glDeleteProgram     = nullptr;
gl_link_program* glLinkProgram         = nullptr;
gl_use_program* glUseProgram           = nullptr;
gl_validate_program* glValidateProgram = nullptr;

gl_gen_buffers* glGenBuffers                             = nullptr;
gl_bind_buffer* glBindBuffer                             = nullptr;
gl_buffer_data* glBufferData                             = nullptr;
gl_vertex_attrib_pointer* glVertexAttribPointer          = nullptr;
gl_enable_vertex_attrib_array* glEnableVertexAttribArray = nullptr;
gl_shader_source* glShaderSource                         = nullptr;
gl_draw_arrays_indirect* glDrawArraysIndirect            = nullptr;
gl_draw_arrays_instanced* glDrawArraysInstanced          = nullptr;

gl_get_shader_iv* glGetShaderiv               = nullptr;
gl_bind_attrib_location* glBindAttribLocation = nullptr;
gl_get_program_iv* glGetProgramiv             = nullptr;

gl_uniform1f* glUniform1f                     = nullptr;
gl_uniform2f* glUniform2f                     = nullptr;
gl_uniform3f* glUniform3f                     = nullptr;
gl_uniform4f* glUniform4f                     = nullptr;
gl_uniform1i* glUniform1i                     = nullptr;
gl_uniform2i* glUniform2i                     = nullptr;
gl_uniform3i* glUniform3i                     = nullptr;
gl_uniform4i* glUniform4i                     = nullptr;
gl_uniform1fv* glUniform1fv                   = nullptr;
gl_uniform2fv* glUniform2fv                   = nullptr;
gl_uniform3fv* glUniform3fv                   = nullptr;
gl_uniform4fv* glUniform4fv                   = nullptr;
gl_uniform1iv* glUniform1iv                   = nullptr;
gl_uniform2iv* glUniform2iv                   = nullptr;
gl_uniform3iv* glUniform3iv                   = nullptr;
gl_uniform4iv* glUniform4iv                   = nullptr;
gl_uniform_matrix2fv* glUniformMatrix2fv      = nullptr;
gl_uniform_matrix3fv* glUniformMatrix3fv      = nullptr;
gl_uniform_matrix4fv* glUniformMatrix4fv      = nullptr;
gl_get_uniform_location* glGetUniformLocation = nullptr;

typedef void APIENTRY glDeleteBuffersType( GLsizei n, const GLuint* buffers );
glDeleteBuffersType* glDeleteBuffers = nullptr;

typedef void APIENTRY gl_bind_vertex_array( GLuint array );
gl_bind_vertex_array* glBindVertexArray = nullptr;
typedef void APIENTRY gl_delete_vertex_arrays( GLsizei n, const GLuint* arrays );
gl_delete_vertex_arrays* glDeleteVertexArrays = nullptr;
typedef void APIENTRY gl_gen_vertex_arrays( GLsizei n, GLuint* arrays );
gl_gen_vertex_arrays* glGenVertexArrays = nullptr;

typedef GLint APIENTRY gl_get_attrib_location( GLuint program, const GLchar* name );
gl_get_attrib_location* glGetAttribLocation = nullptr;

typedef void APIENTRY glBindFragDataLocationType( GLuint program, GLuint color,
												  const GLchar* name );
glBindFragDataLocationType* glBindFragDataLocation = nullptr;

typedef void( APIENTRY* GLDEBUGPROC )( GLenum source, GLenum type, GLuint id, GLenum severity,
									   GLsizei length, const GLchar* message,
									   const void* userParam );

typedef void APIENTRY glDebugMessageCallbackType( GLDEBUGPROC callback, const void* userParam );
glDebugMessageCallbackType* glDebugMessageCallback = nullptr;

typedef void* APIENTRY glMapBufferType( GLenum target, GLenum access );
glMapBufferType* glMapBuffer = nullptr;
typedef GLboolean APIENTRY glUnmapBufferType( GLenum target );
glUnmapBufferType* glUnmapBuffer = nullptr;

typedef void* APIENTRY glMapBufferRangeType( GLenum target, GLintptr offset, GLsizeiptr length,
											 GLbitfield access );
glMapBufferRangeType* glMapBufferRange = nullptr;
typedef void APIENTRY glFlushMappedBufferRangeType( GLenum target, GLintptr offset,
													GLsizeiptr length );
glFlushMappedBufferRangeType* glFlushMappedBufferRange = nullptr;

typedef void APIENTRY glActiveTextureType( GLenum texture );
glActiveTextureType* glActiveTexture = nullptr;

typedef void APIENTRY glVertexAttribP4uiType( GLuint index, GLenum type, GLboolean normalized,
                                              GLuint value );
glVertexAttribP4uiType* glVertexAttribP4ui = nullptr;

typedef void APIENTRY glVertexAttrib4fType( GLuint index, GLfloat x, GLfloat y, GLfloat z,
                                            GLfloat w );
glVertexAttrib4fType* glVertexAttrib4f = nullptr;

// Context functions
typedef HGLRC APIENTRY wgl_create_context_attribs_arb( HDC hDC, HGLRC hShareContext,
													   const int* attribList );
wgl_create_context_attribs_arb* wglCreateContextAttribsARB = nullptr;

typedef BOOL APIENTRY wgl_choose_pixel_format_arb( HDC hdc, const int* piAttribIList,
												   const FLOAT* pfAttribFList, UINT nMaxFormats,
												   int* piFormats, UINT* nNumFormats );
wgl_choose_pixel_format_arb* wglChoosePixelFormatARB = nullptr;

typedef BOOL APIENTRY wglSwapIntervalEXTType( int interval );
wglSwapIntervalEXTType* wglSwapIntervalEXT = nullptr;

/* GL 3.1 */
typedef void APIENTRY glPrimitiveRestartIndexType( GLuint index );
glPrimitiveRestartIndexType* glPrimitiveRestartIndex = nullptr;

/* GL 3.2 */
typedef void APIENTRY glDrawElementsBaseVertexType( GLenum mode, GLsizei count, GLenum type,
                                                    const void* indices, GLint basevertex );
glDrawElementsBaseVertexType* glDrawElementsBaseVertex = nullptr;
typedef void APIENTRY glDrawRangeElementsBaseVertexType( GLenum mode, GLuint start, GLuint end,
                                                         GLsizei count, GLenum type,
                                                         const void* indices, GLint basevertex );
glDrawRangeElementsBaseVertexType* glDrawRangeElementsBaseVertex = nullptr;

// utility
#define BUFFER_OFFSET( i ) ( ( (char*)nullptr ) + ( i ) )

static bool win32BindOpenGlFunctions()
{
	auto instance			  = GetModuleHandle( nullptr );
	WNDCLASSW windowClass	 = {};
	windowClass.lpfnWndProc   = DefWindowProc;
	windowClass.hInstance	 = instance;
	windowClass.lpszClassName = L"fake_window_class_only_for_binding_opengl_functions";

	auto atom = RegisterClassW( &windowClass );
	if( !atom ) {
		return false;
	}
	SCOPE_EXIT( & ) {
		UnregisterClassW( MAKEINTATOM( atom ), instance );
	};
	auto hwnd = CreateWindowExW( 0, MAKEINTATOM( atom ), L"", 0, 0, 0, 0, 0, nullptr, nullptr,
								 instance, 0 );
	if( !hwnd ) {
		return false;
	}
	auto hdc = GetDC( hwnd );
	SCOPE_EXIT( & ) {
		ReleaseDC( hwnd, hdc );
		DestroyWindow( hwnd );
	};

	PIXELFORMATDESCRIPTOR pfd = {};
	pfd.nSize				  = sizeof( PIXELFORMATDESCRIPTOR );
	pfd.nVersion			  = 1;
	pfd.dwFlags				  = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType			  = PFD_TYPE_RGBA;
	pfd.cColorBits			  = 32;
	pfd.cAlphaBits			  = 8;
	pfd.iLayerType			  = PFD_MAIN_PLANE;
	pfd.cDepthBits			  = 24;
	pfd.cStencilBits		  = 8;

	auto pixelFormat = ChoosePixelFormat( hdc, &pfd );
	if( !SetPixelFormat( hdc, pixelFormat, &pfd ) ) {
		LOG( ERROR, "Failed to set dummy pixel format" );
		return false;
	}

	auto openglContext = wglCreateContext( hdc );
	if( !openglContext ) {
		return false;
	}
	SCOPE_EXIT( & ) {
		wglMakeCurrent( nullptr, nullptr );
		wglDeleteContext( openglContext );
	};
	if( !wglMakeCurrent( hdc, openglContext ) ) {
		return false;
	}

	wglCreateContextAttribsARB =
		(wgl_create_context_attribs_arb*)wglGetProcAddress( "wglCreateContextAttribsARB" );
	wglChoosePixelFormatARB =
		(wgl_choose_pixel_format_arb*)wglGetProcAddress( "wglChoosePixelFormatARB" );
	if( !wglCreateContextAttribsARB || !wglChoosePixelFormatARB ) {
		return false;
	}

	return true;
}

static void APIENTRY win32OpenGlDebugCallback( GLenum source, GLenum type, GLuint id,
                                               GLenum severity, GLsizei length,
                                               const GLchar* message, const void* userParam );

enum OpenGlAttributeLocations {
	AL_position,
	AL_color,
	AL_texCoords0,
	AL_normal0,
};

struct OpenGlIngameShader {
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint program;

	// shader uniform values
	struct {
		vec3 lightPosition;
		vec4 lightColor;
		float ambientStrength;
	} values;

	// uniform locations
	GLint worldViewProj;
	GLint model;
	GLint screenDepthOffset;
	GLint ambientStrength;
	GLint lightColor;
	GLint lightPosition;
};
struct OpenGlNoLightingShader {
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint program;

	// uniform locations
	GLint worldViewProj;
	GLint screenDepthOffset;
};
struct OpenGlVertexBuffer {
	GLuint vertexArrayObjectId;
	GLuint vertexBufferId;
	GLuint indexBufferId;
	Vertex* vertices;  // these get mapped to the gpu
	uint16* indices;   // these get mapped to the gpu
	GLsizei lastVerticesCount;
	GLsizei verticesCount;
	GLsizei verticesCapacity;
	GLsizei lastIndicesCount;
	GLsizei indicesCount;
	GLsizei indicesCapacity;

	bool mappedBuffers;
};
struct OpenGlMesh {
	GLuint vertexArrayObjectId;
	GLuint vertexBufferId;
	GLuint indexBufferId;
	int32 verticesCount;
	int32 indicesCount;
};
struct OpenGlContext {
	HGLRC renderContext;
	float width;
	float height;

	// uniform locations
	GLint worldViewProj;
	GLint model;
	GLint screenDepthOffset;

	OpenGlIngameShader shader;
	OpenGlNoLightingShader noLightingShader;
	GLuint plainWhiteTexture;  // TODO: move this to the atlas texture

	GLuint currentTextures[2];
	GLuint currentProgram;
	ProjectionType currentProjectionType;
	mat4 projections[2];
	bool8 renderStates[(int)RenderStateType::Count];

	OpenGlVertexBuffer dynamicBuffer;

	UArray< OpenGlMesh > meshes;
};

MeshId win32UploadMeshToGpu( Mesh mesh )
{
	MeshId result = {};
	auto context  = Win32AppContext.openGlContext;
	assert( context->meshes.remaining() );
	if( context->meshes.remaining() ) {
		auto dest = context->meshes.emplace_back();
		result.id = context->meshes.size();
		glGenVertexArrays( 1, &dest->vertexArrayObjectId );
		glBindVertexArray( dest->vertexArrayObjectId );
		glGenBuffers( 1, &dest->vertexBufferId );
		glGenBuffers( 1, &dest->indexBufferId );
		glBindBuffer( GL_ARRAY_BUFFER, dest->vertexBufferId );
		glBufferData( GL_ARRAY_BUFFER, mesh.verticesCount * sizeof( Vertex ), mesh.vertices,
		              GL_STATIC_DRAW );
		glEnableVertexAttribArray( AL_position );
		glVertexAttribPointer( AL_position, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), 0 );
		glEnableVertexAttribArray( AL_color );
		glVertexAttribPointer( AL_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( Vertex ),
		                       (void*)offsetof( Vertex, color ) );
		glEnableVertexAttribArray( AL_texCoords0 );
		glVertexAttribPointer( AL_texCoords0, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ),
		                       (void*)offsetof( Vertex, texCoords ) );
		glEnableVertexAttribArray( AL_normal0 );
		glVertexAttribPointer( AL_normal0, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof( Vertex ),
		                       (void*)offsetof( Vertex, normal ) );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, dest->indexBufferId );
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, mesh.indicesCount * sizeof( uint16 ), mesh.indices,
		              GL_STATIC_DRAW );
		assert( dest->vertexArrayObjectId != 0 );

		dest->verticesCount = mesh.verticesCount;
		dest->indicesCount  = mesh.indicesCount;
	}
	return result;
}

static OpenGlVertexBuffer win32InitOpenGlVertexBuffer( GLsizei verticesCapacity,
                                                       GLsizei indicesCapacity )
{
	assert( verticesCapacity > 0 );
	assert( indicesCapacity > 0 );
	OpenGlVertexBuffer result = {};
	glGenVertexArrays( 1, &result.vertexArrayObjectId );
	glBindVertexArray( result.vertexArrayObjectId );
	glGenBuffers( 1, &result.vertexBufferId );
	glGenBuffers( 1, &result.indexBufferId );
	result.verticesCapacity = verticesCapacity;
	result.indicesCapacity  = indicesCapacity;
	return result;
}
static void win32MapBuffers( OpenGlVertexBuffer* vb )
{
	assert( !vb->mappedBuffers );
	vb->lastVerticesCount = 0;
	vb->verticesCount = 0;
	vb->lastIndicesCount  = 0;
	vb->indicesCount  = 0;

	GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
	glBindVertexArray( vb->vertexArrayObjectId );
	glBindBuffer( GL_ARRAY_BUFFER, vb->vertexBufferId );
	GLsizeiptr vertexBufferSize = vb->verticesCapacity * sizeof( Vertex );
	glBufferData( GL_ARRAY_BUFFER, vertexBufferSize, nullptr, GL_STREAM_DRAW );
	vb->vertices = (Vertex*)glMapBufferRange( GL_ARRAY_BUFFER, 0, vertexBufferSize, access );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vb->indexBufferId );
	GLsizeiptr indexBufferSize = vb->indicesCapacity * sizeof( uint16 );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, nullptr, GL_STREAM_DRAW );
	vb->indices = (uint16*)glMapBufferRange( GL_ELEMENT_ARRAY_BUFFER, 0, indexBufferSize, access );

	assert( vb->vertices && vb->indices );
	assert_alignment( vb->vertices, alignof( Vertex ) );
	assert_alignment( vb->indices, alignof( uint16 ) );
	vb->mappedBuffers = true;
}
static void win32MapBuffersRange( OpenGlVertexBuffer* vb )
{
	vb->lastVerticesCount += vb->verticesCount;
	vb->lastIndicesCount += vb->indicesCount;
	vb->verticesCount = 0;
	vb->indicesCount = 0;

	GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
	glBindVertexArray( vb->vertexArrayObjectId );
	glBindBuffer( GL_ARRAY_BUFFER, vb->vertexBufferId );
	GLsizeiptr vertexBufferSize =
	    ( vb->verticesCapacity - vb->lastVerticesCount ) * sizeof( Vertex );
	GLintptr vertexBufferOffset = vb->lastVerticesCount * sizeof( Vertex );
	vb->vertices =
	    (Vertex*)glMapBufferRange( GL_ARRAY_BUFFER, vertexBufferOffset, vertexBufferSize, access );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vb->indexBufferId );
	GLsizeiptr indexBufferSize = ( vb->indicesCapacity - vb->lastIndicesCount ) * sizeof( uint16 );
	GLintptr indexBufferOffset = vb->lastIndicesCount * sizeof( uint16 );
	vb->indices = (uint16*)glMapBufferRange( GL_ELEMENT_ARRAY_BUFFER, indexBufferOffset,
	                                         indexBufferSize, access );

	assert( vb->vertices && vb->indices );
	assert_alignment( vb->vertices, alignof( Vertex ) );
	assert_alignment( vb->indices, alignof( uint16 ) );
	vb->mappedBuffers = true;
}
static void win32UnmapBuffers( OpenGlVertexBuffer* vb )
{
	assert( vb->mappedBuffers );
	glBindVertexArray( vb->vertexArrayObjectId );
	DEBUG_WRAP( auto result0 = ) glUnmapBuffer( GL_ARRAY_BUFFER );
	DEBUG_WRAP( auto result1 = ) glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );
	assert( result0 != GL_FALSE && result1 != GL_FALSE );
	vb->mappedBuffers = false;
	vb->vertices      = nullptr;
	vb->indices       = nullptr;
}
static void win32RenderBuffers( OpenGlContext* context, mat4* projections, GLenum mode )
{
	auto vb = &context->dynamicBuffer;
	win32UnmapBuffers( vb );
	auto current = &projections[valueof( context->currentProjectionType )];
	glUniformMatrix4fv( context->worldViewProj, 1, GL_FALSE, current->m );
	auto identity = matrixIdentity();
	glUniformMatrix4fv( context->model, 1, GL_FALSE, identity.m );
	glUniform1f( context->screenDepthOffset, 0 );
	glDrawElementsBaseVertex( mode, vb->indicesCount, GL_UNSIGNED_SHORT,
	                          BUFFER_OFFSET( vb->lastIndicesCount * sizeof( uint16 ) ),
	                          vb->lastVerticesCount );

	++Win32AppContext.info->drawCalls;
	Win32AppContext.info->vertices += vb->verticesCount;
	Win32AppContext.info->indices += vb->indicesCount;
}
static void win32RenderAndFlushBuffers( OpenGlContext* context, mat4* projections, GLenum mode )
{
	if( context->dynamicBuffer.indicesCount ) {
		assert( context );
		win32RenderBuffers( context, projections, mode );
		win32MapBuffersRange( &context->dynamicBuffer );
	}
}
static void win32RenderAndResetBuffers( OpenGlContext* context, mat4* projections, GLenum mode )
{
	assert( context );
	win32RenderBuffers( context, projections, mode );
	win32MapBuffers( &context->dynamicBuffer );
}

static bool win32VertexBufferHasSpace( OpenGlVertexBuffer* vb, GLsizei verticesCount,
                                       GLsizei indicesCount )
{
	auto remainingVerticesCount =
	    vb->verticesCapacity - ( vb->lastVerticesCount + vb->verticesCount );
	auto remainingIndicesCount = vb->indicesCapacity - ( vb->lastIndicesCount + vb->indicesCount );
	return verticesCount <= remainingVerticesCount && indicesCount <= remainingIndicesCount;
}

TextureId toTextureId( GLuint id ) { return {(int32)id}; }
GLuint toOpenGlId( TextureId id ) { return (GLuint)id.id ;}
GLuint toOpenGlId( ShaderId id ) { return (GLuint)id.id ;}

static GLuint win32UploadImageToGpu( ImageData image )
{
	assert( Win32AppContext.window.hwnd );

	/*auto tempRow = new uint8[image.width];
	// we need to flip the image upside down because opengl wants the lower left corner of the image
	// for glTexImage2D
	for( intmax y = 0; y < image.height / 2; ++y ) {
		auto current = image.data + y * image.width;
		auto last = image.data + ( image.height - y - 2 ) * image.width;
		memcpy( tempRow, current, image.width * sizeof( uint8 ) );
		memcpy( current, last, image.width * sizeof( uint8 ) );
		memcpy( last, tempRow, image.width * sizeof( uint8 ) );
	}
	delete[] tempRow; */
	// change rgba to argb
#if 0
	// turn rgba to argb
	for( intmax y = 0; y < image.height; ++y ) {
		for( intmax x = 0; x < image.width; ++x ) {
			auto current = image.data + ( x + y * image.width ) * 4;
			auto r = current[0];
			auto g = current[1];
			auto b = current[2];
			auto a = current[3];

			current[0] = a;
			current[1] = r;
			current[2] = g;
			current[3] = b;
		}
	}
#endif

	// TODO: change this to upload to an atlas instead
	GLuint result = 0;
	glGenTextures( 1, &result );
	if( result != 0 ) {
		glBindTexture( GL_TEXTURE_2D, result );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0, GL_RGBA,
					  GL_UNSIGNED_BYTE, image.data );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}
	return result;
}

static GLuint win32CreatePlainWhiteTexture()
{
	const int32 width  = 64;
	const int32 height = 64;
	uint8 rawData[width * height * 4];
	for( intmax i = 0; i < countof( rawData ); ++i ) {
		rawData[i] = 0xFF;
	}
	ImageData image = {rawData, width, height};
	return win32UploadImageToGpu( image );
}

static OpenGlContext win32CreateOpenGlContext( StackAllocator* allocator, HDC hdc )
{
	// TODO: check gpu capabilities
	// TODO: GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS needs to be at least 2

	assert( wglCreateContextAttribsARB );
	assert( wglChoosePixelFormatARB );
	OpenGlContext result = {};

	const int32 MaxOpenGlMeshCount = 100;
	result.meshes                  = makeUArray( allocator, OpenGlMesh, MaxOpenGlMeshCount );

	const int pfAttribList[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		0, // End
	};

	int pixelFormat;
	UINT numFormats;
	if( !wglChoosePixelFormatARB( hdc, pfAttribList, nullptr, 1, &pixelFormat, &numFormats ) ) {
		return {};
	}

	PIXELFORMATDESCRIPTOR unusedDesc;
	if( !SetPixelFormat( hdc, pixelFormat, &unusedDesc ) ) {
		LOG( ERROR, "Failed to set pixel format" );
		return {};
	}

	int contextAttribList[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 0,
		WGL_CONTEXT_FLAGS_ARB, 0
#ifdef GAME_DEBUG
		| WGL_CONTEXT_DEBUG_BIT_ARB
#endif
		,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0, // End
	};
	result.renderContext = wglCreateContextAttribsARB( hdc, nullptr, contextAttribList );
	if( !result.renderContext ) {
		return {};
	}
	auto guard = makeScopeGuard( [&]() { wglDeleteContext( result.renderContext ); } );
	if( !wglMakeCurrent( hdc, result.renderContext ) ) {
		return {};
	}

	// auto extionsions = glGetString( GL_EXTENSIONS );

	// TODO: check extionsions string whether we can call wglGetProcAddress on the opengl functions

	#define wgl_get_proc_address( name ) name = (decltype( name ))wglGetProcAddress( #name )

	wgl_get_proc_address( glAttachShader );
	wgl_get_proc_address( glCompileShader );
	wgl_get_proc_address( glCreateShader );
	wgl_get_proc_address( glDeleteShader );
	wgl_get_proc_address( glDetachShader );
	wgl_get_proc_address( glCreateProgram );
	wgl_get_proc_address( glDeleteProgram );
	wgl_get_proc_address( glLinkProgram );
	wgl_get_proc_address( glUseProgram );
	wgl_get_proc_address( glValidateProgram );
	wgl_get_proc_address( glGenBuffers );
	wgl_get_proc_address( glDeleteBuffers );
	wgl_get_proc_address( glBindBuffer );
	wgl_get_proc_address( glBufferData );
	wgl_get_proc_address( glVertexAttribPointer );
	wgl_get_proc_address( glEnableVertexAttribArray );
	wgl_get_proc_address( glShaderSource );
	wgl_get_proc_address( glDrawArraysIndirect );
	wgl_get_proc_address( glDrawArraysInstanced );
	wgl_get_proc_address( glGetShaderiv );
	wgl_get_proc_address( glBindAttribLocation );
	wgl_get_proc_address( glGetProgramiv );
	wgl_get_proc_address( glUniform1f );
	wgl_get_proc_address( glUniform2f );
	wgl_get_proc_address( glUniform3f );
	wgl_get_proc_address( glUniform4f );
	wgl_get_proc_address( glUniform1i );
	wgl_get_proc_address( glUniform2i );
	wgl_get_proc_address( glUniform3i );
	wgl_get_proc_address( glUniform4i );
	wgl_get_proc_address( glUniform1fv );
	wgl_get_proc_address( glUniform2fv );
	wgl_get_proc_address( glUniform3fv );
	wgl_get_proc_address( glUniform4fv );
	wgl_get_proc_address( glUniform1iv );
	wgl_get_proc_address( glUniform2iv );
	wgl_get_proc_address( glUniform3iv );
	wgl_get_proc_address( glUniform4iv );
	wgl_get_proc_address( glUniformMatrix2fv );
	wgl_get_proc_address( glUniformMatrix3fv );
	wgl_get_proc_address( glUniformMatrix4fv );
	wgl_get_proc_address( glGetUniformLocation );
	wgl_get_proc_address( glBindVertexArray );
	wgl_get_proc_address( glDeleteVertexArrays );
	wgl_get_proc_address( glGenVertexArrays );
	wgl_get_proc_address( glGetAttribLocation );
	wgl_get_proc_address( glBindFragDataLocation );
	wgl_get_proc_address( glMapBuffer );
	wgl_get_proc_address( glUnmapBuffer );
	wgl_get_proc_address( glMapBufferRange );
	wgl_get_proc_address( glFlushMappedBufferRange );
	wgl_get_proc_address( glActiveTexture );
	wgl_get_proc_address( glVertexAttribP4ui );
	wgl_get_proc_address( glVertexAttrib4f );
	wgl_get_proc_address( glDrawElementsBaseVertex );
	wgl_get_proc_address( glDrawRangeElementsBaseVertex );
	wgl_get_proc_address( glPrimitiveRestartIndex );
	wgl_get_proc_address( wglSwapIntervalEXT );
	wgl_get_proc_address( glDebugMessageCallback );

	#undef wgl_get_proc_address

	if( glDebugMessageCallback ) {
		glDebugMessageCallback( win32OpenGlDebugCallback, nullptr );
	}

	if( !glAttachShader || !glCompileShader || !glCreateShader || !glDeleteShader || !glDetachShader
	    || !glCreateProgram || !glDeleteProgram || !glLinkProgram || !glUseProgram
	    || !glValidateProgram || !glGenBuffers || !glDeleteBuffers || !glBindBuffer || !glBufferData
	    || !glVertexAttribPointer || !glEnableVertexAttribArray || !glShaderSource
	    || !glDrawArraysIndirect || !glDrawArraysInstanced /*|| !glDrawArrays*/
	    || !glGetShaderiv || !glBindAttribLocation || !glGetProgramiv || !glUniform1f
	    || !glUniform2f || !glUniform3f || !glUniform4f || !glUniform1i || !glUniform2i
	    || !glUniform3i || !glUniform4i || !glUniform1fv || !glUniform2fv || !glUniform3fv
	    || !glUniform4fv || !glUniform1iv || !glUniform2iv || !glUniform3iv || !glUniform4iv
	    || !glUniformMatrix2fv || !glUniformMatrix3fv || !glUniformMatrix4fv
	    || !glGetUniformLocation || !glBindVertexArray || !glDeleteVertexArrays
	    || !glGenVertexArrays || !glGetAttribLocation || !glBindFragDataLocation || !glMapBuffer
	    || !glUnmapBuffer || !glMapBufferRange || !glFlushMappedBufferRange || !glActiveTexture
	    || !glVertexAttribP4ui || !glVertexAttrib4f || !glDrawElementsBaseVertex
	    || !glDrawRangeElementsBaseVertex || !wglSwapIntervalEXT || !glPrimitiveRestartIndex ) {
		return {};
	}

	guard.dismiss();
	return result;
}

#include "win32_opengl_shaders.cpp"

static GLuint loadShader( const GLchar* data, GLint length, GLenum type )
{
	auto shader = glCreateShader( type );
	if( !shader ) {
		return 0;
	}
	auto lengthP = ( length ) ? ( &length ) : ( nullptr );
	glShaderSource( shader, 1, &data, lengthP );
	glCompileShader( shader );

	GLint compileStatus;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &compileStatus );
	if( compileStatus == GL_FALSE ) {
		glDeleteShader( shader );
		return 0;
	}
	return shader;
}

struct OpenGlShaderProgram {
	GLuint vertex;
	GLuint fragment;
	GLuint program;
};
static OpenGlShaderProgram loadProgram( StringView vertexShader, StringView fragmentShader )
{
	OpenGlShaderProgram result = {};
	result.vertex =
	    loadShader( (const GLchar*)vertexShader.data(), vertexShader.size(), GL_VERTEX_SHADER );
	if( !result.vertex ) {
		return {};
	}
	result.fragment = loadShader( (const GLchar*)fragmentShader.data(), fragmentShader.size(),
	                              GL_FRAGMENT_SHADER );
	if( !result.fragment ) {
		glDeleteShader( result.vertex );
		return {};
	}
	result.program = glCreateProgram();
	if( !result.program ) {
		glDeleteShader( result.fragment );
		glDeleteShader( result.vertex );
		return {};
	}
	glAttachShader( result.program, result.vertex );
	glAttachShader( result.program, result.fragment );

	glBindFragDataLocation( result.program, 0, "outColor" );

	glBindAttribLocation( result.program, AL_position, "position" );
	glBindAttribLocation( result.program, AL_color, "color" );
	glBindAttribLocation( result.program, AL_texCoords0, "texCoords0" );
	glBindAttribLocation( result.program, AL_normal0, "normal0" );

	glLinkProgram( result.program );
	GLint linkStatus;
	glGetProgramiv( result.program, GL_LINK_STATUS, &linkStatus );
	if( linkStatus == GL_FALSE ) {
		glDeleteShader( result.fragment );
		glDeleteShader( result.vertex );
		glDeleteProgram( result.program );
		return {};
	}
	auto texture0Location = glGetUniformLocation( result.program, "texture0" );
	if( texture0Location >= 0 ) {
		glUseProgram( result.program );
		glUniform1i( texture0Location, 0 );
	}
	return result;
}

static bool win32InitIngameShaders( OpenGlContext* context )
{
	auto shader = &context->shader;

	auto prog = loadProgram( ingameVertexShaderSource, ingameFragmentShaderSource );
	if( !prog.program ) {
		return false;
	}
	shader->vertexShader   = prog.vertex;
	shader->fragmentShader = prog.fragment;
	shader->program        = prog.program;

	shader->worldViewProj     = glGetUniformLocation( prog.program, "worldViewProj" );
	shader->model             = glGetUniformLocation( prog.program, "model" );
	shader->screenDepthOffset = glGetUniformLocation( prog.program, "screenDepthOffset" );
	shader->ambientStrength   = glGetUniformLocation( prog.program, "ambientStrength" );
	shader->lightColor        = glGetUniformLocation( prog.program, "lightColor" );
	shader->lightPosition     = glGetUniformLocation( prog.program, "lightPosition" );
	if( shader->worldViewProj < 0 || shader->model < 0 || shader->ambientStrength < 0
	    || shader->lightColor < 0 || shader->lightPosition < 0 ) {
		return false;
	}

	return true;
}
static bool win32InitGuiShaders( OpenGlContext* context )
{
	auto shader = &context->noLightingShader;

	auto prog = loadProgram( noLightingVertexShaderSource, noLightingFragmentShaderSource );
	if( !prog.program ) {
		return false;
	}
	shader->vertexShader   = prog.vertex;
	shader->fragmentShader = prog.fragment;
	shader->program        = prog.program;

	shader->worldViewProj     = glGetUniformLocation( prog.program, "worldViewProj" );
	shader->screenDepthOffset = glGetUniformLocation( prog.program, "screenDepthOffset" );
	return true;
}
static bool win32InitShaders( OpenGlContext* context )
{
	return win32InitIngameShaders( context ) && win32InitGuiShaders( context );
}

ShaderId openGlLoadShaderProgram( StringView vertexShader, StringView fragmentShader )
{
	auto vertex   = win32ReadWholeFileInternal( vertexShader );
	auto fragment = win32ReadWholeFileInternal( fragmentShader );
	auto prog     = loadProgram( vertex, fragment );
	if( !prog.program ) {
		return {};
	}

	glDetachShader( prog.program, prog.vertex );
	glDetachShader( prog.program, prog.fragment );
	glDeleteShader( prog.vertex );
	glDeleteShader( prog.fragment );
	return {(int32)prog.program};
}

void openGlDeleteShaderProgram( ShaderId id )
{
	glDeleteProgram( (GLuint)id.id );
}

void openGlClear( Color clearColor )
{
	auto c = getColorF( clearColor );
	glClearDepth( -1000 );
	glClearColor( c.r, c.g, c.b, c.a );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	Win32AppContext.info->drawCalls = 0;
	Win32AppContext.info->vertices = 0;
	Win32AppContext.info->indices = 0;
}

void openGlSetAttribs( OpenGlContext* context )
{
	glEnableVertexAttribArray( AL_position );
	glVertexAttribPointer( AL_position, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), 0 );
	glEnableVertexAttribArray( AL_color );
	glVertexAttribPointer( AL_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( Vertex ),
	                       BUFFER_OFFSET( offsetof( Vertex, color ) ) );
	glEnableVertexAttribArray( AL_texCoords0 );
	glVertexAttribPointer( AL_texCoords0, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ),
	                       BUFFER_OFFSET( offsetof( Vertex, texCoords ) ) );
	glEnableVertexAttribArray( AL_normal0 );
	glVertexAttribPointer( AL_normal0, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof( Vertex ),
	                       BUFFER_OFFSET( offsetof( Vertex, normal ) ) );
}
void openGlPrepareCustomShader( OpenGlContext* context )
{
	auto program = context->currentProgram;
	glUseProgram( program );
	openGlSetAttribs( context );

	auto worldViewProj     = glGetUniformLocation( program, "worldViewProj" );
	auto model             = glGetUniformLocation( program, "model" );
	auto screenDepthOffset = glGetUniformLocation( program, "screenDepthOffset" );

	context->worldViewProj     = worldViewProj;
	context->model             = model;
	context->screenDepthOffset = screenDepthOffset;
}
void openGlPrepareIngameRender( OpenGlContext* context )
{
	auto shader = &context->shader;
	glUseProgram( shader->program );

	openGlSetAttribs( context );

	glUniform1f( shader->ambientStrength, shader->values.ambientStrength );
	glUniform4f( shader->lightColor, shader->values.lightColor.r, shader->values.lightColor.g,
	             shader->values.lightColor.b, shader->values.lightColor.a );
	glUniform3f( shader->lightPosition, shader->values.lightPosition.x,
	             shader->values.lightPosition.y, shader->values.lightPosition.z );

	context->worldViewProj     = shader->worldViewProj;
	context->model             = shader->model;
	context->screenDepthOffset = shader->screenDepthOffset;

}
void openGlPrepareNoLightingRender( OpenGlContext* context )
{
	auto shader = &context->noLightingShader;
	glUseProgram( shader->program );
	glEnableVertexAttribArray( AL_position );
	glVertexAttribPointer( AL_position, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), 0 );
	glEnableVertexAttribArray( AL_color );
	glVertexAttribPointer( AL_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof( Vertex ),
	                       (void*)offsetof( Vertex, color ) );
	glEnableVertexAttribArray( AL_texCoords0 );
	glVertexAttribPointer( AL_texCoords0, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ),
	                       (void*)offsetof( Vertex, texCoords ) );
	context->worldViewProj     = shader->worldViewProj;
	context->model             = -1;
	context->screenDepthOffset = shader->screenDepthOffset;
}

void openGlPrepareRender( OpenGlContext* context, bool wireframe )
{
	if( wireframe ) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	} else {
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}

	glBindVertexArray( context->dynamicBuffer.vertexArrayObjectId );
	glBindBuffer( GL_ARRAY_BUFFER, context->dynamicBuffer.vertexBufferId );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, context->dynamicBuffer.indexBufferId );
	openGlPrepareIngameRender( context );
}
void openGlPrepareShader( OpenGlContext* context )
{
	if( context->currentProgram ) {
		openGlPrepareCustomShader( context );
	} else if( context->renderStates[valueof( RenderStateType::Lighting )] ) {
		openGlPrepareIngameRender( context );
	} else {
		openGlPrepareNoLightingRender( context );
	}
}

void openGlSetProjection( OpenGlContext* context, mat4* projections, ProjectionType type )
{
	// set initial worldViewProj
	auto matrix = &projections[valueof( type )];
	if( type == ProjectionType::Orthogonal ) {
		glDisable( GL_DEPTH_TEST );
		openGlPrepareNoLightingRender( context );
	} else {
		if( context->renderStates[valueof( RenderStateType::DepthTest )] ) {
			glEnable( GL_DEPTH_TEST );
		} else {
			glDisable( GL_DEPTH_TEST );
		}
		openGlPrepareShader( context );
	}
	glUniformMatrix4fv( context->worldViewProj, 1, GL_FALSE, matrix->m );
	context->currentProjectionType = type;
}

static void win32SetRenderState( OpenGlContext* context, mat4* projections, RenderStateType type,
                                 bool enabled )
{
	assert( context );
	assert( valueof( type ) >= 0 && valueof( type ) < valueof( RenderStateType::Count ) );
	if( context->renderStates[valueof( type )] != enabled ) {
		win32RenderAndFlushBuffers( context, projections, GL_TRIANGLES );

		context->renderStates[valueof( type )] = enabled;
		if( enabled ) {
			switch( type ) {
				case RenderStateType::DepthTest: {
					glEnable( GL_DEPTH_TEST );
					break;
				}
				case RenderStateType::DepthWrite: {
					glDepthMask( GL_TRUE );
					break;
				}
				case RenderStateType::Lighting: {
					openGlPrepareShader( context );
					break;
				}
				case RenderStateType::Scissor: {
					glEnable( GL_SCISSOR_TEST );
					break;
				}
				case RenderStateType::BackFaceCulling: {
					glEnable( GL_CULL_FACE );
					break;
				}
				case RenderStateType::CullFrontFace: {
					glCullFace( GL_FRONT );
					break;
				}
				InvalidDefaultCase;
			}
		} else {
			switch( type ) {
				case RenderStateType::DepthTest: {
					glDisable( GL_DEPTH_TEST );
					break;
				}
				case RenderStateType::DepthWrite: {
					glDepthMask( GL_FALSE );
					break;
				}
				case RenderStateType::Lighting: {
					openGlPrepareShader( context );
					break;
				}
				case RenderStateType::Scissor: {
					glDisable( GL_SCISSOR_TEST );
					break;
				}
				case RenderStateType::BackFaceCulling: {
					glDisable( GL_CULL_FACE );
					break;
				}
				case RenderStateType::CullFrontFace: {
					glCullFace( GL_BACK );
					break;
				}
				InvalidDefaultCase;
			}
		}
	}
}

static void win32ProcessRenderCommands( OpenGlContext* context, RenderCommands* renderCommands )
{
	assert( context );
	assert( isValid( renderCommands ) );

	auto vb = &context->dynamicBuffer;
	win32MapBuffers( vb );

	context->currentProgram = 0;
	context->shader.values.ambientStrength = renderCommands->ambientStrength;
	context->shader.values.lightColor      = getColorF( renderCommands->lightColor );
	context->shader.values.lightPosition   = renderCommands->lightPosition;

	mat4 projections[2] = {renderCommands->view * context->projections[0], context->projections[1]};
	openGlSetProjection( context, projections, context->currentProjectionType );

	assert( vb->vertices );
	assert( vb->indices );
	assert( vb->mappedBuffers );
	auto stream = getRenderCommandsStream( renderCommands );
	while( stream.size ) {
		auto header = getRenderCommandsHeader( &stream );
		switch( header->type ) {
			case RenderCommandEntryType::Mesh: {
				auto body = getRenderCommandMesh( &stream, header );
				auto mesh = &body->mesh;
				if( mesh->verticesCount > vb->verticesCapacity ) {
					LOG( ERROR, "Mesh vertices count is bigger than vertex buffer capacity" );
					break;
					// TODO: if we know that the mesh is composed of triangles, we could split it
					// into multiple parts and render each individually
				}
				if( !win32VertexBufferHasSpace( vb, mesh->verticesCount, mesh->indicesCount ) ) {
					// we need to reset the buffers since we need new memory
					win32RenderAndResetBuffers( context, projections, GL_TRIANGLES );
				}

				// upload the vertices to the gpu
				auto startIndex = safe_truncate< uint16 >( vb->verticesCount );
				copy( vb->vertices + vb->verticesCount, mesh->vertices, mesh->verticesCount );
				vb->verticesCount += mesh->verticesCount;

				auto meshIndices = mesh->indices;
				auto indices     = vb->indices + vb->indicesCount;
				auto end         = indices + mesh->indicesCount;
				while( indices < end ) {
					*indices = *( meshIndices ) + startIndex;
					++meshIndices;
					++indices;
				}
				vb->indicesCount += mesh->indicesCount;
				break;
			}
			case RenderCommandEntryType::LineMesh: {
				auto body = getRenderCommandLineMesh( &stream, header );
				auto mesh = &body->mesh;
				if( mesh->verticesCount > vb->verticesCapacity ) {
					LOG( ERROR, "Mesh vertices count is bigger than vertex buffer capacity" );
					break;
				}
				if( !win32VertexBufferHasSpace( vb, mesh->verticesCount, mesh->indicesCount ) ) {
					// we need to reset the buffers since we need new memory
					win32RenderAndResetBuffers( context, projections, GL_TRIANGLES );
				} else {
					win32RenderAndFlushBuffers( context, projections, GL_TRIANGLES );
				}
				// upload the vertices to the gpu
				auto startIndex = safe_truncate< uint16 >( vb->verticesCount );
				copy( vb->vertices + vb->verticesCount, mesh->vertices, mesh->verticesCount );
				vb->verticesCount += mesh->verticesCount;

				auto meshIndices = mesh->indices;
				auto indices     = vb->indices + vb->indicesCount;
				auto end         = indices + mesh->indicesCount;
				while( indices < end ) {
					*indices = *( meshIndices ) + startIndex;
					++meshIndices;
					++indices;
				}
				vb->indicesCount += mesh->indicesCount;
				win32RenderAndFlushBuffers( context, projections, GL_LINE_STRIP );
				break;
			}
			case RenderCommandEntryType::StaticMesh: {
				auto body = getRenderCommandBody( &stream, header, RenderCommandStaticMesh );
				if( body->meshId ) {
					auto mesh = &context->meshes[body->meshId.id - 1];
					glBindVertexArray( mesh->vertexArrayObjectId );
					auto& current = projections[valueof( context->currentProjectionType )];
					auto matrix   = body->matrix * current;
					glUniformMatrix4fv( context->worldViewProj, 1, GL_FALSE, matrix.m );
					glUniformMatrix4fv( context->model, 1, GL_FALSE, body->matrix.m );
					glUniform1f( context->screenDepthOffset, body->screenDepthOffset );
					glDrawElements( GL_TRIANGLES, mesh->indicesCount, GL_UNSIGNED_SHORT, nullptr );
					glBindVertexArray( vb->vertexArrayObjectId );

					++Win32AppContext.info->drawCalls;
					Win32AppContext.info->vertices += mesh->verticesCount;
					Win32AppContext.info->indices += mesh->indicesCount;
				}
				break;
			}
			case RenderCommandEntryType::SetTexture: {
				auto body = getRenderCommandBody( &stream, header, RenderCommandSetTexture );
				assert( body->stage >= 0 && body->stage < 2 );
				auto id = toOpenGlId( body->id );
				if( id == 0 ) {
					id = context->plainWhiteTexture;
				}
				if( context->currentTextures[body->stage] != id ) {
					win32RenderAndFlushBuffers( context, projections, GL_TRIANGLES );

					glActiveTexture( GL_TEXTURE0 + body->stage );
					glBindTexture( GL_TEXTURE_2D, id );
					context->currentTextures[body->stage] = id;
				}
				break;
			}
			case RenderCommandEntryType::SetShader: {
				auto body = getRenderCommandBody( &stream, header, RenderCommandSetShader );
				auto id = toOpenGlId( body->id );
				if( id != context->currentProgram ) {
					win32RenderAndFlushBuffers( context, projections, GL_TRIANGLES );
					context->currentProgram = id;
					openGlPrepareShader( context );
				}
				break;
			}
			case RenderCommandEntryType::SetProjection: {
				auto body = getRenderCommandBody( &stream, header, RenderCommandSetProjection );
				assert( valueof( body->projectionType ) >= 0
				        && valueof( body->projectionType ) < 2 );
				if( context->currentProjectionType != body->projectionType ) {
					win32RenderAndFlushBuffers( context, projections, GL_TRIANGLES );

					openGlSetProjection( context, projections, body->projectionType );
				}
				break;
			}
			case RenderCommandEntryType::SetProjectionMatrix: {
				auto body =
				    getRenderCommandBody( &stream, header, RenderCommandSetProjectionMatrix );
				if( context->currentProjectionType == body->projectionType ) {
					win32RenderAndFlushBuffers( context, projections, GL_TRIANGLES );
				}
				switch( body->projectionType ) {
					case ProjectionType::Perspective: {
						projections[0] = renderCommands->view * body->matrix;
						break;
					}
					case ProjectionType::Orthogonal: {
						projections[1] = body->matrix;
						break;
					}
					InvalidDefaultCase;
				}
				break;
			}
			case RenderCommandEntryType::SetScissorRect: {
				auto body = getRenderCommandBody( &stream, header, RenderCommandSetScissorRect );
				glScissor( body->scissor.left, (int32)context->height - body->scissor.bottom,
				           width( body->scissor ), height( body->scissor ) );
				break;
			}
			case RenderCommandEntryType::SetRenderState: {
				auto body = getRenderCommandBody( &stream, header, RenderCommandSetRenderState );
				win32SetRenderState( context, projections, body->renderStateType, body->enabled );
				break;
			}
			case RenderCommandEntryType::Jump: {
				auto body = getRenderCommandBody( &stream, header, RenderCommandJump );
				stream.ptr = body->jumpDestination;
				break;
			}
			InvalidDefaultCase;
		}
	}

	win32RenderBuffers( context, projections, GL_TRIANGLES );
}

static bool win32InitOpenGL( OpenGlContext* context, float width, float height )
{
	context->width  = width;
	context->height = height;
	if( !win32InitShaders( context ) ) {
		return false;
	}

	context->dynamicBuffer     = win32InitOpenGlVertexBuffer( 10000, 60000 );
	context->plainWhiteTexture = win32CreatePlainWhiteTexture();
	if( !context->plainWhiteTexture ) {
		return false;
	}

	// projections can be changed by the game layer, so do not rely on projections being exactly
	// like initialized here
	auto aspect = width / height;
	context->projections[valueof( ProjectionType::Perspective )] =
	    matrixPerspectiveFovProjection( degreesToRadians( 65 ), aspect, -1, 1 );
	context->projections[valueof( ProjectionType::Orthogonal )] =
	    matrixOrthogonalProjection( 0, 0, width, height, -1, 1 );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	// win32SetupBuffers( context );
	// glDisable( GL_CULL_FACE );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	glFrontFace( GL_CW );
	// glDepth
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_GREATER );
	glAlphaFunc( GL_GREATER, 0 );
	glEnable( GL_ALPHA_TEST );

	glEnable( GL_PRIMITIVE_RESTART );
	glPrimitiveRestartIndex( MeshPrimitiveRestart );

	// TODO: read up on vsync and targeting 60 fps
	// TODO: do we need glFinish?
	wglSwapIntervalEXT( 0 );

	context->renderStates[valueof( RenderStateType::DepthTest )] = true;
	context->renderStates[valueof( RenderStateType::DepthWrite )] = true;
	context->renderStates[valueof( RenderStateType::Lighting )] = true;

	return true;
}

static void APIENTRY win32OpenGlDebugCallback( GLenum source, GLenum type, GLuint id,
                                               GLenum severity, GLsizei length,
                                               const GLchar* message, const void* userParam )
{
	OutputDebugStringA( message );
}