#!/bin/sh
set -e


if [ "$1" = "clean" ]; then

    rm -f dp.zip
    rm -rf gen
    rm -rf dev/gen

elif [ "$1" = "zip" ]; then

   rm -rf dp.zip
   7z a dp.zip dev dif.c pat.c build.sh build.bat README.md LICENSE.txt
   printf "\nCreated dp.zip\n"

elif [ "$1" = "rt" ]; then

    cd dev
    ./runtests.sh rt
    cd ..

elif [ "$1" = "todos" ]; then

    cd dev
    ./runtests.sh todos
    cd ..

elif [ "$1" = "ct" ]; then
    cd dev
    ./runtests.sh clean
    cd ..

elif [ "$1" = "all" ]; then

    if [ "$1" = "all" ]; then
        mkdir -p ./dev/gen
        gcc ./dev/helper/fmod.c -o ./dev/gen/fmod
    fi
    
    mkdir -p gen
    gcc dif.c -o ./gen/dif
    gcc pat.c -o ./gen/pat
    EXIT_OK=1
else
    printf "  Targets\n"
    printf "     clean  Clean build targets\n"
    printf "     ct     Clean tests\n"
    printf "     all    Build all programs\n"
    printf "     rt     Run tests\n"
    printf "     todos  Convert tests to DOS\n"
    printf "     zip    Create dp.zip containing all source files\n"
    EXIT_OK=1
fi
