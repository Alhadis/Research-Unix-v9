	stime = 25
.globl	_stime
.globl  cerror

_stime:
	movl	sp@(4),a0
	movl	a0@,sp@(4) | copy time to set
	pea	stime
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
