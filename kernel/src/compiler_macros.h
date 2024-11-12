#pragma once

/* Compiler specific macros for the code */

#define WUR __attribute__((warn_unused_result))

#if (!defined(__GNUC__) || defined(__clang__))
    #define attribute__kmalloc __attribute__((malloc))
#else
    #define attribute__kmalloc __attribute__((malloc(kfree)))
#endif
