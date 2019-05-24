# C library -- write

# nwritten = write(file, buffer, count);
#
# nwritten == -1 means error

	write = 4
.globl	_write
.globl  cerror

_write:
	linkw	a6,#0
	pea	write
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
