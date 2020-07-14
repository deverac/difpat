@echo off
if "%1" == "" goto errparm


rem Special case to check for directory.
if not exist .\tests\%1\NUL goto errdir


cd .\tests\%1

if not exist ..\..\gen\fiter.exe goto errfiter

rem -p  Do not pause
rem l   Lowercase
rem b   Brief
rem ad  Attrib directories
dir /-p /l /b /ad t* > tnames.txt



rem Initialize counter to 0.
call ..\cnt.bat init

set TNUM=0

:loop
    rem Increment counter
    call ..\cnt.bat
    set TNUM=%counter%

    set /e TNAME=..\..\gen\fiter.exe tnames.txt %TNUM%
    if "%TNAME%" == "" goto alldone

    if not exist runtest.bat goto errbat

    call runtest.bat %TNAME%
    if ERRORLEVEL 1 goto errtst
    goto loop



:alldone
    echo.
    echo === Completed %1 tests ===
    cd ..\..
    goto fin


:errtst
    echo === Error tst ===
    cd ..\..
    goto finerr


:errbat
    echo === Error: runtest.bat does not exist ===
    cd ..\..
    goto finerr


:errfiter
    echo === Error: Missing fiter.exe. Run 'build all' to create ===
    cd ..\..
    goto finerr


:errdir
    echo === Error: '%1' dir not found ===
    goto finerr


:errparm
    echo === Error: Missing parameter ===
    goto finerr


:finerr
    call .\tests\serr.bat 1
    goto end


:fin
    call .\tests\serr.bat 0
    goto end


:end
    if exist .\tests\%1\tnames.txt del .\tests\%1\tnames.txt
    call .\tests\cnt.bat clear
    set TNUM=
    set TNAME=
