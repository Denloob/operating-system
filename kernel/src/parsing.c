#include "parsing.h"
#include "assert.h"
#include "memory.h"
#include "string.h"

res parsing_parse_filepath(const char *path, parsing_FilePathParsingResult *out)
{
    out->must_be_dir = false;
    size_t path_len = strlen(path);
    if (path_len >= FS_MAX_FILEPATH_LEN)
    {
        return res_parsing_path_TOO_LONG;
    }

    if (path[0] != '/')
    {
        return res_parsing_path_NOT_AN_ABSOLUTE_PATH;
    }

    const char *last_slash = memrchr(path, '/', path_len);
    assert(last_slash && "there's always at least one");

    const char *filepath_end = path + path_len - 1;

    const char *child_end = filepath_end;

    // If we got a pointer to a slash like             v
    //                                         /foo/bar/
    if (last_slash == filepath_end || *(last_slash + 1) == '\0')
    {
        out->must_be_dir = true;

        while (last_slash != path && *last_slash == '/')
        {
            last_slash--;
        }

        child_end = last_slash;
        last_slash = memrchr(path, '/', last_slash - path + 1);

        if (last_slash == NULL || child_end == last_slash)
        {
            return res_parsing_path_PATH_DOESNT_CONTAIN_DIR_NAME;
        }
    }

    if (last_slash == path)
    {
        strcpy(out->parent, "/");
    }
    else
    {
        assert(last_slash - path < sizeof(out->parent));
        memmove(out->parent, path, last_slash - path);
        out->parent[last_slash - path] = '\0';
    }

    assert(child_end - last_slash < sizeof(out->child));

    memmove(out->child, last_slash + 1, child_end - last_slash);
    out->child[child_end - last_slash] = '\0';

    return res_OK;
}

// GCC doesn't like that we compare char pointers, but in the case of `res` it's
// perfectly fine.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstring-compare"
void test_parsing_filepath()
{
    parsing_FilePathParsingResult result;

    res rs = parsing_parse_filepath("foo/bar", &result);
    assert((uint64_t)rs == (uint64_t)res_parsing_path_NOT_AN_ABSOLUTE_PATH);

    char too_long[FS_MAX_FILEPATH_LEN + 1];
    memset(too_long, 'a', FS_MAX_FILEPATH_LEN);
    too_long[FS_MAX_FILEPATH_LEN] = '\0';
    rs = parsing_parse_filepath(too_long, &result);
    assert((uint64_t)rs == (uint64_t)res_parsing_path_TOO_LONG);

    rs = parsing_parse_filepath("/", &result);
    assert((uint64_t)rs == (uint64_t)res_parsing_path_PATH_DOESNT_CONTAIN_DIR_NAME);
    rs = parsing_parse_filepath("/////", &result);
    assert((uint64_t)rs == (uint64_t)res_parsing_path_PATH_DOESNT_CONTAIN_DIR_NAME);

    rs = parsing_parse_filepath("/foo", &result);
    assert((uint64_t)rs == (uint64_t)res_OK);
    assert(strcmp(result.parent, "/") == 0);
    assert(strcmp(result.child, "foo") == 0);
    assert(result.must_be_dir == false);

    rs = parsing_parse_filepath("/foo/bar", &result);
    assert((uint64_t)rs == (uint64_t)res_OK);
    assert(strcmp(result.parent, "/foo") == 0);
    assert(strcmp(result.child, "bar") == 0);
    assert(result.must_be_dir == false);

    rs = parsing_parse_filepath("/foo/bar/", &result);
    assert((uint64_t)rs == (uint64_t)res_OK);
    assert(strcmp(result.parent, "/foo") == 0);
    assert(strcmp(result.child, "bar") == 0);
    assert(result.must_be_dir == true);

    rs = parsing_parse_filepath("/file.txt", &result);
    assert((uint64_t)rs == (uint64_t)res_OK);
    assert(strcmp(result.parent, "/") == 0);
    assert(strcmp(result.child, "file.txt") == 0);
    assert(result.must_be_dir == false);

    rs = parsing_parse_filepath("/foo//bar", &result);
    assert((uint64_t)rs == (uint64_t)res_OK);
    assert(strcmp(result.parent, "/foo/") == 0);
    assert(strcmp(result.child, "bar") == 0);
    assert(result.must_be_dir == false);

    rs = parsing_parse_filepath("/a//b/c//", &result);
    assert((uint64_t)rs == (uint64_t)res_OK);
    assert(strcmp(result.parent, "/a//b") == 0);
    assert(strcmp(result.child, "c") == 0);
    assert(result.must_be_dir == true);

    char valid_long[FS_MAX_FILEPATH_LEN];
    memset(valid_long, 'a', FS_MAX_FILEPATH_LEN - 1);
    valid_long[FS_MAX_FILEPATH_LEN - 1] = '\0';
    valid_long[0] = '/';
    rs = parsing_parse_filepath(valid_long, &result);
    assert((uint64_t)rs == (uint64_t)res_OK);
    assert(strcmp(result.parent, "/") == 0);
    assert(strcmp(result.child, valid_long + 1) == 0);
    assert(result.must_be_dir == false);
}
#pragma GCC diagnostic pop
