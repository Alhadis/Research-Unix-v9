# C library -- times

	vtimes = 64+43
.globl	_vtimes

_vtimes:
	pea	vtimes
	trap	#0
	rts
