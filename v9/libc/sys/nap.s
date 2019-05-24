# C library-- nap

# error = nap(nticks)

	nap = 64+41
.globl	_nap

_nap:
	pea	nap
	trap	#0
	rts
