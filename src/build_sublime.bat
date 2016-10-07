@echo off
rem myvarsall sets up path so that it contains correct cl based on target
call myvarsall.bat %1

set "project_path=V:"
if EXIST "../builds/" goto :DO_BUILD
mkdir "../builds/"
:DO_BUILD
pushd "../builds/"
call ../src/build.bat %1 %2 debug
popd

exit /b 0