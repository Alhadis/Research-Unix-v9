# C library -- stat

# error = stat(string, statbuf);

# char statbuf[36]

	stat = 18
.globl	_stat
.globl  cerror

_stat:
	linkw	a6,#0
	pea	stat
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
