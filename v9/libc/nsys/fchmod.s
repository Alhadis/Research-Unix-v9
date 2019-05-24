
# C library -- chmod

# error = fchmod(fd, mode);

	fchmod = 31
.globl	_fchmod

_fchmod:
	linkw	a6,#0
	pea	fchmod
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
