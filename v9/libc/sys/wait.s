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
	pea	wait
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	tstl	sp@(4)		| status desired?
	beql	nostatus	| no
	movl	sp@(4),a0
	movl	d1,a0@		| store child's status
nostatus:
	rts
