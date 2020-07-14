#!/bin/sh

set -e

# Ensure sort is in expected order.
# This is also important to match the DOS 'sort /a' order.
LC_ALL=C

if [ ! "$1" = "clean" ]; then
    # dif and pat.
    if ! dif --version > /dev/null 2>&1; then
        PATH=$(pwd)/../gen:$PATH
    fi

    # fmod
    if ! fmod > /dev/null 2>&1; then
        PATH=$(pwd)/gen:$PATH
    fi


    if ! dif --version > /dev/null 2>&1; then
        printf "Error: dif not found\n"
        exit 1
    fi

    if ! pat --version > /dev/null 2>&1; then
        printf "Error: pat not found\n"
        exit 1
    fi
fi

EXIT_OK=0 # Init to false.


finish() {

    if [ $EXIT_OK -eq 1 ]; then
        printf "\n=== Completed successfully ===\n"
        exit 0
    else
        printf "\n=== ERROR: A test failed or an error occured ===\n"
        exit 1
    fi
}

trap finish EXIT

clean() {
    cd tests/dif
    for dir in t*; do
        rm -f "$dir/_expect.txt"
        rm -f "$dir/output.txt"
        rm -f "$dir/expect-d.txt"
    done
    cd ../..

    cd tests/pat
    for dir in t*; do
        rm -f "$dir/patch.txt"
        rm -f "$dir/output.txt"
        rm -f "$dir/expect-d.txt"
        rm -rf "$dir/tmp"
        rm -rf "$dir/d1copy"
    done
    cd ../..
}


runtest() {
    desc="$1"
    dir="$2"
    arg1="$3"
    arg2="$4"

    echo "$desc" \($dir\)
    cd $dir

    DIF=dif
    exp="expect-u.txt"

    if [ -d dira ]; then
        cmd1="$DIF $arg1 dira dirb | sort"
    else
        cmd1="$DIF $arg1 a.txt b.txt"
    fi


    printf "  $cmd1\n"
    set +e
    # Must 'eval' so that quoted args are interpreted correctly.
    eval $cmd1 > output.txt
    if [ "$?" = "2" ]; then
        printf "$DIF exited with error.\n"
        exit 1
    fi

    if [ ! "$arg2" = "" ]; then
        exp="_expect.txt"
        if [ -d dira ]; then
            cmd2="diff $arg2 dira dirb | sort"
        else
            cmd2="diff $arg2 a.txt b.txt"
        fi
        printf "  $cmd2\n"
        # Must 'eval' so that quoted args are interpreted correctly.
        eval $cmd2 > $exp
        if [ "$?" = "2" ]; then
            printf "diff exited with error.\n"
            exit 1
        fi
    fi
    set -e

    if [ $dir = t-dirs ]; then
        sed -i -e 's/^dif /diff --normal /g' output.txt
    fi
    if [ $dir = tr ]; then
        sed -i -e 's/^dif -r /diff --recursive /g' output.txt
    fi
    if [ $dir = tr-re ]; then
        sed -i -e 's/^dif -r -e /diff --recursive --new-file /g' output.txt
    fi

    # Compare actual output with expected output.
    diff output.txt $exp

    cd ..
}


runDifTests() {
    printf "============= DIF tests =============\n"
  cd tests/dif
  for dir in t*; do
    printf "\n"
    if [ -d $dir ]; then
      case "$dir" in
        t-dirs  ) runtest "Default format dirs" $dir "" "--normal"                   ;;
        ta-bang ) runtest "Alt bang"            $dir "-a efghiJ -g -b"               ;;
        ta-equal) runtest "Alt equal"           $dir "-a efghIj -g -b"               ;;
        ta-gt   ) runtest "Alt greater-than"    $dir "-a eFghij"                     ;;
        ta-lt   ) runtest "Alt less-than"       $dir "-a Efghij"                     ;;
        ta-minus) runtest "Alt minus"           $dir "-a efgHij -u"                  ;;
        ta-plus ) runtest "Alt plus"            $dir "-a efGhij -u"                  ;;
        tb      ) runtest "Brief"               $dir "-b" "--brief"                  ;;
        tb-mod  ) runtest "Brief differs"       $dir "-b"                            ;;
        tb-same ) runtest "Brief matches"       $dir "-b"                            ;;
        tb-sameg) runtest "Brief matches g"     $dir "-b -g"                         ;;
        td      ) runtest "Show data"           $dir "-d"                            ;;
        te-left ) runtest "Empty left"          $dir "-e" "--new-file"               ;;
        te-right) runtest "Empty right"         $dir "-e" "--new-file"               ;;
        th      ) runtest "Show help"           $dir "-h"                            ;;
        tl-m-d  ) runtest "Compare Mac/DOS"     $dir "-l"                            ;;
        tl-u-m  ) runtest "Compare Unix/Mac"    $dir "-l"                            ;;
        tl-u-d  ) runtest "Compare Unix/DOS"    $dir "-l"                            ;;
        tn      ) runtest "Normal format"       $dir "-n" "--normal"                 ;;
        tn-1    ) runtest "Normal adds"         $dir "-n" "--normal"                 ;;
        tn-2    ) runtest "Normal deletes"      $dir "-n" "--normal"                 ;;
        tn-dflt ) runtest "Default format"      $dir "" "--normal"                   ;;
        tr      ) runtest "Recurse"             $dir "-r" "--recursive"              ;;
        tr-rb   ) runtest "Recurse brief"       $dir "-r -b" "--recursive --brief"   ;;
        tr-rbg  ) runtest "Recurse brief gloss" $dir "-r -b -g"                      ;;
        tr-re   ) runtest "Recurse empty"       $dir "-r -e" "--recursive --new-file";;
        ts-spc  ) runtest "Ignore spaces"       $dir "-s" "--ignore-space-change"    ;;
        ts-tab  ) runtest "Ignore Tabs"         $dir "-s" "--ignore-all-space"       ;;
        tq      ) runtest "Quiet"               $dir "-q"                            ;;
        tt      ) runtest "Two col"             $dir "-t"                            ;;
        tt-g    ) runtest "Two col gloss"       $dir "-t -g"                         ;;
        tt-w    ) runtest "Two col width"       $dir "-t -w 10"                      ;;
        tu      ) runtest "Unified format"      $dir "-u" "-U 0"                     ;;
        tv      ) runtest "Show version"        $dir "-v"                            ;;
        *       ) echo Unhandled test case $dir; exit 1   ;;
      esac
    fi
  done
  cd ../..
}


runtest0() {
    desc="$1"
    dir="$2"
    arg="$3"

    echo "$desc" \($dir\)
    cd $dir

    PAT=pat

    cmd1="$PAT $arg"

    printf "  $cmd1\n"
    $cmd1 > output.txt

    diff expect-u.txt output.txt

    cd ..
}

runtest1() {
    desc="$1"
    dir="$2"
    arg1="$3"

    echo "$desc" \($dir\)
    cd $dir

    DIF=dif
    PAT=pat

    cmd1="$DIF $arg1 a.txt b.txt"

    printf "  $cmd1\n"
    set +e
    $cmd1 > patch.txt
    if [ "$?" = "2" ]; then
        printf "$DIF exited with error.\n"
        exit 1
    fi
    set -e

    cmd2="$PAT -s patch.txt a.txt"

    printf "  $cmd2\n"
    $cmd2 > output.txt

    # Compare actual output with original file.
    diff b.txt output.txt

    cd ..
}


runtest6() {
    desc="$1"
    dir="$2"
    arg1="$3"

    echo "$desc" \($dir\)
    cd $dir

    DIF=dif
    PAT=pat

    cmd1="$DIF $arg1 a.txt b.txt"

    printf "  $cmd1\n"
    set +e
    $cmd1 > patch.txt
    if [ "$?" = "2" ]; then
        printf "$DIF exited with error.\n"
        exit 1
    fi
    set -e

    cp a.txt output.txt
    cmd2="$PAT patch.txt output.txt"

    printf "  $cmd2\n"
    $cmd2

    # Compare actual output with original file.
    diff b.txt output.txt

    cd ..
}


runtest2() {
    desc="$1"
    dir="$2"
    arg1="$3"
    arg2="$4"

    echo "$desc" \($dir\)
    cd $dir

    DIF=dif
    PAT=pat

    # dif directories
    cmd1="$DIF $arg1 dir1 dir2"
    printf "  $cmd1\n"
    set +e
    $cmd1 > patch.txt
    if [ "$?" = "2" ]; then
        printf "$DIF exited with error.\n"
        exit 1
    fi
    set -e

    # pat directory.
    d1=d1copy
    rm -rf $d1
    cp -r dir1 $d1
    cmd2="$PAT patch.txt $d1"
    printf "  $cmd2\n"
    $cmd2

    cmd3="diff $arg2 $d1 dir2 | sort"
    printf "  $cmd3\n"
    eval $cmd3 > output.txt


    diff expect-u.txt output.txt
    rm -rf $d1

    cd ..
}

runtest3() {
    desc="$1"
    dir="$2"
    nam="$3"
    expected_val="$4"

    echo "$desc" \($dir\)
    cd $dir

    PAT=pat

    cmd1="$PAT -t $nam"

    printf "  $cmd1\n"
    set +e
    $cmd1 > output.txt
    actual_val=$?
    set -e

    if [ ! "$actual_val" = "$expected_val" ]; then
        printf "Unexpected return value. Expected '$expected_val', actual '$actual_val'.\n"
        exit 1
    fi
    
    diff expect-u.txt output.txt

    cd ..
}


runtest4() {
    desc="$1"
    dir="$2"
    arg="$3"

    echo "$desc" \($dir\)
    cd $dir

    WORKDIR=tmp
    PAT=pat
    
    rm -rf $WORKDIR
    cmd1="$PAT $arg pat.txt a.txt"
    
    printf "  $cmd1\n"
    $cmd1 > output.txt

    if [ -d $WORKDIR ]; then
        rm -rf $WORKDIR
    else
        printf "Error: Expected '%s' to exist.\n" $WORKDIR
        exit 1
    fi

    cd ..
}

runtest5() {
    desc="$1"
    dir="$2"
    arg="$3"
    WORKDIR="$4"

    echo "$desc" \($dir\)
    cd $dir

    PAT=pat
    
    mkdir -p $WORKDIR
    cmd1="$PAT $arg pat.txt a.txt"
    
    printf "  $cmd1\n"
    $cmd1 > output.txt

    if [ -d $WORKDIR ]; then
        printf "Error: Dir '%s' should have been deleted.\n" $WORKDIR
        exit 1
    else
        rm -rf $WORKDIR
    fi

    cd ..
}


runtest7() {
    desc="$1"
    dir="$2"
    arg1="$3"

    echo "$desc" \($dir\)
    cd $dir

    DIF=dif
    PAT=pat

    cmd1="$DIF $arg1 a.txt b.txt"

    printf "  $cmd1\n"
    set +e
    $cmd1 > patch.txt
    if [ "$?" = "2" ]; then
        printf "$DIF exited with error.\n"
        exit 1
    fi
    set -e

    cp a.txt output.txt
    cmd2="$PAT --create patch.txt output.txt"
    printf "  $cmd2\n"
    $cmd2

    # These should not differ.
    diff a.txt output.txt
    diff b.txt tmp/00000001.tgt

    cd ..
}


runPatTests() {
  printf "\n============= PAT tests =============\n"
  cd tests/pat
  for dir in t*; do
    printf "\n"
    if [ -d $dir ]; then
      case "$dir" in
        th      ) runtest0 "Show help"              $dir "-h"                    ;;
        tk      ) runtest4 "Keep workdir"           $dir "-k"                    ;;
        tn      ) runtest6 "Normal change in place" $dir ""                      ;;
        tn-add  ) runtest1 "Normal add"             $dir ""                      ;;
        tn-chg  ) runtest1 "Normal change"          $dir ""                      ;;
        tn-creat) runtest7 "Normal create"          $dir ""                      ;;
        tn-del  ) runtest1 "Normal delete"          $dir ""                      ;;
        tn-mix  ) runtest1 "Normal mix"             $dir ""                      ;;
        tnd     ) runtest2 "Norm dir"               $dir ""      ""              ;;
        tnd-r   ) runtest2 "Norm dir recurse"       $dir "-r"    "-r"            ;;
        tnd-re  ) runtest2 "Norm dir recurse empty" $dir "-r -e" "-r -N"         ;;
        tt-i-un ) runtest3 "Type indet uni-nrm"     $dir "type-iun.pat" "2"      ;;
        tt-nrm-m) runtest3 "Type normal multi"      $dir "type-nm.pat"  "4"      ;;
        tt-nrm-s) runtest3 "Type normal single"     $dir "type-ns.pat"  "3"      ;;
        tt-uni-m) runtest3 "Type unified multi"     $dir "type-um.pat"  "6"      ;;
        tt-uni-s) runtest3 "Type unified single"    $dir "type-us.pat"  "5"      ;;
        tt-unkno) runtest3 "Type unknown"           $dir "type-u.pat"   "1"      ;;
        tq      ) runtest0 "Quiet"                  $dir "-q -h"                 ;;
        tu      ) runtest6 "Unified chng in place"  $dir "-u"                    ;;
        tu-add  ) runtest1 "Unified add"            $dir "-u"                    ;;
        tu-chg  ) runtest1 "Unified change"         $dir "-u"                    ;;
        tu-creat) runtest7 "Unified create"         $dir "-u"                    ;;
        tu-del  ) runtest1 "Unified del"            $dir "-u"                    ;;
        tu-mix  ) runtest1 "Unified mix"            $dir "-u"                    ;;
        tud     ) runtest2 "Unif dir"               $dir "-u"       "-U 0"       ;;
        tud-r   ) runtest2 "Unif dir recurse"       $dir "-u -r"    "-U 0 -r"    ;;
        tud-re  ) runtest2 "Unif dir recurse empty" $dir "-u -r -e" "-U 0 -r -N" ;;
        tv      ) runtest0 "Show version"           $dir "-v"                    ;;
        tw-alt  ) runtest5 "Alt workdir exists"     $dir "-w altdir" "altdir"    ;;
        tw-defau) runtest5 "Default workdir exists" $dir "-w tmp" "tmp"          ;;
        *       ) echo Unhandled test case $dir; exit 1   ;;
      esac
    fi
  done
  cd ../..
}

redactDifDosTimestamp() {
    dir=$1

    $FMOD $dir/expect-d.txt 1 29 1 X # Second
    $FMOD $dir/expect-d.txt 1 31 9 0 # Nanoseconds
    $FMOD $dir/expect-d.txt 1 41 5 X # Timezone

    $FMOD $dir/expect-d.txt 2 29 1 X # Second
    $FMOD $dir/expect-d.txt 2 31 9 0 # Nanoseconds
    $FMOD $dir/expect-d.txt 2 41 5 X # Timezone
}

redactDifDosHash() {
    dir=$1

    $FMOD $dir/expect-d.txt 4 11 20 X # Hash
    $FMOD $dir/expect-d.txt 6 12 20 X # Hash

    $FMOD $dir/expect-d.txt 11 11 20 X # Hash
    $FMOD $dir/expect-d.txt 13 12 20 X # Hash
}

redactDifDosPrefix() {
    dir=$1

    $FMOD $dir/expect-d.txt 1 8 7 X # Program name
    for lin in $(seq 3 17); do
        $FMOD $dir/expect-d.txt $lin 3 1 X # Prefix '-'
        $FMOD $dir/expect-d.txt $lin 7 2 X # Prefix '--'
    done
}

redactDifDosVersion() {
    dir=$1

    $FMOD $dir/expect-d.txt 1 1 7 X # Program name
}

adjustOutputN() {
    dir=$1

    sed -i -e 's/\//\\/g' $dir/expect-d.txt # Convert '/' to '\'.
    sed -i -e 's/^diff --normal /dif.exe \/n /g' $dir/expect-d.txt
    sort $dir/expect-d.txt -o $dir/expect-d.txt
}

adjustOutputR() {
    dir=$1

    sed -i -e 's/\//\\/g' $dir/expect-d.txt # Convert '/' to '\'.
    sed -i -e 's/^diff --recursive /dif.exe \/r /g' $dir/expect-d.txt
    sort $dir/expect-d.txt -o $dir/expect-d.txt
}

adjustOutputRB() {
    dir=$1

    sed -i -e 's/\//\\/g' $dir/expect-d.txt # Convert '/' to '\'.
    sort $dir/expect-d.txt -o $dir/expect-d.txt
}

adjustOutputRE() {
    dir=$1

    sed -i -e 's/\//\\/g' $dir/expect-d.txt # Convert '/' to '\'.
    sed -i -e 's/^diff --recursive --new-file /dif.exe \/r \/e /g' $dir/expect-d.txt
    sort $dir/expect-d.txt -o $dir/expect-d.txt
}

writeExpectD() {

  for dir in t*; do
    if [ -d $dir ]; then
        cd $dir
        if [ -e expect-u.txt ]; then
            sed 's/$/\r/g' < expect-u.txt > expect-d.txt
        elif [ -e _expect.txt ]; then
            sed 's/$/\r/g' < _expect.txt > expect-d.txt
        fi
        rm -f output.txt
        cd ..
    fi
  done
}


redactPatDosPrefix() {
    dir=$1

    $FMOD $dir/expect-d.txt 1 8 7 X # Program name
    $FMOD $dir/expect-d.txt 2 8 7 X # Program name
    $FMOD $dir/expect-d.txt 3 8 7 X # Program name

    $FMOD $dir/expect-d.txt 3 17 1 X # Prefix '-t'
    $FMOD $dir/expect-d.txt 3 22 2 X # Prefix '--type'

    for lin in $(seq 5 12); do
        $FMOD $dir/expect-d.txt $lin 3 1 X # Prefix '-'
        $FMOD $dir/expect-d.txt $lin 7 2 X # Prefix '--'
    done

    $FMOD $dir/expect-d.txt 15  9 1 X # Prefix '-c'
    $FMOD $dir/expect-d.txt 15 22 1 X # Prefix '-k'
}

redactPatDosVersion() {
    dir=$1

    $FMOD $dir/expect-d.txt 1 1 7 X # Program name
}

# Convert from Unix to DOS.
todos() {
  FMOD=fmod

  cd tests/dif
  printf "Creating dif tests in DOS format\n"
  writeExpectD
  redactDifDosTimestamp ta-minus
  redactDifDosTimestamp ta-plus
  redactDifDosTimestamp tc
  redactDifDosTimestamp tu
  redactDifDosHash td
  redactDifDosPrefix th
  redactDifDosVersion tv

  adjustOutputN t-dirs
  adjustOutputR tr
  adjustOutputRB tr-rb
  adjustOutputRB tr-rbg
  adjustOutputRE tr-re
  cd ../..


  cd tests/pat
  printf "Creating pat tests in DOS format\n"
  writeExpectD
  redactPatDosPrefix th
  redactPatDosVersion tv
  cd ../..
}


if [ "$1" = "clean" ]; then
    clean
    EXIT_OK=1
elif [ "$1" = "todos" ]; then
    todos
    EXIT_OK=1
elif [ "$1" = "rt" ]; then
    clean
    runDifTests
    runPatTests
    EXIT_OK=1
else
    printf "\n"
    printf "  Targets:\n"
    printf "     dif    Build dif and pat programs\n"
    printf "     all    Build programs for testing\n"
    printf "     clean  Clean build targets\n"
    printf "     rt     Run tests\n"
    printf "\n"
    EXIT_OK=1
fi
