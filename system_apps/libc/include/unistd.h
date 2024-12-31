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

ssize_t read(const char *path, void *buf, size_t count);
ssize_t write(const char *path, const void *buf, size_t count);

void *sbrk(intptr_t increment);
int brk(void *addr);
