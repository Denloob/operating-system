#pragma once

__attribute__((noreturn))
void assert_fail(const char *assertion, const char *file,
                 const char *line, const char *function);

#define __assert_STRINGIFY_HELPER(x) #x
#define __assert_STR(x) __assert_STRINGIFY_HELPER(x)

#define assert(expr)                                                                \
    (expr) ? (void)0                                                                \
           : assert_fail(#expr, __FILE__, __assert_STR(__LINE__), __PRETTY_FUNCTION__)
