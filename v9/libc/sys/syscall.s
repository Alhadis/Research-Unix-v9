	.globl	_syscall
	.globl	cerror2
_syscall:
	movl	sp@(4),d0	| syscall number
	movl	sp@,sp@(4)	| fix arguments
	movl	d0,sp@		| place syscall number on back on stack
	trap	#0
	bcs	L1
	movl	sp@,sp@-	| restore stack
	movl	d0,a0		| for sbreak
	rts
L1:
	movl	sp@,sp@-	| restore stack
	jmp	cerror2
