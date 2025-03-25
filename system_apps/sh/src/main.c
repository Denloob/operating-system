#include "parser.h"
#include "smartptr.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/syscall.h>

int cd(ShellCommand *cmd)
{
    if (cmd->length != 2)
    {
        puts("cd: Invalid amount of args for cd command");
        return 1;
    }

    int ret = chdir(cmd->shell_command[1]);
    if (ret == -1)
    {
        printf("cd: %s: No such directory\n", cmd->shell_command[1]);
        return 1;
    }

    return 0;
}

#define MAX_JOB_COUNT 1024

pid_t g_jobs[MAX_JOB_COUNT];
int g_jobs_cursor;

void jobs_init()
{
    for (int i = 0; i < MAX_JOB_COUNT; i++)
    {
        g_jobs[i] = -1;
    }
}

void jobs_refresh()
{
    for (int i = 0; i < MAX_JOB_COUNT; i++)
    {
        const pid_t pid = g_jobs[i];
        if (pid == -1)
            continue;

        int wstatus;
        const pid_t res = waitpid(pid, &wstatus, WNOHANG);
        if (res == pid || res == -1)
        {
            g_jobs[i] = -1;
        }
    }
}

__attribute__((warn_unused_result))
bool jobs_insert(pid_t pid)
{
    assert(pid != -1 && g_jobs_cursor < MAX_JOB_COUNT);

    int starting_cursor_pos = g_jobs_cursor;
    while (g_jobs[g_jobs_cursor] != -1)
    {
        g_jobs_cursor++;
        g_jobs_cursor %= MAX_JOB_COUNT;

        if (g_jobs_cursor == starting_cursor_pos)
        {
            return false;
        }
    }

    g_jobs[g_jobs_cursor] = pid;
    return true;
}

#define MAX_PATH_LENGTH 512

typedef struct {
    int8_t return_code;
    bool execution_successful;
} ExecutionResult;

ExecutionResult execute_expression(char *expr, size_t expr_length, char *path)
{
    ShellCommand *cmd = parser_parse_shell_expression(expr, expr_length);
    if (cmd == NULL)
    {
        return (ExecutionResult){.execution_successful = false};
    }
    defer({ parser_release_parsed_expression(cmd); });

    if (cmd->length == 0)
    {
        return (ExecutionResult){.execution_successful = false};
    }

    // Special case for built-in `cd` command
    if (strcmp(cmd->shell_command[0], "cd") == 0)
    {
        int ret = cd(cmd);
        return (ExecutionResult){.execution_successful = true, .return_code = ret};
    }

    pid_t pid = -1;
    bool is_binary_path = strchr(cmd->shell_command[0], '/') != NULL;
    if (is_binary_path)
    {
        pid = execve_new(cmd->shell_command[0], &cmd->shell_command[0]);
    }
    else
    {
        char *original_command = cmd->shell_command[0];
        defer({ free(original_command); });
        cmd->shell_command[0] = malloc(MAX_PATH_LENGTH);
        if (cmd->shell_command[0] == NULL)
        {
            puts("Ran out of memory trying to run the command");

            // defer will cleanup `cmd` and original_command. It's ok to free NULL.
            return (ExecutionResult){.execution_successful = false};
        }

        size_t path_idx = 0;
        do
        {
            if (path[path_idx] == '\0')
            {
                break;
            }

            char *path_end = strchrnul(&path[path_idx], ':');

            size_t path_end_idx = path_end - path;
            size_t path_len = path_end_idx - path_idx;
            size_t path_begin = path_idx;

            if (path[path_end_idx] == '\0')
            {
                path_idx = path_end_idx;
            }
            else
            {
                path_idx = path_end_idx + 1;
            }

            if (path_len == 0)
            {
                continue; // Skip
            }

            if (path_len + 1 >= MAX_PATH_LENGTH)
            {
                puts("Path too long, aborting");
                break;
            }

            memmove(cmd->shell_command[0], &path[path_begin], path_len);
            cmd->shell_command[0][path_len] = '/';
            cmd->shell_command[0][path_len + 1] = '\0';
            strncat(cmd->shell_command[0], original_command, MAX_PATH_LENGTH);
            pid = execve_new(cmd->shell_command[0], &cmd->shell_command[0]);
        }
        while (pid == -1);
    }

    if (pid == -1)
    {
        printf("Unknown command: '%s'\n", cmd->shell_command[0]);
        return (ExecutionResult){.execution_successful = false};
    }

    if (cmd->is_async)
    {
        jobs_refresh();
        bool success = jobs_insert(pid);
        if (!success)
        {
            puts("shcore: warning: jobs list is full, this process will be lost");
        }
        return (ExecutionResult){.execution_successful = true, .return_code = 0};
    }

    int wstatus;
    pid_t child_pid = waitpid(pid, &wstatus, 0);
    if (child_pid == -1)
    {
        printf("waitpid: something went wrong\n");
        return (ExecutionResult){.execution_successful = true, .return_code = -1};
    }

    assert(WIFEXITED(wstatus));
    int return_code = WEXITSTATUS(wstatus);
    return (ExecutionResult){.execution_successful = true, .return_code = return_code};
}

int main(int argc, char **argv)
{
#define INITIAL_PATH_SIZE MAX_PATH_LENGTH
    char *path = calloc(1, INITIAL_PATH_SIZE);
    strcpy(path, "/bin");

    if (argc > 1)
    {
        if (strcmp(argv[1], "-c"))
        {
            printf("shcore: %s: invalid options\n", argv[1]);
            return 1;
        }

        if (argc < 2)
        {
            printf("shcore: %s: option requires an argument\n", argv[1]);
            return 1;
        }

        ExecutionResult res = execute_expression(argv[2], strlen(argv[2]), path);
        if (res.execution_successful)
        {
            return res.return_code;
        }

        return INT8_MAX;
    }

    int8_t return_code = 0;

    jobs_init();

    syscall(SYS_CreateWindow , 0);

#define CWD_MAX_LEN MAX_PATH_LENGTH
    char cwd[CWD_MAX_LEN] = {0};
    while (true)
    {
        jobs_refresh();

        getcwd(cwd, sizeof(cwd));

        if (return_code == 0)
        {
            printf("shcore %s$ ", cwd);
        }
        else
        {
            printf("shcore %s [%d]$ ", cwd, return_code);
        }

        char buf[1024] = {0};
        size_t input_len = fread(buf, 1, sizeof(buf), stdin);
        assert(input_len > 0);
        if (buf[input_len - 1] != '\n')
        {
            puts("\nshcore: command too long");
            continue;
        }
        buf[input_len - 1] = '\0';
        input_len--;

        ExecutionResult res = execute_expression(buf, input_len, path);
        if (res.execution_successful)
        {
            return_code = res.return_code;
        }
    }
}
