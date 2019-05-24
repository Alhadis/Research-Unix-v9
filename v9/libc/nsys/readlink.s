# C library -- readlink

# nread = readlink(name, buf, count)
#

	readlink = 58
.globl	_readlink
.globl  cerror

_readlink:
	linkw	a6,#0
	pea	readlink
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
