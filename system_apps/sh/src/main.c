#include "parser.h"
#include "smartptr.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>

int main()
{
    int8_t return_code = 0;
    while (true)
    {
        if (return_code == 0)
        {
            fputs("shcore $ ", stdout);
        }
        else
        {
            printf("shcore [%d]$ ", return_code);
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
