`dif.exe` - Compares files or directories and can generate a _patchfile_.

`pat.exe` - Applies a _patchfile_ to a file or a directory.

`dif.exe` and `pat.exe` were created to run on (16-bit) FreeDOS, but can also be built to run on (64-bit) Linux. 

`dif.exe` generates patchfiles in 'Normal' format or (a restricted) 'Unified' format.


## dif ##

    Usage:  DIF.EXE [options] NAME1 NAME2

      /n,  /normal     Output Normal format (default)
      /u,  /unified    Output Unified format
      /b,  /brief      Report only if NAMEs differ or not
      /e,  /empty      Treat absent files as empty
      /r,  /recurse    Recurse subdirectories
      /t,  /twocol     Show two-column output
      /w,  /width N    Column width of two-column output
      /g,  /gloss      Prefix line with char indicating change
      /s,  /space      Ignore space and tab differences
      /l,  /lineend    Ignore line ending
      /a,  /alt CHARS  Set alternate chars
      /q,  /quiet      Suppress normal output
      /h,  /help       Show this help
      /v,  /version    Show version
      /d,  /data       Show data structure (for developer)

    NAME1 and NAME2 must either both be files or both be directories.
    CHARS is 1-6 chars which replace corresponding chars in the string '<>+-=%' and
    may need to be quoted to prevent shell interpretation or if spaces are used.
    Exit status is 0 if inputs are the same, 1 if different, 2 if trouble.

The `/gloss` option is only applicable when using `/brief` or `/twocol`. It will print a character at the start of each line. The characters and their meanings are as follows:

    %  Line was changed.
    =  Line is equal.
    <  Line is only in NAME1 (NAME1 is a file).
    >  Line is only in NAME2 (NAME2 is a file).
    A  File is only in NAME1 (NAME1 is a directory).
    B  File is only in NAME2 (NAME2 is a directory).

The characters, excluding 'A' and 'B', can be changed with the `/alt` option.

#### Comparing files ####

Generate a patchfile by comparing two files.

    dif.exe file1.txt file2.txt          > patch.txt   ; Normal format.
    dif.exe file1.txt file2.txt /unified > patch.txt   ; Unified format.

#### Comparing directories ####

Generate a patchfile by comparing two directories.

    dif.exe dir1 dir2          > patch.txt   ; Normal format
    dif.exe dir1 dir2 /unified > patch.txt   ; Unified format


## pat ##

    Usage: PAT.EXE [options] PATCHFILE ORIGFILE
           PAT.EXE [options] PATCHFILE ORIGDIR
           PAT.EXE [/t |  /type] PATCHFILE

      /w,  /workdir DIR   Name of workdir (default: 'tmp')
      /k,  /keep          Do not delete workdir on exit
      /q,  /quiet         Suppress normal output
      /s,  /screen        Output to screen
      /c,  /create        Create patches but do not update files
      /t,  /type          Print type of PATCHFILE
      /h,  /help          Show this help
      /v,  /version       Show version

    DIR can be a max of 8 chars; anything longer will be truncated.
    Use of '/c' implies '/k'; the patched files are stored in the workdir.


#### Patching a file ####

Apply changes in `patch.txt` to `origfile.txt`. `origfile.txt` will be modified.

    pat.exe patch.txt origfile.txt


Apply changes in `patch.txt` to `origfile.txt`. Save the output in `result.txt`. `origfile.txt` will not be modified.

    pat.exe /screen patch.txt origfile.txt > result.txt


Apply changes in `patch.txt` to `origfile.txt`. The result will be saved in `.\tmp\00000001.tgt`. `origfile.txt` will not be modified.

    pat.exe /create patch.txt origfile.txt


#### Patching a directory ####

Apply `patch.txt` to `origdir`. Files in `origdir` will be updated. 

    pat.exe patch.txt origdir

Apply `patch.txt` to `origdir`. The patched files will be created in the work directory `tmp`. Files in `origdir` will not be updated.

    pat.exe /create patch.txt origdir

When patching a directory, a patchfile can only modify an existing file, or create a new file; it cannot delete an existing file.

Both '/' and '\' are interpreted as a directory separator. This allows a
patchfile to be used on both Linux and DOS, but means that paths cannot contain
escaped characters.

#### Display type of patchfile ####

To display the patchfile type:

    pat.exe /type patch.txt

Normally, the exit value of `pat.exe` is 0 on success, or 1 on failure. However, when the `/type` option is specified, the exit value of `pat.exe` will be one of the following:

    1  "Unknown type"
    2  "Indeterminate type"
    3  "Normal diff of file"
    4  "Normal diff of directory"
    5  "Unified diff of file"
    6  "Unified diff of directory"

"Unknown type" means the patchfile is not recognized. "Indeterminate type" means the patchfile matched both Normal and Unified format.

When patching, a temporary work directory may be created. If pat.exe succeeds, the work directory will be deleted when pat.exe exits. If pat.exe fails, the directory may or may not be deleted depending on the error. If the `/keep` option is specified, the directory will not be deleted.

The work directory will contain three types of files: `.cmd`, `.pat`, `.tgt`. The `.cmd` files contain the command used to create the patch files. The `.pat` files contain the patch data. The `.tgt` files contain a patched version of the original file. 


## Developer Info ##

The build script uses the Open Watcom C Compiler (`Development\OW` FreeDOS package) to build `dif.exe` and `pat.exe`.

Patchfiles generated by `dif.exe` should exactly match those generated by `diff`, and thus, should work with `patch`. Similarly, provided only certain options are used, patchfiles generated by `diff` should work with `pat.exe`.

dif option | diff option           | relationship
-----------|-----------------------|-------------
/normal    | --normal              | exactly the same
/unified   | --unified=0           | exactly the same
/empty     | --new-file            | exactly the same
/recurse   | --recursive           | exactly the same
/brief     | --brief               | exactly the same
/space     | --ignore-space-change | similar, but not exact
/twocol    | --side-by-side        | similar, but not exact
/width     | --width               | /width is half of --width
/alt       | GFMT                  | loosely similar

Both `dif.exe` and `pat.exe` accept the option `/?` which is an alias for `/help` but is not shown on the help screen.

File size on FreeDOS:

    dif.exe    45 Kb
    pat.exe    37 Kb

File size on Linux (and comparison to `diff` and `patch`):

    dif    38 Kb   (diff    130 Kb)
    pat    27 Kb   (patch   151 Kb)

A full install (source and executables) of difpat on FreeDOs requires about 224 Kb

`dif.exe` and `pat.exe` are not a replacement for `diff` and `patch`; `diff` and `patch` have more features and are much faster.

## Tests ##

Several tests have been written for `dif.exe` and `pat.exe` to ensure that they perform as expected. With appropriate adjustments, the tests run under both FreeDOS and Linux. The testing framework is crude - tests will fail without giving a reason. Determining why a test actually failed requires a bit of investigation. The following are requirements for the testing 'framework':

    * Tests for `dif` reside in the `.\dev\tests\dif` directory.
    * Tests for `pat` reside in the `.\dev\tests\pat` directory.
    * Each test must be a directory.
    * The directory name must be eight chars or less.
    * The directory name must begin with the letter 't'.

The following conventions are used, but need not be adhered to.

> Test names have the format: `tA-DDDDD`. 'A' is the primary option that is being tested; it is followed by a dash. The five remaining characters ('DDDDD') are the description. The description might be a (condensed) word or it might contain other options that are bing tested.

> When comparing or patching files, `a.txt` and `b.txt` should be used. When comparing or patching directories, `dir1` and `dir2` should be used.

> `expect-u.txt` is the expected output of running the test on Linux (Unix). `expect-u` will be 'converted' to `expect-d.txt` which is the expected output of running the test on FreeDOS (DOS). In some cases, `expect-u.txt` need not exist because the expected output can be automatically determined.

> Tests generate `output.txt`, which is compared against `expect-u.txt` (or `expect-d.txt`).

> `expect-u.txt` (and `expect-d.txt`) may require special processing. For example, when using `dif` to compare directories, the output must be sorted before comparing because the order that the Operating System returns files cannot be controlled.


## Build FreeDOS package ##

difpat is developed on Linux and 'ported' to FreeDOS. However, the `difpat` FreeDOS package (`difpat.zip`) is 'self-hosting', meaning that once the `difpat` package has been installed, with sources, on FreeDOS, the source can be modified, compiled, and an updated `difpat` FreeDOS package can be built. i.e. Linux is not required.

The build process starts on Linux:

    $ LC_ALL=C          Specify sort order
    $ export LC_ALL
    $ ./build.sh all    Build needed executables
    $ ./build.sh rt     Run tests
    $ ./build.sh todos  Convert tests to DOS format
    $ ./build.sh zip    Create 'dp.zip' containing all files

Copy `dp.zip` to the root directory on the C: drive of your FreeDOS installation.

    C:\> \devel\ow\owsetenv.bat       If needed, setup development environment
    C:\> mkdir a
    C:\> cd a
    C:\A> unzip ..\dp.zip
    C:\A> build.bat all               Build needed executables
    C:\A> build.bat todos             Convert test data
    C:\A> build.bat rt                Run tests
    C:\A> build.bat clean             Remove generated files
    C:\A> build.bat difpat            Build dif.exe and pat.exe
    C:\A> build.bat pkg               Create FreeDOS package 'difpat.zip'

    C:\A> fdnpkg install difpat.zip   (Optional) Install local package

Reboot to update PATH to include `dif.exe` and `pat.exe`.

The FreeDOS package manager does not write empty directories. The tests of `difpat` contain empty directories. To work-around this problem, the tests are zipped when the package is built. The tests are unzipped the first time the build script is run. Zipping the tests also avoids exceeding the maximum path length of FreeDOS (63 chars) when running `build pkg`.


## Helper files ##

`fiter.c` Outputs a line of a file.

    Usage: fiter FILE NUM


`fmod.c` Overwrite LEN chars in FILE with CHAR, starting at LINE, COL.

    Usage: fmod FILE LINE COL LEN [CHAR]
