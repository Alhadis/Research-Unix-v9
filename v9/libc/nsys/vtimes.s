# C library -- times

	vtimes = 64+43
.globl	_vtimes

_vtimes:
	linkw	a6,#0
	pea	vtimes
	trap	#0
	unlk	a6
	rts
