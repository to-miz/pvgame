#pragma once

#ifndef _GAMEDECLARATIONS_H_INCLUDED_
#define _GAMEDECLARATIONS_H_INCLUDED_

typedef TextureId LoadTextureType( StringView filename );
typedef TextureId LoadTextureFromMemoryType( ImageData image );
typedef ImageData LoadImageToMemoryType( StringView filename );
typedef void FreeImageDataType( ImageData* image );
typedef Font LoadFontType( StackAllocator* allocator, StringView utf8Name, int32 size, int32 weight,
                           bool italic, FontUnicodeRequestRanges ranges );
typedef void WriteBufferToFileType( StringView filename, void* buffer, size_t bufferSize );
typedef size_t ReadFileToBufferType( StringView filename, void* buffer, size_t bufferSize );
typedef MeshId UploadMeshType( Mesh mesh );

typedef int32 GetOpenFilenameType( const char* filter, const char* initialDir, bool multiselect,
                                   char* filenameBuffer, int32 filenameBufferSize );
typedef int32 GetSaveFilenameType( const char* filter, const char* initialDir, char* filenameBuffer,
                                   int32 filenameBufferSize );

struct PlatformServices {
	LoadTextureType* loadTexture;
	LoadTextureFromMemoryType* loadTextureFromMemory;
	LoadImageToMemoryType* loadImageToMemory;
	FreeImageDataType* freeImageData;

	LoadFontType* loadFont;
	WriteBufferToFileType* writeBufferToFile;
	ReadFileToBufferType* readFileToBuffer;

	UploadMeshType* uploadMesh;

	GetOpenFilenameType* getOpenFilename;
	GetSaveFilenameType* getSaveFilename;
};

// structures that get passed into the platform layer, get filled and passed back
struct PlatformRemapInfo {
	bool success;
	IngameLog* logStorage;	// log structure for the platform layer to fill 
	TextureMap* textureMap; // textureMap for the platform layer to fill
};

struct PlatformInfo {
	float frameTime;
	float fps;
	float averageFrameTime;
	float averageFps;
	float minFrameTime;
	float minFps;
	float maxFrameTime;
	float maxFps;

	bool recordingInputs;
	bool replayingInputs;
	int32 recordingFrame;
};

#ifdef GAME_DLL
#define GAME_STORAGE extern "C"
#else
#define GAME_STORAGE
#endif

#define INITIALIZE_APP( name, ... )                                                               \
	GAME_STORAGE PlatformRemapInfo name(                                                          \
	    void* memory, size_t size, PlatformServices platformServices, PlatformInfo* platformInfo, \
	    float viewportWidth, float viewportHeight )
#define UPDATE_AND_RENDER( name, ... ) \
	GAME_STORAGE struct RenderCommands* name( void* memory, struct GameInputs* inputs, float dt )
#define RELOAD_APP( name, ... ) GAME_STORAGE PlatformRemapInfo name( void* memory, size_t size )

#ifdef GAME_DLL
INITIALIZE_APP( initializeApp );
UPDATE_AND_RENDER( updateAndRender );
RELOAD_APP( reloadApp );
#endif

#endif  // _GAMEDECLARATIONS_H_INCLUDED_
