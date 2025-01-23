#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/**
 * @brief Calls the syscall of the given number with the provided vargs.
 *
 * @see sys/syscall.h
 */
long syscall(long syscall_number, ...);

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

// Similar to execve, but doesn't replace current process.
int execve_new(const char *path, char *const *argv);

void *sbrk(intptr_t increment);
int brk(void *addr);

void exit(int status) __attribute__((noreturn));

// A successful call to reboot does not return.
int reboot(int op); // @see <sys/reboot.h>

char *getcwd(char *buf, size_t size);
int chdir(const char *path);
