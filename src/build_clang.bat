@echo off

set "current_folder=%cd%"
IF EXIST "../builds/" goto :DO_BUILD
mkdir "../builds/"

:DO_BUILD
pushd "../builds/"
call clang++ %current_folder%/platform/win32/win32main.cpp -fno-show-column -fno-caret-diagnostics -Wall -Wno-missing-braces -pedantic -DUNICODE -I%current_folder%/../lib/ssemath -DARCHITECTURE_X64 -DARCHITECTURE_LITTLE_ENDIAN -DARCHITECTURE_IEEE_754 -DGAME_DEBUG=1 -DGAME_FLIP_Z -DWIN32_LEAN_AND_MEAN -DNOMINMAX -Wl,user32.lib,gdi32.lib,opengl32.lib -m64 -I%current_folder%/../lib/tm -I%current_folder%/../lib/stb -I%current_folder% -o game_clang.exe
popd