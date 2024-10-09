#pragma once

#include <stddef.h>

void *memmove(void *dest, const void *src, int len);    
int memcmp(const void *ptr1, const void *ptr2, size_t num);
void *memset(void *dest, int val, int len);

void init_memory();

void *kernel_malloc(int size);

void kfree(void *pointer , int size);

void init_paging();

void handle_page_fault();

//malloc functions
void set_bit(int bit);
void clear_bit(int bit); 
int test_bit(int bit);

