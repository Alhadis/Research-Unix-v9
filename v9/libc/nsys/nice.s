# C library-- nice

# error = nice(hownice)

	nice = 34
.globl	_nice
.globl  cerror

_nice:
	linkw	a6,#0
	pea	nice
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
