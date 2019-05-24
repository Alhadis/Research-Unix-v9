# C library -- vfork

# pid = vfork();
#
# r1 == 0 in parent process, r1 == 1 in child process.
# r0 == pid of child in parent, r0 == pid of parent in child.
#
# trickery here, due to keith sklower, uses ret to clear the stack,
# and then returns with a jump indirect, since only one person can return
# with a ret off this stack... we do the ret before we vfork!
# 

	vfork = 66
.globl	_vfork

_vfork:
	movl	sp@+,a0
	pea	vfork
	trap	#0
	bcc	vforkok
	jmp	verror
vforkok:
	tstl	d1		| child process ?
	bne	child		| yes
	bcc 	parent		| if c-bit not set, fork ok
.globl	_errno
verror:
	movl	d0,_errno
	movl	#-1,d0
	jmp	a0@
child:
	clrl	d0
parent:
	jmp	a0@
