.section .initrd, "a"
.align 4

.global initrd_start
.global initrd_end

initrd_start:
    .incbin "initrd.tar"
initrd_end:

.section .note.GNU-stack,"",@progbits