# C library -- chdir

# error = chdir(string);

	chdir = 12
.globl	_chdir

_chdir:
	linkw	a6,#0
	pea	chdir
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
