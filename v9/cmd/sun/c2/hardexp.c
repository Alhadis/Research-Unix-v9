#ifndef lint
static	char sccsid[] = "@(#)hardexp.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

extern struct oper * newoperand();
extern struct optree * newtree();

void
ocomplex( operation, subop1, subop2 )
    char operation;
    struct oper * subop1, *subop2;
{
    register struct optree *otp;
    otp = newtree();
    otp->left_t = newoperand( subop1 );
    otp->right_t = subop2 ? newoperand( subop2 ) : NULL;
    otp->op_t = operation;
    subop1->sym_o = (struct sym_bkt *)otp;
    subop1->flags_o |= O_COMPLEX;
}


static void
valcomplex( op )
    struct oper *op;
{
    register struct optree *tp;
    register struct oper *r, *l;
    register int * vp;
    struct sym_bkt *sp;
    int operation;
    vp = (op->type_o == T_INDEX) ? &op->disp_o : &op->value_o;
    if (op->flags_o&O_COMPLEX){
	tp = (struct optree *)op->sym_o;
	operation = tp->op_t;
	l = tp->left_t ;
	valcomplex( l );
	if ((r=tp->right_t) == NULL)
	    operation = -operation; /* so we can tell unary SUB from binary */
	else 
	    valcomplex( r );
	if (l->sym_o || (r && r->sym_o)) {
	    if ( operation == SUB
	    && !(l->flags_o&O_COMPLEX) && !(r->flags_o&O_COMPLEX) 
	    && l->sym_o->csect_s == r->sym_o->csect_s
	    && l->sym_o->csect_s != C_TEXT
	    && l->sym_o->attr_s&S_DEF && r->sym_o->attr_s&S_DEF
	    && l->sym_o->value_s >= 0 && r->sym_o->value_s >= 0 ){
		/* 
		 * special case: it is ok to subtract two symbols in data
		 * space, as long as we know their values.
		 */
		*vp = l->sym_o->value_s - r->sym_o->value_s;
		goto uncomplex;
	    }
	    return;
	}
	switch( operation ){
	case ADD: *vp = l->value_o + r->value_o; break;
	case SUB: *vp = l->value_o - r->value_o; break;
	case MUL: *vp = l->value_o * r->value_o; break;
	case DIV: *vp = l->value_o / r->value_o; break;
	case -SUB: *vp = - l->value_o; break;
	case -NOT: *vp = ~ l->value_o; break;
	}
    uncomplex:
	op->flags_o &= ~O_COMPLEX;
	op->sym_o = NULL;
	if (op->type_o == T_INDEX
	  && !(op->flags_o&(O_INDIRECT|O_BSUPRESS|O_PREINDEX|O_POSTINDEX))
	  && op->disp_o >= -32768 && op->disp_o <= 32767) {
	    /* a T_DISPL disguised as a T_INDEX */
	    op->type_o = T_DISPL;
	    op->value_o = op->disp_o;
	    op->disp_o = 0;
	}
	freeoperand(l);
	freeoperand(r);
	freetree(tp);
    } else {
	if ( op->sym_o == NULL ) return ;
	sp = op->sym_o;
	if ((sp->attr_s & S_DEF) == 0) return ;
	if ( sp->csect_s == C_UNDEF  ) {
	    *vp += sp->value_s;
	    op->sym_o = NULL;
	}
    }
    return ;
}

    
void
rectify()
{
    /* look at all operands. Try to fix up the complex ones */
    register NODE *np;
    register struct oper *op;
    register struct optree *tp;
    register opno, nops;
    struct sym_bkt * sp;
    short m;

    for (np=first.forw; np!= &first; np = np->forw){
	if (np->op==OP_LABEL){
	    np->nref = np->name->nuse_s;
	    continue;
	}
	nops = np->nref;
	for (opno=0; opno<nops; opno++){
	    op = np->ref[opno];
	    if (op->flags_o&O_COMPLEX || op->sym_o)
		valcomplex( op );
	}
	switch( np->op){
	case OP_BRANCH:
	case OP_DBRA:
	    if (op->type_o==T_NORMAL &&
	    !(op->flags_o&O_COMPLEX) && op->value_o==0 ){
	    /* make jump references point right at the label node */
		sp = op->sym_o;
		if (sp->attr_s&S_DEF && sp->csect_s == C_TEXT){
		    np->luse = sp->where_s;
		    np->lnext = np->luse->luse;
		    np->luse->luse = np;
		    switch( np->op){
		    case OP_BRANCH: np->op = OP_JUMP; break;
		    case OP_DBRA:   np->op = OP_DJMP; break;
		    }
		    freeoperand(op);
		    np->nref--;
		    continue;
		}
	    }
	    break;
	case OP_MOVEM:
	    /* try to fix up register use mask */
	    np->ruse = addmask( np->ruse, movemmask( np->subop, RMASK, np->ref ));
	    np->rset = addmask( np->rset, movemmask( np->subop, WMASK, np->ref ));
	    break;
	case OP_MOVE:
	    /*
	     * look for set pattern:
	     *      movw	pc@(6,d?:w),d0
	     *      jmp	pc@(2,d0:w)
	     * Lnnn:
	     *	.word	Lyyy-Lnnn
	     *	  :         :
	     *	  :	    :
	     * This is a C-style switch.
	     * When we find one of these, we want to treat it specially,
	     * so that flow-analysis will recognize it as a multi-way 
	     * branch, and not as an unknown branch. So:
	     *    Make the jmp instruction a OP_JUMP, JIND op/subop;
	     *    Make the .word pseudoops OP_CSWITCH, and make them
	     *        reference Lyyy like branches reference their labels.
	     */
	    /* make sure we recognize the set pattern */
	    if (np->subop!=SUBOP_W 
	    || (op=np->ref[0])->type_o!= T_INDEX || op->reg_o != PCREG
	    ||  op->disp_o!=6  || op->value_o>=A0REG 
	    || (op=np->ref[1])->type_o!= T_REG || op->value_o != D0REG)
		    break;
	    np = np->forw;
	    if (np->op!=OP_BRANCH || (op=np->ref[0])->type_o!=T_INDEX
	    ||  op->reg_o!=PCREG || op->disp_o!=2 || op->value_o!=D0REG
	    ||  op->flags_o!=(O_WINDEX|O_PREINDEX) )
		    break;
	    /* following operand must be label */
	    if (np->forw->op!=OP_LABEL)
		    break;
	    /* all looks good: fiddle the jmp instruction */
	    np->op = OP_JUMP;
	    np->subop = JIND;
	    freeoperand( op );
	    newreference( np->forw, np);
	    np->nref=0;
	    np = np->forw;
	    np->nref += np->name->nuse_s;
	    np= np->forw;
	    while (np->op==OP_WORD){
		int i;
		register NODE *csw;
		NODE *next;
		np->op = OP_CSWITCH;
		next = np->forw;
		for (i=0; i<np->nref; i++){
		    op = np->ref[i];
		    if (!(op->flags_o&O_COMPLEX)) continue; /* trouble */
		    /* CSWITCH can handle only one operand */
		    if (i>0){
			/* this will never happen */
			csw = new();
			csw->back = next->back;
			csw->back->forw = next->back = csw;
			csw->forw=next;
			csw->op = OP_CSWITCH;
			csw->instr = np->instr;
			csw->nref = 0;
		    } else {
			csw = np;
		    }
		    tp = (struct optree *)op->sym_o;
		    freeoperand( tp->right_t );
		    csw->luse = tp->left_t->sym_o->where_s;
		    csw->lnext = csw->luse->luse;
		    csw->luse->luse = csw;
		    freeoperand( tp->left_t );
		    freetree( tp );
		}
		np->nref = 0;
		np = next;
	    }
	    np=np->back; /* so the for-loop increment will do the right thing */
	    break;
	}
    }
    if (first.forw != first.back){
	/*
	 * if this routine is non-empty, then we want to make sure that
	 * its entry-point has at least one reference. Most routines get
	 * this through the .globl pseudo-op, but class static routine
	 * will not. Thus we make sure here.
	 */
	for (np=first.forw; np != &first ; np=np->forw )
	    if (np->op == OP_LABEL ){
		np->nref++;
		break;
	    }
    }
}
