# C library -- mknod

# error = mknod(string, mode, major.minor);

	mknod = 14
.globl	_mknod
.globl  cerror

_mknod:
	pea	mknod
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
