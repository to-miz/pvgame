@echo off
rem varsall.bat sets up path so that it contains clang binaries
call E:\clang\varsall.bat

call build_clang.bat
exit /b 0