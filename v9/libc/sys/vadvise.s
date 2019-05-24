# C library -- vadvise

# error =  vadvise(how);

	vadvise = 64+8
.globl	_vadvise

_vadvise:
	pea	vadvise
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
