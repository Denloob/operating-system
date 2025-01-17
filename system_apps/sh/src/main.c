#include "parser.h"
#include "smartptr.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

int main()
{
    while (true)
    {
        fputs("shcore $ ", stdout);

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

        execve_new(cmd->shell_command[0], &cmd->shell_command[0]);
    }
}
