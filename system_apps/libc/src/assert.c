#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>

void assert_fail(const char *assertion, const char *file, const char *line,
                 const char *function)
{
    fputs(file, stdout);
    putchar(':');
    fputs(line, stdout);
    fputs(": ", stdout);
    fputs(function, stdout);
    fputs(": Assertion failed: ", stdout);
    puts(assertion);

    exit(-1);
}
