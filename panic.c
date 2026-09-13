#include "panic.h"
#include "console.h"
#include "uart.h"
extern void uart_puts(const char *);
void panic(const char *msg){
    uart_puts("KERNEL PANIC\n");
    uart_puts(msg);
    uart_puts("SYSTEM HALTED\n");
    while(1){
        __asm__ volatile("cli;hlt");
    }
}
void panic_at(const char *msg, const char *file, int line)
{
    uart_puts("KERNEL PANIC: ");
    uart_puts(msg);
    uart_puts("\n  at ");
    uart_puts(file);
    uart_puts(":");
    uart_puthex((uint32_t)line);
    uart_puts("\nSYSTEM HALTED\n");
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}