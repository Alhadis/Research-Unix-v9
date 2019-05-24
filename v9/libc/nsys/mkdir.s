# C library -- mkdir

# error = mkdir(string, mode);

	mkdir = 65
.globl	_mkdir

_mkdir:
	linkw	a6,#0
	pea	mkdir	
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
