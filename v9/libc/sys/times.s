# C library -- times

	times = 43
.globl	_times

_times:
	pea	times
	trap	#0
	bcc	noerror
	jmp	cerror
noerror:
	rts
