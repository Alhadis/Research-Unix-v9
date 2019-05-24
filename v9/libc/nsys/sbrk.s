#old = sbrk(increment);
#
#sbrk gets increment more core, and returns a pointer
#	to the beginning of the new core area
#
	break = 17
.globl	_sbrk
.globl _end
.globl  cerror

_sbrk:
	linkw	a6,#0
	addql	#3,sp@(8)
	andl	#0xfffffffc,sp@(8)
	movl	nd,d0
	addl	sp@(8),d0
	movl	d0,sp@-
	jsr	1f
	bcc 	noerr1
	addql	#4,sp
	jmp 	cerror2
noerr1:
	addql	#4,sp
	movl	nd,d0
	movl	sp@(8),d1
	addl	d1,nd
	movl	d0,a0
	unlk	a6
	rts
1:
	linkw	a6,#0
	pea	break
	trap	#0
	unlk	a6
	rts

.globl	_brk
# brk(value)
# as described in man2.
# returns 0 for ok, -1 for error.

_brk:
	linkw	a6,#0
	pea	break
	trap	#0
	bcc 	noerr2
	jmp 	cerror2
noerr2:
	movl	sp@(8),nd
	clrl	d0
	lea	0,a0
	unlk	a6
	rts

	.data
nd:	.long	_end
