#include "panic.h"
#include "keyboard.h"
#include <stdint.h>
#include "uart.h"
#include "paging.h"
#include "console.h"
extern uint32_t alloc_frame(void);
extern void map_page(uint32_t virt, uint32_t phys);

void isr_handler(void)
{
    uint32_t *stack;
    asm volatile("mov %%esp, %0" : "=r"(stack));
    kprint("ISR: eip="); kprint_hex(stack[9]);
    kprint(" cs="); kprint_hex(stack[10]);
    kprint("\n");
    while (1);
}
void page_fault_handler(uint32_t error)
{
    uint32_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    if (!(error & 0x1) && (error & 0x4) && cr2 < 0xC0000000) {
        uint32_t frame = alloc_frame();
        if (frame) {
            map_page(cr2, frame);
            uint8_t *ptr = (uint8_t *)cr2;
            for (int i = 0; i < 4096; i++) ptr[i] = 0;
            return;
        }
    }

    uart_puts("PAGE FAULT\n");
    uart_puts("CR2 = "); uart_puthex(cr2);
    uart_puts("\nERR = "); uart_puthex(error);
    uart_puts("\n");
    while (1) asm volatile("cli; hlt");
}