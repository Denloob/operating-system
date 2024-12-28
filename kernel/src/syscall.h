#pragma once

typedef enum {
    SYSCALL_READ    = 0,
    SYSCALL_WRITE   = 1,

    SYSCALL_BRK     = 12,

    SYSCALL_EXECUTE = 59,
} syscall_Number;

/**
 * @brief Initialize eveyrhting for the syscall instruction to work.
 */
void syscall_initialize();
