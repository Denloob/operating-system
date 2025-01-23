#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <dirent.h>

int main(int argc, char **argv)
{ 
    const char *path = "/";
    if (argc > 1)
    {
        path = argv[1];
    }

    fat16_dirent dir_entries[16];
    long x = getdents(path, dir_entries, 16);
    for (int i = 0; i < x; i++)
    {
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
