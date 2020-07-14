@echo off
rem This bat file uses the FIND command to set ERRORLEVEL.
rem If the first param is '0', ERRORLEVEL will be set to 0.
rem Anything else (including no params) will set ERRORLEVEL to 1.
rem
rem Usage:
rem   call serr.bat 0    ; Set errorlevel to 0.
rem   call serr.bat 1    ; Set errorlevel to 1
rem   call serr.bat      ; (No params) Set errorlevel to 1.

if "%1" == "0" goto set0

rem Set errorlevel to 1
find NUL NUL > NUL
goto end

:set0
rem Set errorlevel to 0
find NUL . > NUL

:end
