@echo off
if "%1" == "" goto :dev
if "%1" == "dev" goto :dev
if "%1" == "pc" goto :pc

:dev
subst v: "C:\Users\Dev\Dropbox\projects\pvgame"
goto :eof

:pc
subst v: "C:\Dropbox\projects\pvgame"
goto :eof
