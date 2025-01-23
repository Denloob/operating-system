#include "parser.h"
#include "smartptr.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <dirent.h>

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

#define CWD_MAX_LEN 512
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

        int fail = execve_new(cmd->shell_command[0], &cmd->shell_command[0]);
        if (fail)
        {
            printf("Unknown command: '%s'\n", cmd->shell_command[0]);
        }
        else
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
