	.data
	.asciz	"@(#)vax.s 1.1 86/02/03 Copyr 1986 Sun Micro"
	.even
	.text
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Emulate VAX instructions on the 68020.
 */

#include "../h/param.h"
#include "../machine/asm_linkage.h"
#include "../machine/mmu.h"
#include "../machine/psl.h"
#include "assym.s"

/*
 * Macro to raise prio level,
 * avoid dropping prio if already at high level.
 * NOTE - Assumes that we are never in "master" mode.
 */
#define	RAISE(level)	\
	movw	sr,d0;	\
	andw	#(SR_SMODE+SR_INTPRI),d0; 	\
	cmpw	#(SR_SMODE+(/**/level*0x100)),d0;	\
	jge	0f;	\
	movw	#(SR_SMODE+(/**/level*0x100)),sr;	\
0:	rts

#define	SETPRI(level)	\
	movw	sr,d0;	\
	movw	#(SR_SMODE+(/**/level*0x100)),sr;	\
	rts

	ENTRY(splimp)
	RAISE(3)

	ENTRY(splnet)
	RAISE(1)

	ENTRY(splie)
	RAISE(3)

	ENTRY(splclock)
	RAISE(5)

	ENTRY(splzs)
	SETPRI(6)

	ENTRY(spl7)
	SETPRI(7)

	ENTRY2(spl6,spl5)
	SETPRI(5)

	ENTRY(spl4)
	SETPRI(4)

	ENTRY(spl3)
	SETPRI(3)

	ENTRY(spl2)
	SETPRI(2)

	ENTRY(spl1)
	SETPRI(1)

	ENTRY(spl0)
	SETPRI(0)

	ENTRY(splx)
	movw	sr,d0
	movw	sp@(6),sr
	rts

	ENTRY(insque)
	movl	sp@(4),a0
	movl	sp@(8),a1
	movl	a1@,a0@
	movl	a1,a0@(4)
	movl	a0,a1@
	movl	a0@,a1
	movl	a0,a1@(4)
	rts

	ENTRY(remque)
	movl	sp@(4),a0
	movl	a0@,a1
	movl	a0@(4),a0
	movl	a1,a0@
	movl	a0,a1@(4)
	rts

	ENTRY(scanc)
	movl	sp@(8),a0	| string
	movl	sp@(12),a1	| table
	movl	sp@(16),d1	| mask
	movl	d2,sp@-
	clrw	d2
	movl	sp@(8),d0	| len
	subqw	#1,d0		| subtract one for dbxx
	jmi	1f
2:
	movb	a0@+,d2		| get the byte from the string
	movb	a1@(0,d2:w),d2	| get the corresponding table entry
	andb	d1,d2		| apply the mask
	dbne	d0,2b		| check for loop termination
1:
	addqw	#1,d0		| dbxx off by one
	movl	sp@+,d2
	rts

/*
 * _whichqs tells which of the 32 queues _qs have processes in them
 * setrq puts processes into queues, remrq removes them from queues
 * The running process is on no queue, other processes are on a
 * queue related to p->p_pri, divided by 4 actually to shrink the
 * 0-127 range of priorities into the 32 available queues.
 */

/*
 * setrq(p), using semi-fancy 68020 instructions.
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
	ENTRY(setrq)
	movl	sp@(4),a0	| get proc pointer
	tstl	a0@(P_RLINK)	| firewall: p->p_rlink must be 0
	jne	1f
	btst	#SPROCIO_BIT-24,a0@(P_FLAG)	| if he's getting PROCIO'd,
	jne	3f		| we leave him alone
	moveq	#31,d1
	clrl	d0
	movb	a0@(P_PRI),d0	| get the priority
	lsrl	#2,d0		| divide by 4
	subl	d0,d1
	lea	_qs,a1
	movl	a1@(4,d1:l:8),a1| qs[d1].ph_rlink, 8 == sizeof (qs[0])
	movl	a1@,a0@		| insque(p, blah)
	movl	a1,a0@(4)
	movl	a0,a1@
	movl	a0@,a1
	movl	a0,a1@(4)
	bfset	_whichqs{d0:#1}	| set appropriate bit in whichqs
3:
	rts
1:
	pea	2f
	jsr	_panic

2:	.asciz	"setrq"
	.even

/*
 * remrq(p), using semi-fancy 68020 instructions
 *
 * Call should be made at spl6().
 */
	ENTRY(remrq)
	movl	sp@(4),a0
	btst	#SPROCIO_BIT-24,a0@(P_FLAG)	| if he's getting PROCIO'd,
	jne	rem2a				| we leave him alone
	clrl	d0	
	movb	a0@(P_PRI),d0
	lsrl	#2,d0		| divide by 4
	bftst	_whichqs{d0:#1}
	jeq	1f
	movl	a0@,a1		| remque(p);
	movl	a0@(4),a0
	movl	a1,a0@
	movl	a0,a1@(4)
	cmpl	a0,a1
	jne	2f		| queue not empty
	bfclr	_whichqs{d0:#1}	| queue empty, clear the bit
2:
	movl	sp@(4),a0
	clrl	a0@(P_RLINK)
rem2a:
	rts
1:
	pea	3f
	jsr	_panic

3:	.asciz	"remrq"
	.even

/*
 * swtch(), using semi-fancy 68020 instructions
 */
	ENTRY(swtch)
	movw	sr,sp@-		| save processor priority
	movl	#1,_noproc
	clrl	_runrun
	bclr	#AST_SCHED_BIT-24,_u+PCB_P0LR
2:
	bfffo	_whichqs{#0:#32},d0 | test if any bit on
	cmpl	#32,d0
	jne	3f		| found one
	stop	#SR_LOW		| must allow interrupts here

	.globl	idle
idle:				| wait here for interrupts
	jra	2b		| try again

3:
	movw	#SR_HIGH,sr	| lock out all so _whichqs == _qs
	moveq	#31,d1
	subl	d0,d1
	bfclr	_whichqs{d0:#1}
	jeq	2b		| proc moved via lbolt interrupt
	lea	_qs,a0
	movl	a0@(0,d1:l:8),a0| get qs[d1].ph_link = p = highest pri process
	movl	a0,d1		| save it
	movl	a0@,a1		| remque(p);
	cmpl	a0,a1		| is queue empty?
	jne	4f
8:
	pea	9f
	jsr	_panic
9:
	.asciz	"swtch"
	.even
4:
	movl	a0@(4),a0
	movl	a1,a0@
	movl	a0,a1@(4)
	cmpl	a0,a1
	jeq	5f		| queue empty
	bfset	_whichqs{d0:#1}	| queue not empty, set appropriate bit
5:
	movl	d1,a0		| restore p
	clrl	_noproc
	tstl	a0@(P_WCHAN)	|| firewalls
	jne	8b		||
	cmpb	#SRUN,a0@(P_STAT) ||
	jne	8b		||
	clrl	a0@(P_RLINK)	||
	addql	#1,_cnt+V_SWTCH
	movl	a0,sp@-
	jsr	_resume
	addqw	#4,sp
	movw	sp@+,sr		| restore processor priority
	rts


/*
 * fpprocp is the pointer to the proc structure whose external
 * state (i.e. registers) was last loaded into the 68881.
 */
	.data
	.globl	_fpprocp
_fpprocp:	.long 0			| struct proc *fpprocp;
	.text

/*
 * masterprocp is the pointer to the proc structure for the currently
 * mapped u area.  It is used to set up the mapping for the u area
 * by the debugger since the u area is not in the Sysmap.
 */
	.data
	.globl	_masterprocp
_masterprocp:	.long	0		| struct proc *masterprocp;
	.text

/*
 * resume(p)
 *
 * Assumes that there is only one real page to worry about (UPAGES = 1),
 * that the kernel stack starts below the u area in the middle of
 * a page, that the redzone is below that and is always marked
 * for no access across all contexts, the default sfc and dfs are
 * set to FC_MAP, and that we are running on the u area kernel stack
 * when we are called.
 */
SAVREGS = 0xFCFC
	ENTRY(resume)
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch if not
	fsave	_u+U_FP_ISTATE		| save internal state
	tstw	_u+U_FP_ISTATE		| test for null state
	jeq	1f			| branch if so
	fmovem	fpc/fps/fpi,_u+U_FPS_CTRL | save control registers
	fmovem	fp0-fp7,_u+U_FPS_REGS	| save fp data registers
	movl	_masterprocp,_fpprocp	| remember whose regs are still loaded
1:
	movl	sp@,_u+PCB_REGS		| save return pc in pcb
	movl	sp@(4),a0		| a0 contains proc pointer
	movw	sr,_u+PCB_SR+2		| save the current psw in u area
	orw	#SR_INTPRI,sr		| mask interrupts
	moveml	#SAVREGS,_u+PCB_REGS+4	| save data/address regs
	movl	a0,_masterprocp		| set proc pointer for new process
	lea	eintstack,sp		| use the interrupt stack
	moveq	#KCONTEXT,d0
	movsb	d0,CONTEXTBASE		| invalidate context
	lea	U_MAPVAL,a1	| a1 has address used to access pme for u area
	movl	a0@(P_ADDR),a2	| get p_addr (address of pte's), a2 is scratch
	movl	a2@,d0			| get u pte
	movsl	d0,a1@			|   and set pme
/*
 * Check to see if we already have context.  If so and
 * SPTECHG bit is not on then set up the next context.
 */
	tstl	a0@(P_CTX)		| check p->p_ctx
	jeq	1f			| if zero, skip ahead
	btst	#SPTECHG_BIT-24,a0@(P_FLAG)	| check (p->p_flag & SPTECHG)
	jne	1f			| if SPTECHG bit is on, skip ahead
	movl	a0@(P_CTX),a1
	movw	a1@(CTX_CONTEXT),d0	| get context number in d0
	movsb	d0,CONTEXTBASE		| set up the context
1:
	tstw	_fppstate		| is fpp present and enabled?
	jle	1f			| branch if not
	tstw	_u+U_FP_ISTATE		| test for null state
	jeq	0f			| branch if so
	cmpl	_fpprocp,a0		| check if we were last proc using fpp
	beq	0f			| if so, jump and skip loading ext regs
	fmovem	_u+U_FPS_REGS:w,fp0-fp7	| restore fp data registers
	fmovem	_u+U_FPS_CTRL,fpc/fps/fpi | restore control registers
0:
	frestore _u+U_FP_ISTATE		| restore internal state
1:
	moveml	_u+PCB_REGS+4,#SAVREGS	| restore data/address regs
| Note: we just changed stacks
	movw	_u+PCB_SR+2,sr
	tstl	_u+PCB_SSWAP
	jeq	1f
	movl	_u+PCB_SSWAP,sp@(4)	| blech...
	clrl	_u+PCB_SSWAP
	movw	#SR_LOW,sr
	jmp	_longjmp
1:
	rts
