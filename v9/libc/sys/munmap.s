# C library -- munmap

	munmap = 64+45
.globl	_munmap
.globl  cerror

_munmap:
	pea	munmap
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
