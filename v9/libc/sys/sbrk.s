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
	addql	#3,sp@(4)
	andl	#0xfffffffc,sp@(4)
	movl	nd,d0
	addl	sp@(4),d0
	movl	d0,sp@-
	jsr	1f
	bcc 	noerr1
	addql	#4,sp
	jmp 	cerror2
noerr1:
	addql	#4,sp
	movl	nd,d0
	movl	sp@(4),d1
	addl	d1,nd
	movl	d0,a0
	rts
1:
	pea	break
	trap	#0
	rts

.globl	_brk
# brk(value)
# as described in man2.
# returns 0 for ok, -1 for error.

_brk:
	pea	break
	trap	#0
	bcc 	noerr2
	jmp 	cerror2
noerr2:
	movl	sp@(4),nd
	clrl	d0
	lea	0,a0
	rts

	.data
nd:	.long	_end
