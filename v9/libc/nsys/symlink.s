# C library -- symlink

# symlink(target, linkname);
#

	symlink = 57
.globl	_symlink
.globl  cerror

_symlink:
	linkw	a6,#0
	pea	symlink
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
