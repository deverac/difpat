#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void main(int argc, char ** argv)
{
    FILE * fp;
    FILE * fp1;
    char * filename;
    unsigned int line;
    unsigned int indent;
    unsigned int len;
    char modchar = ' ';
    unsigned int offset = 0;

    if (argc < 5) {
        printf("Usage: %s FILE LINE COL LEN [CHAR]\n", *argv);
        printf("Overwrite LEN chars in FILE with CHAR, starting at LINE, COL.\n");
        printf("LINE and COL are 1-based. Default CHAR is a space.\n");
        exit(1);
    }

    argv++; // Skip past progname.

    filename = *argv++;
    line = atoi(*argv++);
    indent = atoi(*argv++);
    len = atoi(*argv++);
    if (argc > 5) {
        modchar = *argv[0];
    }

    if (line > 0 && indent > 0 && len > 0) {

        //printf("%s %d %d %d %c\n", filename, line, indent, len, modchar);

        if (line > 1) {
            fp = fopen(filename, "rb");
            if (fp) {
                unsigned int curLine = 1;
                char ch;
                int got_cr = 0;

                while (1) {
                    ch = fgetc(fp);
                    if ( feof(fp) ) {
                        break;
                    }
                    offset++;
                    if (ch == '\r') {
                        got_cr = 1;
                        continue;
                    }
                    if (got_cr && ch == '\n') {
                        curLine++;
                        if (curLine == line) {
                           break;
                        }
                    }
                    got_cr = 0;
                }
                fclose(fp);
            }
        }

        fp1 = fopen(filename, "rb+");
        if (fp1) {
            // indent is 1-based, so subtract 1.
            if (fseek(fp1, offset+indent-1, SEEK_SET) == 0) {
                unsigned int i;
                for (i=0; i<len; i++) {
                    if (fputc(modchar, fp1) != modchar) {
                        break;
                    }
                }
            }
            fclose(fp1);
        }

    } else {
        printf("bad value for line, indent, or length\n");
    }
}
