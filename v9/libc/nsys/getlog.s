	getlog = 64+3

	.globl	_getlogname
_getlogname:
	linkw	a6,#0
	pea	0
1:
	movl	sp@(12),sp@-
	jsr	2f
	addql	#8,sp
	unlk	a6
	rts
2:
	linkw	a6,#0
	pea	getlog
	trap	#0
	bcc	noerror
noerror:
	clrl	d0
	unlk	a6
	rts
	
	.globl	_setlogname
_setlogname:
	linkw	a6,#0
	pea	1
	bra	1b
