/* Flite cst_file_stdio.c: cst_file is a stdio FILE*. */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cst_file.h"
#include "cst_alloc.h"

/* 0x51fe4 */
cst_file cst_fopen(const char *path, int mode)
{
    char cmode[4];

    /* This is kind of hacky. */
    if ((mode & CST_OPEN_WRITE) && (mode & CST_OPEN_READ))
        strcpy(cmode, "r+");
    else if ((mode & CST_OPEN_APPEND) && (mode & CST_OPEN_READ))
        strcpy(cmode, "a+");
    else if (mode & CST_OPEN_WRITE)
        strcpy(cmode, "w");
    else if (mode & CST_OPEN_APPEND)
        strcpy(cmode, "a");
    else if (mode & CST_OPEN_READ)
        strcpy(cmode, "r");

    if (mode & CST_OPEN_BINARY)
        strcat(cmode, "b");

    return fopen(path, cmode);
}

/* 0x520b4 */
long cst_fwrite(cst_file fh, const void *buf, long size, long count)
{
    return fwrite(buf, size, count, fh);
}

/* 0x520cc */
long cst_fread(cst_file fh, void *buf, long size, long count)
{
    return fread(buf, size, count, fh);
}

/* 0x520e4 */
long cst_filesize(cst_file fh)
{
    long pos, epos;

    pos = ftell(fh);
    fseek(fh, 0, SEEK_END);
    epos = ftell(fh);
    fseek(fh, pos, SEEK_SET);
    return epos;
}

/* 0x52128 */
int cst_fgetc(cst_file fh)
{
    return fgetc(fh);
}

/* 0x5212c */
long cst_ftell(cst_file fh)
{
    return ftell(fh);
}

/* 0x52130 */
long cst_fseek(cst_file fh, long pos, int whence)
{
    int w = 0;

    if (whence == CST_SEEK_ABSOLUTE)
        w = SEEK_SET;
    else if (whence == CST_SEEK_RELATIVE)
        w = SEEK_CUR;
    else if (whence == CST_SEEK_ENDREL)
        w = SEEK_END;
    return fseek(fh, pos, w);
}

/* 0x52150 */
int cst_fprintf(cst_file fh, const char *fmt, ...)
{
    va_list args;
    int rv;

    va_start(args, fmt);
    rv = vfprintf(fh, fmt, args);
    va_end(args);
    return rv;
}

/* 0x52180 */
int cst_sprintf(char *s, const char *fmt, ...)
{
    va_list args;
    int rv;

    va_start(args, fmt);
    rv = vsprintf(s, fmt, args);
    va_end(args);
    return rv;
}

/* 0x521b0 */
int cst_fclose(cst_file fh)
{
    return fclose(fh);
}
