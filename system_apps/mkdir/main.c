#include <sys/stat.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        puts("mkdir: missing operand");
    }

    for (int i = 1; i < argc; i++)
    {
        int ret = mkdir(argv[i]);
        if (ret == -1)
        {
            printf("mkdir: %s: Couldn't create directory\n", argv[i]);
        }
    }

    return 0;
}
