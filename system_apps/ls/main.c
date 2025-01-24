#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <dirent.h>

#define TOLOWER(c) (c | ('a' - 'A'))

static void lower_inplace(char *buf)
{
    while (*buf != '\0')
    {
        if (*buf >= 'A' && *buf <= 'Z')
        {
            *buf = TOLOWER(*buf);
        }
        buf++;
    }
}

int main(int argc, char **argv)
{ 
#define CWD_MAX_LEN 512
    char cwd[CWD_MAX_LEN] = {0};
    const char *path;
    if (argc > 1)
    {
        path = argv[1];
    }
    else
    {
        getcwd(cwd, sizeof(cwd));
        path = cwd;
    }

    fat16_dirent dir_entries[16];
    long x = getdents(path, dir_entries, 16);
    for (int i = 0; i < x; i++)
    {
        lower_inplace(dir_entries[i].name);

        printf("Entry %d: Name: %s, Attr: %d, Cluster: %d, Size: %d , MDS-FLAGS: %d\n",
               i,
               dir_entries[i].name,
               dir_entries[i].attr,
               dir_entries[i].cluster,
               dir_entries[i].size ,
               dir_entries[i].mdscore_flags);
    }
    printf("returned value %lx" , x);
    putchar('\n');

    return 0;
}
