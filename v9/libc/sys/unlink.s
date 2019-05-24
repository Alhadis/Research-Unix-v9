# C library -- unlink

# error = unlink(string);
#

	unlink = 10
.globl	_unlink
.globl  cerror

_unlink:
	pea	unlink
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
