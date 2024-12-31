#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct _IO_FILE {
    char path[0];
};

FILE *stdin;
FILE *stdout;
FILE *stderr;

int fputc(int c, FILE *stream)
{
    if (fwrite(&c, 1, 1, stream) == 1)
    {
        return (unsigned char)c;
    }

    return EOF;
}

int putc(int c, FILE *stream)
{
    return fputc(c, stream);
}

int putchar(int c)
{
    return fputc(c, stdout);
}

int fputs(const char *restrict s, FILE *restrict stream)
{
    size_t len = strlen(s);
    if (fwrite(s, 1, len, stream) == len)
    {
        return 0;
    }

    return EOF;
}

int puts(const char *s)
{
    if (fputs(s, stdout) == EOF)
    {
        return EOF;
    }
    if (fputc('\n', stdout) == EOF)
    {
        return EOF;
    }

    return 0;
}

FILE *fopen(const char *restrict path)
{
    FILE *fp = malloc(strlen(path) + 1);
    strcpy(fp->path, path);

    return fp;
}

int fclose(FILE *stream)
{
    free(stream);
    return 0;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream)
{
    return write(stream->path, ptr, size * nmemb) / size;
}

size_t fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream)
{
    return read(stream->path, ptr, size * nmemb) / size;
}
