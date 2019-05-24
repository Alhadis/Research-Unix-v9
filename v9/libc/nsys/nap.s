# C library-- nap

# error = nap(nticks)

	nap = 64+41
.globl	_nap

_nap:
	linkw	a6,#0
	pea	nap
	trap	#0
	unlk	a6
	rts
