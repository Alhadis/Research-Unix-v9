# C library -- fork

# pid = fork();
#
# r1 == 0 in parent process, r1 == 1 in child process.
# r0 == pid of child in parent, r0 == pid of parent in child.

	fork = 2
.globl	_fork
.globl	_vfork

_vfork:
_fork:
	linkw	a6,#0
	pea	fork
	trap	#0
	bcc		forkok
	jmp		cerror
forkok:
	andl	#1,d1
	beq	parent
	clrl	d0		| signify child
parent:
	unlk	a6
	rts
