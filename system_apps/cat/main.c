#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <dirent.h>

int main(int argc, char **argv)
{ 
    FILE *fp = stdin;

    if (argc > 1)
    {
        fp = fopen(argv[1], "r");
        if (fp == NULL)
        {
            printf("cat: %s: No such file", argv[1]);
            return 1;
        }

    }

    assert(fp != NULL);

    size_t bytes_read = 0;
    char buf[1024];
    while ((bytes_read = fread(buf, 1, sizeof(buf), fp)) > 0)
    {
        size_t bytes_written = fwrite(buf, 1, bytes_read, stdout);
        if (bytes_written != bytes_read)
        {
            puts("cat: Output failure");
            return 1;
        }
    }

    fclose(fp);

    return 0;
}
