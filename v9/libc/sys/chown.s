# C library -- chown

# error = chown(string, owner);

	chown = 16
.globl	_chown

_chown:
	pea	chown
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
