|	Copyright (c) 1985 by Sun Microsystems, Inc.

	.globl	__sigtramp, __sigfunc
__sigtramp:
	moveml	#0xC0C0,sp@-	| save C scratch regs
	movl	sp@(16),d0	| get signal number
	lsll	#2,d0		| scale for index
	movl	#__sigfunc,a0	| get array of func ptrs
	movl	a0@(0,d0:l),a0	| get func
	movl	sp@(0+20),sp@-	| push code
	movl	sp@(4+16),sp@-	| push signal number
	jsr	a0@		| call handler
	addql	#8,sp		| pop args
	moveml	sp@+,#0x0303	| restore regs
	addl	#8,sp		| pop signo and code
	pea	94
	trap	#0
