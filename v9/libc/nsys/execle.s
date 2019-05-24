# C library -- execle

# execle(file, arg1, arg2, ... , env);
#

.globl	_execle

_execle:
	linkw	a6,#0
	lea	sp@(8),a0
L1:
	movl	a0@+,d0		| traverse the args
	tstl	d0
	bne	L1
	movl	a0@,sp@-	| env
	pea	sp@(16)		| argv
	movl	sp@(16),sp@-	| file
	jsr	_execve
	addl	#12,sp
	unlk	a6
	rts
