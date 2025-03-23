#include "fs.h"
#include "mmu.h"
#include "program.h"
#include "res.h"
#include <stdbool.h>
#include <stdint.h>

static uint64_t id = 1;

res execve(const char *path, const char *const *argv, PCB *parent, uint64_t *out_pid)
{
    const char *empty_argv[] = {path, NULL};
    if (argv == NULL)
    {
        // program_setup_from_drive deep-copies the `argv` onto the stack,
        //  so it's safe to pass it values with a possibly-lower lifetime.
        argv = empty_argv;
    }

    res rs = program_setup_from_drive(id, parent, g_pml4, &g_fs_fat16, path, (char **)argv); // const-cast here is safe. @see - program_setup_from_drive
    if (!IS_OK(rs))
    {
        return rs;
    }

    if (out_pid) *out_pid = id;
    id++;

    return res_OK;
}
