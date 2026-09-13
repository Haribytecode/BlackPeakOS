#include "heap.h"
#include "paging.h"
#include "console.h"
#define KHEAP_START 0xD0000000
#define PAGE_SIZE 4096
static uint32_t heap_next=KHEAP_START;
static heap_block_t *heap_head=0;
void heap_init(void){
    heap_next=KHEAP_START; ///this heap_init is for telling the bootloader i.e during boot the heap_next is initiated always with starting address of the kernel heap address kheap!!
    heap_head=0;
}
void *kmalloc_page(void){
    uint32_t phys = alloc_frame();
    if (!phys)
        return 0;

    map_page(heap_next, phys);

    void *result = (void *)heap_next;

    // Zero the newly mapped page so no stale data leaks between allocations
    uint8_t *p = (uint8_t *)result;
    for (int i = 0; i < PAGE_SIZE; i++)
        p[i] = 0;

    heap_next += 0x1000;
    return result;
} 
void *kmalloc(uint32_t size){
    if (heap_head == 0) {
        void *page = kmalloc_page();
        if (!page) return 0;
        heap_head = (heap_block_t *)page;
        heap_head->size    = PAGE_SIZE - sizeof(heap_block_t);
        heap_head->is_free = 1;
        heap_head->next    = 0;
    }

retry:
    heap_block_t *current = heap_head;
    while (current != 0) {
        if (current->is_free == 1 && current->size >= size) {

            if (current->size > (size + sizeof(heap_block_t))) {
                heap_block_t *new_block = (heap_block_t *)((uint32_t)current + sizeof(heap_block_t) + size);
                new_block->size    = current->size - size - sizeof(heap_block_t);
                new_block->is_free = 1;
                new_block->next    = current->next;

                current->size = size;
                current->next = new_block;
            }

            current->is_free = 0;
            void *payload = (void *)((uint32_t)current + sizeof(heap_block_t));
            return payload;
        }
        current = current->next;
    }

    // No free block found. Grow the heap by appending a new page at the tail,
    // so that list order always matches physical memory order. This is
    // required for kfree's coalescing to merge correct neighbours.
    void *new_page = kmalloc_page();
    if (!new_page) {
        kprint("HEAP HIGHWAY EXHAUSTED\n");
        return 0;
    }

    heap_block_t *new_block = (heap_block_t *)new_page;
    new_block->size    = PAGE_SIZE - sizeof(heap_block_t);
    new_block->is_free = 1;
    new_block->next    = 0;

    // Walk to tail and append
    heap_block_t *last = heap_head;
    while (last->next != 0)
        last = last->next;
    last->next = new_block;

    goto retry;
}
void kfree(void *ptr){
    if(ptr==0){
        return;
    }
    heap_block_t *block=(heap_block_t *)((uint32_t)ptr-sizeof(heap_block_t));   
    block->is_free=1;
    //coalescing//
    if(block->next!=0 && block->next->is_free==1){
        block->size+=block->next->size+sizeof(heap_block_t);
        block->next=block->next->next; //here block 1's next pointer points to block 3 
    }
    heap_block_t *prev=heap_head;
    while(prev!=0){
        if(prev->next==block){
            if(prev->is_free==1){
                prev->size+=block->size+sizeof(heap_block_t);
                prev->next=block->next;
            }
            break;
        }
        prev=prev->next;
    }


}
