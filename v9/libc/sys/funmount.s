# C library -- funmount

# error = funmount(file)

	funmount = 50
.globl	_funmount
.globl  cerror

_funmount:
	pea	funmount
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
