# C library -- mmap

	mmap = 64+44
.globl	_mmap
.globl  cerror

_mmap:
	pea	mmap
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
