# C library -- acct

	acct = 51
.globl	_acct
.globl  cerror

_acct:
	linkw	a6,#0
	pea	acct
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	unlk	a6
	rts
