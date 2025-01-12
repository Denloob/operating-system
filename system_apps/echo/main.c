#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/syscall.h>

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            fputs(argv[i], stdout);
            putchar(' ');
        }
    }

    putchar('\n');

    return 0;
}
