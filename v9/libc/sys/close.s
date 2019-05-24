# C library -- close

# error =  close(file);

	close = 6
.globl	_close

_close:
	pea	close
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
