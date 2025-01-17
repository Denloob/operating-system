#pragma once

#include "FAT16.h"

typedef size_t (*char_device_ReadWriteFunc)(uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, int minor_number, bool block);

typedef struct char_device_Descriptor {
    int major_number;

    char_device_ReadWriteFunc read;
    char_device_ReadWriteFunc write;

    struct char_device_Descriptor *next;
} char_device_Descriptor;

/**
 * @brief - Write to a character devices described in fat16_File.
 *              Note that we don't actually use FAT16, however
 *              with the lack of proper interfaces, easier to just couple
 *              it in.
 *
 * @return How many bytes written
 */
size_t char_device_write(fat16_File *file, uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, bool block);

/**
 * @brief - Read to a character devices described in fat16_File.
 *              Note that we don't actually use FAT16, however
 *              with the lack of proper interfaces, easier to just couple
 *              it in.
 *
 * @return How many bytes read
 */
size_t char_device_read(fat16_File *file, uint8_t *buffer, uint64_t buffer_size, uint64_t file_offset, bool block);

/**
 * @brief   - Register a character device. The of the descriptor should
 *              not be freed until the device is unregistered.
 * @see     - char_device_unregister
 */
void char_device_register(char_device_Descriptor *desc);

/**
 * @brief   - Unregister a character device with the given major number.
 *              The memory for the descriptor is not released.
 * @see     - char_device_register
 *
 * @return  - true on success, false otherwise. If failed, the given major
 *              number wasn't actually registered.
 */
bool char_device_unregister(int major_number);

