#include <stdint.h>
#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "tss.h"
#include "uart.h"
#include "scheduler.h"
#include "pic.h"
#include "paging.h"
#include "heap.h"
#include "vfs.h"
#include "ramfs.h"
extern void tss_flush(void);
extern void enter_user_mode_v2(void);
extern uint8_t frame_bitmap[MAX_FRAMES];

void task_a(void);
void task_b(void);
void user_task_dummy(void);
static void heap_test(void);
static void heap_stress_test(void);
/* Linker symbols – use & to get their physical addresses */
extern uint32_t __phys_scratch_pt;
extern uint32_t __phys_page_directory;

void kernel_main(void)
{
    // --- Frame allocator init ---
    extern uint8_t frame_bitmap[MAX_FRAMES];
    extern uint32_t __phys_bss_end; // linker symbol, NOT an array

    for (int i = 0; i < MAX_FRAMES; i++)
        frame_bitmap[i] = 0;

    // Physical address of the end of the kernel's BSS (must use &)
    uint32_t kernel_phys_end = (uint32_t)&__phys_bss_end;

    // Mark every frame from 8 MB up to kernel_phys_end as used
    for (uint32_t p = 0x800000; p < kernel_phys_end; p += 0x1000)
    {
        uint32_t idx = (p - 0x800000) / 0x1000;
        if (idx < MAX_FRAMES)
            frame_bitmap[idx] = 1;
    }

    scheduler_init();
    gdt_init();
    tss_init();

    static uint8_t kernel_stack[4096] __attribute__((aligned(16)));
    tss.esp0 = (uint32_t)&kernel_stack[4096];
    tss.ss0 = 0x10;
    tss.iomap_base = sizeof(tss_t);

    tss_flush();
    idt_init();
    heap_init();
    heap_test();
    // heap_stress_test();
             vfs_init();
    ramfs_init();

    vfs_node_t *hello = ramfs_create_file(vfs_root(), "hello");
    if (hello) {
        uint8_t msg[] = "hi from ramfs";
        hello->ops->write(hello, 0, sizeof(msg)-1, msg);

        uint8_t buf[32];
        uint32_t got = hello->ops->read(hello, 0, sizeof(msg)-1, buf);
        buf[got] = '\0';
        kprint("ramfs test: ");
        kprint((char *)buf);
        kprint("\n");
    }
    paging_enable(); // just prints a message

// -------------------------------------------------------------
// 3. Set up the scratch page table (PDE 772) so that
//    create_process_address_space can map temporary pages at
//    virtual address 0xC1000000.
// -------------------------------------------------------------
#define SCRATCH_PDE_INDEX 772
    uint32_t *active_pd = (uint32_t *)0xFFFFF000; // recursive window
    active_pd[SCRATCH_PDE_INDEX] = ((uint32_t)&__phys_scratch_pt) | 3;

    // Full TLB flush to make the scratch PDE immediately visible
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");

    pic_init();
    paging_init(); // just prints a message

    // -------------------------------------------------------------
    // 4. Create three isolated page directories:
    //    - pdA for kernel task A
    //    - pdB for kernel task B
    //    - pdUser for the Ring‑3 user task
    // -------------------------------------------------------------
    uint32_t *pdA = create_process_address_space();
    uint32_t *pdB = create_process_address_space();
    uint32_t *pdUser = create_process_address_space();

    if (!pdA || !pdB || !pdUser)
    {
        kprint("PD creation failed\n");
        while (1)
            ;
    }

    // Kernel task A
    task_t *tA = scheduler_create_task(task_a, 0, 0, 0);
    if (tA)
        tA->cr3 = (uint32_t)pdA;

    // Kernel task B
    task_t *tB = scheduler_create_task(task_b, 0, 0, 0);
    if (tB)
        tB->cr3 = (uint32_t)pdB;

    // -------------------------------------------------------------
    // 5. Prepare the Ring‑3 user process
    // -------------------------------------------------------------
    uint32_t user_eip = 0x00800000; // PDE 2 — private per process
    uint32_t user_esp = 0x00900000; // PDE 2 — private per process

    uint32_t phys_code = alloc_frame();
    mark_frame_used(phys_code);
    map_page(0xD0000000, phys_code);
    uint8_t *tmp = (uint8_t *)0xD0000000;

    // User program: infinite loop doing int 0x80 (syscall) with a delay
    tmp[0] = 0xCD; // int 0x80
    tmp[1] = 0x80;
    tmp[2] = 0xB9; // mov ecx, 0x00A00000
    tmp[3] = 0x00;
    tmp[4] = 0x00;
    tmp[5] = 0xA0;
    tmp[6] = 0x00;
    tmp[7] = 0xE2; // loop $-2   (decrements ecx, jumps back to itself)
    tmp[8] = 0xFE;
    tmp[9] = 0xEB; // jmp $-11   (back to int 0x80)
    tmp[10] = 0xF5;

    // Switch to the user page directory and map user pages
    asm volatile("mov %0, %%cr3" : : "r"(pdUser) : "memory");
    map_page(user_eip, phys_code);

    uint32_t phys_stack = alloc_frame();
    if (phys_stack)
    {
        mark_frame_used(phys_stack);
        map_page(user_esp, phys_stack);
    }

    // Switch back to the kernel's master page directory
    asm volatile("mov %0, %%cr3" : : "r"((uint32_t)&__phys_page_directory) : "memory");

    // Create the user task (is_user = 1)
    task_t *tUser = scheduler_create_task(user_task_dummy, 1, user_eip, user_esp);
    if (tUser)
        tUser->cr3 = (uint32_t)pdUser;

    // -------------------------------------------------------------
    // 6. All tasks ready – enable interrupts and start scheduling
    // -------------------------------------------------------------
    kprint("All tasks created\n");

    asm volatile("sti");
    while (1)
    {
        asm volatile("hlt");
    }
}
static void heap_test(void)
{
    kprint("HEAP TEST START\n");

    uint8_t *a = kmalloc(100);
    uint8_t *b = kmalloc(200);

    if (!a || !b)
    {
        kprint("HEAP TEST FAIL: allocation\n");
        return;
    }

    a[0] = 0xAA;
    a[99] = 0x55;
    b[0] = 0xCC;
    b[199] = 0x33;

    if (a[0] != 0xAA || a[99] != 0x55 ||
        b[0] != 0xCC || b[199] != 0x33)
    {
        kprint("HEAP TEST FAIL: memory corruption\n");
        return;
    }

    kfree(a);
    kfree(b);

    uint8_t *c = kmalloc(300);

    if (!c)
    {
        kprint("HEAP TEST FAIL: reuse\n");
        return;
    }

    c[0] = 0xDE;
    c[299] = 0xAD;

    if (c[0] != 0xDE || c[299] != 0xAD)
    {
        kprint("HEAP TEST FAIL: reused block\n");
        return;
    }

    kfree(c);

    kprint("HEAP TEST PASS\n");
}
static void heap_stress_test(void)
{
#define STRESS_N 200
    static uint8_t *ptrs[STRESS_N];
    static uint32_t sizes[STRESS_N];

    kprint("HEAP STRESS START\n");

    // 1. Allocate 200 blocks of varying sizes, write magic bytes
    for (int i = 0; i < STRESS_N; i++)
    {
        sizes[i] = 8 + (i * 7) % 200; // 8..207 bytes
        ptrs[i] = kmalloc(sizes[i]);
        if (!ptrs[i])
        {
            kprint("HEAP STRESS FAIL at alloc ");
            kprint_dec(i);
            kprint("\n");
            return;
        }
        ptrs[i][0] = (uint8_t)(0xA0 + (i & 0x0F));
        ptrs[i][sizes[i] - 1] = (uint8_t)(0xB0 + (i & 0x0F));
    }

    // 2. Verify every magic byte is intact
    for (int i = 0; i < STRESS_N; i++)
    {
        if (ptrs[i][0] != (uint8_t)(0xA0 + (i & 0x0F)) ||
            ptrs[i][sizes[i] - 1] != (uint8_t)(0xB0 + (i & 0x0F)))
        {
            kprint("HEAP STRESS FAIL: corruption at ");
            kprint_dec(i);
            kprint("\n");
            return;
        }
    }

    // 3. Free in reverse order
    for (int i = STRESS_N - 1; i >= 0; i--)
        kfree(ptrs[i]);

    // 4. Re-allocate the same sizes, write new magic
    for (int i = 0; i < STRESS_N; i++)
    {
        ptrs[i] = kmalloc(sizes[i]);
        if (!ptrs[i])
        {
            kprint("HEAP STRESS FAIL at realloc ");
            kprint_dec(i);
            kprint("\n");
            return;
        }
        ptrs[i][0] = (uint8_t)(0xC0 + (i & 0x0F));
    }

    // 5. Verify, then free
    for (int i = 0; i < STRESS_N; i++)
    {
        if (ptrs[i][0] != (uint8_t)(0xC0 + (i & 0x0F)))
        {
            kprint("HEAP STRESS FAIL: realloc corruption at ");
            kprint_dec(i);
            kprint("\n");
            return;
        }
        kfree(ptrs[i]);
    }

    kprint("HEAP STRESS PASS\n");
}
void task_a(void)
{
    while (1)
    {
        uart_putc('A');
        for (volatile int i = 0; i < 50000000; i++)
        {
        }
    }
}

void task_b(void)
{
    while (1)
    {
        uart_putc('B');
        for (volatile int i = 0; i < 50000000; i++)
        {
        }
    }
}

void user_task_dummy(void)
{
    // This function is never executed – the real user code is the
    // machine‑code blob mapped at 0x200000.
    while (1)
    {
    }
}