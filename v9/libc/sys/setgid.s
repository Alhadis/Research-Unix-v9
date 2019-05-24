# C library -- setgid

# error = setgid(uid);

	setgid = 46
.globl	_setgid
.globl  cerror

_setgid:
	pea	setgid
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
