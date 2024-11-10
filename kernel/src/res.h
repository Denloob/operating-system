#pragma once

#include <stddef.h>

#define IS_ERR(code) ((code) != res_OK)
#define IS_OK(code) ((code) == res_OK)

typedef const char *res;
#define res_OK (res)NULL
