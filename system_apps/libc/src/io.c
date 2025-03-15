#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

struct _IO_FILE {
    int fd;
};

FILE *stdin;
FILE *stdout;
FILE *stderr;

#include <assert.h>
// assert needed for printf.inc
#include "printf.inc"

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

enum {
    FILE_MODE_READ_ONLY  = 0x1,
    FILE_MODE_WRITE_ONLY = 0x2,
    FILE_MODE_APPEND     = 0x4,
    FILE_MODE_PLUS       = 0x8, // To apply to any of the other 3, bitwise OR it.
};

__attribute__((const))
static int state_without_plus(int state)
{
    return state & (~FILE_MODE_PLUS);
}

__attribute__((pure))
static int parse_mode_string_to_flags(const char *mode)
{
    int state = 0;
    for (const char *it = mode; *it; it++)
    {
        if (*it == '+')
        {
            if (state & FILE_MODE_PLUS)
            {
                return -1;
            }

            state |= FILE_MODE_PLUS;

            continue;
        }

        if (state_without_plus(state) != 0)
            return -1;

        switch (*it)
        {
            case 'w':
                state |= FILE_MODE_WRITE_ONLY;
                break;
            case 'a':
                state |= FILE_MODE_APPEND;
                break;
            case 'r':
                state |= FILE_MODE_READ_ONLY;
                break;
            default:
                return -1;
        }
    }

    int flags = 0;
    switch (state_without_plus(state))
    {
        case FILE_MODE_READ_ONLY:
            flags = O_RDONLY;
            break;
        case FILE_MODE_WRITE_ONLY:
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            break;
        case FILE_MODE_APPEND:
            flags = O_WRONLY | O_CREAT | O_APPEND;
            break;
        default:
            assert(false && "Unreachable");
    }

    if (state & FILE_MODE_PLUS)
    {
        flags &= ~(O_RDONLY | O_WRONLY);
        flags |= O_RDWR;
    }

    return flags;
}

FILE *fopen(const char *restrict path, const char *restrict mode)
{
    int flags = parse_mode_string_to_flags(mode);
    if (flags == -1)
    {
        return NULL;
    }

    int fd = open(path, flags);
    if (fd == -1)
    {
        return NULL;
    }

    FILE *fp = malloc(strlen(path) + 1);
    if (fp == NULL)
    {
        return NULL;
    }

    fp->fd = fd;
    return fp;
}

int fclose(FILE *stream)
{
    free(stream);
    return 0;
}

int fseek(FILE *stream, long offset, int whence)
{
    int64_t pos = lseek(stream->fd, offset, whence);
    if (pos < 0)
        return -1;
    return 0;
}

long ftell(FILE *stream)
{
    int64_t pos = lseek(stream->fd, 0, SEEK_CUR);
    if (pos < 0)
        return -1;

    return pos;
}

void rewind(FILE *stream)
{
    int res = fseek(stream, 0, SEEK_SET);
    (void)res; // Unused
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream)
{
    ssize_t bytes_written = write(stream->fd, ptr, size * nmemb);
    if (bytes_written < 0)
    {
        return 0;
    }

    return bytes_written / size;
}

size_t fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream)
{
    ssize_t bytes_read = read(stream->fd, ptr, size * nmemb);
    if (bytes_read < 0)
    {
        return 0;
    }

    return bytes_read / size;
}
