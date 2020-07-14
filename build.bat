@echo off
rem FreeDOS does not set errorlevel when a command is not found. To 'fix' this
rem we set the errorlevel to 1 prior to executing wcl. If wcl succeeds,
rem errorlevel will be 0; otherwise errorlevel will be 1.


if not exist dev\NUL unzip dev.zip
if not exist dev\NUL goto errdev
if exist dev.zip del dev.zip


if "%1" == "difpat" goto difpat
if "%1" == "all"    goto all
if "%1" == "clean"  goto clean
if "%1" == "rt"     goto rt
if "%1" == "todos"  goto todos
if "%1" == "pkg"    goto pkg


echo.
echo   Targets:
echo      clean   Clean generated files
echo      difpat  Build dif.exe and pat.exe
echo      all     Run difpat and build other programs for testing.
echo      todos   Convert directory files to DOS format
echo      rt      Run tests
echo      pkg     Build DOS package
echo.
goto fin


:pkg
    set PKG=pkg
    set GRP=devel
    set NAM=difpat
    if exist %PKG%\NUL deltree /y %PKG% > NUL
    if exist difpat.zip del difpat.zip  > NUL

    mkdir %PKG%
    mkdir %PKG%\appinfo
    set LSM=%PKG%\appinfo\difpat.lsm
    echo Begin3> %LSM%
    echo Title:          difpat>> %LSM%
    echo Version:        0.1>> %LSM%
    echo Entered-date:   2020-07-05>> %LSM%
    echo Description:    Diff and patch files or directories.>> %LSM%
    echo Keywords:       freedos dif pat>> %LSM%
    echo Author:         >> %LSM%
    echo Maintained-by:  >> %LSM%
    echo Primary-site:   >> %LSM%
    echo Alternate-site: >> %LSM%
    echo Original-site:  >> %LSM%
    echo Platforms:      DOS>> %LSM%
    echo Copying-policy: GPL>> %LSM%
    echo End>> %LSM%

    mkdir %PKG%\%GRP%
    mkdir %PKG%\source
    mkdir %PKG%\source\%NAM%
    if not exist %PKG%\source\%NAM%\NUL goto err1pkg

    copy build.bat   %PKG%\source\%NAM%
    copy dif.c       %PKG%\source\%NAM%
    copy LICENSE.txt %PKG%\source\%NAM%
    copy pat.c       %PKG%\source\%NAM%
    copy readme.md   %PKG%\source\%NAM%

    rem When the package source is installed, the FreeDOS package manager
    rem (perhaps sensibly) does not write empty directories. The difpat tests
    rem contain empty directories. To work-around this, the tests are zipped.
    rem Zipping also avoids exceeding the FreeDOS max path length (63 chars).
    zip -r %PKG%\source\%NAM%\dev.zip dev
    if not exist %PKG%\source\%NAM%\dev.zip goto err2pkg

    mkdir %PKG%\%GRP%\%NAM%
    copy .\gen\dif.exe %PKG%\%GRP%\%NAM%
    if not exist %PKG%\%GRP%\%NAM%\dif.exe goto err3pkg
    copy .\gen\pat.exe %PKG%\%GRP%\%NAM%
    if not exist %PKG%\%GRP%\%NAM%\pat.exe goto err3pkg
    copy .\dev\helper\difpat.txt %PKG%\%GRP%\%NAM%
    if not exist %PKG%\%GRP%\%NAM%\difpat.txt goto err3pkg

    mkdir %PKG%\links
    echo %GRP%\%NAM%\dif.exe > %PKG%\links\dif.bat
    echo %GRP%\%NAM%\pat.exe > %PKG%\links\pat.bat
    if not exist %PKG%\links\pat.bat goto err4pkg

    cd .\%PKG%
    rem  -9  Max compression
    rem  -r  Recurse into directories
    zip -9 -r ..\difpat.zip *
    if not exist ..\difpat.zip goto err5pkg
    cd ..

    echo.
    echo The difpat.zip package has been created.
    echo.
    echo To install: fdnpkg install difpat.zip
    echo         or: fdnpkg install-wsrc difpat.zip
    echo  To remove: fdnpkg remove difpat
    goto fin


:todos
    rem Remove Unix scripts
    if exist build.sh del build.sh
    if exist dev\runtests.sh del dev\runtests.sh

    rem Convert files in directories from Unix to DOS format.
    cd dev\tests\pat
    call dir2dos.bat
    cd ..\..\..
    goto fin


:rt
    set OLDPATH=%PATH%
    set /e CURDIR=cd

    rem Setting CTTY to NUL re-assigns stdout, stderr and stdin (the keyboard,
    rem so input is ignored). As a result, pausing or prompting for input
    rem prior to restoring with the CTTY CON command requires a reboot to 
    rem regain control. Setting CTTY to CON restores stdout, stderr, stdin.

    ctty NUL
    dif.exe /v > chk.tmp
    ctty CON
    find /I "dif.exe" chk.tmp > NUL
    if errorlevel 1 PATH=%CURDIR%\gen;%PATH%

    ctty NUL
    fmod.exe > chk.tmp
    ctty CON
    find /I "usage:" chk.tmp > NUL
    if errorlevel 1 PATH=%CURDIR%\dev\gen;%PATH%


    del chk.tmp
    set CURDIR=

    cd .\dev

    call runtests.bat dif
    if errorlevel 1 goto err1up

    call runtests.bat pat
    if errorlevel 1 goto err1up

    cd ..

    goto fin



:clean
    if not exist .\dev\gen\fiter.exe goto errclean
    cd dev\tests
    call clean.bat
    cd ..\..
    if exist gen\NUL     deltree /y .\gen     > NUL
    if exist dev\gen\NUL deltree /y .\dev\gen > NUL
    if exist pkg\NUL     deltree /y .\pkg     > NUL
    if exist difpat.zip del difpat.zip
    goto fin


:all
    rem Check if wcl is callable.
    call .\dev\tests\serr.bat 1
    wcl /q
    if errorlevel 1 goto errwcl

    if not exist .\dev\NUL goto finerr
    if not exist .\dev\gen\NUL mkdir .\dev\gen
    cd .\dev\gen

    echo.
    echo == Building fiter.exe ==
    wcl /q ..\helper\fiter.c
    if errorlevel 1 goto err2up

    echo.
    echo == Building fmod.exe ==
    wcl /q ..\helper\fmod.c
    if errorlevel 1 goto err2up

    cd ..\..
    goto difpat


:difpat
    if not exist dif.c goto finerr
    if not exist pat.c goto finerr

    rem Check if wcl is callable.
    call .\dev\tests\serr.bat 1
    wcl /q
    if errorlevel 1 goto errwcl

    if not exist .\gen\NUL mkdir .\gen
    cd gen

    rem /mc  Compact memory model: 64Kb code, max memory for data.

    echo.
    echo == Building dif.exe ==
    wcl /q /mc ..\dif.c
    if errorlevel 1 goto err1up

    echo.
    echo == Building pat.exe ==
    wcl /q /mc ..\pat.c
    if errorlevel 1 goto err1up

    cd ..
    goto fin


:err1pkg
    echo Error creating directory structure. Building package failed.
    goto finerr

:err2pkg
    echo Error copying source files. Building package failed. 
    goto finerr

:err3pkg
    echo Error copying executable files. Building package failed.
    goto finerr

:err4pkg
    echo Error building links. Building package failed. 
    goto finerr

:err5pkg
    echo Error creating zip file. Building package failed.
    goto finerr

:errwcl
    echo Run c:\devel\ow\owsetenv.bat to setup Open Watcom development environment.
    goto finerr

:errdev
    echo Error extracting dev.zip
    goto finerr

:err2up
    cd ..\..
    goto finerr

:err1up
    cd ..
    goto finerr

:errclean
    echo Error: Missing helper binaries. Run 'build all' to build them.
    goto finerr

:finerr
    echo Error occurred.
    call .\dev\tests\serr.bat 1
    goto end

:fin
    call .\dev\tests\serr.bat 0
    goto end

:end
    if not "%OLDPATH%" == "" set PATH=%OLDPATH%
    set OLDPATH=
