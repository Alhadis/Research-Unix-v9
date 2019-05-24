# C library -- kill

	kill = 37
.globl	_kill
.globl cerror

_kill:
	pea	kill
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	clrl	d0
	rts
