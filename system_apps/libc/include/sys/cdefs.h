#pragma once


#define _WUR __attribute__((warn_unused_result))

#if (!defined(__GNUC__) || defined(__clang__))
    #define __attribute_malloc __attribute__((malloc))
#else
    #define __attribute_malloc __attribute__((malloc(free)))
#endif
