#ifndef IO_H
#define IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ("outb %0, %1"
                      :
                      : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0"
                      : "=a"(ret)
                      : "Nd"(port));
    return ret;
}
static inline uint32_t irq_save(void)
{
    uint32_t flags;
    asm volatile("pushfl; popl %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static inline void irq_restore(uint32_t flags)
{
    asm volatile("pushl %0; popfl" :: "r"(flags) : "memory", "cc");
}
#endif