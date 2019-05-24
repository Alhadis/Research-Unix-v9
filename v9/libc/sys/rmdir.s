# C library -- rmdir

# error = rmdir(string);

	rmdir = 64
.globl	_rmdir

_rmdir:
	pea	rmdir
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
