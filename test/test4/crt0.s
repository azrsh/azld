.text
.globl _start
_start:
    xor %rbp,%rbp
    xor %rax, %rax
    call main
    mov %rax, %rdi
    mov $60, %rax
    syscall
