# C library -- times

	times = 43
.globl	_times

_times:
	linkw	a6,#0
	pea	times
	trap	#0
	bcc	noerror
	jmp	cerror
noerror:
	unlk	a6
	rts
