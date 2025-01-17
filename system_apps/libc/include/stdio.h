#pragma once

#include <stddef.h>

#include "printf.h"

typedef struct _IO_FILE FILE;

/* Standard streams.  */
extern FILE *stdin;         /* Standard input stream.  */
extern FILE *stdout;        /* Standard output stream.  */
extern FILE *stderr;        /* Standard error output stream.  */
/* C89/C99 say they're macros.  Make them happy.  */
#define stdin stdin
#define stdout stdout
#define stderr stderr

#define EOF -1

int fputc(int c, FILE *stream);
int putc(int c, FILE *stream);
int putchar(int c);

FILE *fopen(const char *restrict path);
int fclose(FILE *stream);

size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream);
size_t fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream);

int fputs(const char *restrict s, FILE *restrict stream);
int puts(const char *s);
