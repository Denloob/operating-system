#pragma once

typedef enum {
    SYSCALL_READ,
    SYSCALL_WRITE,
    SYSCALL_EXECUTE,
} syscall_Number;

/**
 * @brief Initialize eveyrhting for the syscall instruction to work.
 */
void syscall_initialize();
