# C library -- execv

# execv(file, argv);
#
# where argv is a vector argv[0] ... argv[x], 0
# last vector element must be 0

.globl	_execv
.globl	_environ

_execv:
	movl	_environ,sp@-	|  default environ
	movl	sp@(12),sp@-	|  argv
	movl	sp@(12),sp@-	|  file
	jsr	_execve
	addl	#12,sp
	rts
