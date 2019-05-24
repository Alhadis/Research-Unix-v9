# C library -- mmap

	mmap = 64+44
.globl	_mmap
.globl  cerror

_mmap:
	linkw	a6,#0
	pea	mmap
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
