@echo off
set TDIR=%1

if "%TDIR%" == "" goto errparm

set TARGS=
if %TDIR% == t-dirs   set TARGS=/n
if %TDIR% == ta-bang  set TARGS=/a efghiJ /g /b
if %TDIR% == ta-equal set TARGS=/a efghIj /g /b
if %TDIR% == ta-gt    set TARGS=/a eFghij
if %TDIR% == ta-lt    set TARGS=/a Efghij
if %TDIR% == ta-minus set TARGS=/a efgHij /u
if %TDIR% == ta-plus  set TARGS=/a efGhij /u
if %TDIR% == tb       set TARGS=/b
if %TDIR% == tb-mod   set TARGS=/b
if %TDIR% == tb-same  set TARGS=/b
if %TDIR% == tb-sameg set TARGS=/b /g
if %TDIR% == tc       set TARGS=/c
if %TDIR% == td       set TARGS=/d
if %TDIR% == te-left  set TARGS=/e
if %TDIR% == te-right set TARGS=/e
if %TDIR% == th       set TARGS=/h
if %TDIR% == tl-m-d   set TARGS=/l
if %TDIR% == tl-u-d   set TARGS=/l
if %TDIR% == tl-u-m   set TARGS=/l
if %TDIR% == tn       set TARGS=/n
if %TDIR% == tn-1     set TARGS=/n
if %TDIR% == tn-2     set TARGS=/n
if %TDIR% == tn-dflt  set TARGS=
if %TDIR% == tq       set TARGS=/q
if %TDIR% == tr       set TARGS=/r
if %TDIR% == tr-rb    set TARGS=/r /b
if %TDIR% == tr-rbg   set TARGS=/r /b /g
if %TDIR% == tr-re    set TARGS=/r /e
if %TDIR% == ts-spc   set TARGS=/s
if %TDIR% == ts-tab   set TARGS=/s
if %TDIR% == tt       set TARGS=/t
if %TDIR% == tt-g     set TARGS=/t /g
if %TDIR% == tt-w     set TARGS=/t /w 10
if %TDIR% == tu       set TARGS=/u
if %TDIR% == tv       set TARGS=/v



set DODIR=0
if exist %TDIR%\dira\NUL set DODIR=1


if %TDIR% == tn-dflt goto dotest
if "%TARGS%" == "" goto errname


:dotest
    echo.
    echo Executing dif\%TDIR%

    cd %TDIR%

    set NAM1=dira
    set NAM2=dirb

    if %DODIR%==1 goto rundif

    set NAM1=a.txt
    set NAM2=b.txt


:rundif
    rem
    rem Run diff 
    rem
    set DIFCMD=dif.exe %TARGS% %NAM1% %NAM2%
    echo   %DIFCMD%
    %DIFCMD% > output.txt
    if ERRORLEVEL 2 goto errdif


    rem
    rem Adjust output
    rem
    if %TDIR% == ta-minus call ..\..\redact.bat difTimestamp
    if %TDIR% == ta-plus  call ..\..\redact.bat difTimestamp
    if %TDIR% == tc       call ..\..\redact.bat difTimestamp
    if %TDIR% == tu       call ..\..\redact.bat difTimestamp
    if %TDIR% == td       call ..\..\redact.bat difHash
    if %TDIR% == th       call ..\..\redact.bat difPrefix
    if %TDIR% == tv       call ..\..\redact.bat difVersion


    set FCARGS=
    if %DODIR% == 0 goto validate

    set FCARGS=/C
    ren output.txt out.txt
    sort /a out.txt > output.txt
    del out.txt


:validate
    rem
    rem Validate output
    rem
    set FCCMD=fc.exe %FCARGS% output.txt expect-d.txt
    echo   %FCCMD%
    %FCCMD% > NUL
    if ERRORLEVEL 1 goto errfc

    cd ..

    goto fin


:errdif
    cd ..
    echo Diff failed: %DIFCMD%
    goto errn


:errfc
    cd ..
    echo Validation failed: %FCCMD%
    goto errn


:errparm
    echo No test specified
    goto errn


:errname
    echo Unhandled test %TDIR%
    goto errn


:errn
    call ..\serr.bat 1
    goto end


:fin
    call ..\serr.bat 0
    goto end

:end
    set DIFCMD=
    set FCCMD=
    set NAM1=
    set NAM2=
    set TARGS=
    set TDIR=
    set DODIR=
