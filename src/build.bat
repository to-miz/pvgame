@echo off

rem call vcvarsall.bat here

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

if %debug% == 1 goto :is_debug
if %debug% == 0 goto :is_release

:is_debug
set "defines=/DGAME_DEBUG=1 /D_DEBUG"
goto :building

:is_release
set "defines="
goto :building

:building
set "current_folder=%cd%"
IF EXIST "../builds/" goto :DO_BUILD
mkdir "../builds/"

:DO_BUILD
set "libraries=/I%current_folder%/../lib/ssemath/ /I%current_folder%/../lib/tm/ /I%current_folder%/../lib/stb/"
pushd "../builds/"

rem check whether we are doing a full build, only exe or only dll
if "%2" == "" goto :full_build
if "%2" == "dll" goto :dll_build

:full_build
cl %platform_defines% %defines% /DGAME_FLIP_Z /W4 /Oi /I%current_folder% %libraries% /FC /D_DEBUG /DNOMINMAX /DUNICODE /EHsc- /Od /Zi %current_folder%/platform/win32/win32main.cpp /MDd /link user32.lib gdi32.lib opengl32.lib Comdlg32.lib /SUBSYSTEM:WINDOWS /OUT:game.exe /INCREMENTAL:NO /nologo
if "%2" == "exe" goto :end

:dll_build
del game_dll_*.pdb > NUL 2> NUL
rem the game will not be reloading the dll as long as there is a lock.tmp file
rem this ensures that the pdb is successfully loaded with the dll, since the dll is being
rem written before the pdb
echo "Waiting for pdb to finish being written" > lock.tmp

cl %platform_defines% %defines% /DGAME_DLL /DGAME_FLIP_Z /W4 /Oi /I%current_folder% %libraries% /FC /D_DEBUG /DWIN32_LEAN_AND_MEAN /DNOMINMAX /DUNICODE /EHsc- /Od /Zi %current_folder%/game_main.cpp /MDd /LD /link user32.lib /SUBSYSTEM:WINDOWS /OUT:game_dll.dll /PDB:game_dll_%RANDOM%.pdb /INCREMENTAL:NO /EXPORT:initializeApp /EXPORT:updateAndRender /EXPORT:reloadApp /nologo
del lock.tmp > NUL 2> NUL

:end
popd