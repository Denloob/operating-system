#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <sys/cdefs.h>

typedef struct {
    char **shell_command;
    size_t length;

    bool is_async;
} ShellCommand;

/**
 * @brief Release the result of parser_parse_shell_expression
 */
void parser_release_parsed_expression(ShellCommand *cmd);

/**
 * @brief Parse the given `expression` of `length` into
 *          strings, separated by spaces, accounting for quotes and escaped
 *          characters.
 *
 * @return Dynamically allocated ShellCommand. Must be released with parser_release_parsed_expression.
 *          On failure returns NULL.
 * @see parser_release_parsed_expression
 */
ShellCommand *parser_parse_shell_expression(const char *expression, size_t length)
                             __attribute_malloc(parser_release_parsed_expression);
