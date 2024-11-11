#pragma once

#include "macro_utils.h"
#include "io.h"

__attribute__((noreturn))
void assert_fail(const char *assertion, const char *file,
                 const char *line, const char *function);

#define assert(expr)                                                                \
    (expr) ? (void)0                                                                \
           : assert_fail(#expr, __FILE__, STR(__LINE__), __PRETTY_FUNCTION__)
