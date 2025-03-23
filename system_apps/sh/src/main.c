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
#include <dirent.h>
#include <sys/syscall.h>

void cd(ShellCommand *cmd)
{
    if (cmd->length != 2)
    {
        puts("cd: Invalid amount of args for cd command");
        return;
    }

    int ret = chdir(cmd->shell_command[1]);
    if (ret == -1)
    {
        printf("cd: %s: No such directory\n", cmd->shell_command[1]);
    }
}

int main()
{
    int8_t return_code = 0;

    syscall(SYS_CreateWindow , 0);
#define MAX_PATH_LENGTH 512

#define INITIAL_PATH_SIZE MAX_PATH_LENGTH
    char *path = calloc(1, INITIAL_PATH_SIZE);
    strcpy(path, "/bin");

#define CWD_MAX_LEN MAX_PATH_LENGTH
    char cwd[CWD_MAX_LEN] = {0};
    while (true)
    {
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

        ShellCommand *cmd = parser_parse_shell_expression(buf, input_len);
        if (cmd == NULL)
        {
            continue;
        }
        defer({ parser_release_parsed_expression(cmd); });

        if (cmd->length == 0)
        {
            continue;
        }

        // Special case for built-in `cd` command
        if (strcmp(cmd->shell_command[0], "cd") == 0)
        {
            cd(cmd);
            continue;
        }

        int fail = -1;
        bool is_binary_path = strchr(cmd->shell_command[0], '/') != NULL;
        if (is_binary_path)
        {
            fail = execve_new(cmd->shell_command[0], &cmd->shell_command[0]);
        }
        else
        {
            char *original_command = cmd->shell_command[0];
            defer({ free(original_command); });
            cmd->shell_command[0] = malloc(MAX_PATH_LENGTH);
            if (cmd->shell_command[0] == NULL)
            {
                puts("Ran out of memory trying to run the command");
                continue; // defer will cleanup `cmd` and original_command. It's ok to free NULL.
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
                fail = execve_new(cmd->shell_command[0], &cmd->shell_command[0]);
            }
            while (fail == -1);
        }

        if (fail)
        {
            printf("Unknown command: '%s'\n", cmd->shell_command[0]);
        }
        else if (!cmd->is_async)
        {
            int wstatus;
            pid_t child_pid = waitpid(-1, &wstatus, 0);
            if (child_pid == -1)
            {
                printf("waitpid: something went wrong\n");
            }
            else
            {
                assert(WIFEXITED(wstatus));
                return_code = WEXITSTATUS(wstatus);
            }
        }
    }
}
