	.data
	.asciz	"@(#)movc.s 1.1 86/02/03 Copyr 1984 Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

#include "../machine/asm_linkage.h"

| Copy a block of storage - must not overlap ( from + len <= to)
| Usage: bcopy(from, to, count)
	ENTRY(bcopy)
	movl	sp@(4),a0	| from
	movl	sp@(8),a1	| to
	movl	sp@(12),d0	| get count
	jle	ret		| return if not positive
| If from address odd, move one byte to 
| try to make things even
	movw	a0,d1		| from
	btst	#0,d1		| test for odd bit in from
	jeq	1$		| even, skip
	movb	a0@+,a1@+	| move a byte
	subql	#1,d0		| adjust count
| If to address is odd now, we have to do byte moves
1$:	movl	a1,d1		| low bit one if mutual oddness
	btst	#0,d1		| test low bit
	jne	bytes		| do it slow and easy
| The addresses are now both even 
| Now move longs
	movl	d0,d1		| save count
	lsrl	#2,d0		| get # longs
	jra	3$		| enter long move loop
| The following loop runs in loop mode on 68010
2$:	movl	a0@+,a1@+	| move a long
3$:	dbra	d0,2$		| decr, br if >= 0
| Now up to 3 bytes remain to be moved
	movl	d1,d0		| restore count
	andl	#3,d0		| mod sizeof long
	jra	bytes		| go do bytes

| Here if we have to move byte-by-byte because
| the pointers didn't line up.  68010 loop mode is used.
bloop:	movb	a0@+,a1@+	| loop mode byte moves
bytes:	dbra	d0,bloop
ret:	rts

| Block copy with possibly overlapped operands
	ENTRY(ovbcopy)
	movl	sp@(4),a0	| from
	movl	sp@(8),a1	| to
	movl	sp@(12),d0	| get count
	jle	ret		| return if not positive
	cmpl	a0,a1		| check direction of copy
	jgt	bwd		| do it backwards
| Here if from > to - copy bytes forward
	jra	2$
| Loop mode byte copy
1$:	movb	a0@+,a1@+
2$:	dbra	d0,1$
	rts
| Here if from < to - copy bytes backwards
bwd:	addl	d0,a0		| get end of from area
	addl	d0,a1		| get end of to area
	jra	2$		| enter loop
| Loop mode byte copy
1$:	movb	a0@-,a1@-
2$:	dbra	d0,1$
	rts


| Zero block of storage
| Usage: bzero(addr, length)
	ENTRY2(bzero,blkclr)
	movl	sp@(4),a1	| address
	movl	sp@(8),d0	| length
	clrl	d1		| use zero register to avoid clr fetches
	btst	#0,sp@(7)	| odd address?
	jeq	1$		| no, skip
	movb	d1,a1@+		| do one byte
	subql	#1,d0		| to adjust to even address
1$:	movl	d0,a0		| save possibly adjusted count
	lsrl	#2,d0		| get count of longs
	jra	3$		| go to loop test
| Here is the fast inner loop - loop mode on 68010
2$:	movl	d1,a1@+		| store long
3$:	dbra	d0,2$		| decr count; br until done
| Now up to 3 bytes remain to be cleared
	movl	a0,d0		| restore count
	btst	#1,d0		| need a short word?
	jeq	4$		| no, skip
	movw	d1,a1@+		| do a short
4$:	btst	#0,d0		| need another byte
	jeq	5$		| no, skip
	movb	d1,a1@+		| do a byte
5$:	rts			| all done
