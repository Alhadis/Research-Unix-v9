# C library -- vadvise

# error =  vadvise(how);

	vadvise = 64+8
.globl	_vadvise

_vadvise:
	linkw	a6,#0
	pea	vadvise
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
