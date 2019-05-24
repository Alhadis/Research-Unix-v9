# C library -- fmount

# error = fmount(type, fd, file, flag)

	fmount = 26
.globl	_fmount
.globl  cerror

_fmount:
	pea	fmount
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
