#ifndef HEAP_H
#define HEAP_H
#include <stdint.h>
void heap_init(void);
void *kmalloc_page(void);
void kfree(void *ptr);
void heap_dump(void);
#endif

typedef struct heap_block
{ // THE METADATA BLOCKER i.e WHAT'S INSIDE HEADER BLOCK!
    uint32_t size;
    uint8_t is_free;
    struct heap_block *next;
} heap_block_t;               // for closing the definition of heap_block
void *kmalloc(uint32_t size); // The size of the trailing payload that is requested by the kernel :AVOIDS FATAL COMPILATION ERROR: