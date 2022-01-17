.text
.globl _start
_start:
    xor %rbp,%rbp
    xor %rax, %rax
    call callee1
    call callee2
    mov $60, %rax
    mov $0, %rdi
    syscall

callee1:
    mov $1, %rax
    mov $1, %rdi
    mov $msg, %rsi
    mov $len, %rdx
    syscall
    ret

.globl callee2
callee2:
    mov $1, %rax
    mov $1, %rdi
    mov $msg, %rsi
    mov $len, %rdx
    syscall
    ret

.data
msg:
	.ascii "Hello, World!\n"
	len = . - msg

