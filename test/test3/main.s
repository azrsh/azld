.text
.global main
main:
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
