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
	pea	dup
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
