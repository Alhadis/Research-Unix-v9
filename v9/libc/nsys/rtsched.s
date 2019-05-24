# C library -- rtsched

# file = rtsched(algorithm, param);
#
# file == -1 if error

	rtsched = 64+23
.globl	_rtsched

_rtsched:
	linkw	a6,#0
	pea	rtsched
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
