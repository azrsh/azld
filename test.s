.text
.global main
main:
mov $1, %rax
mov $1, %rdi
mov $msg, %rsi
mov $len, %rdx
syscall
mov $60, %rax
mov $0, %rdi
syscall
.data
msg:
	.ascii "Hello, World!\n"
	len = . - msg

