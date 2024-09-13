#pragma once

void *memmove(void *dest, const void *src, int len);

void *memset(void *dest, int val, int len);

void init_memory();

void *kernel_malloc(int size);

void kfree(void *pointer);

void init_paging();

void handle_page_fault();
