#include <stdio.h> // printf
#include <stdlib.h> // exit
#include <ctype.h> // isdigit
#include <libgen.h> // basename
#include <limits.h> // UINT_MAX
#include <string.h> // strlen
#include <sys/stat.h> // struct stat

#include <unistd.h> // rmdir
#if defined(__DOS__)
#  include <direct.h>
#else
#  include <dirent.h>
#endif


#define __ERRMSG(fmt, ...) fprintf(stderr, fmt "%s", __VA_ARGS__)
#define ERRMSG(...) __ERRMSG(__VA_ARGS__, "")


#define __MSG(fmt, ...)                    \
    do {                                   \
        if (!opt.quiet) {                  \
            printf(fmt "%s", __VA_ARGS__); \
        }                                  \
    } while (0)

#define MSG(...) __MSG(__VA_ARGS__, "")

#if defined(__DOS__)
#    define MKDIR(d) mkdir(d)
#else
#    define MKDIR(d) mkdir(d, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#endif

#define VERSION "0.1"

#if !defined(uint)
    typedef unsigned int uint;
#endif

// The Open Watcom compiler defines '__I86__' for 16-bit compilation. See
// 'Open Watcom C/C++ User's Guide' for other targets, if needed.
// http://www.openwatcom.org/doc.php
#if defined(__I86__)
    // DOS 3.3 (16-bit) has limit 63 characters.
    #define MAX_PATH_LEN 63
#else
    // Windows has a limit of 260 characters.
    // Linux has a limit of 255 characters.
    #define MAX_PATH_LEN 255
#endif

char PATHA[MAX_PATH_LEN];
char PATHB[MAX_PATH_LEN];

#if defined(__DOS__) || defined(__WINDOWS__) || defined(__NT__)
    #define OPT_PREFIX1 "/"
    #define OPT_PREFIX2 "/"
    #define DIRSEP '\\'
    #define ALTSEP '/'
#else
    #define OPT_PREFIX1 "-"
    #define OPT_PREFIX2 "--"
    #define DIRSEP '/'
    #define ALTSEP '\\'
#endif


#define PASS 1
#define FAIL 0

#define EXIT_OK  0
#define EXIT_ERR 1

#define TYPE_UNKNOWN         1
#define TYPE_INDETERMINATE   2
#define TYPE_NORMAL_SINGLE   3
#define TYPE_NORMAL_MULTI    4
#define TYPE_UNIFIED_SINGLE  5
#define TYPE_UNIFIED_MULTI   6

struct Opt {
    int create;
    int help;
    int indexF1;
    int indexF2;
    int keep;
    int quiet;
    int screen;
    int type;
    int version;
    int workdirIndex;
};

struct Opt opt;

#define NORM_XXX '_'
#define NORM_ADD 'a'
#define NORM_CHG 'c'
#define NORM_DEL 'd'

struct HunkHeader {
    uint abeg;
    uint aend;
    uint bbeg;
    uint bend;
    int  type;
};



char workdir[9] = "tmp"; // 8 + 1 for null.
char cmdName[22]; // 8 + dirsep + 8.3 + 1 for null.
char patName[22]; // 8 + dirsep + 8.3 + 1 for null.
char tgtName[22]; // 8 + dirsep + 8.3 + 1 for null.


// 2^64 - 1 == 18,446,744,073,709,551,615 which is 20 digits long.
#define UINT_LEN 21 // 20 + 1 for null.



int
isFile(const char * path)
{
    struct stat path_stat;
    if (stat(path, &path_stat)) {
        return FAIL;
    }
    return S_ISREG(path_stat.st_mode);
}


int
isDirectory(const char * path)
{
    struct stat path_stat;
    if (stat(path, &path_stat)) {
        return FAIL;
    }
    return S_ISDIR(path_stat.st_mode);
}


// Reads a character from fp and compares it to ch.
// Returns PASS if read character equals ch.
// Otherwise returns FAIL.
int
cmpCh (FILE * fp, char ch)
{
    char c = fgetc(fp);
    if ( feof(fp) ) {
        return FAIL;
    }
    return (c == ch);
}


// Reads a character from fp and returns it as ch.
// Returns PASS if a character is read.
// Otherwise returns FAIL.
int
readCh (FILE * fp, char * ch)
{
    *ch = fgetc(fp);
    if ( feof(fp) ) {
        return FAIL;
    }
    return PASS;
}

// Converts buf to a uint value.
// If buf is successfully converted, the returned value will be one more than
// the actual converted value. e.g. The string '24' will return 25.
// Zero will be returned if buf cannot be converted, or if buf is empty, or
// if val == UINT_MAX.
uint
toUint(char * buf, uint buflen)
{
    uint k = 0;
    int non_zero = 0;
    if (buflen > 0) {
        for (k=0; k<buflen; k++) {
            if (buf[k] != '0') {
                non_zero = 1;
            }
        }
        if (non_zero) {
            uint val = atoi(buf);
            if (val > 0) {
                return (val == UINT_MAX ? 0 : val + 1);
            }
        } else {
            return 1;
        }
    }
    return 0;
}


// Reads a string of digits from fp and converts them to a uint val.
// If val is zero, an error occured.
// Otherwise, val is one more than the actual converted value.
// ch holds the last character that was read (which will be a non-digit).
void
readNum(FILE * fp, char * ch, uint * val)
{
    char buf[UINT_LEN];
    uint i = 0;
    int got_r = 0;

    do {
        *ch = fgetc(fp);
        if ( feof(fp) ) {
            i++;
            break;
        }
        buf[i] = *ch;
        i++;
        if (i >= UINT_LEN) {
            buf[0] = '\0';
            return;
        }
    } while (isdigit(*ch));
    i--;
    buf[i] = '\0';
    *val = toUint(&buf[0], i);
}


// Populates a Normal HunkHeader.
// The returned HunkHeader values will be one more than the actual values.
// e.g. A value of '0' indicates that no value was read, or an error occured.
// e.g. A value of '1' indicates that a value of '0' was read.
int
readNormalHunkHeader(FILE * fp, struct HunkHeader * hh)
{
    char ch;

    hh->abeg=0;
    hh->aend=0;
    hh->bbeg=0;
    hh->bend=0;
    hh->type = NORM_XXX;

    readNum(fp, &ch, &(hh->abeg));
    if (ch == ',') {
        readNum(fp, &ch, &(hh->aend));
    }
    if ( ch == NORM_ADD ||  ch == NORM_DEL || ch == NORM_CHG) {
        hh->type = ch;
        readNum(fp, &ch, &(hh->bbeg));
        if (ch == ',') {
            readNum(fp, &ch, &(hh->bend));
        }
        if (ch == '\r') {
            readCh(fp, &ch); // Read '\n'.
            if (ch == '\n') {
                return PASS;
            }
        } else if (ch == '\n') {
            return PASS;
        }
    }
    return FAIL;
}


// Populates a Unified HunkHeader.
// The returned HunkHeader values will be one more than the actual values.
// e.g. A value of '0' indicates that no value was read, or an error occured.
// e.g. A value of '1' indicates that a value of '0' was read.
int
readUnifiedHunkHeader(FILE * fp, struct HunkHeader * hh)
{
    char ch;

    hh->abeg=0;
    hh->aend=0;
    hh->bbeg=0;
    hh->bend=0;
    hh->type = ' '; // Not used.

    if (cmpCh(fp, '@')) {
        if (cmpCh(fp, '@')) {
            if (cmpCh(fp, ' ')) {
                if (cmpCh(fp, '-')) {
                    readNum(fp, &ch, &(hh->abeg));
                    if (ch == ',') {
                        readNum(fp, &ch, &(hh->aend));
                    }
                    if (ch == ' ') {
                        if (cmpCh(fp, '+')) {
                            readNum(fp, &ch, &(hh->bbeg));
                            if (ch == ',') {
                                readNum(fp, &ch, &(hh->bend));
                            }
                            if (ch == ' ') {
                                if (cmpCh(fp, '@')) {
                                    if (cmpCh(fp, '@')) {
                                        readCh(fp, &ch);
                                        if (ch == '\n') {
                                            return PASS;
                                        }
                                        if (ch == '\r') {
                                             return cmpCh(fp, '\n');
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    while (!feof(fp)) {
        if (fgetc(fp) == '\n') {
            break;
        }
    }
    return FAIL;
}


// Search for '<' and '>' at beginning of line.
// Returns 1 if bare.
// Returns 2 or more if directory (even if a single file is is directory)
// Otherwise returns 0.
uint
isNormalPatch(char * fname)
{
    struct HunkHeader hh;
    FILE * fp = NULL;
    char ch;
    uint cnt_lt = 0;
    uint cnt_gt = 0;
    uint cnt_dash = 0; // Counted, but ignored
    uint cnt_dig = 0; // Counted, but ignored
    uint cnt_files = 0; // Is 'at least', not 'exactly'.
    int isFile = 0;

    int check_next = 1;
    if (( fp = fopen(fname, "rb") )) {
        if (readNormalHunkHeader(fp, &hh)) {
            if (hh.abeg > 0 && hh.bbeg > 0) {
                isFile = 1;
            }
        }
        rewind(fp);
        while (1) {
            ch = fgetc(fp);
            if (feof(fp)) {
                break;
            }
            if (check_next) {
                if ( ch == '<' ) {
                    cnt_lt++;
                } else if ( ch == '>' ) {
                    cnt_gt++;
                } else if (ch == '-') {
                    cnt_dash++;
                } else if (isdigit(ch)) {
                    cnt_dig++;
                } else {
                    cnt_files++;
                }
            }
            if (ch == '\n') {
                check_next = 1;
                continue;
            }
            check_next = 0;
        }
        fclose(fp);
    }
    if (cnt_lt == 0 && cnt_gt == 0) {
        return 0;
    }
    // if (isFile && cnt_files == 0) {
    //     return 1;
    // }
    if (cnt_files > 0) {
        if (cnt_files == 1) { cnt_files++; }
        return cnt_files;
    } else {
        if (isFile) {
            return 1;
        }
    }
//    return (cnt_files == 0 ? 1 : cnt_files);
    return 0;
}


// Search for '+++ ' at beginning of line.
// Returns 1 if patch is a file.
// Returns 2 or more if patch is directory.
// Otherwise, returns 0.
uint
isUnifiedPatch(char * fname)
{
    struct HunkHeader hh;
    FILE * fp = NULL;
    char ch;
    uint cnt_plus = 0;
    uint cnt_files = 0;
    int check_next = 1;
    uint rep = 0;
    int in_plus = 0;
    if (( fp = fopen(fname, "rb") )) {
        while (1) {
            ch = fgetc(fp);
            if (feof(fp)) {
                break;
            }
            if (ch == '+' && in_plus) {
                rep++;
                continue;
            } else if (ch == ' ') {
                if (in_plus && rep == 3) {
                    cnt_plus++;
                }
            }
            rep = 0;
            in_plus = 0;
            if (check_next) {
                if ( ch == '+' ) {
                    in_plus = 1;
                    rep = 1;
                } else if (ch == '-' || ch == '@') {
                    // Ignore
                } else {
                    cnt_files++;
                }
            }
            if (ch == '\n') {
                check_next = 1;
                continue;
            }
            check_next = 0;
        }
        fclose(fp);
    }
    if (cnt_plus > 0) {
        if (cnt_files == 0) return 1;
        if (cnt_files == 1) { cnt_files++; }
        return cnt_files;
    }
    return 0;
}


int
detectPatchType(char * fname)
{
    uint n_file_cnt = isNormalPatch(fname);
    uint u_file_cnt = isUnifiedPatch(fname);
    int n = (n_file_cnt > 0);
    int u = (u_file_cnt > 0);
    if (n + u > 1) {
        return TYPE_INDETERMINATE;
    }
    if (n_file_cnt) {
        return (n_file_cnt == 1 ? TYPE_NORMAL_SINGLE : TYPE_NORMAL_MULTI);
    }
    if (u_file_cnt) {
        return (u_file_cnt == 1 ? TYPE_UNIFIED_SINGLE : TYPE_UNIFIED_MULTI);
    }
    return TYPE_UNKNOWN;
}


// Reads num_lines lines from fp.
// If show is 1, print lines to screen.
// If show is 0, do not print lines to screen.
void
readLines(FILE * fp, uint num_lines, int show)
{
    char ch;
    uint n = 0;
    if (num_lines > 0) {
        while (1) {
            ch = fgetc(fp);
            if ( feof(fp) ) {
                return;
            }
            if (show) {
                MSG("%c", ch);
            }
            if (ch == '\n') {
                n++;
                if (n == num_lines) {
                    return;
                }
            }
        }
    }
}


void
showLines(FILE * fp, uint num_lines)
{
    readLines(fp, num_lines, 1);
}


void
skipLines(FILE * fp, uint num_lines)
{
    readLines(fp, num_lines, 0);
}


// Read num_lines from pat.
// Skips offset chars at the beginning of each line and then writes remaining
// characters in line to tgt.
// If tgt is null, print to screen.
void
writePatLines(FILE * pat, uint num_lines, FILE * tgt, int offset)
{
    char ch='\0';
    int got_r = 0;
    uint n = 0;
    uint c = 0;
    if (num_lines > 0) {
        while (1) {
            ch = fgetc(pat);
            c++;
            if ( feof(pat) ) {
                return;
            }

#if defined(__DOS__)
            if (ch == '\r') {
                got_r = 1;
                continue;
            }
#endif
            if (c > offset) {
                if (tgt) {
#if defined(__DOS__)
                    if (got_r) {
                        fputc('\r', tgt);
                    }
#endif
                    fputc(ch, tgt);
                } else {
                    MSG("%c", ch);
                }
            }
            got_r = 0;
            if (ch == '\n') {
                c = 0;
                n++;
                if (n == num_lines) {
                    return;
                }
            }
        }
    }
}


// Read lines from src until cur_line_num equals to_line_num.
// Writes read characters to tgt.
// If tgt is null, print to screen.
void
writeSrcLines(FILE * src, uint * cur_line_num, uint to_line_num, FILE * tgt)
{
    char ch;
    while (*cur_line_num < to_line_num) {
        ch = fgetc(src);
        if ( feof(src) ) {
            return;
        }
#if defined(__DOS__)
        if (ch == '\r') {
            continue;
        }
#endif

        if (tgt) {
#if defined(__DOS__)
            if (ch == '\n') {
                fputc('\r', tgt);
            }
#endif
            fputc(ch, tgt);
        } else {
            MSG("%c", ch);
        }
        if (ch == '\n') {
            (*cur_line_num)++;
        }
    }
}


// Copies src to tgt until EOF is reached.
// If tgt is NULL, src will be printed to screen.
void
finishLines(FILE * src, FILE * tgt)
{
    char ch;
    while (1) {
        ch = fgetc(src);
        if ( feof(src) ) {
            return;
        }
#if defined(__DOS__)
        if (ch == '\r') {
            continue;
        }
#endif
        if (tgt) {
#if defined(__DOS__)
            if (ch == '\n') {
                fputc('\r', tgt);
            }
#endif
            fputc(ch, tgt);
        } else {
            MSG("%c", ch);
        }
    }
}


// Correct Normal HunkHeader values to real values.
void
correctNormalHunkHeader(struct HunkHeader * hh)
{
    (hh->abeg)--;
    (hh->bbeg)--;
    if (hh->aend > 0) (hh->aend)--;
    if (hh->bend > 0) (hh->bend)--;
}


// Correct Unified HunkHeader values to real values.
void
correctUnifiedHunkHeader(struct HunkHeader * hh)
{
    (hh->abeg)--;
    (hh->bbeg)--;
    if (hh->aend == 0) {
        hh->aend = 1;
    } else {
        (hh->aend)--;
    }
    if (hh->bend == 0) {
        hh->bend = 1;
    } else {
        (hh->bend)--;
    }
}


int
patchUnifiedFile(FILE * pat, FILE * src, FILE * tgt)
{
    uint alen = 0;
    uint blen = 0;
    int retval = PASS;
    struct HunkHeader hh;
    uint offset = 1;
    uint src_line_num = 0;

    int read2 = 0;
    char ch;
    int vv;

    // Ignore two lines (which contain filenames and dates).
    while (read2 < 2) {
        ch = fgetc(pat);
        if (feof(pat)) {
            return FAIL;
        }
        if (ch == '\n') {
            read2++;
        }
    }

    while (1) {
        if (readUnifiedHunkHeader(pat, &hh)) {
            if (hh.abeg > 0 && hh.bbeg > 0) {
                correctUnifiedHunkHeader(&hh);
                alen = hh.aend;
                blen = hh.bend;
                vv = (alen == 0 && blen > 0 ? 0 : 1);
                if (hh.abeg > 0) {
                    writeSrcLines(src, &src_line_num,  (hh.abeg-vv), tgt);
                    if (vv == 1) {
                        skipLines(src, alen);
                        src_line_num += alen;
                    }
                }
                skipLines(pat, alen);
                if (blen > 0) {
                    writePatLines(pat, blen, tgt, offset);
                }
            } else {
                ERRMSG("Invalid hunk header values\n");
                retval = FAIL;
                break;
            }
        } else {
            if (!feof(pat)) {
                ERRMSG("Error reading hunk header\n");
                retval = FAIL;
            }
            break;
        }
    }
    finishLines(src, tgt);
    return retval;
}


int
patchNormalFile(FILE * pat, FILE * src, FILE * tgt)
{
    uint alen = 0;
    uint blen = 0;
    int retval = PASS;
    struct HunkHeader hh;
    uint src_line_num = 0;
    uint offset = 2;

    while (1) {
        if (readNormalHunkHeader(pat, &hh)) {
            if (hh.abeg > 0 && hh.bbeg > 0) {
                correctNormalHunkHeader(&hh);
                alen = (hh.aend == 0 ? 1 : hh.aend - hh.abeg + 1);
                blen = (hh.bend == 0 ? 1 : hh.bend - hh.bbeg + 1);
                if (hh.type == NORM_ADD) {
                    if (hh.abeg > 0) {
                        writeSrcLines(src, &src_line_num, hh.abeg, tgt);
                    }
                    writePatLines(pat, blen, tgt, offset);
                } else if (hh.type == NORM_DEL) {
                    skipLines(pat, alen);
                    if (hh.abeg > 0) {
                        writeSrcLines(src, &src_line_num, hh.abeg-1, tgt);
                    }
                    skipLines(src, alen);
                    src_line_num += alen;
                } else if (hh.type == NORM_CHG) {
                    writeSrcLines(src, &src_line_num, hh.abeg -1, tgt);
                    skipLines(src, alen);
                    src_line_num += alen;
                    skipLines(pat, alen+1); // +1 to skip divider.
                    writePatLines(pat, blen, tgt, offset);
                } else {
                    ERRMSG("Bad type\n");
                    retval = FAIL;
                }
            } else {
                if ( feof(pat) ) {
                    finishLines(src, tgt);
                } else {
                    ERRMSG("Invalid Normal HunkHeader\n");
                    retval = FAIL;
                }
                break;
            }
        } else {
            if ( feof(pat) ) {
                finishLines(src, tgt);
            } else {
                retval = FAIL;
            }
            break;
        }
    }
    return retval;
}


// Both '/' and '\' are interpreted as a directory separator.
void
makePath(char * path)
{
    FILE * fp = NULL;
    char * p;
    char dir[MAX_PATH_LEN];
    uint i = 0;

    if (isFile(path)) {
        return;
    }

    p = path;
    while (*p) {
        if (*p == DIRSEP || *p == ALTSEP) {
            dir[i] = '\0';
            if ( ! isDirectory(dir) ) {
                MKDIR(dir);
            }
            dir[i] = DIRSEP;
        } else {
            dir[i] = *p;
        }
        i++;
        p++;
    }

    if (( fp = fopen(path, "wb") )) {
        fclose(fp);
    } else {
        ERRMSG("Failed to create file: %s.\n", path);
    }
}

int
patchFile_helper(int type, FILE * pat, FILE * src, FILE * tgt)
{
    switch(type) {
        case TYPE_NORMAL_SINGLE:
            return patchNormalFile(pat, src, tgt);
        case TYPE_UNIFIED_SINGLE:
            return patchUnifiedFile(pat, src, tgt);
        default:
            ERRMSG("Unhandled file type\n");
    }
    return FAIL;
}


// Copy srcFilename to tgtFilename.
int
cp(char * srcFilename, char * tgtFilename)
{
    FILE * src = NULL;
    FILE * tgt = NULL;
    char ch;
    int retval = FAIL;
    long src_sz = 0;
    long tgt_sz = 0;

    if (( src = fopen(srcFilename, "rb") )) {
        if (fseek(src, 0L, SEEK_END) != 0) {
            fclose(src);
            return FAIL;
        }
        src_sz = ftell(src);
        if (src_sz == -1) {
            fclose(src);
            return FAIL;
        }
        rewind(src);
        if (( tgt = fopen(tgtFilename, "wb") )) {
            while (1) {
                ch = fgetc(src);
                if (feof(src)) {
                    break;
                }
                fputc(ch, tgt);
                if (feof(tgt)) {
                    break;
                }
            }
            fclose(tgt);
        }
        fclose(src);
    }

    // EOF and error is indistinguishable. If file sizes match,
    // we assume that copy was successful.
    if (( tgt = fopen(tgtFilename, "rb") )) {
        if (fseek(tgt, 0L, SEEK_END) != 0) {
            fclose(tgt);
            return FAIL;
        }

        tgt_sz = ftell(tgt);

        if (tgt_sz == -1) {
            fclose(tgt);
            return FAIL;
        }

        if (src_sz == tgt_sz) {
            retval = PASS;
        }
        fclose(tgt);
    }

    return retval;
}


int
patchFile(int type, char * patFile, char * srcFile, char * tgtFile)
{
    FILE * src = NULL;
    FILE * pat = NULL;
    FILE * tgt = NULL;
    int retval = FAIL;

    if (( pat = fopen(patFile, "rb") )) {
        if (( src = fopen(srcFile, "rb") )) {
            if (opt.screen) {
                retval = patchFile_helper(type, pat, src, tgt);
            } else {
                if (( tgt = fopen(tgtFile, "wb") )) {
                    retval = patchFile_helper(type, pat, src, tgt);
                    fclose(tgt);
                }
            }
            fclose(src);
        }
        fclose(pat);
    }
    if (retval == PASS && ! opt.screen && ! opt.create) {
        retval = cp(tgtFile, srcFile);
    }
    return retval;
}


// Copies srcbuf into tgtbuf, changing the initial directory to match the
// initial directory of dirname.
// If tgtbuf is not large enough to hold change, it is set to NULL.
// Both '/' and '\' are interpreted as a directory separator.
void
adjustPath(char * dirname, char * srcbuf, uint srcbufLen, char * tgtbuf, uint tgtbufLen)
{
    uint tgtpos = 0;
    char * tgt_start = tgtbuf;
    while (*dirname) {
        if (*dirname == DIRSEP || *dirname == ALTSEP) {
            break;
        }
        tgtpos++;
        if (tgtpos >= tgtbufLen) {
            *tgt_start = '\0';
            return;
        }
        *tgtbuf = *dirname;
        dirname++;
        tgtbuf++;
    }
    while (*srcbuf) {
        if (*srcbuf == DIRSEP || *srcbuf == ALTSEP) {
            break;
        }
        srcbuf++;
    }
    *tgtbuf = DIRSEP;
    while (*srcbuf) {
        tgtpos++;
        if (tgtpos >= tgtbufLen) {
            *tgt_start = '\0';
            return;
        }
        *tgtbuf = *srcbuf;
        srcbuf++;
        tgtbuf++;
    }
    *tgtbuf = '\0';
}

// Extracts two paths from a 'command' file.
// Filename is expected to contain a 'command'. e.g.
//      dif -r dir1/dira/file.txt dir2/dira/file.txt
// The two paths, which may be quoted, are extracted (excluding quotes).
// The first path (dir1/...) is copied to buf1.
// The seccond path (dir2/...) is copied to buf2.
// If an error occurs, both buf1 and buf2 are be set to NULL.
void
extractPathnames(char * filename, char * buf1, uint buf1len, char * buf2, uint buf2len)
{
    FILE * fp = NULL;
    char ch;
    int in_quotes = 0;
    uint ia = 0;
    uint ib = 0;
    buf1[0] = '\0';
    buf2[0] = '\0';
    if (( fp = fopen(filename, "rb") )) {
        while (1) {
            ch = fgetc(fp);
            if (feof(fp)) {
                break;
            }
            if (ch == '\r' || ch == '\n') {
                buf2[ib] = '\0';
                continue;
            }
            if (ch == '"') {
                in_quotes = !in_quotes;
                continue;
            }
            if (ch == ' ') {
                if (!in_quotes) {
                    buf2[ib] = '\0';
                    if (*buf2 != OPT_PREFIX1[0]) {
                        uint sz = strlen(buf2)+1;  // +1 for null
                        if (sz < buf1len) {
                            strncpy(buf1, buf2, sz);
                        } else {
                            ERRMSG("String length %d exceeds buffer size %d\n", sz, buf1len);
                            buf1[0] = '\0';
                            buf2[0] = '\0';
                            break;
                        }
                    }
                    buf2[0] = '\0';
                    ib = 0;
                    continue;
                }
            }
            buf2[ib] = ch;
            ib++;
            if (ib >= buf2len) {
                ERRMSG("Exceeded buffer length");
                buf1[0] = '\0';
                buf2[0] = '\0';
            }
        }
        fclose(fp);
    }
}


void
_finishMulti(int type, char * dirName, uint num)
{
    extractPathnames(cmdName, PATHA, MAX_PATH_LEN, PATHB, MAX_PATH_LEN);
    adjustPath(dirName, PATHA, MAX_PATH_LEN, PATHB, MAX_PATH_LEN);
    makePath(PATHB);
    sprintf(tgtName, "%s%c%08u.tgt", workdir, DIRSEP, num);
    patchFile(type, patName, PATHB, tgtName);
    if (! opt.create) {
        cp(tgtName, PATHB);
    }
}


int
splitMultiNormal(char * filename, char * dirname)
{
    FILE * fp = NULL;
    FILE * nn = NULL;
    FILE * dat = NULL;
    int check_next = 1;
    char ch;
    int start_it = 0;
    uint num = 1;
    int err = 0;
    int type = TYPE_NORMAL_SINGLE;
    if (( fp = fopen(filename, "rb") )) {
        while (1) {
            ch = fgetc(fp);
            if (feof(fp)) {
                break;
            }

            if (check_next) {
                if ( ch == '<' || ch == '>' || ch == '-' ) {
                } else if (isdigit(ch)) {
                    if (dat == NULL) {
                        sprintf(patName, "%s%c%08u.pat", workdir, DIRSEP, num);
                        if (! ( dat = fopen(patName, "wb") )) {
                            err = 1;
                            ERRMSG("Error opening %s\n", patName);
                            break;
                        }
                    }
                } else {
                    if (dat) {
                        fclose(dat);
                        dat = NULL;
                        _finishMulti(type, dirname, num);
                        num++;
                    }

                    if (nn) {
                        fclose(nn);
                        nn = NULL;
                    }

                    sprintf(cmdName, "%s%c%08u.cmd", workdir, DIRSEP, num);
                    if (! ( nn = fopen(cmdName, "wb") )) {
                        err = 1;
// meme
                        ERRMSG("Error opening %s\n", cmdName);
                        break;
                    }
                }
            }

            if (dat) {
                fputc(ch, dat);
            }

            if (nn) {
                fputc(ch, nn);
            }

            if (ch == '\n') {
                if (nn) {
                    fclose(nn);
                    nn=NULL;
                }
                check_next = 1;
                continue;
            }
            check_next = 0;
        }

        if (nn) {
            fclose(nn);
            nn=NULL;
        }

        if (dat) {
            fclose(dat);
            dat = NULL;
            _finishMulti(type, dirname, num);
            num++;
        }

        fclose(fp);
    }
    return (err == 0 ? PASS : FAIL);
}


int
splitMultiUnified(char * filename, char * dirname)
{
    FILE * fp = NULL;
    FILE * nn = NULL;
    FILE * dat = NULL;
    int check_next = 1;
    char ch;
    uint num = 0;
    int err = 0;
    int type = TYPE_UNIFIED_SINGLE;

    int open_cmd = 1;
    int open_pat = 0;

    if (( fp = fopen(filename, "rb") )) {
        while (1) {
            ch = fgetc(fp);
            if (feof(fp)) {
                break;
            }

            if (check_next) {
                if (ch != '-' && ch != '+' && ch != '@') {
                    if (dat) {
                        fclose(dat);
                        _finishMulti(type, dirname, num);
                        dat = NULL;
                    }
                    if (nn) {
                        fclose(nn);
                        nn = NULL;
                    }
                    sprintf(cmdName, "%s%c%08u.cmd", workdir, DIRSEP, num);
                    if (! ( nn = fopen(cmdName, "wb") )) {
                        err = 1;
// meme
                        ERRMSG("Error opening %s\n", cmdName);
                        break;
                    }
                } else if (nn && ch == '-') {
                    if (nn) {
                        fclose(nn);
                        nn = NULL;
                    }
                    if (dat) {   
                        fclose(dat);
                        dat = NULL;
                    }
                    num++;
                    sprintf(patName, "%s%c%08u.pat", workdir, DIRSEP, num);
                    if (! ( dat = fopen(patName, "wb") )) {
                        err = 1;
                        ERRMSG("Error opening %s\n", patName);
                        break;
                    }
                }
            }

            if (dat) {
                fputc(ch, dat);
            }

            if (nn) {
                fputc(ch, nn);
            }

            if (ch == '\n') {
                check_next = 1;
                continue;
            }
            check_next = 0;
        }

        if (nn) {
            fclose(nn);
            nn=NULL;
        }

        if (dat) {
            fclose(dat);
            dat = NULL;
            _finishMulti(type, dirname, num);
            num++;
        }

        fclose(fp);
    }
    return (err == 0 ? PASS : FAIL);
}


int
patchDir(int type, char * filename, char * dirname)
{
    if (isFile(filename)) {
        if (isDirectory(dirname)) {
            switch(type) {
                case TYPE_NORMAL_MULTI:
                    return splitMultiNormal(filename, dirname);
                case TYPE_UNIFIED_MULTI:
                    return splitMultiUnified(filename, dirname);
                default:
                    ERRMSG("Unhandled multi-type %d\n", type);
            }
        } else {
            ERRMSG("'%s' is not a directory\n", dirname);
        }
    } else {
        ERRMSG("'%s' is not a file\n", filename);
    }
    return FAIL;
}


int
processFilenames(char * fname1, char * fname2)
{

    int type = detectPatchType(fname1);
    switch(type) {
        case TYPE_NORMAL_SINGLE:
        case TYPE_UNIFIED_SINGLE:
            if (isFile(fname2)) {
                sprintf(tgtName, "%s%c%08u.tgt", workdir, DIRSEP, 1);
                return patchFile(type, fname1, fname2, tgtName);
            } else {
                ERRMSG("%s is a patch for a file, but %s is not a file.\n", fname1, fname2);
                return FAIL;
            }
        case TYPE_NORMAL_MULTI:
        case TYPE_UNIFIED_MULTI:
            if (isDirectory(fname2)) {
                return patchDir(type, fname1, fname2);
            } else {
                ERRMSG("%s is a patch for a directory, but %s is not a directory.\n", fname1, fname2);
                return FAIL;
            }
        case TYPE_UNKNOWN:
            MSG("Could not detect type\n");
            break;
        case TYPE_INDETERMINATE:
            MSG("Indeterminate type\n");
            break;
        default:
            ERRMSG("Unhandled type case %d\n", type);
            break;
    }
    return FAIL;
}


int
startsWith(char * str, char * prefix)
{
    if (strlen(str) >= strlen(prefix)) {
        while (*prefix) {
            if (*str != *prefix) {
                return FAIL;
            }
            prefix++;
            str++;
        }
    }
    return PASS;
}


int
hasOptPrefix1(char * str)
{
    return startsWith(str, OPT_PREFIX1);
}


int
hasOptPrefix2(char * str)
{
    return startsWith(str, OPT_PREFIX2);
}


int
isArg(char * arg, char ch, char * str)
{
    if (hasOptPrefix1(arg) && strlen(arg) == 2) {
        return arg[1] == ch;
    } else if (hasOptPrefix2(arg)) {
        arg += strlen(OPT_PREFIX2);
        if (strlen(arg) == strlen(str)) {
            while (*arg++ && *str++) {
                if (*arg != *str) {
                    return FAIL;
                }
            }
            return PASS;
        }
    }
    return FAIL;
}


uint
showType(char * filename)
{
    uint type = detectPatchType(filename);
    switch(type) {
        case TYPE_NORMAL_SINGLE:
            MSG("Normal diff of file\n");
            break;
        case TYPE_NORMAL_MULTI:
            MSG("Normal diff of directory\n");
            break;
        case TYPE_UNIFIED_SINGLE:
            MSG("Unified diff of file\n");
            break;
        case TYPE_UNIFIED_MULTI:
            MSG("Unified diff of directory\n");
            break;
        case TYPE_UNKNOWN:
            MSG("Unknown type\n");
            break;
        case TYPE_INDETERMINATE:
            MSG("Indeterminate type\n");
            break;
        default:
            ERRMSG("Unhandled type case %d\n", type);
            break;
    }
    return type;
}



void
showVersion(char * progname)
{
    MSG("%7s v%s\n", basename(progname), VERSION);
}


void
showHelp( char * progname )
{
    // Specify '%2s' to make fmod simpler: Unix uses '--'; DOS uses '/'.
    MSG("Usage: %7s [options] PATCHFILE ORIGFILE\n", basename(progname));
    MSG("       %7s [options] PATCHFILE ORIGDIR\n", basename(progname));
    MSG("       %7s [%st | %2stype] PATCHFILE\n", basename(progname), OPT_PREFIX1, OPT_PREFIX2);
    MSG("\n");
    MSG("  %sw, %2sworkdir DIR   Name of workdir (default: 'tmp')\n",       OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sk, %2skeep          Do not delete workdir on exit\n",          OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sq, %2squiet         Suppress normal output\n",                 OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %ss, %2sscreen        Output to screen\n",                       OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sc, %2screate        Create patches but do not update files\n", OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %st, %2stype          Print type of PATCHFILE\n",                OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sh, %2shelp          Show this help\n",                         OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sv, %2sversion       Show version\n",                           OPT_PREFIX1, OPT_PREFIX2);
    MSG("\n");
    MSG("DIR can be a max of 8 chars; anything longer will be truncated.\n");
    MSG("Use of '%sc' implies '%sk'; the patched files are stored in the workdir.\n", OPT_PREFIX1, OPT_PREFIX1 );
}



void
initOpts()
{
    opt.create       = 0;
    opt.help         = 0;
    opt.indexF1      = 0;
    opt.indexF2      = 0;
    opt.keep         = 0;
    opt.quiet        = 0;
    opt.screen       = 0;
    opt.type         = 0;
    opt.version      = 0;
    opt.workdirIndex = 0;
}


void
parseArgs(int argc, char** argv )
{
    char * progname = *argv;
    uint n = 1;

    while (n < argc) {
        if      (0) {}
        else if (isArg(argv[n], 'c', "create" )) { opt.create   = 1; }
        else if (isArg(argv[n], 'h', "help"   )) { opt.help     = 1; }
        else if (isArg(argv[n], 'k', "keep"   )) { opt.keep     = 1; }
        else if (isArg(argv[n], 'q', "quiet"  )) { opt.quiet    = 1; }
        else if (isArg(argv[n], 's', "screen" )) { opt.screen   = 1; }
        else if (isArg(argv[n], 't', "type"   )) { opt.type     = 1; }
        else if (isArg(argv[n], 'v', "version")) { opt.version  = 1; }
        else if (isArg(argv[n], 'w', "workdir")) { opt.workdirIndex = ++n; }
#if defined(__DOS__)
        // 'Secret' option.
        else if (strcmp(argv[n], "/?") == 0)      { opt.help     = 1; }
#endif
        else if (hasOptPrefix1(argv[n])) {
            ERRMSG("Unrecognized option: %s\n", argv[n]);
            exit(EXIT_ERR);
        }
        else if (opt.indexF1 == 0) { opt.indexF1 = n; }
        else if (opt.indexF2 == 0) { opt.indexF2 = n; }
        else {
            ERRMSG("Too many files or dirs specified. Exactly two are required.\n");
            exit(EXIT_ERR);
        }
        if (n < argc) {
            n++;
        }
    }

    if (opt.help) {
        showHelp(progname);
        exit(EXIT_OK);
    }
    
    if (opt.version) {
        showVersion(progname);
        exit(EXIT_OK);
    }

    if (opt.workdirIndex > 0 && opt.workdirIndex < argc) {
        uint i;
        char * wd = argv[opt.workdirIndex];
        uint wd_len = strlen(wd);
        for (i=0; (i < wd_len) && (i < 8) && (wd[i] != DIRSEP) && (wd[i] != '.'); i++) {
            workdir[i] = wd[i];
        }
        workdir[i] = '\0';
    }

    if (opt.indexF1 == 0) {
       ERRMSG("Missing NAME1\n");
       exit(EXIT_ERR);
    }
    
    if (opt.indexF2 == 0 && opt.type == 0) {
       ERRMSG("Missing NAME2\n");
       exit(EXIT_ERR);
    }

    if (opt.create) {
        opt.keep = 1;
    }
}


void
trimSlash(char * name)
{
    uint pos = strlen(name);
    if (pos > 0) {
        pos--;
        while (name[pos] == DIRSEP) {
            name[pos] = '\0';
            pos--;
        }
    }
}


int
createWorkdir(char * dir)
{
    if (isDirectory(dir)) {
        return (opt.workdirIndex != 0 && strcasecmp(dir, workdir) == 0);
    } else {
        return (MKDIR(dir) == 0);
    }
}


int
deleteWorkdir(char * path)
{
    DIR * dir;
    struct dirent * ent;
    int err = 0;
    char nam[22]; // 8 + dirsep + 8.3 + null

    if (opt.keep) {
        return PASS;
    }

    if (strlen(path) <= 8) {
        dir = opendir(path);
        if (dir) {
            while ((ent = readdir(dir)) != NULL) {
                if (strlen(ent->d_name) <= 12) { // len(8.3) == 12
                    sprintf(nam, "%s%c%s", path, DIRSEP, ent->d_name);
                    if (isFile(nam)) {
                        if (remove(nam) != 0) {
                            err = 1;
                            break;
                        }
                    }
                }
            }
            closedir(dir);
            if (!err) {
                return (rmdir(path) == 0);
            }
        }
    }
    return FAIL;
}


int
doPatching( char * fname1, char * fname2 )
{
    if (createWorkdir(workdir)) {
        if (processFilenames(fname1, fname2)) {
            return deleteWorkdir(workdir);
        }
    } else {
        ERRMSG("Dir '%s' exists. Delete, rename, or supply the '%sw' option.\n", workdir, OPT_PREFIX1);
    }        
    return FAIL;
}


// void setOpenFilesPerProcess( void )    
// {
// 	_asm mov ah, 67h
// 	_asm mov bx, 30
// 	_asm int 21h
// 	return;
// }

int
main ( int argc, char** argv )
{
    uint retval;


// setOpenFilesPerProcess()

    initOpts();
    
    parseArgs(argc, argv);

    if (opt.type) {
        return showType(argv[opt.indexF1]);
    }

    retval = doPatching(argv[opt.indexF1], argv[opt.indexF2]);
    
    return (retval == PASS ? EXIT_OK : EXIT_ERR);
}
