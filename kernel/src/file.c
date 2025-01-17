#include "file.h"
#include "assert.h"
#include "char_device.h"

static size_t internal_fread(void *ptr, size_t size, size_t count, FILE *stream, bool block)
{
    size_t bytes_read = 0;
    switch (fat16_get_mdscore_flags(&stream->file))
    {
        case fat16_MDSCoreFlags_FILE:
            bytes_read = fat16_read(&stream->file, ptr, size * count, stream->offset);
            break;
        case fat16_MDSCoreFlags_DEVICE:
            bytes_read = char_device_read(&stream->file, ptr, size * count, stream->offset, block);
            break;
    }
    stream->offset += bytes_read;

    return bytes_read / size;
}

size_t fread(void *ptr, size_t size, size_t count, FILE *stream)
{
    return internal_fread(ptr, size, count, stream, false);
}

size_t process_fread(void *ptr, size_t size, size_t count, FILE *stream)
{
    return internal_fread(ptr, size, count, stream, true);
}

static size_t internal_fwrite(void *ptr, size_t size, size_t count, FILE *stream, bool block)
{
    size_t bytes_written = 0;
    switch (fat16_get_mdscore_flags(&stream->file))
    {
        case fat16_MDSCoreFlags_FILE:
            bytes_written = fat16_write(&stream->file, ptr , size * count, stream->offset);
            break;
        case fat16_MDSCoreFlags_DEVICE:
            bytes_written = char_device_write(&stream->file, ptr, size * count, stream->offset, block);
            break;
    }
    stream->offset += bytes_written;

    return bytes_written / size;
}

size_t fwrite(void *ptr, size_t size, size_t count, FILE *stream)
{
    return internal_fwrite(ptr, size, count, stream, false);
}

size_t process_fwrite(void *ptr, size_t size, size_t count, FILE *stream)
{
    return internal_fwrite(ptr, size, count, stream, true);
}

int fseek(FILE *stream, int64_t offset, file_Whence whence)
{
    // TODO: fseek for chr device
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
