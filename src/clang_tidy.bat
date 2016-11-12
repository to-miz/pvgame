@echo off
setlocal
IF [%project_path%] == [] (
	echo project_path not specified
	exit /b -1
)

rem fast, slow, analize
set "analize=slow"
if "%analize%" == "fast" (
set checks="-clang-analyzer-deadcode.DeadStores,-readability-implicit-bool-cast,-modernize-use-using,-modernize-use-bool-literals,-misc-unused-parameters,-misc-definitions-in-headers,-misc-macro-parentheses,-readability-named-parameter,-google-runtime-references,-readability-else-after-return,-misc-noexcept-move-constructor,-readability-inconsistent-declaration-parameter-name,-readability-braces-around-statements,-misc-misplaced-widening-cast,-modernize-raw-string-literal"
)
if "%analize%" == "slow" (
set checks="-clang-analyzer-deadcode.DeadStores,google-runtime-*,misc-*,modernize-*,performance-*,readability-*,-readability-implicit-bool-cast,-modernize-use-using,-modernize-use-bool-literals,-misc-unused-parameters,-misc-definitions-in-headers,-misc-macro-parentheses,-readability-named-parameter,-google-runtime-references,-readability-else-after-return,-misc-noexcept-move-constructor,-readability-inconsistent-declaration-parameter-name,-readability-braces-around-statements,-misc-misplaced-widening-cast,-modernize-raw-string-literal,-readability-static-definition-in-anonymous-namespace,-google-runtime-int"
)
if "%analize%" == "analize" (
set checks="clang-analyzer-*,-clang-diagnostic-warning,-clang-analyzer-deadcode.DeadStores,-clang-analyzer-alpha.unix.cstring.BufferOverlap"
)

rem set headers="(src/|lib/tm/)"
set headers=.*

if "%1" == "" goto :full_build
if "%1" == "dll" goto :dll_build

:full_build
clang-tidy -header-filter=%headers% -checks=%checks% %project_path%/src/platform/win32/win32main.cpp -- -fno-show-column -fno-caret-diagnostics -Wall -Wno-missing-braces -pedantic -DUNICODE -I%project_path%/lib/ssemath -DARCHITECTURE_X64 -DARCHITECTURE_LITTLE_ENDIAN -DARCHITECTURE_IEEE_754 -DGAME_DEBUG=1 -DGAME_FLIP_Z -DNOMINMAX -Wl,user32.lib,gdi32.lib,opengl32.lib -m64 -I%project_path%/lib/tm -I%project_path%/lib/stb -I%project_path%/src

if "%1" == "exe" goto :end
:dll_build
clang-tidy -header-filter=%headers% -checks=%checks% %project_path%/src/game_main.cpp -- -fno-show-column -fno-caret-diagnostics -Wall -Wno-missing-braces -pedantic -DUNICODE -I../lib/ssemath -DARCHITECTURE_X64 -DARCHITECTURE_LITTLE_ENDIAN -DARCHITECTURE_IEEE_754 -DGAME_DEBUG=1 -DGAME_FLIP_Z -DWIN32_LEAN_AND_MEAN -DNOMINMAX -Wl,user32.lib -m64 -I../lib/tm -I../lib/stb -DGAME_DLL

:end
exit /b 0
