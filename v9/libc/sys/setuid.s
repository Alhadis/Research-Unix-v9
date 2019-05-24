# C library -- setuid

# error = setuid(uid);

	setuid = 23
.globl	_setuid
.globl  cerror

_setuid:
	pea	setuid
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
