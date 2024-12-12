#include "test_filesystem.h"
#include "fs.h"
#include "assert.h"
#include "memory.h"
#include "FAT16.h"
#include "file.h"

static void test_file_creation_and_writing()
{
    // Create a new file using fat16_create_file
    const char *filename = "testfile.txt";
    fat16_File file1;
    bool success = fat16_create_file_with_return(&file1 , &g_fs_fat16, filename);
    assert(success && "fat16_create_file");
    printf("File '%s' created successfully\n", filename);

    // Prepare the buffer with data to write
    const uint64_t buffer_size = 5;  // Example size
    uint8_t buffer_to_write[] = "Hello, FAT16! This is a simple test.";  // Declare text directly
    FILE file;
    file.file = file1;
    file.offset = 0;
    // Write data to the file using fwrite
   
    uint64_t bytes_written = fwrite(buffer_to_write, 1, buffer_size, &file);
    assert(bytes_written == buffer_size && "fwrite didn't write the expected number of bytes");//STOPS HERE


    // Seek to the beginning and read the data back
    fseek(&file, 0, SEEK_SET);
    uint8_t read_buffer[512];
    filename = (const char *)file.file.file_entry.filename;
    fat16_get_file_chain(file.file.ref->drive , &file.file.ref->bpb, filename, file.file.chain);
    size_t bytes_read = fread(read_buffer, 1, 5, &file);
    assert(bytes_read == buffer_size && "fread didn't read the expected number of bytes");

    // Verify that the data read is the same as what was written
    assert(memcmp(buffer_to_write, read_buffer, buffer_size) == 0 && "Data mismatch between write and read");

    // Seek to a specific position and check file position with ftell
    fseek(&file, 5, SEEK_SET);
    uint64_t file_pos = ftell(&file);
    assert(file_pos == 5 && "ftell position mismatch");
}

void test_filesystem()
{
    test_file_creation_and_writing();
}