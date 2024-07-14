#include "io.h"

#define assert(expr)                                                           \
    do                                                                         \
    {                                                                          \
        if (!(expr))                                                           \
        {                                                                      \
            puts(#expr);                                                       \
            asm("hlt");                                                        \
        }                                                                      \
    } while (0)
