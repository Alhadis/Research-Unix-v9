# C library -- kill

	kill = 37
.globl	_kill
.globl cerror

_kill:
	linkw	a6,#0
	pea	kill
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	unlk	a6
	rts
