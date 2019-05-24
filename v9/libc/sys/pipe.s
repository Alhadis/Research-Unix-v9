# pipe -- C library

#	pipe(f)
#	int f[2];

	pipe = 42
.globl	_pipe
.globl  cerror

_pipe:
	pea	pipe
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	movl	sp@(4),a0
	movl	d0,a0@+
	movl	d1,a0@
	clrl	d0
	rts
