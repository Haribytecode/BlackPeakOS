#include "paging.h"
#include "console.h"
#include <stdint.h>
#include "uart.h"


#define SCRATCH_PDE_INDEX 772
#define SCRATCH_PT_VIRT   0xC1000000

extern uint32_t __phys_page_directory;
extern uint32_t __phys_first_pt;
extern const uint32_t __phys_kernel_pt[];
extern const uint32_t __phys_second_pt[];
extern uint32_t __phys_bss_end;

uint32_t page_directory[1024]   __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));
uint32_t second_page_table[1024] __attribute__((aligned(4096)));
uint32_t kernel_page_table[1024] __attribute__((aligned(4096)));
uint32_t scratch_page_table[1024] __attribute__((aligned(4096)));

uint8_t frame_bitmap[MAX_FRAMES];

void mark_frame_used(uint32_t phys)
{
    if (phys >= 0x800000) {
        uint32_t idx = (phys - 0x800000) / 0x1000;
        if (idx < MAX_FRAMES)
            frame_bitmap[idx] = 1;
    }
}


void paging_init(void)
{
    kprint("Paging runtime initialized\n");
}

void paging_enable(void)
{
    kprint("PAGING ON\n");
}

uint32_t alloc_frame(void)
{
    for (int i = 0; i < MAX_FRAMES; i++) {
        if (frame_bitmap[i] == 0) {
            frame_bitmap[i] = 1;
            return 0x800000 + (i * 0x1000);   // physical addresses start at 8 MB
        }
    }
    return 0;
}

void free_frame(uint32_t phys)
{
    uint32_t idx = (phys - 0x800000) / 0x1000;
    if (idx < MAX_FRAMES)
        frame_bitmap[idx] = 0;
}

void map_page(uint32_t virt, uint32_t phys)
{
    uint32_t dir_index = virt >> 22;
    uint32_t tab_index = (virt >> 12) & 0x3FF;
    uint32_t flags = 3;
    if (virt < 0xC0000000) flags |= 4;

    uint32_t *pd = (uint32_t *)0xFFFFF000;

    if (!(pd[dir_index] & 1)) {
        uint32_t pt_phys = alloc_frame();
        if (!pt_phys) { kprint("OUT OF MEMORY\n"); while(1); }
        pd[dir_index] = pt_phys | (flags & 0x7);
        uint32_t *pt = (uint32_t *)(0xFFC00000 + dir_index * 0x1000);
        for (int i = 0; i < 1024; i++) pt[i] = 0;
    }

    uint32_t *pt = (uint32_t *)(0xFFC00000 + dir_index * 0x1000);
    pt[tab_index] = phys | flags;
    asm volatile("invlpg (%0)" : : "r"(virt));
}
#define SCRATCH_PDE_INDEX 772
#define SCRATCH_PT_VIRT   0xC1000000

uint32_t *create_process_address_space(void)
{
    uint32_t new_pd_phys = alloc_frame();
    if (!new_pd_phys) return 0;

    uint32_t *master_pd = (uint32_t *)0xFFFFF000;

    // Map the new page directory into the scratch window
    scratch_page_table[0] = new_pd_phys | 3;
    asm volatile("invlpg (%0)" : : "r"((void *)SCRATCH_PT_VIRT) : "memory");

    volatile uint32_t *new_pd = (volatile uint32_t *)SCRATCH_PT_VIRT;

    // 1. Zero user space (entries 0‑767) but keep identity maps
    for (int i = 0; i < 768; i++)
        new_pd[i] = 0;

    // 2. Preserve the essential identity-mapped PDEs (0‑1)
    //    so the bootstrap kernel stack (low memory) remains accessible
    new_pd[0] = master_pd[0];   // 0–4 MB
    new_pd[1] = master_pd[1];   // 4–8 MB

    // 3. Share kernel PDEs (768‑1022), isolate scratch slot
    for (int i = 768; i < 1022; i++) {
        if (i == SCRATCH_PDE_INDEX)
            new_pd[i] = 0;
        else
            new_pd[i] = master_pd[i];
    }

    // 4. Recursive self‑mapping
    new_pd[1023] = new_pd_phys | 3;

    // 5. Unmap scratch window
    scratch_page_table[0] = 0;
    asm volatile("invlpg (%0)" : : "r"((void *)SCRATCH_PT_VIRT) : "memory");

    return (uint32_t *)new_pd_phys;
}