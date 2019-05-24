# C library -- unlink

# error = unlink(string);
#

	unlk = 10
.globl	_unlink
.globl  cerror

_unlink:
	linkw	a6,#0
	pea	unlk
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
