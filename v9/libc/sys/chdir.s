# C library -- chdir

# error = chdir(string);

	chdir = 12
.globl	_chdir

_chdir:
	pea	chdir
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
