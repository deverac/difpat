@echo off
rem The names of test directories must begin with 't'.
rem The names of test directories are used as labels.
rem To avoid accidentally jumping to the wrong label,
rem labels that are 8 chars or shorter should not start with the letter 't'.

set TDIR=%1
if "%TDIR%" == "" goto errparm

echo.
echo Executing pat\%TDIR%
set TARGS=

cd %TDIR%

if exist tmp\NUL deltree /y tmp > NUL


if %TDIR% == th       goto dotest0
if %TDIR% == tq       goto dotest0
if %TDIR% == tv       goto dotest0

if %TDIR% == tn-add   goto dotest1
if %TDIR% == tn-chg   goto dotest1
if %TDIR% == tn-del   goto dotest1
if %TDIR% == tn-mix   goto dotest1
if %TDIR% == tu-add   goto dotest1
if %TDIR% == tu-chg   goto dotest1
if %TDIR% == tu-del   goto dotest1
if %TDIR% == tu-mix   goto dotest1

if %TDIR% == tnd      goto dotest2
if %TDIR% == tnd-r    goto dotest2
if %TDIR% == tnd-re   goto dotest2
if %TDIR% == tud      goto dotest2
if %TDIR% == tud-r    goto dotest2
if %TDIR% == tud-re   goto dotest2

if %TDIR% == tt-i-un  goto dotest3
if %TDIR% == tt-nrm-m goto dotest3
if %TDIR% == tt-nrm-s goto dotest3
if %TDIR% == tt-uni-m goto dotest3
if %TDIR% == tt-uni-s goto dotest3
if %TDIR% == tt-unkno goto dotest3

if %TDIR% == tk       goto dotest4

if %TDIR% == tw-alt   goto dotest5
if %TDIR% == tw-defau goto dotest5

if %TDIR% == tn       goto dotest6
if %TDIR% == tu       goto dotest6

if %TDIR% == tn-creat goto dotest7
if %TDIR% == tu-creat goto dotest7

echo Unhandled test case %TDIR%
goto errfin





rem Test single-arg options.
:dotest0
    if %TDIR% == th       set TARGS=/h
    if %TDIR% == tq       set TARGS=/q /h
    if %TDIR% == tv       set TARGS=/v
    rem
    rem Run pat.exe
    rem
    set PATCMD=pat.exe %TARGS%
    echo   %PATCMD%
    %PATCMD% > output.txt
    if ERRORLEVEL 1 goto errpat
    rem
    rem Redact output
    rem
    if %TDIR% == th       call ..\..\redact.bat patPrefix
    if %TDIR% == tv       call ..\..\redact.bat patVersion
    rem
    rem Compare expected output
    rem
    set FCCMD=fc.exe %FCARGS% output.txt expect-d.txt
    echo   %FCCMD%
    %FCCMD% > NUL
    if ERRORLEVEL 1 goto errfc
    goto fin





rem Pat a single file.
:dotest1
    set TARGS=
    rem
    rem Create patch
    rem
    set DIFCMD=dif.exe %TARGS% a.txt b.txt
    echo   %DIFCMD%
    %DIFCMD% > patch.txt
    if ERRORLEVEL 2 goto errdif
    rem
    rem Patch file
    rem
    set PATCMD=pat.exe /s patch.txt a.txt
    echo   %PATCMD%
    %PATCMD% > output.txt
    if ERRORLEVEL 1 goto errpat
    rem
    rem Convert Unix to DOS
    rem Redirects can not be echoed, so command is quoted.
    rem
    echo   "trch \n ^J < b.txt > b-dos.txt"
    trch \n ^J < b.txt > b-dos.txt
    rem
    rem Compare patched file
    rem
    set FCCMD=fc.exe b-dos.txt output.txt
    echo   %FCCMD%
    %FCCMD% > NUL
    if ERRORLEVEL 1 goto errfc
    goto fin






rem Test patching directories.
:dotest2
    goto %TDIR%
:tnd
:tud
    set DIFARGS=
    set FCARGS=
    goto tst2start
:tnd-r
:tnd-re
:tud-r
:tud-re
    rem FC has no equivalent option to '/e', so '/r' and '/r /e' are the same.
    set DIFARGS=/r /e
    set FCARGS=/s
    goto tst2start
:tst2start
    rem
    rem Generate patch
    rem
    set DIFCMD=dif %DIFARGS% dir1 dir2
    echo   %DIFCMD%
    %DIFCMD% > patch.txt
    if errorlevel 2 goto errdif
    set d1=d1copy
    if exist %d1%\NUL deltree /y %d1% > NUL
    xcopy /e dir1 %d1%\ > NUL
    rem
    rem Patch directory
    rem
    set PATCMD=pat /k patch.txt %d1%
    echo   %PATCMD%
    %PATCMD%
    rem
    rem Compare patched directory
    rem
    set FCCMD=fc %FCARGS% d1copy dir2
    echo   %FCCMD%
    %FCCMD% > NUL
    if errorlevel 1 goto errfc
    if exist %d1%\NUL deltree /y %d1% > NUL
    goto fin




rem Test '/t' option.
:dotest3
    goto %TDIR%
:tt-i-un
    set FNAM=type-iun.pat
    set ERR_VAL=3
    set EXP_VAL=2
    goto tst3start
:tt-nrm-m
    set FNAM=type-nm.pat
    set ERR_VAL=5
    set EXP_VAL=4
    goto tst3start
:tt-nrm-s
    set FNAM=type-ns.pat
    set ERR_VAL=4
    set EXP_VAL=3
    goto tst3start
:tt-uni-m
    set FNAM=type-um.pat
    set ERR_VAL=7
    set EXP_VAL=6
    goto tst3start
:tt-uni-s
    set FNAM=type-us.pat
    set ERR_VAL=6
    set EXP_VAL=5
    goto tst3start
:tt-unkno
    set FNAM=type-u.pat
    set ERR_VAL=2
    set EXP_VAL=1
    goto tst3start
:tst3start
    set PATCMD=pat /t %FNAM%
    echo   %PATCMD%
    %PATCMD% > output.txt
    if errorlevel %ERR_VAL% goto errpat
    if errorlevel %EXP_VAL% goto tst3ok
    goto errpat
:tst3ok
    set FCCMD=fc.exe expect-d.txt output.txt
    echo   %FCCMD%
    %FCCMD% > NUL
    if ERRORLEVEL 1 goto errfc
    goto fin




rem Test '/k' option.
:dotest4
    set WORKDIR=tmp
    set PATARGS=/k
    if exist %WORKDIR%\NUL deltree /y %WORKDIR% > NUL
    if not exist %WORKDIR%\NUL goto tst4pat
    echo "Error: Expected '%WORKDIR%' to not exist"
    goto errn
:tst4pat
    set PATCMD=pat %PATARGS% pat.txt a.txt
    echo   %PATCMD%
    %PATCMD% > output.txt
    if errorlevel 1 goto errfin
    if exist %WORKDIR%\NUL goto tst4ok
    echo "Error: Expected '%WORKDIR%' to exist"
    goto errfin
:tst4ok
    deltree /y %WORKDIR% > NUL
    goto fin




rem Test '/w workdir' option.
:dotest5
    goto %TDIR%
:tw-alt
    set WORKDIR=altdir
    goto tst5start
:tw-defau
    set WORKDIR=tmp
    goto tst5start
:tst5start
    set PATARGS=/w %WORKDIR%
    if exist %WORKDIR%\NUL deltree /y %WORKDIR% > NUL
    mkdir %WORKDIR%
    set PATCMD=pat %PATARGS% pat.txt a.txt
    echo   %PATCMD%
    %PATCMD% > output.txt
    if errorlevel 1 goto errfin
    deltree /y %WORKDIR% > NUL

    goto fin






rem Pat file in place. (i.e. Do not use /screen option)
:dotest6
    if %TDIR% == tn       set TARGS=/n
    if %TDIR% == tu       set TARGS=/u
    rem
    rem Create patch
    rem
    set DIFCMD=dif.exe %TARGS% a.txt b.txt
    echo   %DIFCMD%
    %DIFCMD% > patch.txt
    if ERRORLEVEL 2 goto errdif
    rem
    rem Patch file
    rem
    copy a.txt output.txt > NUL
    set PATCMD=pat.exe patch.txt output.txt
    echo   %PATCMD%
    %PATCMD%
    if ERRORLEVEL 1 goto errpat
    rem
    rem Convert Unix to DOS
    rem Redirects can not be echoed, so command is quoted.
    rem
    echo   "trch \n ^J < b.txt > b-dos.txt"
    trch \n ^J < b.txt > b-dos.txt
    rem
    rem Compare patched file
    rem
    set FCCMD=fc.exe b-dos.txt output.txt
    echo   %FCCMD%
    %FCCMD% > NUL
    if ERRORLEVEL 1 goto errfc
    goto fin







rem Test /create option (for single file).
:dotest7
    if %TDIR% == tn-cmpos    set TARGS=/n
    if %TDIR% == tu-cmpos    set TARGS=/u
    rem
    rem Create patch
    rem
    set DIFCMD=dif.exe %TARGS% a.txt b.txt
    echo   %DIFCMD%
    %DIFCMD% > patch.txt
    if ERRORLEVEL 2 goto errdif
    rem
    rem Patch file
    rem
    copy a.txt output.txt > NUL
    set PATCMD=pat.exe /create patch.txt output.txt
    echo   %PATCMD%
    %PATCMD%
    if ERRORLEVEL 1 goto errpat
    rem
    rem Convert Unix to DOS
    rem Redirects can not be echoed, so command is quoted.
    rem
    echo   "trch \n ^J < b.txt > b-dos.txt"
    trch \n ^J < b.txt > b-dos.txt
    rem
    rem Compare output.txt, which should match a.txt.
    rem
    set FCCMD=fc.exe a.txt output.txt
    echo   %FCCMD%
    %FCCMD% > NUL
    if ERRORLEVEL 1 goto errfc
    rem
    rem Compare b-dos.txt, which should match generated target file.
    rem
    set FCCMD=fc.exe b-dos.txt tmp\00000001.tgt 
    echo   %FCCMD%
    %FCCMD% > NUL
    if ERRORLEVEL 1 goto errfc
    goto fin







:errdif
    echo Error: Dif failed: %DIFCMD%
    goto errfin


:errpat
    echo Error: Pat failed: %PATCMD%
    goto errfin


:errfc
    echo Error: Validation failed: %FCCMD%
    goto errfin


:errname
    echo Error: Unhandled test %TDIR%
    goto errfin



:errparm
    echo Error: No test specified
    call ..\serr.bat 1
    goto end


:errfin
    cd ..
    call ..\serr.bat 1
    goto end


:fin
    cd ..
    call ..\serr.bat 0
    goto end


:end
    set DIFCMD=
    set PATCMD=
    set FCCMD=
    set NAM1=
    set NAM2=
    set TARGS=
    set PATARGS=
    set DIFARGS=
    set FCARGS=
    set ERR_VAL=
    set EXP_VAL=
    set TDIR=
    set WORKDIR=
    set FNAM=
    set D1=
