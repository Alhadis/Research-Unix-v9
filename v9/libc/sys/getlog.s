	getlog = 64+3

	.globl	_getlogname
_getlogname:
	pea	0
1:
	movl	sp@(8),sp@-
	pea	getlog
	trap	#0
	bcc	noerror
	addql	#8,sp
	jmp	cerror
noerror:
	addql	#8,sp
	clrl	d0
	rts

	.globl	_setlogname
_setlogname:
	pea	1
	bra	1b
