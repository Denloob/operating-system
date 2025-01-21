#pragma once

#include "usermode.h"

uint64_t waitpid(uint64_t pid, usermode_mem *wstatus, int options);
