# C library -- lstat
#   (stat without link following)

# lstat(name, &statb);
#

	lstat = 40
.globl	_lstat
.globl  cerror

_lstat:
	linkw	a6,#0
	pea	lstat
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
