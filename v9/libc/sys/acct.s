# C library -- acct

	acct = 51
.globl	_acct
.globl  cerror

_acct:
	pea	acct
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	rts
