# C return sequence which
# sets errno, returns -1.

.globl	cerror
.globl	cerror2
.comm	_errno,4

cerror:
	movl	d0,_errno
	movl	#-1,d0
	rts

cerror2:
	movl	d0,_errno
	movl	#-1,d0
	movl	#-1,a0
	rts
