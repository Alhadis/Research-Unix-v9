# C library -- execl

# execl(file, arg1, arg2, ... , 0);
#

.globl	_execl

_execl:
	pea	sp@(8)
	movl	sp@(8),sp@-
	jsr	_execv
	addql	#8,sp
	rts
