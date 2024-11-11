#include "assert.h"
#include "io.h"

void assert_fail(const char *assertion, const char *file, const char *line,
                 const char *function)
{
    put(file);
    putc(':');
    put(line);
    put(": ");
    put(function);
    put(": Assertion failed: ");
    puts(assertion);
    cli_hlt();
}
