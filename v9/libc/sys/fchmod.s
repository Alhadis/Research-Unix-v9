
# C library -- chmod

# error = fchmod(fd, mode);

	fchmod = 31
.globl	_fchmod

_fchmod:
	pea	fchmod
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
