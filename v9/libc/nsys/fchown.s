# C library -- fchown

# error = fchown(fd, owner);

	fchown = 32
.globl	_fchown

_fchown:
	linkw	a6,#0
	pea	fchown
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
