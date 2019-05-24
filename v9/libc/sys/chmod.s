# C library -- chmod

# error = chmod(string, mode);

	chmod = 15
.globl	_chmod

_chmod:
	pea	chmod
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
