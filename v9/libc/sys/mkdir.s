# C library -- mkdir

# error = mkdir(string, mode);

	mkdir = 65
.globl	_mkdir

_mkdir:
	pea	mkdir	
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
