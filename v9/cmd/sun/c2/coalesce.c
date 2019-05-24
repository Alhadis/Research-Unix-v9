
#ifndef lint
static	char sccsid[] = "@(#)coalesce.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

extern struct oper *newoperand();
extern struct ins_bkt *moveq;

#define previ( p ) { do p=p->back; while( ISDIRECTIVE(p->op) ); }
#define nexti( p ) { do p=p->forw; while( ISDIRECTIVE(p->op) ); }

#define ispushop(o) ((o)->type_o == T_PREDEC && (o)->value_o == SPREG)
#define ispopop(o) ((o)->type_o == T_POSTINC && (o)->value_o == SPREG)

extern int bytesize[];
#define BYTESIZE( s ) (bytesize[ (int)(s)])

/*
 * Can register r1 be replaced by a new register r2?
 * The answer is yes, provided that:
 *	1. all uses of r1 in (p) are explicit
 *	2. r1 and r2 are of the same type (a-reg, d-reg, or fp-reg)
 * Certain exceptions to rule 2 are permissible if both r1 and r2 are
 * a-regs or d-regs.
 */
int
can_replace(p, r1, r2)
    register NODE *p;
    register r1,r2;
{
    register struct oper *o;
    register i;
    int r1_found;

    /* search for explicit uses of r1 */
    r1_found = 0;
    for (i = 0; i < p->nref; i++) {
	o = p->ref[i];
	switch(o->type_o) {
	case T_REG:
	    if (o->value_o == r1) {
		r1_found++;
		if ((p->op == OP_MOVE || p->op == OP_ADD || p->op == OP_SUB)
		  && p->subop == SUBOP_L
		  && !inmask(CCREG, p->forw->rlive)
		  && !inmask(FPCCREG, p->forw->rlive)) {
		    int ok;
		    o->value_o = r2;
		    ok = operand_ok(p->instr, p->ref[0], p->ref[1], 0);
		    o->value_o = r1;
		    if (ok) continue;
		}
		if (reg_access[r1] == reg_access[r2]) {
		    continue;
		}
		return 0;
	    }
	    continue;
	case T_DEFER:
	case T_POSTINC:
	case T_PREDEC:
	    if (o->value_o == r1) {
		return 0;
	    }
	    continue;
	case T_DISPL:
	    if (o->reg_o == r1) {
		return 0;
	    }
	    continue;
	case T_INDEX:
	    if (o->reg_o == r1) {
		return 0;
	    }
	    if (o->value_o == r1) {
		/* a-regs and d-regs can be used interchangeably here */
		r1_found++;
	    }
	    continue;
	case T_REGPAIR:
	    if (o->reg_o == r1 || o->value_o == r1) {
		return 0;
	    }
	    continue;
	}
	if (o->flags_o&O_BFLD) {
	    if (o->flags_o&O_BFOREG && o->bfoffset_o == r1) {
		return 0;
	    }
	    if (o->flags_o&O_BFWREG && o->bfwidth_o == r1) {
		return 0;
	    }
	}
    }
    /*
     * we know that r1 is used or set by p,
     * but if r1_found == 0, we haven't found an
     * explicit use.  In this case we should bail out.
     */
    return(r1_found);
}

void
replace(p, r1, r2)
    register NODE *p;
    register r1, r2;
{
    register struct oper *o;
    register i;

    /* search for explicit uses of r1 */
    for (i = 0; i < p->nref; i++) {
	o = p->ref[i];
	switch(o->type_o) {
	case T_REG:
	case T_DEFER:
	case T_POSTINC:
	case T_PREDEC:
	    if (o->value_o == r1) {
		o->value_o = r2;
	    }
	    continue;
	case T_DISPL:
	    if (o->reg_o == r1) {
		o->reg_o = r2;
	    }
	    continue;
	case T_INDEX:
	case T_REGPAIR:
	    if (o->reg_o == r1) {
		o->reg_o = r2;
	    }
	    if (o->value_o == r1) {
		o->value_o = r2;
	    }
	    continue;
	}
	if (o->flags_o&O_BFLD) {
	    if (o->flags_o&O_BFOREG && o->bfoffset_o == r1) {
		o->bfoffset_o = r2;
	    }
	    if (o->flags_o&O_BFWREG && o->bfwidth_o == r1) {
		o->bfwidth_o = r2;
	    }
	    continue;
	} /* if */
    } /* for */
    installinstruct(p, p->instr);
} /* replace */

int
next_member(regtype, regset)
    int regtype;
    regmask regset;
{
    register regno;

    switch(regtype) {
    case AM_DREG:
	/* try for d-reg first */
	for(regno = D0REG; regno <= D7REG; regno++) {
	    if (inmask(regno,regset))
		return(regno);
	}
	for(regno = A0REG; regno < A6REG; regno++) {
	    if (inmask(regno,regset))
		return(regno);
	}
	break;
    case AM_AREG:
	/* try for a-reg first */
	for(regno = A0REG; regno < A6REG; regno++) {
	    if (inmask(regno,regset))
		return(regno);
	}
	for(regno = D0REG; regno <= D7REG; regno++) {
	    if (inmask(regno,regset))
		return(regno);
	}
	break;
    case AM_FREG:
	/* only another fp-reg will do */
	for(regno = FP0REG; regno <= FP7REG; regno++) {
	    if (inmask(regno,regset))
		return(regno);
	}
	break;
    }
    return(-1);
}

/*
 * look for a register that is not live anywhere in (b)..(n).
 * regtype is the type of register preferred (or required,
 * in the case of a floating point register). Returns -1 if
 * no suitable register is found.
 */
int
find_freereg(b, n, regtype)
    register NODE *b, *n;
    int regtype;
{
    register NODE *p,*bb;
    regmask liveset;
    register regno;

    bb = b->back;
    liveset = n->rlive;
    for (p = n; p != bb; p = p->back) {
	liveset = addmask(liveset, p->rlive);
    }
    liveset = addmask(liveset, p->rlive);
    return(next_member(regtype, notmask(liveset)));
}

/*
 * try to replace register r1 with register r2 in the
 * set of nodes (b)..(n) (assumed to be a basic block).
 * r2 is assumed to be available in (b)..(n).
 * Returns 1 if the new assignment succeeds.
 * Otherwise returns 0 and leaves (b)..(n) unchanged.
 */
int
can_reassign_reg(b,n,r1,r2)
    register NODE *b,*n;
    register r1;
    register r2;
{
    register NODE *p,*bb;
    regmask l;

    if (!inmask(r1, b->rlive) && !inmask(r1, n->forw->rlive)) {
	/* r1 and r2 must have the same type or must both be a/d regs */
	if (reg_access[r1] != reg_access[r2]
	  && (!dareg(r1) || !dareg(r2))) {
	    return(0);
	}
	/* (b)..(n) contains the lifetime of r1 */
	bb = b->back;
	for (p = n; p != bb; p = p->back) {
	    if (ISINSTRUC(p->op)) {
	        if ((inmask(r1, p->ruse) || inmask(r1, p->rset))
		  && !can_replace(p, r1, r2)) {
		    return(0);
		}
	    }
	}
	return(1);
    }
    return(0);
} /* can_reassign_reg */

void
reassign_reg(b,n,r1,r2)
    register NODE *b,*n;
    register r1;
    register r2;
{
    register NODE *p,*bb;
    regmask l;

    l = n->forw->rlive;
    bb = b->back;
    for (p = n; p != bb; p = p->back) {
	if (ISINSTRUC(p->op)
	  && (inmask(r1, p->ruse) || inmask(r1, p->rset)))
	    replace(p, r1, r2);
	l = p->rlive = compute_normal(p, l);
    }
}


static struct ins_bkt *
move_inst(b,n,x,y)
    register NODE *b, *n;
    register struct oper *x, *y;
{
    if (operand_ok(b->instr, x, y, 0)) {
	return(b->instr);
    } else if (operand_ok(n->instr, x, y, 0)) {
	return(n->instr);
    } else {
	/* try special cases involving floating point */
	int Xisfp, Yisfp;
	Xisfp = Yisfp = 0;
	if (x->type_o==T_REG && freg(x->value_o))
	    Xisfp = 1;
	if (y->type_o==T_REG && freg(y->value_o))
	    Yisfp = 1;
	if (!Xisfp && !Yisfp
	  && !inmask(FPCCREG, n->forw->rlive)) {
	    /* rewrite as (b) mov{b,w,l} X,Y */
	    switch(b->subop) {
	    case SUBOP_B:
		return(sopcode("movb"));
	    case SUBOP_W:
		return(sopcode("movw"));
	    case SUBOP_L:
	    case SUBOP_S:
		return(sopcode("movl"));
	    default:
		/* if larger than a word, forget it */
		return(NULL);
	    }
	} else if (Xisfp && Yisfp) {
	    /* floating point reg-reg move */
	    return(sopcode("fmovex"));
	}
    }
    return(NULL);
}

/*
 * Is this instruction a reg-reg operation ?
 * To be safe, we require the instruction to use all of
 * both registers, with no data conversions.
 */
static int
reg_reg_op(p)
    register NODE *p;
{
    register struct oper *rm, *rn;

    if (p->nref == 2) {
	rm = p->ref[0]; rn = p->ref[1];
	if(rm != NULL && rm->type_o == T_REG && datareg(rm->value_o)
	  && rn != NULL && rn->type_o == T_REG && datareg(rn->value_o)) {
	    switch (p->subop) {
	    case SUBOP_L:
		return(dareg(rm->value_o) && dareg(rn->value_o));
	    case SUBOP_X:
		return(freg(rm->value_o) && freg(rn->value_o));
	    }
	}
    }
    return(0);
}

static int
iscommuteop(p)
    NODE *p;
{
    char *s;

    switch(p->op){
    case OP_ADD:
    case OP_AND:
    case OP_OR:
    case OP_EOR:
    	return(reg_reg_op(p));
    case OP_OTHER:
	/*
	 * careful: multiplication isn't really commutative
	 * unless it is also known to be reg-reg
	 */
	s = p->instr->text_i;
	if (!strcmp(s,"mulsl") || !strcmp(s,"mulul")
	  || !strcmp(s,"fmulx") || !strcmp(s, "faddx"))
	    return(reg_reg_op(p));
    }
    return(0);
}

/*
 * returns 1 if an instruction does implied data conversion
 * to match its destination operand (assumed to be the last operand).
 * In the 68xxx processors this occurs in cases involving address
 * registers or floating point registers.
 */
int
implied_conversion(p)
    register NODE *p;
{
    struct oper *o;

    if (p->nref > 0) {
	o = p->ref[p->nref-1];
	if (datareg_addr(o)) {
	    if (areg(o->value_o)) return (p->subop != SUBOP_L);
	    if (freg(o->value_o)) return (p->subop != SUBOP_X);
	}
    }
    return 0;
}

/*
 * coalesce register usage over subranges of a basic block,
 * eliminating unnecessary reg-reg moves.
 */
int
coalesce()
{
    register NODE *n;
    register NODE *p,*b;
    register struct oper *o;
    register struct ins_bkt *ip;
    struct oper *rm, *rn;
    NODE *q;
    register int regno;
    extern NODE *deletenode();
    int changes;
    regmask l;

    changes = 0;
    for (n=first.forw; n != &first; n=n->forw) {

	/*
	 * Pattern #1: given
	 * 	(b) move X,rn
	 *	    ....
	 * 	(n) move rn,Y
	 *
	 * rewrite #1: if possible, rewrite as:
	 *	(b) move X,Y
	 *	    ....
	 *	(n) (deleted)
	 *
	 * rewrite #2: if possible, rewrite as:
	 *	(b) (deleted)
	 *	    ....
	 *	(n) move X,Y
	 *
	 * requirements:
	 *	-- (b) and (n) must be moves of the same type
	 *	-- (n) must not do sign-extension or other data conversion
	 *	-- X must not be an immediate operand.
	 *	-- Y must be a legal replacement for rn in (b).
	 *	-- neither X nor Y can be a control register or i/o device
	 * 	-- rn must not be live at (n).
	 *	-- rn must be neither used nor set in the interim.
	 *	-- side effects of X and Y must not be used in the interim.
	 *	-- condition codes should not be live at either (b) or (n).
	 *
	 * for #1:
	 *	-- Y must not be used or modified in the interim.
	 *
	 * for #2:
	 *	-- X must not be modified in the interim.
	 */
	o = n->ref[0];
	if (n->op == OP_MOVE && o->type_o == T_REG && datareg(o->value_o)
	  && !implied_conversion(n)
	  && !inmask(o->value_o, n->forw->rlive)) {
	    struct oper *x, *y;
	    x = y = NULL;
	    regno = o->value_o;
	    b = n;
	    do {
		previ(b);
		if (b->op == OP_FIRST || b->op == OP_LABEL
		  || ISBRANCH(b->op)) {
		    /* flow of control to reach this point is unknown */
		    break;
		}
		if (b->op == OP_MOVE && b->subop == n->subop
		  && b->ref[1]->type_o == T_REG
		  && b->ref[1]->value_o == o->value_o) {
		    /* have move x,rn;...;move rn,y */
		    x = b->ref[0];
		    y = n->ref[1];
		    /* do checks on x,y */
		    if (x->type_o == T_IMMED 
		      || x->type_o == T_REG && !datareg(x->value_o)
		      || y->type_o == T_REG && !datareg(y->value_o)
		      || inmask(CCREG, b->forw->rlive)
		      || inmask(CCREG, n->forw->rlive)
		      || inmask(FPCCREG, b->forw->rlive)
		      || inmask(FPCCREG, n->forw->rlive)) {
			/* can't optimize */
			x = y = NULL;
		    }
		    break;
		}
		if (inmask(regno,b->rset) || inmask(regno,b->ruse)) {
		    /* unknown use/modification of regno */
		    break;
		}
	    } while (1);
	    /*
	     * if the preceding search yielded a suitable operand
	     * pair, try to delete either node (b) or node (n).
	     */
	    if (y != NULL) {
		int can_move_x = (x->type_o == T_REG || cancache(x));
		int can_move_y = (y->type_o == T_REG || cancache(y));
		/* determine side effects of (n) other than defining Y */
		if (y->type_o == T_REG) {
		    l = submask(n->rset, MAKEWMASK(y->value_o, BW|WW|LW));
		} else {
		    l = n->rset;
		}
		if (emptymask(andmask(l, n->forw->rlive))) {
		    /*
		     * search for defs/uses of Y and defs affecting access to Y;
		     * if none are found, move def of Y back to node (b).
		     */
		    p = n;
		    previ(p);
		    while (p != b) {
			if (may_touch(p, y, n->subop, RMASK|WMASK)) {
			    can_move_y = 0;
			    break;
			}
			if (!emptymask(andmask(n->ruse, p->rset))) {
			    /* try salvaging by reassigning registers */
			    int r1, r2;
			    if (p->op != OP_MOVE
			      || p->ref[1]->type_o != T_REG ) {
				/* def is implicit; give up */
				can_move_y = 0;
				break;
			    }
			    r1 = p->ref[1]->value_o;
			    r2 = find_freereg(p, n, reg_access[r1]);
			    if (r2 == -1 || !can_reassign_reg(p, n, r1, r2)) {
				can_move_y = 0;
				break;
			    }
			    reassign_reg(p, n, r1, r2);
			}
			previ(p);
		    }
		    if (can_move_y && (ip = move_inst(b, n, x, y)) != NULL) {
			/* rewrite */
			freeoperand(b->ref[1]);
			b->ref[1] = newoperand(y);
			installinstruct(b, ip);
			n = deletenode(n);
			changes++;
			meter.redunm++;
			if (ispopop(x) && ispushop(y)) {
			    b = deletenode(b);
			    changes++;
			    meter.redunm++;
			}
			l = n->forw->rlive;
			for (p = n; p != b; p = p->back) {
			    l = p->rlive = compute_normal(p, l);
			}
			continue;
		    } /* if can_move_y */
		} /* if */
		/*
		 * search for defs of X and defs affecting access to X;
		 * if none are found, move use of X forward to node (n).
		 */
		p = n;
		previ(p);
		while (p != b) {
		    if (may_touch(p, x, n->subop, WMASK)) {
			can_move_x = 0;
			break;
		    }
		    if (!emptymask(andmask(b->ruse, p->rset))) {
			/* try salvaging by reassigning registers */
			int r1, r2;
			if (p->op != OP_MOVE
			  || p->ref[1]->type_o != T_REG ) {
			    /* def is implicit; give up */
			    can_move_x = 0;
			    break;
			}
			r1 = p->ref[1]->value_o;
			r2 = find_freereg(p, n, reg_access[r1]);
			if (r2 == -1 || !can_reassign_reg(p, n, r1, r2)) {
			    can_move_x = 0;
			    break;
			}
			reassign_reg(p, n, r1, r2);
		    }
		    previ(p);
		}
		if (can_move_x && (ip = move_inst(b, n, x, y)) != NULL) {
		    /* rewrite */
		    freeoperand(n->ref[0]);
		    n->ref[0] = newoperand(x);
		    installinstruct(n, ip);
		    b = deletenode(b);
		    changes++;
		    meter.redunm++;
		    if (ispopop(x) && ispushop(y)) {
			n = deletenode(n);
			changes++;
			meter.redunm++;
		    }
		    l = n->forw->rlive;
		    for (p = n; p != b; p = p->back) {
			l = p->rlive = compute_normal(p, l);
		    }
		    continue;
		} /* if can_move_x */
	    } /* y != NULL */
	} /* if */

	/*
	 * Pattern #2: given
	 * 	(b) move X,rn
	 *	    ....
	 *	    (computations involving rn)
	 *	    ....
	 * 	(n) move rn,rm
	 *
	 * if possible, rewrite as:
	 *	(b) move X,rm
	 *	    ....
	 *	    (all defs/uses of rn changed to def/use rm)
	 *	    ....
	 *	(n) (deleted)
	 *
	 * requirements:
	 * 	-- rn must not be live at (n).
	 *	-- condition codes should not be live at (n).
	 *	-- rm must neither be used nor set in the interim.
	 *	-- all defs/uses of rn in (b)..(n) must be replaceable with rm.
	 */
	o = n->ref[0];
	if (n->op == OP_MOVE && reg_reg_op(n)
	  && !inmask(CCREG, n->forw->rlive)
	  && !inmask(FPCCREG, n->forw->rlive)
	  && !inmask(o->value_o, n->forw->rlive)) {
	    regno = n->ref[1]->value_o;
	    b = n;
	    do {
		previ(b);
		if (b->op == OP_FIRST || b->op == OP_LABEL
		  || ISBRANCH(b->op)) {
		    /* flow of control to reach this point is unknown */
		    break;
		}
		if (b->op == OP_MOVE && b->subop == n->subop
		  && sameops(o, b->ref[1])
		  && !inmask(regno, b->forw->rlive)) {
		    /* have move x,rn;...;move rn,rm */
		    if (can_reassign_reg(b, n, o->value_o, regno)) {
			reassign_reg(b, n, o->value_o, regno);
			b->ref[1]->value_o = regno;
			n = deletenode(n);
			changes++;
			meter.redunm++;
			l = n->forw->rlive;
			for (p = n; p != b; p = p->back) {
			    l = p->rlive = compute_normal(p, l);
			}
		    }
		    break;
		}
		if (inmask(regno,b->rset) || inmask(regno,b->ruse)) {
		    /* unknown use/modification of regno */
		    break;
		}
	    } while (1);
	} /* if */

	/*
	 * Pattern #3: (forward copy propagation)
	 * 	(n) move rm,rn	(rm is dead after (n))
	 *	    ....
	 *	    (computations involving rn)
	 *	    ....
	 * 	(p) lastuse(rn) (rn is dead after (p))
	 *
	 * if possible, rewrite as:
	 *	(n) (deleted)
	 *	    ....
	 *	    (all defs/uses of rn changed to def/use rm)
	 *	    ....
	 *	(p) lastuse(rm)
	 *
	 * requirements:
	 *	-- the move at (n) must not involve data conversions.
	 *	-- rm must not be live after (n).
	 * 	-- rn must not be live after (p).
	 *	-- rm must neither be used nor set in the interim.
	 *	-- condition codes must not be live after (n).
	 *	-- if condition codes are live at (p), rm and rn must be
	 *	   of the same type.
	 *	-- all defs/uses of rn in (n)..(p) must be replaceable with rm.
	 */
	if (n->op == OP_MOVE && reg_reg_op(n)
	  && !inmask(CCREG, n->forw->rlive)
	  && !inmask(FPCCREG, n->forw->rlive)
	  && !inmask(n->ref[0]->value_o, n->forw->rlive)) {
	    rm = n->ref[0];
	    rn = n->ref[1];
	    regno = rn->value_o;
	    p = n;
	    do {
		/* scan forward for last use of value in rn */
		nexti(p);
		if (p->op == OP_FIRST || p->op == OP_LABEL
		  || ISBRANCH(p->op)) {
		    /* flow of control to reach this point is unknown */
		    break;
		}
		if (!inmask(regno, p->forw->rlive)) {
		    if (can_reassign_reg(n, p, regno, rm->value_o)) {
			reassign_reg(n, p, regno, rm->value_o);
			n = deletenode(n);
			changes++;
			meter.redunm++;
			n->rlive = compute_normal(n, n->forw->rlive);
		    }
		    break;
		}
		if (inmask(rm->value_o,p->rset)||inmask(rm->value_o,p->ruse)) {
		    /* unknown use/modification of rm */
		    break;
		}
	    } while(1);
	} /* if */

	/*
	 * Pattern #4: (similar to #3, but exploits commutativity)
	 * 	(n) move rm,rn
	 *	(q) <op> ro,rn	(ro is dead after (q))
	 *	    ....
	 *	    (computations involving rn)
	 *	    ....
	 * 	(p) lastuse(rn) (rn is dead after (p))
	 *
	 * if possible, rewrite as:
	 *	(n) (deleted)
	 *	(q) <op> rm,ro
	 *	    ....
	 *	    (all defs/uses of rn changed to def/use ro)
	 *	    ....
	 *	(p) lastuse(ro)
	 *
	 * requirements:
	 *	-- the move at (n) must not involve data conversions.
	 *	-- <op> must be commutative
	 *	-- ro must not be live after (q).
	 *	-- condition codes must not be live after (q).
	 *	-- if condition codes are live after (p), rn and ro
	 *	   must be registers of the same type.
	 * 	-- rn must not be live after (p).
	 *	-- ro must neither be used nor set in the interim.
	 *	-- all defs/uses of rn in (q)..(p) must be replaceable with ro.
	 */
	if (n->op == OP_MOVE && reg_reg_op(n)) {
	    rm = n->ref[0];
	    rn = n->ref[1];
	    q = n;
	    nexti(q);
	    if (iscommuteop(q)
	      && sameops(rn, q->ref[1])
	      && operand_ok(q->instr, rm, (o = q->ref[0]), 0)
	      && !inmask(o->value_o, q->forw->rlive)
	      && !inmask(CCREG, q->forw->rlive)
	      && !inmask(FPCCREG, q->forw->rlive) ) {
		/*
		 * ro is dead, and its last use is in
		 * a commutative operator with rn.  Scan for
		 * the last use of the value in rn.
		 */
		regno = rn->value_o;
		p = q;
		do {
		    nexti(p);
		    if (p->op == OP_FIRST || p->op == OP_LABEL
		      || ISBRANCH(p->op)) {
			/* flow of control to reach this point is unknown */
			break;
		    }
		    if (!inmask(regno, p->forw->rlive)) {
			if (can_reassign_reg(n, p, regno, o->value_o)) {
			    /*
			     * use rm as the first operand of q,
			     * propagate ro through the lifetime of rn,
			     * and delete the move from rm to rn.
			     */
			    q->ref[0] = newoperand(rm);
			    reassign_reg(n, p, regno, o->value_o);
			    freeoperand(o);
			    n = deletenode(n);
			    changes++;
			    meter.redunm++;
			    n->rlive = compute_normal(n, n->forw->rlive);
			}
			break;
		    } /* if */
		    if(inmask(o->value_o,p->ruse)||inmask(o->value_o,p->rset)) {
			/* unknown use/def of ro */
			break;
		    }
		} while (1);
	    } /* if */
	} /* if */

    } /* for */
    return(changes);
}
