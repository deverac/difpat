#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void main(int argc, char ** argv)
{
    char evName[] = "FITER";
    char * arg2;
    char * filename;
    long num;
    char * numstr = NULL;
    FILE * fp;

    if (argc < 2) {
        printf("Usage: %s FILE NUM\n", *argv);
        printf("Output line number NUM of FILE.\n");
        exit(1);
    }

    argv++;
    filename = *argv++;

    if (argc > 2) {
        if (isdigit(*argv[0])) {
            numstr = *argv;
        } else {
            arg2 = *argv;
        }
    } else {
        arg2 = &evName;
    }

    if (! numstr) {
        if( (numstr = getenv( arg2 )) == NULL ) {
            fprintf(stderr, "Failed to read %s\n", arg2);
            exit(1);
        }
    }

    num = atoi(numstr);

    if (num > 0) {
        fp = fopen(filename, "rb");
        if (fp) {
            int curLine = 1;
            char ch;
            int doOutput = (num == 1);
            int got_cr = 0;
            while (1) {
                ch = fgetc(fp);
                if ( feof(fp) ) {
                    if (doOutput) {
                        if (got_cr) {
                            printf("\n");
                        } else {
                            printf("\r\n");
                        }
                    }
                    break;
                }
                if (doOutput) {
                    printf("%c", ch);
                }
                if (ch == '\r') {
                    got_cr = 1;
                    continue;
                }
                if (got_cr && ch == '\n') {
                    if (doOutput) {
                        break;
                    }
                    curLine++;
                    if (curLine == num) {
                        doOutput = 1;
                    }
                }
                got_cr = 0;
            }
            fclose(fp);
        }
    } else {
        fprintf(stderr, "Failed to convert number\n");
        exit(1);
    }

}
