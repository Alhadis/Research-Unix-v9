# C library -- mknod

# error = mknod(string, mode, major.minor);

	mknod = 14
.globl	_mknod
.globl  cerror

_mknod:
	linkw	a6,#0
	pea	mknod
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
