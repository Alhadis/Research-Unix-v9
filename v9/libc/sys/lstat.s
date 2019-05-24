# C library -- lstat
#   (stat without link following)

# lstat(name, &statb);
#

	lstat = 40
.globl	_lstat
.globl  cerror

_lstat:
	pea	lstat
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
