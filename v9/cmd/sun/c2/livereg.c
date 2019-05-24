
#ifndef lint
static	char sccsid[] = "@(#)livereg.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

#define MAXLIVE 10000
NODE *heap_o_nodes[MAXLIVE];
NODE **nextnode = heap_o_nodes;
int nlive = 0;

static void
pushlive( m )
    NODE *m;
{
    if (++nlive>MAXLIVE) sys_error("nursury overflow\n");
    *(nextnode++) = m;
}

static NODE *
poplive()
{
    if (nlive<=0) return (NODE *)NULL;
    nlive--;
    return *(--nextnode);
}

#define killreg(m) (m)

regmask
compute_meet( mp, ll, lr )
    NODE *mp;
    regmask ll, lr;
{
    /* the liveset compuation of a "meet" node: conditional branch */
    /* return ((ll|lr)&~killreg(mp->rset))|mp->ruse; */
    return (addmask(submask(addmask(ll, lr) , killreg(mp->rset)), mp->ruse));
}

regmask
compute_indirect( mp )
    NODE *mp;
{
    register NODE *np = mp->forw->forw;
    regmask r; 

    r  = regmask0;
    while (np->op==OP_CSWITCH){
	r = addmask( r, np->rlive);
	np=np->forw;
    }
    /* return (r&~killreg(mp->rset)) | mp->ruse; */
    return ( addmask( submask( r, killreg(mp->rset) ), mp->ruse ));
}

regmask
compute_normal( mp, ll )
    NODE *mp;
    regmask ll;
{
    /* the liveset compuattion of a normal, boring instruction */
    /* return (ll & ~killreg(mp->rset)) | mp->ruse ; */
    return ( addmask( submask( ll, killreg(mp->rset)), mp->ruse));
}

livereg(){
    /*
     * Each node in the program has a field called rlive; this is where
     * we keep track of registers and pieces of registers that will
     * be used before they're next written. This set represents the state
     * of the registers BEFORE entering the node.
     * Here's our strategy:
     *  A) find all the return nodes: they kill all register except d0/d1.
     *     put them on the stack. as well as unconditional jumps ( this gets
     *     us into all loops, and will get better when we get smarter. )
     *  B) if the stack is empty, we're done; else take a node off the stack.
     *  C) look at the predecessor node. It is one of these:
     *    i) a label: has the same liveset as its successor. Find all of the
     *       uses of the label we can, and for each, compute:
     *       a) if an unconditional branch, then liveset  is same as label's. If
     *          it is different, change it and put it on the stack. 
     *          (Stacking may be superfluous here...we'll see).
     *       b) a binary (conditional) branch. compute liveset as the product
     *          of the livesets of its successors, plus the registers it reads,
     *          less those it writes . If this set changes
     *          from previous value, put the node on the stack.
     *       c) A multiway branch: same as binary branch.
     *   ii) an unconditional branch: liveset does not depend on successor.
     *       go back to step B).
     *  iii) a conditional branch. Compute as i.b) above.  <If value 
     *       does not change , go back to step B and get a new node, 
     *       else continue.> <<-- this may be wrong.
     *   iv) a multiway branch: same as conditional branch.
     *    v) a straight-line instruction: liveset is successor liveset, plus
     *       registers it sets, less those it reads. continue back.
     * D) Loop on step (C) until we either stop because of a stable branch,
     *    ( see C.ii , C.iii), or until we fall off the beginning. Then
     *    go back to B.
     */

    register NODE * p, *puse, *csw;
    regmask l, newl;

    for (p=first.forw; p != &first; p=p->forw){
	/*
	 * recompute live sets starting from null sets --
	 * This is a last-minute kludge for 3.0, to patch over a
	 * problem that occurs in content() (failure to update
	 * live sets when a constant or memory reference is replaced
	 * by a register).  Remove the following assignment when the
	 * problem is fixed correctly.
	 */
	p->rlive = regmask0;
	if (p->op == OP_EXIT){
	    p->rlive = compute_normal( p, regmask_all );
	    p->rlive = submask( p->rlive, MAKERMASK( CCREG, LR) );
	    p->rlive = submask( p->rlive, MAKERMASK( FPCCREG, LR) );
	    p->rlive = submask( p->rlive, MAKERMASK( FP0REG, LR) );
	    p->rlive = submask( p->rlive, MAKERMASK( FP0REG+1, LR) );
	    p->rlive = submask( p->rlive, MAKERMASK( A0REG, LR) );
	    p->rlive = submask( p->rlive, MAKERMASK( A0REG+1, LR) );
	    pushlive( p );
	} else if (p->op==OP_JUMP && p->subop==JALL){
	    pushlive( p );
	} else if (p->op==OP_BRANCH){
	    p->rlive = regmask_all; /* don't know -- must consider all live */
	    pushlive( p );
	}
    }

    while ((p=poplive())!=NULL){
	l = p->rlive; 
	while( (p=p->back) != &first ){
	    switch (p->op){
	    case OP_LABEL:
		p->rlive = l;
		for (puse=p->luse; puse ; puse=puse->lnext){
		    /* its a jump */
		    if (puse->op ==OP_CSWITCH){
			/* part of a multi-way branch */
			if (!samemask( puse->rlive, l )){
			    csw = puse;
			    puse->rlive=l;
			    while (csw->op != OP_JUMP)
				csw=csw->back;
			    /* now found indirect jump */
			    newl = compute_indirect(csw);
			    if (!samemask( csw->rlive , newl) ){
				csw->rlive = newl;
				pushlive( csw );
			    }
			}
			continue;
		    } else if (puse->subop == JALL){
			newl = l;
		    } else {
			newl = compute_meet( puse, puse->forw->rlive, l);
		    }
		    if (!samemask( puse->rlive , newl) ){
			puse->rlive = newl;
			pushlive( puse );
		    }
		}
		continue;
	    case OP_JUMP:
	    case OP_DJMP:
		if (p->subop == JALL)
		    goto popnew;
		else if (p->subop == JIND){
		    newl = compute_indirect( p );
		}else{
		    newl = compute_meet( p,p->forw->rlive, p->luse->rlive );
		}
		p->rlive = l = newl;
		continue;
	    case OP_CSWITCH:
		goto popnew;
	    case OP_BRANCH:
		/* know nothing about its behavior--guess the worst*/
		l = regmask_all;
		continue;
	    case OP_EXIT:
		goto popnew;
	    default:
		l = p->rlive = compute_normal( p, l);
		continue;
	    }
	}
    popnew:;
    }
}
