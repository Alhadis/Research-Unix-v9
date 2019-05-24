# C library -- setjmp, longjmp
#
#	longjmp(env, val)
#		will generate a "return(val)" from the last call to
#	setjmp(env)
#		by restoring a2 - a7, d2 - d7, pc, and sr from env
#		and doing a return
	.text
	.globl _setjmp
	.globl _longjmp

_setjmp:
	movl	sp@+,a1			| save pc and restore stack
	movl	sp@,a0
	moveml	#0xfcfc,a0@		| a2 - a7, d2 - d7
	movl	a1,a0@(48)		| pc
	clrl	d0			| return 0
	jmp	a1@

_longjmp:
	movl	sp@(4),a0
	movl	sp@(8),d0		| the return value
	movl	a0@(48),a1		| return pc
	moveml	a0@,#0xfcfc		| restore registers
	jmp	a1@
