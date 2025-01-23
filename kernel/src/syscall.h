#pragma once

typedef enum {
    SYSCALL_READ    = 0,
    SYSCALL_WRITE   = 1,
    SYSCALL_OPEN    = 2,

    SYSCALL_BRK     = 12,

    SYSCALL_EXECUTE = 59,
    SYSCALL_EXIT    = 60,

    SYSCALL_GETCWD  = 79,
    SYSCALL_CHDIR   = 80,

    SYSCALL_REBOOT  = 169,

    SYSCALL_GETDENTS = 217,

    SYSCALL_WAITPID = 1001,

} syscall_Number;

typedef enum {
    SYSCALL_REBOOT_HALT = 0xcdef0123,
    SYSCALL_REBOOT_POWER_OFF = 0x4321fedc,
    SYSCALL_REBOOT_RESTART = 0x1234567,

    SYSCALL_REBOOT_MAGIC1 = 0x306cf987f239b786,
    SYSCALL_REBOOT_MAGIC2 = 0xee85b32bebeeb1f1,
} syscall_RebootCode;

typedef enum {
    SYSCALL_OPEN_FLAGS_READ_ONLY   = 0x0,
    SYSCALL_OPEN_FLAGS_WRITE_ONLY  = 0x1,
    SYSCALL_OPEN_FLAGS_READ_WRITE  = 0x2,
    SYSCALL_OPEN_FLAGS_CREATE =  0x40,
    SYSCALL_OPEN_FLAGS_TRUNC  = 0x200,
    SYSCALL_OPEN_FLAGS_APPEND = 0x400,
} syscall_OpenFlags;

/**
 * @brief Initialize eveyrhting for the syscall instruction to work.
 */
void syscall_initialize();
