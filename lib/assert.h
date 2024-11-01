#include "io.h"

#define assert(expr)                                                           \
    do                                                                         \
    {                                                                          \
        if (!(expr))                                                           \
        {                                                                      \
            puts(#expr);                                                       \
            asm("cli; hlt");                                                   \
        }                                                                      \
    } while (0)
