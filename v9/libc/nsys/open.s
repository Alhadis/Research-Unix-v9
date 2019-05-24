# C library -- open

# file = open(string, mode)
#
# file == -1 means error

	open = 5
.globl	_open
.globl  cerror

_open:
	linkw	a6,#0
	pea	open
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
