# C library -- fstat

# error = fstat(file, statbuf);

# char statbuf[34]

	fstat = 28
.globl	_fstat

_fstat:
	pea	fstat
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
