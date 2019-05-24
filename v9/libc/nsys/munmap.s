# C library -- munmap

	munmap = 64+45
.globl	_munmap
.globl  cerror

_munmap:
	linkw	a6,#0
	pea	munmap
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
