# C library -- read

# nread = read(file, buffer, count);
#
# nread ==0 means eof; nread == -1 means error

	read = 3
.globl	_read
.globl  cerror

_read:
	linkw	a6,#0
	pea	read
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
