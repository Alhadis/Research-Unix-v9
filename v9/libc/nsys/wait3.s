# C library -- wait3

# pid = wait3(&status, flags, &vmstat);
#
# pid == -1 if error
# status indicates fate of process, if given
# flags may indicate process is not to hang or
# that untraced stopped children are to be reported.
# vmstat optionally returns detailed resource usage information
#

	wait3 = 7		| same as wait!
.globl	_wait3
.globl  cerror

_wait3:
	linkw	a6,#0
	movl	sp@(12),d0	| make it easy for system to get
	movl	sp@(16),d1	| these extra arguments
	pea	wait3
	orb	#0x1f,cc	| flags wait3()
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	tstl	sp@(8)		| status desired?
	beql	nostatus	| no
	movl	sp@(8),a1
	movl	d1,a1@		| store child's status
nostatus:
	unlk	a6
	rts
