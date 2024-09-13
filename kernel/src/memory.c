#include "memory.h"

#define PAGE_SIZE 4096  // 4 KB
#define MEMORY_SIZE (1024 * 1024 * 128)  //128 MB
#define PAGES (MEMORY_SIZE / PAGE_SIZE)
#define BITMAP_SIZE (MEMORY_SIZE / PAGE_SIZE / 8)

unsigned char memory_bitmap[BITMAP_SIZE];

void *memmove(void *dest, const void *src, int len)
{
  char *d = dest;
  const char *s = src;
  if (d < s)
    while (len--)
      *d++ = *s++;
  else
    {
      char *lasts = s + (len-1);
      char *lastd = d + (len-1);
      while (len--)
        *lastd-- = *lasts--;
    }
  return dest;
}

void *memset(void *dest, int val, int len)
{
  unsigned char *ptr = dest;
  while (len-- > 0)
    *ptr++ = val;
  return dest;
}

void init_memory()
{
    memset(memory_bitmap,0,BITMAP_SIZE);
}
void set_bit(int bit)
{
    memory_bitmap[bit / 8] |= (1 << (bit % 8));
}

void clear_bit(int bit)
{
    memory_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

int test_bit(int bit)
{
    return memory_bitmap[bit / 8] & (1 << (bit % 8));
}

void *kernel_malloc(int size)
{
    if (size <= 0) return 0;

   int num_pages = (size + PAGES- 1) / PAGES;  

   int start_page = -1;
   int free_pages = 0;

    for (int i = 0; i < PAGES; i++) 
    {
        if (!test_bit(i)) 
        { 
            if (start_page == -1) 
            {
                start_page = i;  
            }
            free_pages++;

            if (free_pages == num_pages)
            {
               for (int j = start_page; j < start_page + num_pages; j++)
               {
                    set_bit(j); 
               }
                return (void *)(start_page * PAGE_SIZE);
            }
        }
        else
        {
            start_page = -1;
            free_pages = 0;
        }
    }

    // No block was found
    return 0;
}

void kfree(void *ptr , int size)
{
    if (!ptr || size <= 0) return;

    int start_page = (int)ptr / PAGE_SIZE;
    int num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE; 

    for (int i = start_page; i < start_page + num_pages; i++) 
    {
        clear_bit(i);  
    }
}

