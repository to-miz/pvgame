#pragma once

#ifndef _GAMEDECLARATIONS_H_INCLUDED_
#define _GAMEDECLARATIONS_H_INCLUDED_

typedef TextureId LoadTextureType( StringView filename );
typedef TextureId LoadTextureFromMemoryType( ImageData image );
typedef void DeleteTextureType( TextureId id );
typedef ImageData LoadImageToMemoryType( StringView filename );
typedef void FreeImageDataType( ImageData* image );
typedef Font LoadFontType( StackAllocator* allocator, StringView utf8Name, int32 size, int32 weight,
                           bool italic, FontUnicodeRequestRanges ranges );
typedef void WriteBufferToFileType( StringView filename, void* buffer, size_t bufferSize );
typedef size_t ReadFileToBufferType( StringView filename, void* buffer, size_t bufferSize );
typedef MeshId UploadMeshType( Mesh mesh );

typedef ShaderId LoadShaderType( StringView vertexShader, StringView fragmentShader );
typedef void DeleteShaderType( ShaderId shader );

// TODO: get rid of MAX_PATH dependency
#ifndef MAX_PATH
	#define MAX_PATH 260
#endif
typedef short_string< MAX_PATH > FilenameString;
static const char* DefaultFilter = "All\0*.*\0";
static const char* JsonFilter = "Json\0*.json\0All\0*.*\0";

typedef int32 GetOpenFilenameType( const char* filter, const char* initialDir, bool multiselect,
                                   char* filenameBuffer, int32 filenameBufferSize );
typedef int32 GetSaveFilenameType( const char* filter, const char* initialDir, char* filenameBuffer,
                                   int32 filenameBufferSize );
typedef int32 GetTimeStampStringType( char* buffer, int32 size );

typedef StringView GetKeyboardKeyNameType( VirtualKeyEnumValues key );

typedef void* MallocType( size_t size );
typedef void* ReallocType( void* ptr, size_t size );
typedef void MfreeType( void* ptr );

typedef void* AllocateType( size_t size, uint32 alignment );
typedef void* ReallocateType( void* ptr, size_t newSize, size_t oldSize, uint32 alignment );
typedef void FreeType( void* ptr, size_t size, uint32 alignment );

struct PlatformServices {
	LoadTextureType* loadTexture;
	LoadTextureFromMemoryType* loadTextureFromMemory;
	DeleteTextureType* deleteTexture;
	LoadImageToMemoryType* loadImageToMemory;
	FreeImageDataType* freeImageData;

	LoadFontType* loadFont;
	WriteBufferToFileType* writeBufferToFile;
	ReadFileToBufferType* readFileToBuffer;

	UploadMeshType* uploadMesh;

	GetOpenFilenameType* getOpenFilename;
	GetSaveFilenameType* getSaveFilename;

	GetKeyboardKeyNameType* getKeyboardKeyName;
	GetTimeStampStringType* getTimeStampString;

	// malloc
	MallocType* malloc;
	ReallocType* realloc;
	ReallocType* reallocInPlace;
	MfreeType* free;

	AllocateType* allocate;
	ReallocateType* reallocate;
	ReallocateType* reallocateInPlace;
	FreeType* deallocate;

	// shader
	LoadShaderType* loadShader;
	DeleteShaderType* deleteShader;
};

// structures that get passed into the platform layer, get filled and passed back
struct PlatformRemapInfo {
	bool success;
	IngameLog* logStorage;  // log structure for the platform layer to fill
	string_logger* debugLogger;
	TextureMap* textureMap;  // textureMap for the platform layer to fill

	int32 width;  // requested window size
	int32 height;
};

struct PlatformInfo {
	float totalFrameTime;  // total frame time
	float gameTime;        // amount of time we spent on updating game
	float renderTime;      // amount of time we spent on rendering

	float averageFrameTime;
	float averageGameTime;
	float averageRenderTime;

	float fps;
	float averageFps;
	float minFrameTime;
	float minFps;
	float maxFrameTime;
	float maxFps;

	bool recordingInputs;
	bool replayingInputs;
	int32 recordingFrame;

	int32 vertices;
	int32 indices;
	int32 drawCalls;

	size_t mallocAllocated;
	size_t mallocFree;
	size_t mallocFootprint;
};

#ifdef GAME_DLL
	#define GAME_STORAGE extern "C"
#else
	#define GAME_STORAGE
#endif

#define INITIALIZE_APP( name )           \
	GAME_STORAGE PlatformRemapInfo name( \
	    void* memory, size_t size, PlatformServices platformServices, PlatformInfo* platformInfo )
#define UPDATE_AND_RENDER( name ) \
	GAME_STORAGE struct RenderCommands* name( void* memory, struct GameInputs* inputs, float dt )
#define RELOAD_APP( name ) GAME_STORAGE PlatformRemapInfo name( void* memory, size_t size )

#ifdef GAME_DLL
INITIALIZE_APP( initializeApp );
UPDATE_AND_RENDER( updateAndRender );
RELOAD_APP( reloadApp );
#endif

#endif  // _GAMEDECLARATIONS_H_INCLUDED_
