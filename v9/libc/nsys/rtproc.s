# C library -- rtproc

# file = rtproc(pri, ssize);
#
# file == -1 if error

	rtproc = 64+22
.globl	_rtproc

_rtproc:
	linkw	a6,#0
	pea	rtproc
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
