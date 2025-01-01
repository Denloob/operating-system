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
            puts("\nCommand too long");
            continue;
        }

        buf[input_len - 1] = '\0';

        fputs("Executing ", stdout);
        puts(buf);
    }

    while (true) { /* hang */ };
}
