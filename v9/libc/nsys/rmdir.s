# C library -- rmdir

# error = rmdir(string);

	rmdir = 64
.globl	_rmdir

_rmdir:
	linkw	a6,#0
	pea	rmdir
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
