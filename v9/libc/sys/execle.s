# C library -- execle

# execle(file, arg1, arg2, ... , env);
#

.globl	_execle

_execle:
	lea	sp@(4),a0
L1:
	movl	a0@+,d0		| traverse the args
	tstl	d0
	bne	L1
	movl	a0@,sp@-	| env
	pea	sp@(12)		| argv
	movl	sp@(12),sp@-	| file
	jsr	_execve
	addl	#12,sp
	rts
