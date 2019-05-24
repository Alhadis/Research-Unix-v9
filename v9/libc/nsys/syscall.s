	.globl	_syscall
	.comm	_errno,4
_syscall:
	movl	sp@(4),d0	| syscall number
	movl	sp@+,sp@	| fix arguments
	linkw	a6,#0
	movl	d0,sp@-		| place syscall number on back on stack
	trap	#0
	unlk	a6
	bcs	L1
	movl	sp@,sp@-	| restore stack
	movl	d0,a0		| for sbreak
	rts
L1:
	movl	sp@,sp@-	| restore stack
	movl	d0,_errno
	movl	#-1,d0
	movl	d0,a0
	rts
