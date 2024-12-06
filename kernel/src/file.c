#include "file.h"
#include "assert.h"

size_t fread(void *ptr, size_t size, size_t count, FILE *stream)
{
    size_t bytes_read = fat16_read(&stream->file, ptr, size * count, stream->offset);
    stream->offset += bytes_read;

    return bytes_read / size;
}

size_t fwrite(void *ptr, size_t size, size_t count, FILE *stream)
{
    size_t bytes_written = fat16_write(&stream->file, ptr , size * count, stream->offset);
    stream->offset += bytes_written;

    return bytes_written / size;
}

int fseek(FILE *stream, int64_t offset, file_Whence whence)
{
    uint64_t new_offset;
    switch (whence)
    {
        case SEEK_END:
        {
            const uint64_t filesize = stream->file.file_entry.fileSize;
            if (__builtin_add_overflow(filesize, offset, &new_offset))
            {
                return -1;
            }
            break;
        }
        case SEEK_CUR:
            if (__builtin_add_overflow(stream->offset, offset, &new_offset))
            {
                return -1;
            }
            break;
        case SEEK_SET:
            if (offset < 0)
            {
                return -1;
            }
            new_offset = offset;
            break;
        default:
            return -1;
    }

    stream->offset = new_offset;
    return 0;
}

long ftell(FILE *stream)
{
    return stream->offset;
}
