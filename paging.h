#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_ENTRIES 1024
#define PAGE_SIZE 4096
#define MAX_FRAMES 4096
#define KERNEL_BASE 0xc0000000                  //the beginning of new Kernel architecture in VM!
uint32_t *create_process_address_space(void);
extern uint32_t page_directory[PAGE_ENTRIES];
extern uint32_t first_page_table[PAGE_ENTRIES];
extern uint32_t second_page_table[PAGE_ENTRIES];  //added now  for second page table for day 36
extern uint32_t next_free_frame;
extern uint8_t frame_bitmap[MAX_FRAMES];
uint32_t alloc_frame(void);
void free_frame(uint32_t phys);


void paging_init();
void paging_enable();
void map_page(uint32_t virt,uint32_t phys);
void* phys_to_virt(uint32_t phys);
void _finalize_higher_half(void);
void mark_frame_used(uint32_t phys);

#endif