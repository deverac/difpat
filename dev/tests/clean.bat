@echo off
rem This deletes files generated by tests.

set DIRS_TMP=dirs.tmp

rem 'dir' parameters
rem -p  Do not pause
rem l   Lowercase
rem b   Brief
rem ad  Attrib directories


set TOP=dif
dir /-p /l /b /ad %TOP%\t* > %DIRS_TMP%
rem Initialize counter to 0.
call cnt.bat init
:loop1
    rem Increment 'counter'
    call cnt.bat
    set /e TDIR=..\gen\fiter.exe %DIRS_TMP% %counter%
    if "%TDIR%" == "" goto difend
    echo Cleaning %TOP%\%TDIR%
    if exist %TOP%\%TDIR%\_expect.txt   del %TOP%\%TDIR%\_expect.txt  > NUL
    if exist %TOP%\%TDIR%\expect-u.txt  del %TOP%\%TDIR%\expect-u.txt > NUL
    if exist %TOP%\%TDIR%\output.txt    del %TOP%\%TDIR%\output.txt   > NUL
    goto loop1
:difend


set TOP=pat
dir /-p /l /b /ad %TOP%\t* > %DIRS_TMP%
rem Initialize counter to 0.
call cnt.bat init
:loop2
    rem Increment 'counter'
    call cnt.bat
    set /e TDIR=..\gen\fiter.exe %DIRS_TMP% %counter%
    if "%TDIR%" == "" goto patend
    echo Cleaning %TOP%\%TDIR%
    if exist %TOP%\%TDIR%\expect-u.txt  del %TOP%\%TDIR%\expect-u.txt > NUL
    if exist %TOP%\%TDIR%\output.txt    del %TOP%\%TDIR%\output.txt   > NUL
    if exist %TOP%\%TDIR%\patch.txt     del %TOP%\%TDIR%\patch.txt    > NUL
    if exist %TOP%\%TDIR%\tmp\NUL       deltree /y %TOP%\%TDIR%\tmp   > NUL
    goto loop2
:patend


del %DIRS_TMP%
call cnt.bat clear

set DIRS_TMP=
set TDIR=
set TOP=
