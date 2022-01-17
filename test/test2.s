.text
.globl main
main:
    xor %rbp,%rbp
    xor %rax, %rax
    call callee
    mov %rax, %rdi
    mov $60, %rax
    syscall

callee:
    mov $1, %rax
    mov $1, %rdi
    mov $msg, %rsi
    mov $len, %rdx
    syscall
    mov $0, %rax
    ret

.data
msg:
	.ascii "Hello, World!\n"
	len = . - msg

