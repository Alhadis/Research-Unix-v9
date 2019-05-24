# pipe -- C library

#	pipe(f)
#	int f[2];

	pipe = 42
.globl	_pipe
.globl  cerror

_pipe:
	linkw	a6,#0
	pea	pipe
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	movl	sp@(8),a0
	movl	d0,a0@+
	movl	d1,a0@
	clrl	d0
	unlk	a6
	rts
