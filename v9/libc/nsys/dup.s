# C library -- dup

#	f = dup(of [ ,nf])
#	f == -1 for error

	dup = 41
.globl	_dup
.globl	_dup2
.globl	cerror

_dup2:
	bset	#6,sp@(7)
_dup:
	linkw	a6,#0
	pea	dup
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
