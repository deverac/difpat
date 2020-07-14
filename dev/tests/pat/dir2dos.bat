@echo off
rem This converts all files in test directories from Unix to DOS format.
rem It also deletes files with invalid filenames in the directories.
rem This should only be run on the 'pat' tests, and not the 'dif' tests,
rem because certain 'dif' test files must have different line endings in
rem order to test comparing them.
  
if not exist ..\..\gen\fiter.exe goto errfiter

set OUT_TMP=out.tmp
set DIRS_TMP=dirs.tmp
set FILS_TMP=files.tmp


rem Construct list of all test directories.
rem -p  Do not pause
rem l   Lowercase
rem b   Brief
rem ad  Attrib directories
dir /-p /l /b /ad t* > %DIRS_TMP%




rem Inspect test directories and construct list of files in 'dir1' and 'dir2'.
type NUL > %FILS_TMP%

rem Initialize counter to 0.
call ..\cnt.bat init

echo Collecting
:loop1

    rem Increment 'counter'
    call ..\cnt.bat

    set /e NAM=..\..\gen\fiter.exe %DIRS_TMP% %counter%
    if "%NAM%" == "" goto procfiles

    rem If dir1 exists, assume both dir1 and dir2 exist.
    if not exist %NAM%\dir1\NUL goto loop1

    rem List all files in dir1 and dir2 (recursively)
    rem -p   Do not pause
    rem l    Lowercase
    rem b    Brief
    rem s    Recurse
    rem a-d  Exclude directories
    dir /-p /l /b /s /a-d %NAM%\dir1 >> %FILS_TMP%
    dir /-p /l /b /s /a-d %NAM%\dir2 >> %FILS_TMP%

    goto loop1




rem Convert listed files from Unix format to DOS format.
:procfiles
    call ..\cnt.bat init
    echo Converting

:loop2
    rem Increment 'counter'
    call ..\cnt.bat

    set /e NAM=..\..\gen\fiter.exe %FILS_TMP% %counter%
    if "%NAM%" == "" goto alldone

    rem Ignore directories
    if exist %NAM%\NUL goto loop2

    rem Only continue with valid filenames; delete invalid filenames.
    if exist %NAM% goto convert
    del "%NAM%"

    goto loop2

:convert
    trch \n ^J < %NAM% > %OUT_TMP%
    copy /y %OUT_TMP% %NAM% > NUL

    goto loop2





:alldone
    if exist %OUT_TMP%  del %OUT_TMP%
    if exist %DIRS_TMP% del %DIRS_TMP%
    if exist %FILS_TMP% del %FILS_TMP%
    echo.
    echo === Completed DOS conversions ==
    goto fin


:errfiter
    echo Error: fiter.exe does not exist. Run 'build all' to build it.
    goto fin

:fin
    call ..\cnt.bat clear
    goto end

:end
    set NAM=
    set OUT_TMP=
    set DIRS_TMP=
    set FILS_TMP=
