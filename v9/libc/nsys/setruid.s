# C library -- setruid

# error = setruid(uid);

	setruid = 56
.globl	_setruid
.globl  cerror

_setruid:
	linkw	a6,#0
	pea	setruid
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
