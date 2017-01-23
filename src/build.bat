@echo off
setlocal

rem call vcvarsall.bat here

IF [%project_path%] == [] (
	echo project_path not specified
	exit /b -1
)

if "%1" == "" goto :x64
if %1 == x86 goto :x86
if %1 == x64 goto :x64

:x86
set "platform_defines=/DARCHITECTURE_X86"
goto :begin_build

:x64
set "platform_defines=/DARCHITECTURE_X64"
goto :begin_build

:begin_build
set "platform_defines=/DARCHITECTURE_LITTLE_ENDIAN /DARCHITECTURE_IEEE_754 %platform_defines%"
set debug=1

if "%2" == "release" ( set "debug=0" )
if "%3" == "release" ( set "debug=0" )

set "defines=/DNOMINMAX /DUNICODE /DGAME_FLIP_Z /D_CRT_SECURE_NO_WARNINGS"

if %debug% == 1 goto :is_debug
if %debug% == 0 goto :is_release

:is_debug
set "defines=%defines% /DGAME_DEBUG=1 /D_DEBUG"
set "cl_switches=/Od /Zi /MDd"
set "link_switches="
goto :building

:is_release
set "defines=%defines% /DNDEBUG"
set "cl_switches=/MD /GS- /Gy /fp:fast /Ox /Oy- /GL /Oi /O2"
set "link_switches=/LTCG"
goto :building

:building

set "libraries=/I%project_path%/lib/ssemath/ /I%project_path%/lib/tm/ /I%project_path%/lib/stb/ /I%project_path%/lib/dlmalloc/"
set "includes=/I%project_path%/src/ %libraries%"

rem check whether we are doing a full build, only exe or only dll
if "%2" == "" goto :full_build
if "%2" == "dll" goto :dll_build

:full_build
cl %cl_switches% /W4 /Oi /FC /EHsc- /I%project_path%/lib/dlmalloc/ %project_path%/src/platform/win32/win32malloc.cpp -c /nologo
cl %platform_defines% %defines% %cl_switches% /W4 /Oi %includes% /FC /EHsc- %project_path%/src/platform/win32/win32main.cpp win32malloc.obj /link user32.lib gdi32.lib opengl32.lib Comdlg32.lib Shlwapi.lib %link_switches% /NODEFAULTLIB:MSVCRT /SUBSYSTEM:WINDOWS /OUT:game.exe /INCREMENTAL:NO /nologo
if "%2" == "exe" goto :end

:dll_build
del game_dll_*.pdb > NUL 2> NUL
rem the game will not be reloading the dll as long as there is a lock.tmp file
rem this ensures that the pdb is successfully loaded with the dll, since the dll is being
rem written before the pdb
echo "Waiting for pdb to finish being written" > lock.tmp

cl %platform_defines% %defines% %cl_switches% /DGAME_DLL /W4 /Oi %includes% /FC /DWIN32_LEAN_AND_MEAN /EHsc- %project_path%/src/game_main.cpp /LD /link user32.lib /SUBSYSTEM:WINDOWS /OUT:game_dll.dll /PDB:game_dll_%RANDOM%.pdb /INCREMENTAL:NO /EXPORT:initializeApp /EXPORT:updateAndRender /EXPORT:reloadApp /nologo
del lock.tmp > NUL 2> NUL

:end
