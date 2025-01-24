#include "test_filesystem.h"
#include "fs.h"
#include "assert.h"
#include "io.h"
#include "memory.h"
#include "FAT16.h"
#include "file.h"

static void test_file_creation_and_writing()
{
    // Create a new file using fat16_create_file
    const char *filename = "/testfile.txt";
    fat16_File file1;
    bool success = fat16_create_file( &g_fs_fat16, filename, &file1,NULL);
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
    fat16_get_file_chain(file.file.ref, &file.file.file_entry, file.file.chain);
    size_t bytes_read = fread(read_buffer, 1, 5, &file);
    assert(bytes_read == buffer_size && "fread didn't read the expected number of bytes");

    // Verify that the data read is the same as what was written
    assert(memcmp(buffer_to_write, read_buffer, buffer_size) == 0 && "Data mismatch between write and read");

    // Seek to a specific position and check file position with ftell
    fseek(&file, 5, SEEK_SET);
    uint64_t file_pos = ftell(&file);
    assert(file_pos == 5 && "ftell position mismatch");

    // Multi-sector writing + allocation
    uint8_t long_buffer[0x1000] = {0};
    const int check_index = 513;
    const int check_value = 42;
    long_buffer[check_index] = check_value; 

    fseek(&file, 0, SEEK_SET);
    bytes_written = fwrite(long_buffer, 1, sizeof(long_buffer), &file);
    assert(bytes_written == sizeof(long_buffer) && "fwrite didn't write the expected number of bytes");//STOPS HERE

    fseek(&file, check_index, SEEK_SET);
    uint8_t result;
    bytes_read = fread(&result, 1, 1, &file);
    assert(bytes_read == sizeof(result) && "fread couldn't read the expected byte");
    assert(result == check_value && "fread didn't get the expected value");
}

static void test_getdents()
{
    fat16_dirent dir_entries[16]; // Buffer for up to 16 entries
    int max_entries = 16;

    int count = fat16_getdents(0, dir_entries, max_entries ,&g_fs_fat16);
    printf("Number of entries read: %d\n", count);

    assert(count > 0 && "No directory entries found");

    for (int i = 0; i < count; i++) {
        printf("Entry %d: Name: %s, Attr: %d, Cluster: %d, Size: %d , MDS-FLAGS: %d\n",
               i,
               dir_entries[i].name,
               dir_entries[i].attr,
               dir_entries[i].cluster,
               dir_entries[i].size ,
               dir_entries[i].mdscore_flags);
    }

    printf("test_getdents passed successfully.\n");
}
void test_filesystem()
{
    test_file_creation_and_writing();
    test_getdents();
    printf("All tests passed successfully press any key to continue");
    io_wait_key_raw();
}
