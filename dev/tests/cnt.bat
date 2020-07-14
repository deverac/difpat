@echo off
rem Increments 'counter' environment variable from 0 to 9999.
rem When counter reaches max value, it will restart at 0.
rem   call cnt.bat init   (Re-)Initialize counter to 0.
rem   call cnt.bat        Increment counter by 1.
rem   call cnt.bat clear  Clears/Resets env vars used by this batch file.
rem 
rem Since 'global' environment variables are used, only one instance of
rem cnt.bat can be used at a time.
rem
rem To initialize counter to the value 3251:
rem    set c4=3
rem    set c3=2
rem    set c2=5
rem    set c1=0  ; One less than desired value.
rem    call cnt.bat     Increments vars, setting 'counter' to 3251.

if "%1" == "init" goto init
if "%1" == "clear" goto clear


if "%c1%" == "9" goto hop1
set cnam=c1
set cval=%c1%
goto doinc


:hop1
set c1=0
if "%c2%" == "9" goto hop2
set cnam=c2
set cval=%c2%
goto doinc


:hop2
set c2=0
if "%c3%" == "9" goto hop3
set cnam=c3
set cval=%c3%
goto doinc


:hop3
set c3=0
if "%c4%" == "9" goto overflow
set cnam=c4
set cval=%c4%
goto doinc


:doinc
if "%cval%" == "8" set %cnam%=9
if "%cval%" == "7" set %cnam%=8
if "%cval%" == "6" set %cnam%=7
if "%cval%" == "5" set %cnam%=6
if "%cval%" == "4" set %cnam%=5
if "%cval%" == "3" set %cnam%=4
if "%cval%" == "2" set %cnam%=3
if "%cval%" == "1" set %cnam%=2
if "%cval%" == "0" set %cnam%=1
if "%cval%" == ""  set %cnam%=1
goto fin


:overflow
echo "Counter overflow"
:init
set c4=
set c3=
set c2=
set c1=0
set counter=
goto fin


:fin
set cnam=
set cval=
set counter=%c4%%c3%%c2%%c1%
goto end


:clear
set c4=
set c3=
set c2=
set c1=
set cnam=
set cval=
set counter=
goto end

:end
