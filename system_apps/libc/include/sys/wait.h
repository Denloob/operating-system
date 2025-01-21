#pragma once

#include <sys/types.h>

#define WIFEXITED(wstatus) 1 // MDS CORE waitpid returns only if process exited.
#define WEXITSTATUS(wstatus) wstatus // MDS CORE only stores the return code

#define WNOHANG (1 << 0)

pid_t waitpid(pid_t pid, int *wstatus, int options);
