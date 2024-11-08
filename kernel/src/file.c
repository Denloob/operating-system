#include "file.h"
#include "assert.h"

size_t fread(void *ptr, size_t size, size_t count, FILE *stream)
{
    size_t bytes_read = fat16_read(&stream->file, ptr, size * count, stream->offset);
    stream->offset += bytes_read;

    return bytes_read;
}

size_t fwrite(void *ptr, size_t size, size_t count, FILE *stream)
{
    assert(false && "Not implemented"); // TODO: not implemented
}

int fseek(FILE *stream, uint64_t offset, file_Whence whence)
{
    switch (whence)
    {
        case SEEK_END:
            stream->offset = stream->file.file_entry.fileSize + offset;
            break;
        case SEEK_CUR:
            stream->offset += offset;
            break;
        case SEEK_SET:
            stream->offset = offset;
            break;
        default:
            return -1;
    }
    return 0;
}

long ftell(FILE *stream)
{
    return stream->offset;
}
