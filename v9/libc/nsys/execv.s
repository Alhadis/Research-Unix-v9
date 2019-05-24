# C library -- execv

# execv(file, argv);
#
# where argv is a vector argv[0] ... argv[x], 0
# last vector element must be 0

.globl	_execv
.globl	_environ

_execv:
	linkw	a6,#0
	movl	_environ,sp@-	|  default environ
	movl	sp@(16),sp@-	|  argv
	movl	sp@(16),sp@-	|  file
	jsr	_execve
	addl	#12,sp
	unlk	a6
	rts
