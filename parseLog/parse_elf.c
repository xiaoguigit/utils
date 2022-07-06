/*
 * Copyright (c) 2019, ZHT Inc. All rights reserved.
 *
 * mr_xiaogui@163.com
 * date: 2019-08-18
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <linux/elf.h>
#include <stdarg.h>
#include <string.h>
#include "parseLog.h"

static char       elf_info[4096];
static char       shstrtab[1024];
static FILE       *fp;
static Elf32_Shdr *rodata = NULL;

#define NONE      "\e[0m"
#define RED       "\e[0;31m"
#define BLUE      "\e[0;34m"
#define YELLOW    "\033[0;33m"
#define GREEN     "\033[0;32m"

void progressbar(int des_progress)
{
    int  i;
    char x[300];

    memset(x, 0, 300);
    char *ptr     = (char *)("|/-\\");
    char buf[101] = {};
    int  des      = 100 / des_progress;

    for (i = 0; i < des_progress + 1; i++) {
        buf[i] = '#';
        sprintf(&x[0], GREEN "[%s][%d%%][%c]\r"NONE, buf, i * des, ptr[i % 4]);
        printf("\r%s", x);
        fflush(stdout);
        usleep(500000);
    }
    printf("\n");
}

void wait_for_log(const char *file, uint32_t baseOffset)
{
    LOG_MSG       data;
    uint32_t      read_offset;
    unsigned long fileSize;
    int           ret;
    int           i;
    char          *p     = (char *)&data;
    FILE          *pFile = fopen(file, "r");

    fseek(pFile, 0, SEEK_END);
    fileSize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    // get write point
    read_offset = 0;

    while (read_offset != fileSize) {
        ret          = fread((char *)&data, sizeof(char), sizeof(LOG_MSG), pFile);
        find_by_addr(&data, baseOffset);
        read_offset += ret;
        printf("\33[2K\r");
        printf("Parsing elf file: %lf%%", (float)read_offset/(float)fileSize * 100.0);
        fflush(stdout);
    }
    printf("\nParse done\n");
}

/**
 * @brief parse_elf() - parse elf form file
 *
 * @param[in] file - path to elf file
 *
 */
int parse_elf(char *file, char *binFile, uint32_t baseOffset)
{
    int        shnum, ret;
    Elf32_Ehdr elf_head;
    Elf32_Shdr *shdr = (Elf32_Shdr *)&elf_info;

    // open file
    fp = fopen(file, "r");
    if (NULL == fp) {
        printf("open elf file fail\n");
        return -1;
    }

    // read elf head
    ret = fread(&elf_head, sizeof(Elf32_Ehdr), 1, fp);
    if (ret == 0) {
        printf("READ ERROR\n");
        return -1;
    }

    // judge if a elf file
    if ((elf_head.e_ident[0] != 0x7F) || (elf_head.e_ident[1] != 'E') || (elf_head.e_ident[2] != 'L') || (elf_head.e_ident[3] != 'F')) {
        printf("Not a ELF format file\n");
        return -1;
    }

    if (shdr == NULL) {
        printf("shdr malloc failed\n");
        return -1;
    }

    // point to shoff offset
    ret = fseek(fp, elf_head.e_shoff, SEEK_SET);
    if (ret != 0) {
        printf("shdr fseek ERROR %d\n", ret);
        return -1;
    }

    // read shdr
    ret = fread(shdr, sizeof(Elf32_Shdr) * elf_head.e_shnum, 1, fp);
    if (ret == 0) {
        printf("READ ERROR, ret = %d\n", ret);
        return -1;
    }

    rewind(fp);
    // point to shstrtab
    ret = fseek(fp, shdr[elf_head.e_shstrndx].sh_offset, SEEK_SET);
    if (ret != 0) {
        printf("shstrtab fseek ERROR\n");
        return -1;
    }

    // read shstrtab
    ret = fread(&shstrtab, shdr[elf_head.e_shstrndx].sh_size, 1, fp);
    if (ret == 0) {
        printf("READ ERROR, cannot parse elf file.\n");
        return -1;
    }

    char *temp = (char *)&shstrtab;
    for (shnum = 0; shnum < elf_head.e_shnum; shnum++) {
        temp = (char *)&shstrtab;
        temp = temp + shdr[shnum].sh_name;
        if (strcmp(temp, ".rodata") == 0) {
            rodata = &shdr[shnum];
        }
    }
    printf("Read elf successful\n");
    wait_for_log(binFile, baseOffset);
    return 0;
}

/**
 * @brief free_elf() - release elf
 *
 */
void free_elf(void)
{
    fclose(fp);
    fp = NULL;
}

/**
 * @brief vspf() - format print
 *
 */
static int vspf(char *fmt, ...)
{
    char    output[1024];
    va_list argptr;
    int     cnt;

    va_start(argptr, fmt);
    cnt = vsnprintf(output, 1024, fmt, argptr);
    va_end(argptr);
    write_log_file(output);
    return cnt;
}

/**
 * @brief findstr_by_addr() - find string in the file by the address which recv from RPU
 *      and then we will put the string to the file as a format show.
 *
 */
void find_by_addr(void *pdata, uint32_t baseOffset)
{
    LOG_MSG      *msg = (LOG_MSG *)pdata;
    unsigned int offset;
    int          ret;
    char         fmt[2048];
    char         format[1024];
    char         tag[32];

    offset  = msg->format - rodata->sh_addr + rodata->sh_offset;
    offset -=  baseOffset;
    ret     = fseek(fp, offset, SEEK_SET);
    if (ret == 0) {
        // get format str
        fgets(format, sizeof(format), fp);
        if (strstr(format, "%s") != NULL) {
            printf("error: We are not support for string \n");
            return;
        }
    } 

    offset  = msg->tag - rodata->sh_addr + rodata->sh_offset;
    offset -=  baseOffset;
    ret     = fseek(fp, offset, SEEK_SET);
    if (ret == 0) {
        // get format str
        fgets(tag, sizeof(tag), fp);
    }

    // add tag for level
    sprintf(fmt, "%s %s", tag, format);

    // output to file
    vspf(fmt, msg->arg[0], msg->arg[1], msg->arg[2], msg->arg[3], msg->arg[4], msg->arg[5], msg->arg[6], msg->arg[7]);
}
