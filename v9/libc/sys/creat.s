# C library -- creat

# file = creat(string, mode);
#
# file == -1 if error

	creat = 8
.globl	_creat

_creat:
	pea	creat
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
