/*      @(#)map.s 1.1 86/02/03 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Memory Mapping and Paging on the Sun-3.
 * NOTE:  All of these routines assume that the
 * default sfc and dfc have been preset to FC_MAP.
 */

#include "../h/param.h"
#include "../machine/asm_linkage.h"
#include "../machine/mmu.h"
#include "../machine/pte.h"

	.text
	|
	| Sets the pte Referenced and Modified bits based on the
	| pme for page pageno and then clears the these bits in the pme.
	|
	| unloadpgmap(pageno, pte)
	| u_int pageno;
	| struct pte* pte;
	ENTRY(unloadpgmap)
	movl	sp@(8),a1
	movl	a1@,d0			| valid?
	bge	0f			| no, just return
	movl	sp@(4),d0		| get first parameter
	moveq	#PGSHIFT,d1		| set of count for shift
	lsll	d1,d0			| convert to address
	orl	#PAGEBASE,d0		| set to page map base offset
	movl	d0,a0			| move into a0
	movsl	a0@,d0			| read page map entry
	movl	d0,d1
	bge	0f			| skip if invalid
	andl	#(PG_R+PG_M),d0		| save Reference and Modified bits
	orl	d0,a1@			| add these bits into software pte
	andl	#~(PG_R+PG_M),d1	| turn off Reference and Modified bits
	movsl	d1,a0@			| put new page map entry back
0:
	rts				| done

	|
	| Read the page map entry for the given address v
	| and return it in a form suitable for software use.
	|
	| long
	| getpgmap(v)
	| caddr_t v;
	ENTRY(getpgmap)
	movl	sp@(4),d0		| get access address
	andl	#PAGEADDRBITS,d0	| clear extraneous bits
	orl	#PAGEBASE,d0		| set to page map base offset
	movl	d0,a0
	movsl	a0@,d0			| read page map entry
					| no mods needed to make pte from pme
	rts				| done

	|
	| Set the pme for address v using the software pte given.
	|
	| setpgmap(v, pte)
	| caddr_t v;
	| long pte;
	ENTRY(setpgmap)
	movl	sp@(4),d0		| get access address
	andl	#PAGEADDRBITS,d0	| clear extraneous bits
	orl	#PAGEBASE,d0		| set to page map base offset
	movl	d0,a0
	movl	sp@(8),d0		| get page map entry to write
					| no mods need to make pme from pte
	movsl	d0,a0@			| write page map entry
	rts				| done

	|
	| Load the pme for page pageno using the pte given.
	| If the original pme is valid and we are not on
	| a new pmeg, then the R and M bits are preserved
	| from the original pme.
	|
	| loadpgmap(pageno, pte, new)
	| u_int pageno;		/* virtual page to set up */
	| struct pte *pte;	/* pointer to pte to use */
	| int new;		/* new pmeg flag */
	ENTRY(loadpgmap)
	movl	sp@(4),d0		| get first parameter
	moveq	#PGSHIFT,d1		| set of count for shift
	lsll	d1,d0			| convert to address
	andl	#PAGEADDRBITS,d0	| clear extraneous bits
	orl	#PAGEBASE,d0		| set to page map base offset
	movl	d0,a1
	movl	sp@(8),a0		| get address of page map entry to write
	movl	a0@,d0			| read old page map entry
	bge	0f			| skip if not valid
	tstl	sp@(12)			| new pmeg?
	bne	0f			| yes, don't save bits
	movsl	a1@,d1			| get old pme
	andl	#(PG_R+PG_M),d1		| mask off ref and mod bits
	orl	d1,d0			| or them into new pme
0:
	movsl	d0,a1@			| write page map entry
	rts				| done

	|
	| Return the 8 bit segment map entry for the given segment number.
	|
	| u_char
	| getsegmap(segno)
	| u_int segno;
	ENTRY(getsegmap)
	movl	sp@(4),d0		| get segment number
	moveq	#SGSHIFT,d1		| get count for shift
	lsll	d1,d0			| convert to address
	andl	#SEGMENTADDRBITS,d0	| clear extraneous bits
	orl	#SEGMENTBASE,d0		| set to segment map offset
	movl	d0,a0
	moveq	#0,d0			| clear (upper part of) register
	movsb	a0@,d0			| read segment map entry
	rts				| done

	|
	| Set the segment map entry for segno to pm.
	|
	| setsegmap(segno, pm)
	| u_int segno;
	| u_char pm;
	ENTRY(setsegmap)
	movl	sp@(4),d0		| get segment number
	moveq	#SGSHIFT,d1		| get count for shift
	lsll	d1,d0			| convert to address
	andl	#SEGMENTADDRBITS,d0	| clear extraneous bits
	orl	#SEGMENTBASE,d0		| set to segment map offset
	movl	d0,a0
	movl	sp@(8),d0		| get seg map entry to write
	movsb	d0,a0@			| write segment map entry
	rts				| done

	|
	| Return the current [user] context number.
	|
	| int
	| getcontext()
	ENTRY2(getcontext,getusercontext)
	movsb	CONTEXTBASE,d0		| move context reg into result
	andl	#CONTEXTMASK,d0		| clear high-order bits
	rts				| done

	|
	| Set the current [user] context number to uc.
	|
	| setcontext(uc)
	| int uc;
	ENTRY2(setcontext,setusercontext)
	movb	sp@(7),d0		| get context value to set
	movsb	d0,CONTEXTBASE		| move value into context register
	rts				| done
