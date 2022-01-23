.data
.global msg
.global len
msg:
	.ascii "Hello, World!\n"
	len = . - msg
