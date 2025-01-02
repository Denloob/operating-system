#include "parser.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parser_release_parsed_expression(ShellCommand *cmd)
{
    assert(cmd != NULL);

    if (cmd->shell_command != NULL)
    {
        for (size_t i = 0; i < cmd->length; i++)
        {
            free(cmd->shell_command[i]);
            cmd->shell_command[i] = NULL;
        }

        free(cmd->shell_command);
    }

    cmd->shell_command = NULL;
    free(cmd);
}

ShellCommand *parser_parse_shell_expression(const char *expression, size_t length)
{
    ShellCommand *cmd = calloc(1, sizeof(*cmd));
    if (cmd == NULL)
        return NULL;

    bool in_quotes = false;
    bool escaped = false;

    size_t cmd_capacity = 1;
    cmd->shell_command = malloc(sizeof(char *));
    if (cmd->shell_command == NULL)
        goto fail_oom;

    char **cur_token_ptr = NULL;
    size_t cur_token_capacity = 0;
    size_t cur_token_length = 0;
    bool currently_parsing_token = false;
    for (const char *it = expression, *end = it + length; it != end; it++)
    {
        if (cur_token_ptr == NULL)
        {
            if (cmd->length == cmd_capacity)
            {
                cmd_capacity *= 2;

                void *new_buf = realloc(cmd->shell_command, cmd_capacity * sizeof(char *));
                if (new_buf == NULL)
                    goto fail_oom;

                cmd->shell_command = new_buf;
            }

            cur_token_ptr = &cmd->shell_command[cmd->length];
            cmd->length++;

            cur_token_length = 0;
            cur_token_capacity = 1;
            currently_parsing_token = false;
            *cur_token_ptr = malloc(1);
            assert(!in_quotes && !escaped && "Unreachable state");
            if (*cur_token_ptr == NULL)
                goto fail_oom;
        }

        if (cur_token_length == cur_token_capacity)
        {
            cur_token_capacity *= 2;

            void *new_buf = realloc(*cur_token_ptr, cur_token_capacity);
            if (new_buf == NULL)
                goto fail_oom;

            *cur_token_ptr = new_buf;
        }

        if (escaped)
        {
            assert(currently_parsing_token && "Unreachable state");
            (*cur_token_ptr)[cur_token_length] = *it;
            cur_token_length++;
            escaped = false;
            continue;
        }

        if (*it == '\\')
        {
            currently_parsing_token = true;
            escaped = true;
            continue;
        }

        if (in_quotes)
        {
            assert(currently_parsing_token && "Unreachable state");
            if (*it == '\'')
            {
                in_quotes = false;
            }
            else
            {
                (*cur_token_ptr)[cur_token_length] = *it;
                cur_token_length++;
            }

            continue;
        }

        if (*it == ' ')
        {
            if (!currently_parsing_token)
            {
                continue; // Skip spaces
            }

            (*cur_token_ptr)[cur_token_length] = '\0';
            cur_token_ptr = NULL;
            currently_parsing_token = false;
            continue;
        }

        currently_parsing_token = true;

        switch (*it)
        {
            case '\'':
                in_quotes = true;
                continue;
            default:
                (*cur_token_ptr)[cur_token_length] = *it;
                cur_token_length++;
                continue;
        }

        assert(false && "Unreachable");
    }

    if (escaped || in_quotes)
    {
        puts("shcore: invalid syntax");
        goto fail;
    }

    if (cur_token_ptr != NULL)
    {
        if (!currently_parsing_token)
        {
            free(cmd->shell_command[cmd->length - 1]);
            cmd->length--;
        }
        else
        {
            void *new_buf = realloc(*cur_token_ptr, cur_token_length + 1); // Make sure there's enough space for a null byte.
            if (new_buf == NULL)
                goto fail_oom;

            *cur_token_ptr = new_buf;
            (*cur_token_ptr)[cur_token_length] = '\0';
        }
    }

    return cmd;

fail_oom:
    puts("shcore: out of memory");
fail:
    parser_release_parsed_expression(cmd);
    return NULL;
}
