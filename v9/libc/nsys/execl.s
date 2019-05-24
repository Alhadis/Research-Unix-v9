# C library -- execl

# execl(file, arg1, arg2, ... , 0);
#

.globl	_execl

_execl:
	linkw	a6,#0
	pea	sp@(12)
	movl	sp@(12),sp@-
	jsr	_execv
	addql	#8,sp
	unlk	a6
	rts
