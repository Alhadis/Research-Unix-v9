# C library -- write

# nwritten = write(file, buffer, count);
#
# nwritten == -1 means error

	write = 4
.globl	_write
.globl  cerror

_write:
	pea	write
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
