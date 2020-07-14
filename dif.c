#include <ctype.h>
#if defined(__DOS__)
#  include <direct.h>
#else
#  include <dirent.h>
#endif
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

// Uncomment the '__CONTEXT__' define to enable Contextual diff.
// A Contextual diff is a bit pointless because no context is output, so
// it's no better than a simple diff.
// Tests will need to be updated.
// #define __CONTEXT__ 0


#define __ERRMSG(fmt, ...) fprintf(stderr, fmt "%s", __VA_ARGS__);
#define ERRMSG(...) __ERRMSG(__VA_ARGS__, "")


#define __MSG(fmt, ...)                    \
    do {                                   \
        if (!opt.quiet) {                  \
            printf(fmt "%s", __VA_ARGS__); \
        }                                  \
    } while (0)

#define MSG(...) __MSG(__VA_ARGS__, "")


#define VERSION "0.1"

// 2^16 = 65535

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
#else
    #define OPT_PREFIX1 "-"
    #define OPT_PREFIX2 "--"
    #define DIRSEP '/'
#endif


#define PASS 1
#define FAIL 0

#define FILES_EQUAL 0
#define FILES_DIFFER 1
#define FILES_ERROR 2

#define EXIT_OK 0
#define EXIT_ERR 2


#define CR '\r'
#define LF '\n'


#if !defined(uint)
    typedef unsigned int uint;
#endif


struct Opt {
    int altchar;
    int brief;
#ifdef __CONTEXT__
    int context;
#endif
    int data;
    int empty;
    int gloss;
    int help;
    int indexF1;
    int indexF2;
    int lineend;
    int normal;
    int quiet;
    int recurse;
    int space;
    int twocol;
    int unified;
    int version;
    int width;
};

struct Line {
    uint offset; // 1-indexed.
    uint length;
    unsigned long hash;
    uint matched_line;
};

// The 'line' array is 1-indexed; the zeroth item is unused.
struct Doc {
    char fname[MAX_PATH_LEN];
    uint num_lines;
    struct Line * line;
    uint buflen;
    char * buf; // Scratch.
    int line_term;
};

struct Doc afile;
struct Doc bfile;

struct Opt opt;

char TWOCOL_LEFT[]  = "<";
char TWOCOL_RIGHT[] = ">";
char TWOCOL_MOD[]   = "%";
char TWOCOL_EQUAL[] = "=";

char ** cmdline;

char NORMAL_DEL[]  = "< ";
char NORMAL_ADD[]  = "> ";

char UNIFIED_ADD[] = "+";
char UNIFIED_DEL[] = "-";

#ifdef __CONTEXT__
char CONTEXT_ADD[] = "+ ";
char CONTEXT_DEL[] = "- ";
char CONTEXT_MOD[] = "! ";
#endif

char BRIEF_MOD[]   = "% ";
char BRIEF_EQUAL[] = "= ";
char BRIEF_LEFT[]  = "A ";
char BRIEF_RIGHT[] = "B ";


// Prototype
void doWalk(uint amin, uint amax, uint bmin, uint bmax);


// From http://www.cse.yorku.ca/~oz/hash.html
unsigned long
hash ( unsigned char *str )
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}


char *
trimRoot(char * path)
{
    char * p = path;
    while (*p++ != DIRSEP);
    return p;
}


int
getLineTerm(const char * fname)
{
    int ans = LF;
    int got_cr = 0;
    FILE * fp = fopen(fname, "rb");
    if (fp) {
        while(1) {
            char ch = fgetc(fp);
            if ( feof(fp) ) {
                break;
            }
            if (ch == LF) {
                ans = LF;
                break;
            }
            if (got_cr) {
                ans = CR;
                break;
            }
            if (ch == CR) {
                got_cr = 1;
            }
        }
        fclose(fp);
    }
    return ans;
}


int
countLines ( const char * fname, uint * lines, uint * chars )
{
    int MAX_LINES_EXCEEDED = 0;
    int MAX_CHARS_EXCEEDED = 0;
    uint len = 0;
    uint max_len = 0;
    char lineTerm = getLineTerm(fname);
    FILE * fp = fopen(fname, "rb");
    *lines = 0;
    if (fp) {
        while(1) {
            char ch = fgetc(fp);
            if ( feof(fp) ) {
                if (len > 0) {
                    (*lines)++;
                }
                break;
            }
            len++;
            if (len >= UINT_MAX-1) {
                MAX_CHARS_EXCEEDED = 1;
                break;
            }
            if (ch == lineTerm) {
                if (len > max_len) {
                    max_len = len;
                }
                len = 0;
                (*lines)++;
                if (*lines >= UINT_MAX-3) { // 3 is 2 extra + 1 in case feof().
                    MAX_LINES_EXCEEDED = 1;
                    break;
                }
                continue;
            }
        }
        fclose(fp);
        *chars = max_len + 1; // +1 for null.
    }
    if (MAX_LINES_EXCEEDED) {
        ERRMSG("%s has too many lines.\n", fname);
        return FAIL;
    }
    if (MAX_CHARS_EXCEEDED) {
        ERRMSG("%s has too many chars on line %u.\n", fname, (*lines)-1);
        return FAIL;
    }
    return PASS;
}


int
isTabSpace(char b) {
    return b == ' ' || b == '\t';
}


// A line ending with '\r\r\n' will be interpreted as '\r' followed by CRLF,
// rather than two '\r' followed by LF.
void
normalizeBuffer(char * buf, char lineTerm)
{
    char * a;
    char * b;
    int in_space;
    int isTs;
    uint sz = strlen(buf);
    if (opt.lineend) {
        if (sz > 0) {
            if (buf[sz - 1] == lineTerm) {
                buf[sz - 1] = LF;
            }
            if (sz > 1) {
                if (buf[sz - 2] == CR && buf[sz - 1] == LF) {
                    buf[sz - 2] = LF;
                    buf[sz - 1] = '\0';
                }
            }
        }
    }

    a = buf;
    b = buf;
    in_space = 0;
    if (opt.space) {
        in_space = isTabSpace(*b);
        while (*b) {
            if (*b == LF) {
                break;
            }
            isTs = isTabSpace(*b);
            if (isTs) {
                if (!in_space) {
                    *a = ' ';
                    a++;
                    in_space = 1;
                }
            } else {
                *a = *b;
                a++;
                in_space = 0;
            }
            b++;
        }
        *a = '\0';
    }
}


void
finishLine( struct Doc * doc, uint lineNum, uint n )
{
    if (n > 0) {
        doc->buf[n] = '\0';
        normalizeBuffer(doc->buf, doc->line_term);
        doc->line[lineNum].length=n;
        doc->line[lineNum].hash=hash(doc->buf);
        doc->line[lineNum].matched_line=0;
    }
}


void
popLines ( struct Doc * doc )
{
    FILE *fp;
    uint line_num=1;
    uint abs_pos=0;
    uint n=0;
    fp = fopen(doc->fname, "rb");
    if (fp) {
        while (1) {
            char ch = fgetc(fp);
            if (feof(fp)) {
                finishLine(doc, line_num, n);
                break;
            }
            abs_pos++;
            doc->buf[n] = ch;
            n++;

            if (n >= doc->buflen) {
                ERRMSG("Line %u too long.\n", line_num);
                fclose(fp);
                exit(EXIT_ERR);
            }

            if (ch == doc->line_term) {
                finishLine(doc, line_num, n);
                line_num++;
                n = 0;
                // If at last line of file, don't write the next array index; it doesn't exist.
                if (line_num <= doc->num_lines) {
                    doc->line[line_num].offset=abs_pos+1; // offset is 1-indexed.
                }
                continue;
            }
        }
        if (abs_pos > 0) {
            doc->line[1].offset=1; // Line is 1-indexed. Offset is 1-indexed.
        }
        fclose(fp);
    }
}


int
makeDoc ( const char * fnam, struct Doc * doc )
{
    uint extra = 2; // Add 1 because 'line' is 1-indexed; add 1 for 'sentinel' record.
    uint i;
    int cl;
    memset(doc->fname, '\0', MAX_PATH_LEN);
    strncpy(doc->fname, fnam, MAX_PATH_LEN-1);
    doc->line = NULL;
    doc->buf = NULL;
    doc->num_lines = 0;
    doc->buflen = 0;
    doc->line_term=getLineTerm(doc->fname);
    cl = countLines(doc->fname, &(doc->num_lines), &(doc->buflen));
    // if (cl || opt.partial) {
    if (cl) {
        doc->line = malloc(sizeof(struct Line) * (doc->num_lines + extra));
        if ( doc->line ) {
            for( i = 0; i < doc->num_lines + extra; i++ ) {
                 (doc->line[i]).offset = 0;
                 (doc->line[i]).length = 0;
                 (doc->line[i]).hash = 0;
                 (doc->line[i]).matched_line = 0;
            }
            if (doc->buflen == 0) {
                return PASS;
            }
            doc->buf = malloc(doc->buflen);
            if (doc->buf) {
                return PASS;
            } else {
                ERRMSG("Program error: malloc buf failed\n");
            }
        } else {
            ERRMSG("Program error: malloc line failed\n");
        }
    }
    return FAIL;
}


void
showDoc ( struct Doc * doc )
{
    uint i;
    MSG("Name: %s\n", doc->fname);
    MSG("Lines: %u\n", doc->num_lines);
    MSG("Buflen: %u\n", doc->buflen);
    for (i=1; i<doc->num_lines+1; i++) {
        struct Line l = doc->line[i];
        // 20 is wide enough to hold an unsigned 64-bit int. It also
        // adds enough padding on DOS systems to make fmod replacement simple.
        MSG("%u:%u:%u:%u  %20lu\n", i, l.offset, l.length, l.matched_line, l.hash);
    }
    MSG("\n");
}


void
cleanup()
{
    if (afile.line) {
        free(afile.line);
        afile.line = NULL;
    }
    if (afile.buf) {
        free(afile.buf);
        afile.buf = NULL;
    }
    if (bfile.line) {
        free(bfile.line);
        bfile.line = NULL;
    }
    if (bfile.buf) {
        free(bfile.buf);
        bfile.buf = NULL;
    }
}


int
isFragMatch ( uint abeg, uint bbeg, uint lines )
{
    uint i;
    if (abeg + lines > afile.num_lines+1) {
        ERRMSG("Program error: AFile exceeded expected length.\n");
        return FAIL;
    }
    if (bbeg + lines > bfile.num_lines+1) {
        ERRMSG("Program error: BFile exceeded expected length.\n");
        return FAIL;
    }
    // Check hash.
    for (i=0; i<lines; i++) {
        if (afile.line[abeg+i].hash != bfile.line[bbeg+i].hash) {
            return FAIL;
        }
    }
    // Assign matching lines.
    for (i=0; i<lines; i++) {
        afile.line[abeg+i].matched_line = bbeg + i;
        bfile.line[bbeg+i].matched_line = abeg + i;
    }
    return PASS;
}


// Walk b over a. i.e. length(a) > length(b)
void
aa ( uint amin, uint amax, uint bmin, uint bmax )
{
    uint lines;
    uint ia;
    uint ib;
    int found=0;
    for (lines=amax-amin+1; lines>0 && !found; lines--) {
        for (ia=amin; ia<=amax-lines+1 && !found; ia++) {
            for (ib=bmin; ib<=bmax-lines+1 && !found; ib++) {
                if (isFragMatch(ia, ib, lines)) {
                    if (! found) {
                        found=1;
                        if (ia-1 >= amin && ib-1 >= bmin) {
                            doWalk(amin, ia-1, bmin, ib-1);
                        }
                        if (ia+lines <= amax && ib+lines <= bmax) {
                            doWalk(ia+lines, amax, ib+lines, bmax);
                        }
                    }
                }
            }
        }
    }
}


// Walk a over b. i.e. length(b) > length(a)
void
bb ( uint amin, uint amax, uint bmin, uint bmax )
{
    uint lines;
    uint ia;
    uint ib;
    int found=0;
    for (lines=bmax-bmin+1; lines>0 && !found; lines--) {
        for (ib=bmin; ib<=bmax-lines+1 && !found; ib++) {
            for (ia=amin; ia<=amax-lines+1 && !found; ia++) {
                if (isFragMatch(ia, ib, lines)) {
                    if (! found) {
                        found=1;
                        if (ia-1 >= amin && ib-1 >= bmin) {
                            doWalk(amin, ia-1, bmin, ib-1);
                        }
                        if (ia+lines <= amax && ib+lines <= bmax) {
                            doWalk(ia+lines, amax, ib+lines, bmax);
                        }
                    }
                }
            }
        }
    }
}


int
isALarger ( uint amin, uint amax, uint bmin, uint bmax )
{
    return (amax - amin) > (bmax - bmin);
}


void
doWalk ( uint amin, uint amax, uint bmin, uint bmax )
{
    if (isALarger(amin, amax, bmin, bmax)) {
        bb( amin, amax, bmin, bmax );
    } else {
        aa( amin, amax, bmin, bmax );
    }
}


void
fillBuff(char * buf, struct Doc * doc, uint i, FILE * fp)
{
    uint offset = doc->line[i].offset;
    *buf = '\0';
    if (offset == 0) {
        return;
    }
    offset--;

    if (fseek(fp, offset, SEEK_SET) != 0) {
        return;
    }

    if (fread(buf, doc->line[i].length, 1, fp) == 0) {
        return;
    }
    buf[doc->line[i].length] = '\0';
    normalizeBuffer(buf, doc->line_term);
}


void
verifyDiff()
{
    int err = 0;

    uint ia;
    uint ib;

    FILE * fpa = NULL;
    FILE * fpb = NULL;

    fpa = fopen(afile.fname, "rb");
    if (!fpa) {
        err = 1;
    }

    fpb = fopen(bfile.fname, "rb");
    if (!fpb) {
        err = 2;
    }

    for (ia=1; ia<afile.num_lines+1 && err==0; ia++) {
        uint ib = afile.line[ia].matched_line;

        if (ib > 0) {

            if (bfile.line[ib].matched_line != ia) {
                err = 3;
                break;
            }

            if (!opt.lineend && !opt.space) {
                if (afile.line[ia].length !=  bfile.line[ib].length) {
                    err = 4;
                    break;
                }
            }

            fillBuff(afile.buf, &afile, ia, fpa);
            fillBuff(bfile.buf, &bfile, ib, fpb);

            if (strcmp(afile.buf, bfile.buf) != 0) {
                afile.line[ia].matched_line = 0;
                bfile.line[ib].matched_line = 0;
            }

        }
    }

    if (fpa) {
        fclose(fpa);
    }
    if (fpb) {
        fclose(fpb);
    }
    if (err) {
        ERRMSG("Error %d validating\n", err);
        exit(EXIT_ERR);
    }
}

void
dodiff ( )
{
    if (afile.num_lines == 0) {
        return;
    }
    if (bfile.num_lines == 0) {
        return;
    }
    doWalk(1, afile.num_lines, 1, bfile.num_lines);
    verifyDiff();
}


int
hasMatchingLine ( )
{
    uint i;
    for (i=1; i<afile.num_lines+1; i++) {
        if (afile.line[i].matched_line > 0) {
            return PASS;
        }
    }
    return FAIL;
}


int
isBothEmpty (  )
{
    return afile.num_lines == 0 && bfile.num_lines == 0;
}


int
isBothSame (  )
{
    uint i;
    uint num;
    uint prevnum = 0;

    if (isBothEmpty()) {
        return FILES_EQUAL;
    }

    if (afile.num_lines != bfile.num_lines) {
        return FILES_DIFFER;
    }

    for (i=1; i<afile.num_lines + 1; i++) {
        num = afile.line[i].matched_line;
        if (num != prevnum + 1) {
            return FILES_DIFFER;
        }
        prevnum = num;
    }
    return FILES_EQUAL;
}



int
isFileReadable(char * path)
{
    FILE *fp = fopen(path, "rb");
    int isReadable = (fp != NULL);
    if (isReadable) {
        fclose(fp);
    }
    return isReadable;
}


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

// Copy line_num from fp into doc. If raw==1, chars are copied verbatim; if
// raw==0 a char less than 32 is copied as a space.
void
copyLine ( FILE * fp, struct Doc * doc, uint line_num, int raw)
{
    uint i;
    uint offset;

    if (!doc->line) {
        return;
    }
    if (line_num > doc->num_lines) {
        return;
    }

    offset=doc->line[line_num].offset;
    if (offset > 0) {
        offset--; // Subtract 1 to get real offset.
        if (fseek(fp, offset, SEEK_SET) == 0) {
            for(i=0; i < doc->line[line_num].length; i++) {
                char ch = fgetc(fp);
                if ( feof(fp) ) {
                    return;
                }
                if (i < doc->buflen) {
                    if (raw == 0 && (ch < ' ')) {
                        doc->buf[i] = ' ';
                    } else {
                        doc->buf[i] = ch;
                    }
                }
                if (ch == doc->line_term) {
                    if (i < doc->buflen - 1) {
                        i++;
                    }
                    doc->buf[i] = '\0';
                    return;
                }
            }
        } else {
            ERRMSG("Seek failed\n");
        }
    } else {
        ERRMSG("Invalid offset\n");
    }
    doc->buf[i] = '\0';
}


// n       Line number
// prefix  Leftpad width
// width   Column width
// x==0    Print no divider
// x==1    Print divider before printing line
// x==2    Print divider after printing line
void
showPadLine ( FILE * fp, struct Doc * doc, uint n, uint prefix, uint width, int x )
{
    uint len = 0;
    uint i=0;

    while (i < prefix) {
        MSG(" ");
        i++;
    }

    if (x==1) {
        MSG("|");
    }

    if (n <= doc->num_lines) { // Sanity check. This should always be true.
        copyLine(fp, doc, n, 0);
        len = doc->line[n].length;
        if (width < len) {
            doc->buf[width] = '\0';
        }
        MSG("%s", doc->buf);
    }

    while (len < width) {
        MSG(" ");
        len++;
    }
    if (x==2) {
        MSG("|");
    }
}


uint
maxLineWidth(struct Doc *doc)
{
    uint i;
    uint max = 0;
    for (i=1; i < doc->num_lines+1; i++) {
        uint val = doc->line[i].length;
        if (val > max) {
            max = val;
        }
    }
    return max;
}


int
showTwoCol ( )
{
    uint acol_width;
    uint bcol_width;

    uint max_iter = (afile.num_lines > bfile.num_lines ? afile.num_lines : bfile.num_lines);

    uint ai = 1;
    uint bi = 1;
    uint am;
    uint bm;
    int afin = 0;
    int bfin = 0;

    FILE * fpa = NULL;
    FILE * fpb = NULL;

    if (opt.width) {
        acol_width = opt.width;
        bcol_width = opt.width;
    } else {
        acol_width = maxLineWidth(&afile);
        bcol_width = maxLineWidth(&bfile);
    }

    if (isFile(afile.fname)) {
        fpa = fopen(afile.fname, "rb");
        if (!fpa) {
            return FAIL;
        }
    }

    if (isFile(bfile.fname)) {
        fpb = fopen(bfile.fname, "rb");
        if (!fpb) {
            fclose(fpa);
            return FAIL;
        }
    }

    while (1) {
        if (ai <= afile.num_lines) {
            am = afile.line[ai].matched_line;
        }
        else {
            afin = 1;
        }

        if (bi <= bfile.num_lines) {
            bm = bfile.line[bi].matched_line;
        }
        else {
            bfin = 1;
        }

        if (afin == 1 && bfin == 1) {
            break;
        } else if (afin ==1) {
            MSG("%s", TWOCOL_RIGHT);
            showPadLine ( fpb, &bfile, bi, acol_width, bcol_width, 1 );
            MSG("\n");
            bi++;
        } else if (bfin == 1) {
            MSG("%s", TWOCOL_LEFT);
            showPadLine ( fpa, &afile, ai, 0, acol_width, 2 );
            MSG("\n");
            ai++;
        } else if (ai == bm) { //   && bi == am) {
            MSG("%s", TWOCOL_EQUAL);
            showPadLine ( fpa, &afile, ai, 0, acol_width, 0 );
            showPadLine ( fpb, &bfile, bi, 0, bcol_width, 1 );
            MSG("\n");
            ai++;
            bi++;
        } else if (am == 0 && bm == 0) {
            MSG("%s", TWOCOL_MOD);
            showPadLine ( fpa, &afile, ai, 0, acol_width, 0 );
            showPadLine ( fpb, &bfile, bi, 0, bcol_width, 1 );
            MSG("\n");
            ai++;
            bi++;
        } else if (am == 0) {
            MSG("%s", TWOCOL_LEFT);
            showPadLine ( fpa, &afile, ai, 0, acol_width, 2 );
            MSG("\n");
            ai++;
        } else if (bm == 0) {
            MSG("%s", TWOCOL_RIGHT);
            showPadLine ( fpb, &bfile, bi, acol_width, bcol_width, 1 );
            MSG("\n");
            bi++;
        } else if (ai == am) {
            MSG("%s", TWOCOL_EQUAL);
            showPadLine ( fpa, &afile, ai, 0, acol_width, 0 );
            showPadLine ( fpb, &bfile, bi, 0, bcol_width, 1 );
            MSG("\n");
            ai++;
            bi++;
        } else
        {
            printf("Unhandled case\n");
        }
    }

    if (fpa) {
        fclose(fpa);
    }
    if (fpb) {
        fclose(fpb);
    }
    return PASS;
}


void
getRunLen ( struct Doc * doc, uint * i, uint * beg, uint * runlen, uint * isZeros)
{
    uint ml;
    uint prevMl;

    if (*i > doc->num_lines) {
        *beg = *i;
        *isZeros = 0;
        *runlen = 0;
        return;
    }

    prevMl = doc->line[*i].matched_line;
    *beg = *i;
    *isZeros = (prevMl == 0);
    *runlen = 1;

    while (1) {
        (*i)++;
        if (*i > doc->num_lines) {
            break;
        }
        ml = doc->line[*i].matched_line;
        if (ml == 0) {
            if (prevMl == 0) {
                (*runlen)++;
            } else {
                break;
            }
        } else {
            if (prevMl == 0) {
                break;
            } else if (ml == prevMl + 1) {
                (*runlen)++;
                prevMl = ml;
            } else {
                break;
            }
        }
    }
}


// Under FreeDOS, printing to screen automatically converts LF to CR, LF.
// If an adjustment is not made to buf, CRLF will be output as CR, CR, LF.
int
adjustLineEnding(char * buf)
{
    int adjusted = 0;
#if defined(__DOS__)
    uint len = strlen(buf);
    if (len > 1) {
        if (buf[len-2] == CR && buf[len-1] == LF) {
            buf[len-2] = LF;
            buf[len-1] = '\0';
            adjusted=1;
        }
    }
#endif
    return adjusted;
}

// Print count lines of doc starting at beg. Prepend prefix.
void
printLines(FILE * fp, struct Doc * doc, uint beg, uint count, char * prefix)
{
    uint i;
    uint x;
    uint isAdjusted = 0;

    for( i = beg; i < beg + count; i++ ) {
        copyLine ( fp, doc, i, 1);
        isAdjusted = adjustLineEnding(doc->buf);
        MSG("%s%s", prefix, doc->buf);
    }
    if (i > 0 && ! isAdjusted) {
        x = doc->line[i-1].length;
        if (doc->buf[x-1] != doc->line_term) {
            MSG("\n\\ No newline at end of file\n");
        }
    }
}

int needQuote(const char * path) {
    while (*path) {
        if (*path == ' ') {
            return PASS;
        }
        path++;
    }
    return FAIL;
}


char *
lowercase(char * str)
{
    char * s = str;
    while (*s) {
        *s = tolower(*s);
        s++;
    }
    return str;
}


void
showCmd(char * patha, char * pathb)
{
    uint i=0;
    char ** arg = cmdline;

    // The Watcom C-library supplies the absolute path to the executable for
    // the '*arg'; we only want the executable name, hence 'basename()'.
    // It also returns the arg in uppercase, which alters tho sort order
    // when comparing output in tests, hence 'lowercase()'.
    MSG("%s ", lowercase(basename(*arg)));
    *arg++;
    while (*arg) {
        i++;
        if (i != opt.indexF1 && i != opt.indexF2) {
            MSG("%s ", *arg);
        }
        *arg++;
    }
    if (needQuote(patha)) {
        MSG("\"%s\" ", patha);
    } else {
        MSG("%s ", patha);
    }
    if (needQuote(pathb)) {
        MSG("\"%s\"\n", pathb);
    } else {
        MSG("%s\n", pathb);
    }
}


void
printNormalHeader(uint bega, uint runa, uint begb, uint runb)
{
    uint aaa = 0;
    uint bbb = 0;
    char ch;
    if (runa == 0) {
        ch = 'a';
        if (bega == 0) {
            aaa=0;
        } else {
            aaa = bega-1;
        }
        bbb = begb;
    } else if (runb == 0) {
        ch = 'd';
        aaa = bega;
        if (begb == 0) {
            bbb = 0;
        } else {
            bbb = begb-1;
        }
    } else {
        ch = 'c';
        aaa = bega;
        bbb = begb;
    }

    MSG("%u", aaa);
    if (runa > 1) {
        MSG(",%u", bega+runa-1);
    }

    MSG("%c", ch);

    MSG("%u", bbb);
    if (runb > 1) {
        MSG(",%u", begb+runb-1);
    }
    MSG("\n");
}


void
showNormalDiff ( )
{
    uint ia = 1;
    uint bega = 0;
    uint runa = 1;
    uint iza;

    uint ib = 1;
    uint begb = 0;
    uint runb = 1;
    uint izb;

    FILE * fpa = NULL;
    FILE * fpb = NULL;

    if (isFile(afile.fname)) {
        fpa = fopen(afile.fname, "rb");
        if (!fpa) {
            return;
        }
    }

    if (isFile(bfile.fname)) {
        fpb = fopen(bfile.fname, "rb");
        if (!fpb) {
            fclose(fpa);
            return;
        }
    }

    getRunLen(&afile, &ia, &bega, &runa, &iza);
    getRunLen(&bfile, &ib, &begb, &runb, &izb);

    while (runa > 0 || runb > 0) {
        if (iza  && !izb) {
            printNormalHeader(bega, runa, begb, 0);
            printLines(fpa, &afile, bega, runa, NORMAL_DEL);
            getRunLen(&afile, &ia, &bega, &runa, &iza);
        } else if (!iza && izb) {
            printNormalHeader(bega, 0, begb, runb);
            printLines(fpb, &bfile, begb, runb, NORMAL_ADD);
            getRunLen(&bfile, &ib, &begb, &runb, &izb);
        } else {
            if (iza && izb) {
                printNormalHeader(bega, runa, begb, runb);
                printLines(fpa, &afile, bega, runa, NORMAL_DEL);
                MSG("---\n");
                printLines(fpb, &bfile, begb, runb, NORMAL_ADD);
            }
            getRunLen(&afile, &ia, &bega, &runa, &iza);
            getRunLen(&bfile, &ib, &begb, &runb, &izb);
        }
    };

    if (fpa) {
        fclose(fpa);
    }
    if (fpb) {
        fclose(fpb);
    }
}


void
printUnifiedHeader(uint bega, uint runa, uint begb, uint runb)
{
    MSG("@@ ");

    if (bega > 0 && runa==0) {
        bega--;
    }
    MSG("-%u", bega);
    if (runa != 1) {
        MSG(",%u", runa);
    }

    MSG(" ");

    if (begb > 0 && runb==0) {
        begb--;
    }
    MSG("+%u", begb);
    if (runb != 1) {
        MSG(",%u", runb);
    }
    MSG(" @@");

    MSG("\n");
}


#if defined(__DOS__)
    void
    getFileModifiedTime(char * filename, time_t * tv_sec, long int * tv_nsec)
    {
        struct stat attrib;
        *tv_sec = 0;
        *tv_nsec = 0;
        if (isFile(filename)) {
            stat(filename, &attrib);
            *tv_sec = attrib.st_mtime;
            *tv_nsec = 0;
        }
    }
#else
    void
    getFileModifiedTime(char * filename, time_t * tv_sec, long int * tv_nsec)
    {
        struct stat attrib;
        *tv_sec = 0;
        *tv_nsec = 0;
        if (isFile(filename)) {
            stat(filename, &attrib);
            *tv_sec = attrib.st_mtim.tv_sec;
            *tv_nsec = attrib.st_mtim.tv_nsec;
        }
    }
#endif


int
formatFileModifiedTime(char *buf, uint buflen, char * filename)
{
    int ret;
    time_t tv_sec;
    long int tv_nsec;

    time_t now = time(NULL);
    struct tm lcl = *localtime(&now);
    struct tm gmt = *gmtime(&now);
    int gmtHour = (lcl.tm_hour > gmt.tm_hour) ? gmt.tm_hour+24 : gmt.tm_hour;

    getFileModifiedTime(filename, &tv_sec, &tv_nsec);
    tzset();
    ret = strftime(buf, buflen, "%F %T", localtime(&tv_sec));
    if (ret) {
        buflen -= ret - 1;
        ret = snprintf(&buf[strlen(buf)],
                        buflen,
                        ".%09ld %03d%02d",
                        tv_nsec,
                        lcl.tm_hour - gmtHour,
                        lcl.tm_min - gmt.tm_min);
        if (ret < buflen) {
            return PASS;
        }
    }
    return FAIL;
}


void
printFilenameHeader(char * prefix, char * filename)
{
    char timestr[] = "9999-12-31 12:59:59.123456789 +9999";
    memset(timestr, '\0', sizeof(timestr));

    MSG("%s ", prefix);
    if (needQuote(filename)) {
        MSG("\"%s\"", filename);
    } else {
        MSG("%s", filename);
    }
    if (formatFileModifiedTime(timestr, sizeof(timestr), filename)) {
        MSG("\t%s", timestr);
    }
    MSG("\n");
}


void
showUnifiedDiff ( )
{
    uint ia = 1;
    uint bega = 0;
    uint runa = 1;
    uint iza;

    uint ib = 1;
    uint begb = 0;
    uint runb = 1;
    uint izb;

    FILE * fpa = NULL;
    FILE * fpb = NULL;

    if (isFile(afile.fname)) {
        fpa = fopen(afile.fname, "rb");
        if (!fpa) {
            return;
        }
    }

    if (isFile(bfile.fname)) {
        fpb = fopen(bfile.fname, "rb");
        if (!fpb) {
            fclose(fpa);
            return;
        }
    }

    printFilenameHeader("---", afile.fname);
    printFilenameHeader("+++", bfile.fname);

    getRunLen(&afile, &ia, &bega, &runa, &iza);
    getRunLen(&bfile, &ib, &begb, &runb, &izb);

    while (runa > 0 || runb > 0) {
        if (iza  && !izb) {
            printUnifiedHeader(bega, runa, begb, 0);
            printLines(fpa, &afile, bega, runa, UNIFIED_DEL);
            getRunLen(&afile, &ia, &bega, &runa, &iza);
        } else if (!iza && izb) {
            printUnifiedHeader(bega, 0, begb, runb);
            printLines(fpb, &bfile, begb, runb, UNIFIED_ADD);
            getRunLen(&bfile, &ib, &begb, &runb, &izb);
        } else {
            if (iza && izb) {
                printUnifiedHeader(bega, runa, begb, runb);
                printLines(fpa, &afile, bega, runa, UNIFIED_DEL);
                printLines(fpb, &bfile, begb, runb, UNIFIED_ADD);
            }
            getRunLen(&afile, &ia, &bega, &runa, &iza);
            getRunLen(&bfile, &ib, &begb, &runb, &izb);
        }
    };

    if (fpa) {
        fclose(fpa);
    }
    if (fpb) {
        fclose(fpb);
    }
}


#ifdef __CONTEXT__
void
printContextHeader(uint bega, uint runa, uint begb, uint runb)
{
    if (begb == 1 && runb == 0) {
        if (bega > 0 && runa == 0) {
            bega--;
        }
        MSG("*** ");
        MSG("%u", bega);
        if (runa > 1) {
            MSG(",%u", bega+runa-1);
        }
        MSG(" ****");
    } else if (bega == 1 && runa == 0) {
        if (begb > 0 && runb == 0) {
            begb--;
        }
        MSG("--- ");
        MSG("%u", begb);
        if (runb > 1) {
            MSG(",%u", begb+runb-1);
        }
        MSG(" ----");
    }
    MSG("\n");
}


void
printContextSep()
{
    MSG("***************\n");
}


void
showContextDiff ( )
{
    uint ia = 1;
    uint bega = 0;
    uint runa = 1;
    uint iza;

    uint ib = 1;
    uint begb = 0;
    uint runb = 1;
    uint izb;

    FILE * fpa = NULL;
    FILE * fpb = NULL;

    if (isFile(afile.fname)) {
        fpa = fopen(afile.fname, "rb");
        if (!fpa) {
            return;
        }
    }

    if (isFile(bfile.fname)) {
        fpb = fopen(bfile.fname, "rb");
        if (!fpb) {
            fclose(fpa);
            return;
        }
    }

    printFilenameHeader("***", afile.fname);
    printFilenameHeader("---", bfile.fname);

    getRunLen(&afile, &ia, &bega, &runa, &iza);
    getRunLen(&bfile, &ib, &begb, &runb, &izb);

    while (runa > 0 || runb > 0) {
        if (iza  && !izb) {
            printContextSep();
            printContextHeader(bega, runa, 1, 0);
            printLines(fpa, &afile, bega, runa, CONTEXT_DEL);
            printContextHeader(1, 0, begb, 0);
            getRunLen(&afile, &ia, &bega, &runa, &iza);
        } else if (!iza && izb) {
            printContextSep();
            printContextHeader(bega, 0, 1, 0);
            printContextHeader(1, 0, begb, runb);
            printLines(fpb, &bfile, begb, runb, CONTEXT_ADD);
            getRunLen(&bfile, &ib, &begb, &runb, &izb);
        } else {
            if (iza && izb) {
                printContextSep();
                printContextHeader(bega, runa, 1, 0);
                printLines(fpa, &afile, bega, runa, CONTEXT_MOD);
                printContextHeader(1, 0, begb, runb);
                printLines(fpb, &bfile, begb, runb, CONTEXT_MOD);
            }
            getRunLen(&afile, &ia, &bega, &runa, &iza);
            getRunLen(&bfile, &ib, &begb, &runb, &izb);
        }
    };

    if (fpa) {
        fclose(fpa);
    }
    if (fpb) {
        fclose(fpb);
    }
}
#endif


void
showBrief()
{
    if (isBothSame()) {
        MSG("%sFiles %s and %s differ\n", BRIEF_MOD, afile.fname, bfile.fname);
    } else {
        if (opt.gloss) {
            MSG("%sFiles %s and %s are identical\n", BRIEF_EQUAL, afile.fname, bfile.fname);
        }
    }
}


char *
_dirname(char * path, uint basename_len)
{
#if defined(__DOS__)
    // The 'dirname()' function provided by the Watcom C library doesn't work
    // as desired (see clib.pdf), so we have to implement it.
    uint path_len = strlen(path);
    if (path_len > basename_len) {
        char * p = path;
        uint dirname_len = path_len - basename_len - 1;
        while (dirname_len != 0) {
            p++;
            dirname_len--;
        }
        *p = '\0';
    }
    return path;
#else
    return dirname(path);
#endif
}


void
onlyIn(const char * gloss, const char * path)
{
    char * str1;
    char * str2;
    if (( str1 = strdup(path) )) {
        if (( str2 = strdup(path) )) {
            char * bname = basename(str2);
            uint bname_len = strlen(bname);
            MSG("%sOnly in %s: %s\n", gloss, _dirname(str1, bname_len), bname);
            free(str2);
        }
        free(str1);
    }
}


int
diffFiles(char * fname1, char * fname2, int sc)
{
    if (opt.brief || !opt.empty) {
        int fa = isFile(fname1);
        int fb = isFile(fname2);

        if (fa && !fb) {
            onlyIn(BRIEF_LEFT, fname1);
            return PASS;
        }
        if (!fa && fb) {
            onlyIn(BRIEF_RIGHT, fname2);
            return PASS;
        }
    }


    if (makeDoc(fname1, &afile)) {
        popLines(&afile);
        if (makeDoc(fname2, &bfile)) {
            popLines(&bfile);
            dodiff();
            if (opt.data) {
              showDoc(&afile);
              showDoc(&bfile);
            }
            if (opt.brief) {
                showBrief();
            } else {

                if (isDirectory(cmdline[opt.indexF1]) && isDirectory(cmdline[opt.indexF2]) ) {
                    uint v = isBothSame();
                    if ( v == FILES_DIFFER) {
                        showCmd(afile.fname, bfile.fname);
                    } else {
                        return v;
                    }
                }

                if (opt.unified) {
                    showUnifiedDiff();
#ifdef __CONTEXT__
                } else if (opt.context) {
                    showContextDiff();
#endif
                } else if (opt.normal) {
                    showNormalDiff();
                } else if (opt.twocol) {
                    showTwoCol();
                }
            }
            return isBothSame();
        }
    }
    return FILES_ERROR;
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


int
parseColWidth(char * str)
{
    if (str) {
        return atoi(str); // Returns 0 on error.
    }
    return FAIL;
}

int
setAltChars(char * str)
{
    if (str) {
        if (*str) { // <
            NORMAL_DEL[0] = *str;
            BRIEF_LEFT[0] = *str;
            TWOCOL_LEFT[0] = *str;
            str++;
        }
        if (*str) { // >
            NORMAL_ADD[0] = *str;
            BRIEF_RIGHT[0] = *str;
            TWOCOL_RIGHT[0] = *str;
            str++;
        }
        if (*str) { // +
            //CONTEXT_ADD[0] = *str;
            UNIFIED_ADD[0] = *str;
            str++;
        }
        if (*str) { // -
            //CONTEXT_DEL[0] = *str;
            UNIFIED_DEL[0] = *str;
            str++;
        }
        if (*str) { // =
            BRIEF_EQUAL[0] = *str;
            TWOCOL_EQUAL[0] = *str;
            str++;
        }
        if (*str) { // %
            //CONTEXT_MOD[0] = *str;
            BRIEF_MOD[0] = *str;
            TWOCOL_MOD[0] = *str;
            str++;
        }
    }
    return 1;
}

int
isNameExists(char * path)
{
    return (access(path, F_OK) == 0);
}



void
showVersion(char * progname)
{
    MSG("%7s v%s\n", basename(progname), VERSION);
}


void
showHelp( char * progname )
{
    // Specify %2s to make fmod simpler.
    // Unix uses '--'; DOS uses '/'.
    MSG("Usage: %7s [options] NAME1 NAME2\n", basename(progname));
    MSG("\n");
    MSG("  %sn, %2snormal     Output Normal format (default)\n",     OPT_PREFIX1, OPT_PREFIX2);
#ifdef __CONTEXT__
    MSG("  %sc, %2scontext    Output Context format\n",              OPT_PREFIX1, OPT_PREFIX2);
#endif
    MSG("  %su, %2sunified    Output Unified format\n",              OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sb, %2sbrief      Report only if NAMEs differ or not\n", OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %se, %2sempty      Treat absent files as empty\n",        OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sr, %2srecurse    Recurse subdirectories\n",             OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %st, %2stwocol     Show two-column output\n",             OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sw, %2swidth N    Column width of two-column output\n",  OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sg, %2sgloss      Prefix line with char indicating change\n", OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %ss, %2sspace      Ignore space and tab differences\n",   OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sl, %2slineend    Ignore line ending\n",                 OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sa, %2salt CHARS  Set alternate chars\n",                OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sq, %2squiet      Suppress normal output\n",             OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sh, %2shelp       Show this help\n",                     OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sv, %2sversion    Show version\n",                       OPT_PREFIX1, OPT_PREFIX2);
    MSG("  %sd, %2sdata       Show data structure (for developer)\n",OPT_PREFIX1, OPT_PREFIX2);
    MSG("\n");
    MSG("NAME1 and NAME2 must either both be files or both be directories.\n");
    MSG("CHARS is 1-6 chars which replace corresponding chars in the string '<>+-=%%' and\n");
    MSG("may need to be quoted to prevent shell interpretation or if spaces are used.\n");
    MSG("Exit status is 0 if inputs are the same, 1 if different, 2 if trouble.\n");
}


void
initOpts()
{
    opt.altchar  = 0;
    opt.brief    = 0;
#ifdef __CONTEXT__
    opt.context  = 0;
#endif
    opt.data     = 0;
    opt.empty    = 0;
    opt.gloss    = 0;
    opt.help     = 0;
    opt.indexF1  = 0;
    opt.indexF2  = 0;
    opt.lineend  = 0;
    opt.normal   = 0;
    opt.quiet    = 0;
    opt.space    = 0;
    opt.twocol   = 0;
    opt.unified  = 0;
    opt.version  = 0;
    opt.width    = 0;
}


void
parseArgs(int argc, char** argv )
{
    char * progname = *argv;
    uint n = 1;

    while (n < argc) {
        if      (0) {}
        else if (isArg(argv[n], 'a', "alt"    )) { opt.altchar = setAltChars(argv[++n]); }
        else if (isArg(argv[n], 'b', "brief"  )) { opt.brief    = 1; }
#ifdef __CONTEXT__
        else if (isArg(argv[n], 'c', "context")) { opt.context  = 1; }
#endif
        else if (isArg(argv[n], 'd', "data"   )) { opt.data     = 1; }
        else if (isArg(argv[n], 'e', "empty"  )) { opt.empty    = 1; }
        else if (isArg(argv[n], 'g', "gloss"  )) { opt.gloss    = 1; }
        else if (isArg(argv[n], 'h', "help"   )) { opt.help     = 1; }
        else if (isArg(argv[n], 'l', "lineend")) { opt.lineend  = 1; }
        else if (isArg(argv[n], 'n', "normal" )) { opt.normal   = 1; }
        else if (isArg(argv[n], 'q', "quiet"  )) { opt.quiet    = 1; }
        else if (isArg(argv[n], 'r', "recurse")) { opt.recurse  = 1; }
        else if (isArg(argv[n], 's', "space"  )) { opt.space    = 1; }
        else if (isArg(argv[n], 't', "twocol" )) { opt.twocol   = 1; }
        else if (isArg(argv[n], 'u', "unified")) { opt.unified  = 1; }
        else if (isArg(argv[n], 'v', "version")) { opt.version  = 1; }
        else if (isArg(argv[n], 'w', "width"  )) { opt.width = parseColWidth(argv[++n]); }
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

    if (opt.indexF1 == 0) {
       ERRMSG("Missing NAME1\n");
       exit(EXIT_ERR);
    }

    if (opt.indexF2 == 0) {
       ERRMSG("Missing NAME2\n");
       exit(EXIT_ERR);
    }

    if (!isNameExists(argv[opt.indexF1]) && !opt.empty) {
        ERRMSG("No such file or directory: %s\n", argv[opt.indexF1]);
        exit(EXIT_ERR);
    }
    
    if (!isNameExists(argv[opt.indexF2]) && !opt.empty) {
        ERRMSG("No such file or directory: %s\n", argv[opt.indexF2]);
        exit(EXIT_ERR);
    }
    
    if (isFile(argv[opt.indexF1])) {
        if (!isFileReadable(argv[opt.indexF1])) {
            ERRMSG("Permission denied: %s\n", argv[opt.indexF1]);
            exit(EXIT_ERR);
        }
    }
    
    if (isFile(argv[opt.indexF2])) {
        if (!isFileReadable(argv[opt.indexF2])) {
            ERRMSG("Permission denied: %s\n", argv[opt.indexF2]);
            exit(EXIT_ERR);
        }
    }

    if (isDirectory(argv[opt.indexF1]) && (!isDirectory(argv[opt.indexF2]))) {
        ERRMSG("Expected %s to be a directory\n", argv[opt.indexF2]);
        exit(EXIT_ERR);
    } else if (isFile(argv[opt.indexF1]) && (!isFile(argv[opt.indexF2])) && !opt.empty) {
        ERRMSG("Expected %s to be a file\n", argv[opt.indexF2]);
        exit(EXIT_ERR);
    }


    if (opt.recurse && (isFile(argv[opt.indexF1]) || isFile(argv[opt.indexF2]))) {
        ERRMSG("Recurse only valid with directories\n");
        exit(EXIT_ERR);
    }

#ifdef __CONTEXT__
    if (!opt.unified && !opt.context && !opt.twocol) {
        opt.normal = 1;
    }
#else
    if (!opt.unified && !opt.twocol) {
        opt.normal = 1;
    }
#endif

    if (!opt.gloss) {
        BRIEF_EQUAL[0] = '\0';
        BRIEF_LEFT[0]  = '\0';
        BRIEF_MOD[0]   = '\0';
        BRIEF_RIGHT[0] = '\0';

        TWOCOL_EQUAL[0] = '\0';
        TWOCOL_LEFT[0]  = '\0';
        TWOCOL_MOD[0]   = '\0';
        TWOCOL_RIGHT[0] = '\0';
    }
}


int
appendLeaf(char * branch, char * leaf)
{
    uint pos = strlen(branch);
    // Subtract 1 for '/' and 1 for null.
    if (pos <= MAX_PATH_LEN - strlen(leaf) - 2) {
        if (pos > 0) {
            branch[pos] = DIRSEP;
            pos++;
        }
        while (*leaf) {
            branch[pos] = *leaf;
            leaf++;
            pos++;
        }
        branch[pos] = '\0';
        return PASS;
    }
    return FAIL;
}


int
trimLeaf(char * branch)
{
    uint pos = strlen(branch);
    if (pos > 0) {
        while (pos > 0) {
            pos--;
            if (branch[pos] == DIRSEP) {
                break;
            }
        }
        branch[pos] = '\0';
        return PASS;
    }
    return FAIL;
}


int
isUsableEntry(struct dirent * ent)
{
    if (ent->d_name) {
#if defined(__DOS__)
        if (! (ent->d_attr & (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_VOLID ) )) {
            if ( (strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0) ) {
                return PASS;
            }
        }
#else
        if (ent->d_name[0] != '.') {
            return PASS;
        }
#endif
    }
    return FAIL;
}


int
diffDirs_helper(char * patha, char * pathb, int depth)
{
    DIR * dir;
    int ret = FAIL;
    struct dirent * ent;

    if (opt.brief || !opt.empty) {
        int da = isDirectory(patha);
        int db = isDirectory(pathb);

        if (da && !db) {
            onlyIn(BRIEF_LEFT, patha);
        }
        if (!da && db) {
            onlyIn(BRIEF_RIGHT, pathb);
        }
        if (da && db && depth > 0 && !opt.recurse) {
            MSG("%sCommon subdirectories: %s and %s\n", BRIEF_EQUAL, patha, pathb);
        }
        if ((da || db) && !opt.recurse && depth > 0) {
            return PASS;
        }
    }

    if (!isDirectory(pathb) && !opt.empty) {
        return ret;
    }

    dir = opendir(patha);
    if (dir) {
        while ((ent = readdir (dir)) != NULL) {
            if (isUsableEntry(ent)) {
                if (appendLeaf(patha, ent->d_name)) {
                    if (appendLeaf(pathb, ent->d_name)) {
                        //if (isDirectory(patha) && recurse) {
                        if (isDirectory(patha)) {
                            ret = diffDirs_helper(patha, pathb, depth+1);
                        } else if (isFile(patha)) {
                            ret = diffFiles(patha, pathb, 1);
                        }

                        trimLeaf(pathb);
                    }
                    trimLeaf(patha);
                }
            }
        }
        closedir(dir);
    }

    dir = opendir(pathb);
    if (dir) {
        while ((ent = readdir (dir)) != NULL) {
            if (isUsableEntry(ent)) {
                if (appendLeaf(pathb, ent->d_name)) {
                    if (appendLeaf(patha, ent->d_name)) {
                        if (isDirectory(pathb) && !isDirectory(patha)) {
                            ret = diffDirs_helper(patha, pathb, depth+1);
                        } else if (isFile(pathb) && !isFile(patha)) {
                            ret = diffFiles(patha, pathb, 1);
                        }
                        trimLeaf(patha);
                    }
                    trimLeaf(pathb);
                }
            }
        }
        closedir(dir);
    }
    return ret;
}


int
diffDirs(char * patha, char * dir1, char * pathb, char * dir2)
{
    int ret = FILES_DIFFER; // Init with differ condition.
    if (appendLeaf(patha, dir1)) {
        if (appendLeaf(pathb, dir2)) {
            ret = diffDirs_helper(patha, pathb, 0);
            trimLeaf(pathb);
        }
        trimLeaf(patha);
    }
    return ret;
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
diffPaths(char * filename1, char * filename2)
{
    if (isDirectory(filename1) && isDirectory(filename2)) {
        trimSlash(filename1);
        trimSlash(filename2);
        return diffDirs(&PATHA[0], filename1, &PATHB[0], filename2);
    } else if (isFile(filename1) || isFile(filename2)) {
        return diffFiles(filename1, filename2, 0);
    } else {
        ERRMSG("Only files or directories can be compared.\n");
        return FAIL;
    }
}


int
main ( int argc, char** argv )
{
    uint retval;
    cmdline = argv;

    atexit(cleanup);
    
    initOpts();
    
    parseArgs(argc, argv);

    retval = diffPaths(argv[opt.indexF1], argv[opt.indexF2]);
    
    cleanup();
    
    return retval;
}
