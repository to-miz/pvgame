#include <cassert>
#include <Core/IntegerTypes.h>
#include <Core/CoreTypes.h>
#include <Warnings.h>
#include <cstdarg>

#include <type_traits>
#include <Core/Macros.h>
#include <cstring>

#include <Core/Math.h>
#include <tm_utility_wrapper.cpp>
#include <Core/CoreFloat.h>
#include <Core/Math.cpp>
#include <Utility.cpp>
#include <Core/Log.h>

#include <Core/NumericLimits.h>
#include <Core/Truncate.h>
#include <Core/NullableInt.h>
#include <Core/Algorithm.h>

#include <Core/StackAllocator.cpp>
#define VEC3_ARGS_AS_CONST_REF GAME_DEBUG
#define VEC4_ARGS_AS_CONST_REF GAME_DEBUG
#include <Core/Vector.cpp>
#define RECT_ARGS_AS_CONST_REF GAME_DEBUG
#include <Core/Rect.h>
#include <Core/Matrix.cpp>
#define AABB_ARGS_AS_CONST_REF GAME_DEBUG
#include <Core/AABB.h>
#define RANGE_ARGS_AS_CONST_REF GAME_DEBUG
#include <Core/Range.h>

#include <Core/ArrayView.cpp>
#include <Core/StringView.cpp>
#include <Core/String.cpp>
#include <Core/Unicode.cpp>
#include <tm_conversion_wrapper.cpp>

#include <Core/ScopeGuard.h>

#include <Windows.h>
#include <Windowsx.h>
#include <Shlwapi.h>
#undef far
#undef near

#include <gl/gl.h>

#include <Core/Color.cpp>
#include <Core/Normal.cpp>
#include <ImageData.h>

#include <VirtualKeys.h>
#include <Inputs.cpp>

struct WindowData {
	HWND hwnd;
	LONG width;
	LONG height;
	bool active;
};

struct OpenGlContext;
struct TextureMap;
struct PlatformInfo;
struct Win32AppContextData {
	WindowData window;
	OpenGlContext* openGlContext;
	TextureMap* textureMap;
	PlatformInfo* info;
};

extern global Win32AppContextData Win32AppContext;

#include <platform/common/WString.cpp>
#include "win32Filesystem.cpp"
#include "win32TextureLoader.cpp"

#include <Core/IntrusiveLinkedList.h>
#define NO_PROFILING
#include <Profiling.cpp>

#include <QuadTexCoords.cpp>
#include <Graphics.h>
#include <Graphics/Font.h>
#include <TextureMap.cpp>
#include <GameDeclarations.h>

#include "win32_opengl.cpp"
#include "win32FontGeneration.cpp"

extern global TextureMap* GlobalTextureMap;
#include "win32PlatformServices.cpp"

// globals
global IngameLog* GlobalIngameLog = nullptr;
global TextureMap* GlobalTextureMap = nullptr;
global Win32AppContextData Win32AppContext = {};

typedef INITIALIZE_APP( InitializeAppType );
typedef UPDATE_AND_RENDER( UpdateAndRenderType );
typedef RELOAD_APP( ReloadAppType );

INITIALIZE_APP( InitializeAppStub ) { return {}; }
UPDATE_AND_RENDER( UpdateAndRenderStub ) { return nullptr; }
RELOAD_APP( ReloadAppStub ) { return {}; }

InitializeAppType* initializeApp     = InitializeAppStub;
UpdateAndRenderType* updateAndRender = UpdateAndRenderStub;
ReloadAppType* reloadApp = ReloadAppStub;

bool isResizing = false;

static LRESULT CALLBACK windowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static void win32ResetInputs( GameInputs* inputs )
{
	for( auto& entry : inputs->keys ) {
		auto wasDown = isKeyDown( entry );
		if( wasDown ) {
			entry.composite = GameInputKeyFlags::WasDown;
		} else {
			entry.composite = 0;
		}
	}
	inputs->mouse.prev = inputs->mouse.position;

	inputs->count = 0;
}
static GameInputKey win32SetKeyStatus( GameInputKey key, bool down )
{
	GameInputKey result;
	result.composite   = ( ( key.composite & GameInputKeyFlags::CountMask ) + 1 );
	uint8 intermediate = result.composite | ( key.composite & GameInputKeyFlags::WasDown );
	if( isKeyDown( {intermediate} ) != down ) {
		++result.composite;
	}
	result.composite |= ( key.composite & GameInputKeyFlags::WasDown );
	return result;
}
static void win32HandleInputs( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                               GameInputs* inputs )
{
	switch( msg ) {
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP: {
			assert( wParam >= 0 && wParam < 256 );
			auto down = ( msg == WM_SYSKEYDOWN || msg == WM_KEYDOWN );
			if( down && ( lParam & ( 1 << 30 ) ) ) {
				// key was down before, this is a repeated key message
				inputs->keys[wParam].composite |= GameInputKeyFlags::Repeated;
				break;
			}
			inputs->keys[wParam] = win32SetKeyStatus( inputs->keys[wParam], down );
			break;
		}
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP: {
			auto down = msg != WM_LBUTTONUP;
			if( down ) {
				SetCapture( hwnd );
			} else {
				ReleaseCapture();
			}
			inputs->keys[VK_LBUTTON] = win32SetKeyStatus( inputs->keys[VK_LBUTTON], down );
			inputs->mouse.position.x = (float)GET_X_LPARAM( lParam );
			inputs->mouse.position.y = (float)GET_Y_LPARAM( lParam );
			break;
		}
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP: {
			auto down = msg != WM_MBUTTONUP;
			if( down ) {
				SetCapture( hwnd );
			} else {
				ReleaseCapture();
			}
			inputs->keys[VK_MBUTTON] = win32SetKeyStatus( inputs->keys[VK_MBUTTON], down );
			inputs->mouse.position.x = (float)GET_X_LPARAM( lParam );
			inputs->mouse.position.y = (float)GET_Y_LPARAM( lParam );
			break;
		}
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP: {
			auto down = msg != WM_RBUTTONUP;
			if( down ) {
				SetCapture( hwnd );
			} else {
				ReleaseCapture();
			}
			inputs->keys[VK_RBUTTON] = win32SetKeyStatus( inputs->keys[VK_RBUTTON], down );
			inputs->mouse.position.x = (float)GET_X_LPARAM( lParam );
			inputs->mouse.position.y = (float)GET_Y_LPARAM( lParam );
			break;
		}
		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP: {
			auto button         = GET_XBUTTON_WPARAM( wParam );
			auto virtualKeyCode = ( button == XBUTTON1 ) ? ( VK_XBUTTON1 ) : ( VK_XBUTTON2 );
			auto down           = msg != WM_XBUTTONUP;
			if( down ) {
				SetCapture( hwnd );
			} else {
				ReleaseCapture();
			}
			inputs->keys[virtualKeyCode] = win32SetKeyStatus( inputs->keys[virtualKeyCode], down );
			inputs->mouse.position.x     = (float)GET_X_LPARAM( lParam );
			inputs->mouse.position.y     = (float)GET_Y_LPARAM( lParam );
			break;
		}
		case WM_MOUSEMOVE: {
			inputs->mouse.position.x = (float)GET_X_LPARAM( lParam );
			inputs->mouse.position.y = (float)GET_Y_LPARAM( lParam );
			break;
		}
		case WM_CHAR: {
			static uint16 surrogateLeadStorage;
			static bool lastCharWasSurrogateLead = false;
			auto cp                              = (uint16)wParam;
			if( cp > 32 ) {
				if( utf16::isSurrogateLead( cp ) ) {
					lastCharWasSurrogateLead = true;
					surrogateLeadStorage     = cp;
				} else {
					uint16 chars[2];
					int32 size = 0;
					if( lastCharWasSurrogateLead ) {
						chars[0]                 = lastCharWasSurrogateLead;
						chars[1]                 = cp;
						size                     = 2;
						lastCharWasSurrogateLead = false;
					} else {
						chars[0] = cp;
						size     = 1;
					}
					inputs->count +=
					    utf8::convertUtf16ToUtf8( chars, size, inputs->chars + inputs->count,
					                              countof( inputs->chars ) - inputs->count );
				}
			}
			break;
		}
		case WM_UNICHAR: {
			// TODO: do we ever handle this?
			break;
		}
	}
}
static void win32FillMouseData( GameMouseInput* mouse, float width, float height )
{
	assert( width > 0 );
	assert( height > 0 );
	mouse->relative.x      = mouse->x / width;
	mouse->relative.y      = mouse->y / height;
	mouse->delta.x         = mouse->x - mouse->prev.x;
	mouse->delta.y         = mouse->y - mouse->prev.y;
	mouse->relativeDelta.x = mouse->delta.x / width;
	mouse->relativeDelta.y = mouse->delta.y / height;
}

double win32PerformanceInit()
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency( &frequency );
	return 1000.0 / (double)frequency.QuadPart;
}
global double Win32PerformanceFrequency = win32PerformanceInit();

double win32PerformanceCounter()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter( &counter );
	return (double)counter.QuadPart * Win32PerformanceFrequency;
}

struct Win32GameDll {
	const wchar_t* dllName;
	const wchar_t* dllCopyName;
	const wchar_t* lockfile;
	FILETIME lastWriteTime;
	HMODULE library;
};

// we will not cleanup the strings returned from this function, the operating system can do the
// cleanup, since we need the strings until the program terminates
wchar_t* win32ConcatStrings( const wchar_t* a, size_t aLen, const wchar_t* b )
{
	auto bLen = wcslen( b );
	auto resultSize = aLen + bLen + 1;
	auto result = new wchar_t[resultSize];
	memcpy( result, a, aLen * sizeof( wchar_t ) );
	memcpy( result + aLen, b, bLen * sizeof( wchar_t ) );
	result[resultSize - 1] = 0;
	return result;

}
Win32GameDll win32MakeGameDllNames( const wchar_t* dllName, const wchar_t* dllCopyName,
                                    const wchar_t* lockfile )
{
	Win32GameDll result = {};
	wchar_t fnBuffer[1024];
	auto size = GetModuleFileNameW( nullptr, fnBuffer, countof( fnBuffer ) );
	// find dir
	for( int i = size - 1; i >= 0 && fnBuffer[i] != L'\\'; --i, --size ) {
	}
	if( size > 0 ) {
		result.dllName = win32ConcatStrings( fnBuffer, size, dllName );
		result.dllCopyName = win32ConcatStrings( fnBuffer, size, dllCopyName );
		result.lockfile = win32ConcatStrings( fnBuffer, size, lockfile );
	}
	return result;
}

bool win32FileExists( const wchar_t* file )
{
	auto exists = true;
	auto handle = CreateFileW( file, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
							   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if( handle == INVALID_HANDLE_VALUE ) {
		exists = false;
	} else {
		CloseHandle( handle );
	}

	return exists;
}
bool win32GetFileWriteTime( const wchar_t* file, FILETIME* outTime )
{
	bool success = false;
	WIN32_FILE_ATTRIBUTE_DATA fileAttr = {};
	if( GetFileAttributesExW( file, GetFileExInfoStandard, &fileAttr ) != 0 ) {
		*outTime = fileAttr.ftLastWriteTime;
		success  = true;
	}
	return success;
}
// returns whether the dll has been reloaded
bool win32LoadGameDll( Win32GameDll* dll )
{
	bool reloaded = false;
	// make sure that there is no lockfile, so that when we do LoadLibrary the pdb is also loaded
	// the lockfile is present as long as the pdb hasn't been finished being written
	if( !win32FileExists( dll->lockfile ) ) {
		FILETIME currentTime;
		if( win32GetFileWriteTime( dll->dllName, &currentTime ) ) {
			if( CompareFileTime( &currentTime, &dll->lastWriteTime ) > 0 ) {
				dll->lastWriteTime = currentTime;
				if( dll->library ) {
					FreeLibrary( dll->library );
					initializeApp   = InitializeAppStub;
					updateAndRender = UpdateAndRenderStub;
					reloadApp       = ReloadAppStub;
				}
				dll->library = nullptr;

				if( CopyFileW( dll->dllName, dll->dllCopyName, false ) ) {
					auto newModule = LoadLibraryW( dll->dllCopyName );
					if( newModule ) {
						initializeApp =
						    (InitializeAppType*)GetProcAddress( newModule, "initializeApp" );
						updateAndRender =
						    (UpdateAndRenderType*)GetProcAddress( newModule, "updateAndRender" );
						reloadApp = (ReloadAppType*)GetProcAddress( newModule, "reloadApp" );
						assert( initializeApp );
						assert( updateAndRender );
						assert( reloadApp );
						if( initializeApp && updateAndRender && reloadApp ) {
							dll->library = newModule;
							reloaded     = true;
						} else {
							FreeLibrary( newModule );
							initializeApp   = InitializeAppStub;
							updateAndRender = UpdateAndRenderStub;
							reloadApp       = ReloadAppStub;
						}
					}
				}
			}
		}
	}
	return reloaded;
}

const int32 Win32MaxRecording = 20000;
struct Win32RecordingFrame {
	GameInputs inputs;
	float elapsedTime;
};
struct Win32InputRecording {
	Win32RecordingFrame entries[Win32MaxRecording];
	int32 count;
	int32 currentInput;
	void* memory;
	size_t memorySize;
};

void win32Remap( PlatformRemapInfo* info )
{
	GlobalIngameLog = info->logStorage;
	GlobalTextureMap = info->textureMap;
}

int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	size_t memorySize = megabytes( 20 );
	auto memory = VirtualAlloc( nullptr, memorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	if( !memory ) {
		LOG( ERROR, "Out of memory" );
		return 0;
	}
	LONG width  = 800;
	LONG height = 800;

	auto dll = win32MakeGameDllNames( L"game_dll.dll", L"game_copy.dll", L"lock.tmp" );
	win32LoadGameDll( &dll );

	win32PopulateKeyboardKeyNames();

	PlatformServices platformServices = {&win32LoadTexture,       &win32LoadTextureFromMemory,
	                                     &win32DeleteTexture,     &loadImageToMemory,
	                                     &freeImageData,          &win32LoadFont,
	                                     &win32WriteBufferToFile, &win32ReadFileToBuffer,
	                                     &win32UploadMeshToGpu,   &win32GetOpenFilename,
	                                     &win32GetSaveFilename,   &win32GetKeyboardKeyName};
	PlatformInfo info    = {};
	Win32AppContext.info = &info;
	auto initializeResult =
	    initializeApp( memory, memorySize, platformServices, &info, (float)width, (float)height );
	if( !initializeResult.success ) {
		LOG( ERROR, "App initialization failed" );
		return 0;
	}
	win32Remap( &initializeResult );
	if( !win32BindOpenGlFunctions() ) {
		return 0;
	}

	DWORD style      = WS_OVERLAPPEDWINDOW;
	DWORD exstyle    = WS_EX_OVERLAPPEDWINDOW;
	DWORD classStyle = CS_HREDRAW | CS_VREDRAW;

	auto instance             = GetModuleHandle( nullptr );
	WNDCLASSEXW windowClass   = {};
	windowClass.cbSize        = sizeof( WNDCLASSEXW );
	windowClass.style         = classStyle;
	windowClass.lpfnWndProc   = windowProc;
	windowClass.hInstance     = instance;
	windowClass.hCursor       = LoadCursorW( nullptr, IDC_ARROW );
	windowClass.hIcon         = LoadIconW( nullptr, IDI_APPLICATION );
	windowClass.lpszClassName = L"some_window_class_name_who_cares";

	auto atom = RegisterClassExW( &windowClass );

	Win32AppContext.window = {nullptr, width, height};
	RECT windowRect  = {0, 0, width, height};
	AdjustWindowRectEx( &windowRect, style, FALSE, exstyle );
	LONG x              = 0;
	LONG y              = 0;
	LONG adjustedWidth  = windowRect.right - windowRect.left;
	LONG adjustedHeight = windowRect.bottom - windowRect.top;
	auto hwnd = CreateWindowExW( exstyle, MAKEINTATOM( atom ), L"MyWindowName", style, x, y,
	                             adjustedWidth, adjustedHeight, nullptr, nullptr, instance, 0 );
	Win32AppContext.window.hwnd = hwnd;
	if( !hwnd ) {
		LOG( ERROR, "CreateWindowExW failed" );
		return 0;
	}
	SetWindowLongPtrW( hwnd, GWLP_USERDATA, (LONG_PTR)&( Win32AppContext.window ) );

	auto platformMemorySize = megabytes( 10 );
	auto platformMemory =
	    VirtualAlloc( nullptr, platformMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	auto platformAllocator = makeStackAllocator( platformMemory, platformMemorySize );

	auto hdc           = GetDC( hwnd );
	auto openGlContext = win32CreateOpenGlContext( &platformAllocator, hdc );
	if( !openGlContext.renderContext ) {
		auto error = GetLastError();
		switch( error ) {
			case ERROR_INVALID_VERSION_ARB: {
				LOG( ERROR, "Failed to create OpenGL context: invalid version" );
				__debugbreak();
				break;
			}
			case ERROR_INVALID_PROFILE_ARB: {
				LOG( ERROR, "Failed to create OpenGL context: invalid profile" );
				__debugbreak();
				break;
			}
			default: {
				LOG( ERROR, "Failed to create OpenGL context" );
				break;
			}
		}
		return 0;
	}
	if( !win32InitOpenGL( &openGlContext, (float)width, (float)height ) ) {
		LOG( ERROR, "Failed to initialize OpenGL" );
		return 0;
	}
	Win32AppContext.openGlContext = &openGlContext;
	ShowWindow( hwnd, SW_SHOW );

	MSG msg;
	auto running = true;
	GameInputs inputs = {};
	GameInputs platformInputs = {};

	// setting maxElapsedTime to a bigger value messes up stepping
	float maxElapsedTime = ( 1000.0f / 60.0f ) /** 1.5f*/;
	double frameTimeAcc = 0;
	double gameTimeAcc = 0;
	double renderTimeAcc = 0;
	intmax loopCounter = 0;
	float minFrameTime = 0;
	float maxFps = 0;
	float maxFrameTime = 0;
	float minFps = 0;
	double elapsedTime = 0;

	bool recordingInputs = false;
	bool replayingInputs = false;
	bool replayStepped = false;
	bool replayStep = false;
	Win32InputRecording* recording = new Win32InputRecording;
	recording->count = 0;
	recording->memory =
	    VirtualAlloc( nullptr, memorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );

	RenderCommands* renderCommands = nullptr;

	while( running ) {
		if( win32LoadGameDll( &dll ) ) {
			auto remapInfo = reloadApp( memory, memorySize );
			win32Remap( &remapInfo );
		}

		double startTime = win32PerformanceCounter();

		openGlContext.width  = (float)Win32AppContext.window.width;
		openGlContext.height = (float)Win32AppContext.window.height;
		if( !replayStepped || replayStep ) {
			win32ResetInputs( &inputs );
		}
		win32ResetInputs( &platformInputs );
		while( PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) ) {
			if( msg.message == WM_QUIT ) {
				running = false;
			}

			TranslateMessage( &msg );
			DispatchMessageW( &msg );

			win32HandleInputs( msg.hwnd, msg.message, msg.wParam, msg.lParam, &inputs );
			win32HandleInputs( msg.hwnd, msg.message, msg.wParam, msg.lParam, &platformInputs );
		}
		win32FillMouseData( &inputs.mouse, openGlContext.width, openGlContext.height );
		win32FillMouseData( &platformInputs.mouse, openGlContext.width, openGlContext.height );
		if( inputs.mouse.locked && Win32AppContext.window.active && !replayingInputs ) {
			RECT rect;
			GetClientRect( hwnd, &rect );
			POINT point = {};
			ClientToScreen( hwnd, &point );
			rect.left += point.x;
			rect.top += point.y;
			rect.right += point.x;
			rect.bottom += point.y;
			ClipCursor( &rect );
			SetCursorPos( point.x + Win32AppContext.window.width / 2,
			              point.y + Win32AppContext.window.height / 2 );
			inputs.mouse.x = (float)( Win32AppContext.window.width / 2 );
			inputs.mouse.y = (float)( Win32AppContext.window.height / 2 );
		} else {
			ClipCursor( nullptr );
		}

		if( isKeyDown( &platformInputs, KC_Escape ) && !inputs.disableEscapeForQuickExit ) {
			running = false;
		}
		if( isHotkeyPressed( &platformInputs, KC_Key_X, KC_Control ) ) {
			replayStepped = false;
			replayStep = false;
		}
		if( isHotkeyPressed( &platformInputs, KC_Key_C, KC_Control ) ) {
			if( !replayStepped ) {
				replayStepped = true;
			} else {
				replayStep = true;
			}
		}

		// do stuff
		// TODO: is this fine for production?
		float clampedElapsedTime = (float)elapsedTime;
		if( clampedElapsedTime > maxElapsedTime ) {
			clampedElapsedTime = maxElapsedTime;
		}

		if( isHotkeyPressed( &platformInputs, KC_Key_P, KC_Control ) ) {
			recordingInputs = !recordingInputs;
			if( recordingInputs ) {
				// we just started recording, initialize
				recording->count = 0;
				memcpy( recording->memory, memory, memorySize );
				recording->memorySize = memorySize;
			}
		}
		if( recordingInputs ) {
			auto currentFrame = &recording->entries[recording->count];
			currentFrame->inputs = inputs;
			currentFrame->elapsedTime = clampedElapsedTime;
			++recording->count;
			info.recordingFrame = recording->count;
		}

		if( isHotkeyPressed( &platformInputs, KC_Key_O, KC_Control ) && recording->count ) {
			replayingInputs = !replayingInputs;
			if( replayingInputs ) {
				memcpy( memory, recording->memory, memorySize );
				recording->currentInput = 0;
			}
		}
		if( replayingInputs ) {
			if( recording->currentInput >= recording->count ) {
				// restart
				memcpy( memory, recording->memory, memorySize );
				recording->currentInput = 0;
			}
			auto currentFrame = &recording->entries[recording->currentInput];
			inputs = currentFrame->inputs;
			clampedElapsedTime = currentFrame->elapsedTime;
			if( !replayStepped || replayStep ) {
				++recording->currentInput;
				info.recordingFrame = recording->currentInput;
			}
		}

		info.recordingInputs = recordingInputs;
		info.replayingInputs = replayingInputs;

		Color clearColor = Color::White;
		if( !replayStepped || replayStep ) {
			auto startTime = win32PerformanceCounter();
			renderCommands = updateAndRender( memory, &inputs, clampedElapsedTime );
			auto gameTime = win32PerformanceCounter() - startTime;
			info.gameTime = (float)gameTime;
			gameTimeAcc += gameTime;

			replayStep = false;
			if( renderCommands ) {
				clearColor = renderCommands->clearColor;
			}
		}

		auto renderStartTime = win32PerformanceCounter();
		openGlClear( clearColor );
		if( renderCommands ) {
			openGlPrepareRender( &openGlContext, renderCommands->wireframe );
			win32ProcessRenderCommands( &openGlContext, renderCommands );
		}
		SwapBuffers( hdc );
		// TODO: is this needed if vsync is turned on?
		// glFinish();

		assert( glGetError() == GL_NO_ERROR );

		double endTime = win32PerformanceCounter();
		auto renderTime = endTime - renderStartTime;
		info.renderTime = (float)renderTime;
		renderTimeAcc += renderTime;

		elapsedTime = endTime - startTime;

		info.totalFrameTime = (float)elapsedTime;
		info.fps = 1000.0f / info.totalFrameTime;

		if( info.totalFrameTime > maxFrameTime ) {
			maxFrameTime = info.totalFrameTime;
			minFps = info.fps;
		}
		if( info.totalFrameTime < minFrameTime || minFrameTime == 0 ) {
			minFrameTime = info.totalFrameTime;
			maxFps = info.fps;
		}
		frameTimeAcc += elapsedTime;
		++loopCounter;
		if( frameTimeAcc >= 1000.0f ) {
			auto counter           = (double)loopCounter;
			info.averageFrameTime  = (float)( frameTimeAcc / counter );
			info.averageGameTime   = (float)( gameTimeAcc / counter );
			info.averageRenderTime = (float)( renderTimeAcc / counter );
			info.averageFps = 1000.0f / info.averageFrameTime;
			info.minFrameTime = minFrameTime;
			info.minFps = minFps;
			info.maxFrameTime = maxFrameTime;
			info.maxFps = maxFps;
			frameTimeAcc = 0;
			gameTimeAcc = 0;
			renderTimeAcc = 0;
			loopCounter = 0;
			minFrameTime = 0;
			minFps = 0;
			maxFrameTime = 0;
			maxFps = 0;
		}
	}
	ReleaseDC( hwnd, hdc );
	if( dll.library ) {
		FreeLibrary( dll.library );
		DeleteFileW( dll.dllCopyName );
	}
	return 0;
}

static LRESULT CALLBACK windowProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	auto windowData = (WindowData*)GetWindowLongPtrW( hwnd, GWLP_USERDATA );
	switch( msg ) {
		case WM_PAINT: {
			PAINTSTRUCT paint;
			BeginPaint( hwnd, &paint );
			EndPaint( hwnd, &paint );
			return 0;
		}
		case WM_SIZE: {
			glViewport( 0, 0, LOWORD( lParam ), HIWORD( lParam ) );
			if( windowData ) {
				windowData->width  = LOWORD( lParam );
				windowData->height = HIWORD( lParam );
			}
			PostMessage( hwnd, WM_PAINT, 0, 0 );
			isResizing = true;
			return 0;
		}
		case WM_ACTIVATE: {
			if( windowData ) {
				windowData->active = ( LOWORD( wParam ) != WA_INACTIVE );
			}
			break;
		}
		case WM_SYSCOMMAND: {
			if( wParam == SC_KEYMENU ) {
				return 0;
			}
			break;
		}
		case WM_CLOSE: {
			PostQuitMessage( 0 );
			return 0;
		}
	}
	return DefWindowProc( hwnd, msg, wParam, lParam );
}