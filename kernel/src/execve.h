#pragma once

#include "pcb.h"
#include "res.h"

/**
 * @brief - Execute the given filepath. Aka, add the file in the given path to
 *              the scheduler queue.
 *
 * @param path - Path to the file to execute
 * @param argv - The argv to pass to the executable. It is copied, hence
 *                  it's safe to pass temporary strings, and there's no need
 *                  to store them.
 * @param parent - Parent of the process. Can be NULL.
 * @return res_OK on success, one of the fail codes on failure.
 */
res execve(const void *path, const char *const *argv, PCB *parent);
