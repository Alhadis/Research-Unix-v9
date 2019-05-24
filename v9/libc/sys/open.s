# C library -- open

# file = open(string, mode)
#
# file == -1 means error

	open = 5
.globl	_open
.globl  cerror

_open:
	pea	open
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
