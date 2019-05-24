# C library -- setruid

# error = setruid(uid);

	setruid = 56
.globl	_setruid
.globl  cerror

_setruid:
	pea	setruid
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
