#pragma once

typedef enum {
    SYSCALL_READ,
    SYSCALL_WRITE,
} syscall_Number;

/**
 * @brief Initialize eveyrhting for the syscall instruction to work.
 */
void syscall_initialize();
