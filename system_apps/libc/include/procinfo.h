#include <stdint.h>
#include <sys/types.h>

#define PROCESS_NAME_MAX_LEN 64

typedef struct {
    uint64_t pid;
    char cwd[256];
    char name[PROCESS_NAME_MAX_LEN];
    int state;
} ProcessInfo;
