# C library -- funmount

# error = funmount(file)

	funmount = 50
.globl	_funmount
.globl  cerror

_funmount:
	linkw	a6,#0
	pea	funmount
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
