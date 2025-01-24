#pragma once

#include "fs.h"
#include "res.h"

#define FS_MAX_FILEPATH_LEN 512
typedef struct {
    char parent[FS_MAX_FILEPATH_LEN];
    char child[FS_MAX_FILEPATH_LEN];
    bool must_be_dir; // true for a path like `/foo/mydir/`. Can be false for dirs too! Like `/foo/mydir`.
} parsing_FilePathParsingResult;

#define res_parsing_path_NOT_AN_ABSOLUTE_PATH "The given path must be absolute"
#define res_parsing_path_PATH_DOESNT_CONTAIN_DIR_NAME "The given path doesn't contain a file or directory name"
#define res_parsing_path_TOO_LONG "The given path is too long"

/**
 * @brief Parses the given null terminated `path` into the parent/child dir.
 *          That is, `/foo/bar/baz` will parse into parent=`/foo/bar`, child=`baz`.
 *          out->must_be_dir is set if the path ends in a slash, like in `/mydir/`
 */
res parsing_parse_filepath(const char *path, parsing_FilePathParsingResult *out);

void test_parsing_filepath();
