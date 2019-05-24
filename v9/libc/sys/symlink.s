# C library -- symlink

# symlink(target, linkname);
#

	symlink = 57
.globl	_symlink
.globl  cerror

_symlink:
	pea	symlink
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
