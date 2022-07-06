#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include <sys/types.h>
#include "parseLog.h"

static FILE *stream;

void shm_log_init(char *path)
{
    if ((stream = fopen(path, "w+")) == NULL) {
        printf("Cannot open output file.\n");
        return;
    }
}

void write_log_file(char *buf)
{
    long      length;
    char      filename[256];
    time_t    now;
    struct tm *curTime;

    if (stream) {
        fwrite(buf, strlen(buf), 1, stream);
        fflush(stream);
    }
}

void printUage(void)
{
    printf("Usage:\n");
    printf(" -a\t set baseOffset for elf file.\n");
    printf(" -b\t set bin file input.\n");
    printf(" -e\t set elf file input.\n");
    printf(" -o\t set output file.\n");
}




int main(int argc, char **argv)
{
    int          status = 0, i = 0;
    int          ch;
    int          flag        = REG_EXTENDED;
    regmatch_t   pmatch[1];
    const size_t nmatch      = 1;
    regex_t      reg;
    const char   *pattern    = "^0x[0-9a-fA-F]{1,8}$";
    unsigned int baseOffset  = 0x0;
    char         binFile[64] = {0};
    char         elfFile[64] = {0};
    char         outFile[64] = {0};
    uint8_t      paramsFlag  = 0;

    while ((ch = getopt(argc, argv, "e:b:o:a:")) != -1) {
        switch (ch) {
            case 'a':
                regcomp(&reg, pattern, flag);
                status = regexec(&reg, optarg, nmatch, pmatch, 0);
                if (status == REG_NOMATCH) {
                    printf("Error string : %s\n", optarg);
                } else if (status == 0) {
                    baseOffset  = strtoul(optarg, NULL, 16);
                    printf("set baseOffset : 0x%x\n", baseOffset);
                    paramsFlag |= 0x01;
                }
                regfree(&reg);
                break;
            case 'b':
                snprintf(binFile, sizeof(binFile), "%s", optarg);
                printf("set binFile : %s\n", binFile);
                paramsFlag |= 0x02;
                break;
            case 'e':
                snprintf(elfFile, sizeof(elfFile), "%s", optarg);
                printf("set elfFile : %s\n", elfFile);
                paramsFlag |= 0x04;
                break;
            case 'o':
                snprintf(outFile, sizeof(outFile), "%s", optarg);
                printf("set outFile : %s\n", outFile);
                paramsFlag |= 0x08;
                break;
            default:
                printf("unknow option :%c\n", ch);
        }
    }
    if ((paramsFlag & 0x0e) != 0x0e) {
        printUage();
        return -1;
    }
    shm_log_init(outFile);
    parse_elf(elfFile, binFile, baseOffset);
  
    return 0;
}

