CC = gcc
LD = ld

CFLAGS = -m32 -std=gnu11 -O0 -g -ffreestanding -fno-builtin -Wall -Wextra -nostdlib -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector
LDFLAGS = -m elf_i386 -T linker.ld


OBJS = boot.o idt_asm.o kernel.o gdt.o gdt_flush.o idt.o isr.o panic.o pic.o keyboard.o uart.o paging.o heap.o scheduler.o timer.o task.o console.o tss.o tss_flush.o user.o  context_switch.o context.o thread.o syscall.o vfs.o ramfs.o initrd.o tarfs.o

all: kernel.elf

# This rule below forces 32-bit mode for both .S and .s files
%.o: %.S
	$(CC) -m32 -ffreestanding -c $< -o $@

%.o: %.s
	$(CC) -m32 -ffreestanding -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.elf $(OBJS)

# Using this we can the  build the final ISO file 
iso: kernel.elf
	cp kernel.elf iso_root/boot/kernel.elf
	grub-mkrescue -o myos.iso iso_root

clean:
	rm -f *.o kernel.elf myos.iso
