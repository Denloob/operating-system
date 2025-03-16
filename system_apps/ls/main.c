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

#define fat16_MDSCore_FLAGS_MASK 0xe7
typedef enum {
    fat16_MDSCoreFlags_FILE   = 0, // A regular fat16 file.
    fat16_MDSCoreFlags_DEVICE = 1, /* This is not a file, it's a device.
                                    * For devices, `firstClusterHigh` is the major device number, and
                                    * `firstClusterLow` is the minor device number.
                                    * The major number is used to identify the driver responsible for the device.
                                    * The minor number is used to identify the part of the driver.
                                    */
} fat16_MDSCoreFlags;
#define fat16_DIRENTRY_ATTR_IS_DIRECTORY 0x10

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
    long entry_count = getdents(path, dir_entries, 16);
    if (entry_count < -1)
    {
        printf("ls: cannot access '%s'\n", path);
        return 1;
    }

    putchar('\n');
    for (int i = 0; i < entry_count; i++)
    {
        lower_inplace(dir_entries[i].name);

        printf("%s    ", dir_entries[i].name);

        if ((dir_entries[i].mdscore_flags & fat16_MDSCore_FLAGS_MASK) == fat16_MDSCoreFlags_DEVICE)
        {
            fputs("<dev>", stdout);
        }
        else if ((dir_entries[i].attr & fat16_DIRENTRY_ATTR_IS_DIRECTORY) != 0)
        {
            fputs("<DIR>", stdout);
        }
        putchar('\n');
    }
    printf("        %ld files\n\n", entry_count);

    return 0;
}
