	stime = 25
.globl	_stime
.globl  cerror

_stime:
	linkw	a6,#0
	movl	sp@(8),a0
	movl	a0@,sp@(8) | copy time to set
	pea	stime
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
