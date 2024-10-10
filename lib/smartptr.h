#pragma once

//
// Header-only library, adding Go-like defer and auto-free to C (gnu99)
// Licensed under MIT License
// Copyright (c) 2024 Denloob
//
// The full license can be found in the end of this file
//

// If you do not wish to have a macro `smartptr` that calls free
// (for example if you don't have libc, remove the following define)
#define smartptr__setting_SUPPORT_FREE

// If you have a problem with the `defer` symbol being defined here, remove,
// remove the following define
#define smartptr__setting_SUPPORT_DEFER

// If you wish, you may enable for clang or any other non-gnu compiler to completely
// ignore most* smartptr/defer statements. This can be useful for example if you
// are using it as an LSP (like clangd).
// * - non-special features like smartptr_func_ptr or smartptr will still work
#if (!defined(__GNUC__) || defined(__clang__))

#if 1
#define smartptr__setting_DO_NOTHING_SPECIAL
#else
#error "Unsupported compiler"
#endif

#endif

#ifdef smartptr__setting_SUPPORT_FREE
#include <stdlib.h>
#endif

#define smartptr__DEFER_CONCAT_HELPER(x, y) x##y
#define smartptr__DEFER_CONCAT(x, y) smartptr__DEFER_CONCAT_HELPER(x, y)
#define smartptr__DEFER_MAKE_UNIQUE(prefix)                                     \
    smartptr__DEFER_CONCAT(prefix, __COUNTER__)

#ifdef smartptr__setting_DO_NOTHING_SPECIAL
#define smartptr__custom_helper(cleanup_block, cleanup_name)
#else
#define smartptr__custom_helper(cleanup_block, cleanup_name)                    \
    void cleanup_name(void *ptr) cleanup_block                                 \
        __attribute__((cleanup(cleanup_name)))
#endif

#define smartptr_block(cleanup_block)                                           \
    smartptr__custom_helper(cleanup_block,                                      \
                           smartptr__DEFER_MAKE_UNIQUE(smartptr__cleanup_func))

#define smartptr_func(func) smartptr_block({ func(*(void **)ptr); })
#define smartptr_func_ptr(func) __attribute__((cleanup(func)))

#ifdef smartptr__setting_SUPPORT_FREE
static void smartptr__free(void *ptr)
{
    free(*(void **)ptr);
}
#define smartptr __attribute__((cleanup(smartptr__free)))
#endif

#ifdef smartptr__setting_SUPPORT_DEFER

#ifdef smartptr__setting_DO_NOTHING_SPECIAL

#define defer(block)

#else

/*
 * @brief Runs the block at the end of the variable scope.
 *
 * @example defer({ puts("Hi"); });
 *
 * @example
 *      char *msg = "Hi";
 *      defer({ puts(msg); });
 *
 * Defers run LIFO style, so
 * defer({ puts("This will run last"); });
 * defer({ puts("This will run first"); });
 */
#define defer(block) smartptr_block(block) char smartptr__DEFER_MAKE_UNIQUE(smartptr__defer_var)


#endif // smartptr__setting_DO_NOTHING_SPECIAL

#endif // smartptr__setting_SUPPORT_DEFER

/*

This file is licensed under the MIT License.

MIT License

Copyright (c) 2024 Denloob

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
