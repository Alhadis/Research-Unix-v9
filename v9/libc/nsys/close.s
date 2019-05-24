# C library -- close

# error =  close(file);

	close = 6
.globl	_close

_close:
	linkw	a6,#0
	pea	close
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
