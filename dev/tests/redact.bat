@echo off
rem Overwrite specific locations of output.txt with 'X's.
rem First and only parameter should be one of:
rem    difTimestamp
rem    difHash
rem    difPrefix
rem    difVersion
rem    difSort
rem    patPrefix
rem    patVersion
rem No error checking is performed.

goto r_%1




:r_
echo Missing parameter
goto end




:r_difTimestamp
    rem Redact the seconds (e.g. 19:48:2X). For some reason, the timestamp value
    rem returned by stat() sometimes differs by one second.
    fmod.exe output.txt 1 29 1 X
    fmod.exe output.txt 2 29 1 X

    rem Redact the timezone info (e.g. -0400).
    fmod.exe output.txt 1 41 5 X
    fmod.exe output.txt 2 41 5 X
    goto end




:r_difHash
    rem Redact hash values. 64-bit hash values are 20 chars long. DOS uses 16-bit
    rem hash values, which are not 20 chars long, but the outputs must match. 
    fmod.exe output.txt 4 11 20 X
    fmod.exe output.txt 6 12 20 X

    fmod.exe output.txt 11 11 20 X
    fmod.exe output.txt 13 12 20 X
    goto end




:r_difPrefix
    rem Redact program name.
    fmod.exe output.txt 1 8 7 X

    rem Redact prefixes of options. e.g. '/q,  /quiet'
    set LINES=(3 4 5 6 7 8 9 10 11 12 13 14 15 16 17)
    for %%i in %LINES% do fmod.exe output.txt %%i 3 1 X
    for %%i in %LINES% do fmod.exe output.txt %%i 7 2 X
    set LINES=
    goto end




:r_difVersion
    rem Redact program name.
    fmod.exe output.txt 1 1 7 X
    goto end




:r_difSort
    rem This does not redact anything; it is a utility function.
    rem When iterating directory entries, files may be returned in any order
    rem and so must be sorted before comparing.
    ren output.txt _out.txt
    sort _out.txt > output.txt
    del _out.txt > NUL

    ren expect-d.txt _exp.txt
    sort _exp.txt > expect-d.txt
    del _exp.txt > NUL
    goto end


:r_patPrefix
    rem Redact program name.
    fmod.exe output.txt 1 8 7 X
    fmod.exe output.txt 2 8 7 X
    fmod.exe output.txt 3 8 7 X

    fmod.exe output.txt 3 17 1 X   ; '/t'
    fmod.exe output.txt 3 22 2 X   ; '/type'

    rem Redact prefixes of options. e.g. '/q,  /quiet'
    set LINES=(5 6 7 8 9 10 11 12)
    for %%i in %LINES% do fmod.exe output.txt %%i 3 1 X
    for %%i in %LINES% do fmod.exe output.txt %%i 7 2 X

    fmod.exe output.txt 15  9 1 X   ; '/c'
    fmod.exe output.txt 15 22 1 X   ; '/k'

    set LINES=
    goto end


:r_patVersion
    rem Redact program name.
    fmod.exe output.txt 1 1 7 X
    goto end

:end
