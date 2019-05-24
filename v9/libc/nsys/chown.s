# C library -- chown

# error = chown(string, owner);

	chown = 16
.globl	_chown

_chown:
	linkw	a6,#0
	pea	chown
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
