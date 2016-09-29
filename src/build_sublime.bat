@echo off
rem myvarsall sets up path so that it contains correct cl based on target
call myvarsall.bat %1
call build.bat %1 %2
exit /b 0