@echo off
rem myvarsall sets up path so that it contains correct cl based on target
call clangvars.bat

set "project_path=V:"
call clang_tidy.bat %1

exit /b 0