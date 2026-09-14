#include "heap.h"
#include "paging.h"
#include "console.h"

#define KHEAP_START 0xD0000000
#define PAGE_SIZE   4096

static uint32_t      heap_next = KHEAP_START;
static heap_block_t *heap_head = 0;

void heap_init(void) {
    heap_next = KHEAP_START;
    heap_head = 0;
}

void *kmalloc_page(void) {
    uint32_t phys = alloc_frame();
    if (!phys) return 0;
    map_page(heap_next, phys);
    uint8_t *p = (uint8_t *)heap_next;
    for (int i = 0; i < PAGE_SIZE; i++) p[i] = 0;
    heap_next += PAGE_SIZE;
    return (void *)p;
}

void *kmalloc(uint32_t size) {
    /* align size to 4 bytes so every header stays 4-byte aligned */
    size = (size + 3) & ~3u;

    if (!heap_head) {
        void *pg = kmalloc_page();
        if (!pg) return 0;
        heap_head = (heap_block_t *)pg;
        heap_head->size    = PAGE_SIZE - sizeof(heap_block_t);
        heap_head->is_free = 1;
        heap_head->next    = 0;
    }

retry:;
    heap_block_t *cur = heap_head;
    while (cur) {
        if (cur->is_free && cur->size >= size) {
            /* split only if the remainder can hold a header + ≥4 bytes */
            if (cur->size >= size + sizeof(heap_block_t) + 4) {
                heap_block_t *tail = (heap_block_t *)
                    ((uint8_t *)cur + sizeof(heap_block_t) + size);
                tail->size    = cur->size - size - sizeof(heap_block_t);
                tail->is_free = 1;
                tail->next    = cur->next;
                cur->size     = size;
                cur->next     = tail;
            }
            cur->is_free = 0;
            return (void *)((uint8_t *)cur + sizeof(heap_block_t));
        }
        cur = cur->next;
    }

    /* grow heap by one page, link at tail */
    void *pg = kmalloc_page();
    if (!pg) { kprint("HEAP EXHAUSTED\n"); return 0; }

    heap_block_t *nb = (heap_block_t *)pg;
    nb->size    = PAGE_SIZE - sizeof(heap_block_t);
    nb->is_free = 1;
    nb->next    = 0;

    heap_block_t *last = heap_head;
    while (last->next) last = last->next;
    last->next = nb;
    goto retry;
}

void kfree(void *ptr) {
    if (!ptr) return;

    heap_block_t *blk = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    blk->is_free = 1;

    /* forward coalesce */
    while (blk->next && blk->next->is_free) {
        blk->size += sizeof(heap_block_t) + blk->next->size;
        blk->next  = blk->next->next;
    }

    /* backward coalesce: walk from head each time */
    heap_block_t *prev = heap_head;
    while (prev && prev->next != blk) prev = prev->next;

    if (prev && prev->is_free) {
        prev->size += sizeof(heap_block_t) + blk->size;
        prev->next  = blk->next;
        /* blk is now interior to prev — nothing points to it */
    }
}
void heap_dump(void) {
    kprint("--- HEAP DUMP ---\n");
    heap_block_t *b = heap_head;
    uint32_t i = 0;
    while (b != 0 && i < 50) {
        kprint("blk "); kprint_dec(i);
        kprint(" @ ");  kprint_hex((uint32_t)b);
        kprint(" size="); kprint_dec(b->size);
        kprint(" free="); kprint_dec(b->is_free);
        kprint("\n");
        b = b->next;
        i++;
    }
    kprint("heap_next = "); kprint_hex(heap_next); kprint("\n");
    kprint("----------------\n");
}