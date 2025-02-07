#include <stdint.h>
#include <sys/types.h>

typedef struct
{
    char name[13];
    uint8_t attr;
    uint16_t cluster; // First cluster
    uint32_t size;
    uint8_t mdscore_flags;
}fat16_dirent;

ssize_t getdents(const char *fd, void *dirp, size_t count);
