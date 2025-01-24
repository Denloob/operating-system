#pragma once

#include "res.h"
#include "assert.h"
#include <stdbool.h>

typedef struct {
    char parent[0];
    char child[0];
    bool must_be_dir;
} parsing_FilePathParsingResult;

static inline 
res parsing_parse_filepath(const char *path, parsing_FilePathParsingResult *out)
{
    assert(false && "Unimplemented");
}
