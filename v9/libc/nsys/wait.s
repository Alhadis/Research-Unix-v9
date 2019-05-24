# C library -- wait

# pid = wait(0);
#   or,
# pid = wait(&status);
#
# pid == -1 if error
# status indicates fate of process, if given

	wait = 7
.globl	_wait
.globl  cerror

_wait:
	linkw	a6,#0
	pea	wait
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	tstl	sp@(8)		| status desired?
	beql	nostatus	| no
	movl	sp@(8),a0
	movl	d1,a0@		| store child's status
nostatus:
	unlk	a6
	rts
