#include "assert.h"
#include "io.h"
#include "vga.h"

void assert_fail(const char *assertion, const char *file, const char *line,
                 const char *function)
{
    if (vga_current_mode() != VGA_MODE_TYPE_TEXT)
    {
        vga_restore_default_color_palette();
        vga_mode_text();
        io_clear_vga();
    }

    put(file);
    putc(':');
    put(line);
    put(": ");
    put(function);
    put(": Assertion failed: ");
    puts(assertion);
    cli_hlt();
}
