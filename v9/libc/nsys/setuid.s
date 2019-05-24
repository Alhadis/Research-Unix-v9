# C library -- setuid

# error = setuid(uid);

	setuid = 23
.globl	_setuid
.globl  cerror

_setuid:
	linkw	a6,#0
	pea	setuid
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
