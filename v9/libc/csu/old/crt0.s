# C runtime startoff

	exit = 1
.globl	_exit
.globl	start
.globl	_main
.globl	_environ

#
#	C language startup routine

start:
	subql	#8,sp
	movl	sp@(8),sp@	| argc
	lea	sp@(12),a0
	movl	a0,sp@(4)	| argv
	movl	a0@,d1
L1:
	tstl	a0@+		| null args term ?
	bne	L1
	cmpl	a0,d1	  	| end of 'env' or 'argv' ?
	blt	L2
	tstl	a0@-		| envp's are in list
L2:
	movl	a0,sp@(8)  	| env
	movl	a0,_environ	| indir is 0 if no env ; not 0 if env
	jsr	_main
	addql	#8,sp
	movl	d0,sp@-
	jsr	_exit
	addql	#4,sp
	pea	exit
	trap	#0
#
	.data
_environ:	.long	0
