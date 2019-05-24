# C library -- fstat

# error = fstat(file, statbuf);

# char statbuf[34]

	fstat = 28
.globl	_fstat

_fstat:
	linkw	a6,#0
	pea	fstat
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
