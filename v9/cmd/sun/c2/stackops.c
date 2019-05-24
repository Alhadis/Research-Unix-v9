
#ifndef lint
static	char sccsid[] = "@(#)stackops.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

extern struct oper *newoperand();

#define previ( p ) { do p=p->back; while( ISDIRECTIVE(p->op) ); }

#define ispushop(o) ((o)->type_o == T_PREDEC && (o)->value_o == SPREG)
#define ispopop(o) ((o)->type_o == T_POSTINC && (o)->value_o == SPREG)

extern int bytesize[];
#define BYTESIZE( s ) (bytesize[ (int)(s)])

/*
 * replace
 * 	(b)->move X,sp@-
 *	     ...
 *	(n)->move sp@+,Rn
 * with
 *	(b)->move X,Rn
 *	     ...
 *	(n) deleted
 * Note that if Rn is live after (n), it now becomes live
 * at all of the points in between (b) and (n).
 */

static NODE *
movedef(b,n,ip)
    register NODE *b;
    NODE *n;
    struct ins_bkt *ip;
{
    register NODE *p;
    regmask l;

    if (b->nref == 2)
	freeoperand(b->ref[1]);
    else 
	b->nref = 2;
    b->ref[1] = newoperand(n->ref[1]);
    installinstruct(b,ip);
    n = deletenode(n);
    l = n->forw->rlive;
    for (p = n; p != b; p = p->back) {
	l = p->rlive = compute_normal(p, l);
    }
    return n;
}

/*
 * replace
 * 	(b)->pea X
 *	     ...
 *	(n)->move sp@+,dn
 * with
 *	(b)->lea X,ar
 *	     movl ar,dn
 *	     ...
 *	(n)  (deleted)
 *
 * This is like the previous transformation with an
 * adjustment for the fact that 'lea' does not work
 * with a d-register destination.
 */
static NODE*
movedef2(b, n, ar, ip)
    register NODE *b;
    register NODE *n;
    int ar;
    struct ins_bkt *ip;
{
    register NODE *p;
    register struct oper *o;
    regmask l;

    /*
     * rewrite (b) as "lea X,ar"
     */
    b->nref = 2;
    o = newoperand(n->ref[1]);
    o->value_o = ar;
    b->ref[1] = o;
    installinstruct(b,ip);
    /*
     * insert "movl ar,dn" following (b)
     */
    p = new();
    p->nref = 2;
    o = newoperand(o);
    o->value_o = ar;
    p->ref[0] = o;
    o = newoperand(o);
    o->value_o = n->ref[1]->value_o;
    p->ref[1] = o;
    cannibalize(p, "movl");
    insert(p,b);
    /*
     * delete (n), update register lifetime information
     */
    n = deletenode(n);
    l = n->forw->rlive;
    for (p = n; p != b; p = p->back) {
	l = p->rlive = compute_normal(p, l);
    }
    return n;
}


/*
 * replace
 * 	(b)->move X,sp@-
 *	     ...
 *	(n)-><op> sp@+,...
 * with
 *	(b) (deleted)
 *	     ...
 *	(n)-><op> X,...
 * Note that if X is a register, it now becomes live
 * at all of the points in between (b) and (n).
 */
static NODE *
moveuse(b, n, srce, ip)
    NODE *b,*n;
    struct oper *srce;
    struct ins_bkt *ip;
{
    register NODE *p;
    regmask l;

    freeoperand(n->ref[0]);
    n->ref[0] = srce;
    b = deletenode(b);
    installinstruct(n,ip);
    l = n->forw->rlive;
    for (p = n; p != b; p = p->back) {
	l = p->rlive = compute_normal(p, l);
    }
    return(n);
}


/*
 * returns the "extended mode" instruction, if any,
 * corresponding to the the instruction described by (ip).
 */
struct ins_bkt *
extendedop(ip)
    struct ins_bkt *ip;
{
    char buf[64];

    (void)strcpy(buf, ip->text_i);
    buf[strlen(buf)-1] = 'x';
    return(sopcode(buf));
}

/*
 * returns 1 if operands (op1) and (op2) may overlap
 */
int
mayoverlap(op1, op2, subop1, subop2)
    register struct oper *op1;
    register struct oper *op2;
    subop_t subop1;
    subop_t subop2;
{
    int r1, r2;
    struct sym_bkt *sym1, *sym2;
    int lb1, lb2, ub1, ub2;	/* byte offset bounds of op1 and op2 */
    int glb, lub;		/* greatest lower & least upper bounds */

    r1 = -1;
    r2 = -1;
    lb1 = 0;
    lb2 = 0;
    sym1 = NULL;
    sym2 = NULL;
    switch(op1->type_o) {
    case T_IMMED:
	return(0);
    case T_REG:
	return(op2->type_o == T_REG && op1->value_o == op2->value_o);
    case T_POSTINC:
    case T_PREDEC:
	/* assume sp@+ and sp@- only overlap with each other */
	if (op1->value_o == SPREG) {
	    if (op2->type_o == T_POSTINC || op2->type_o == T_PREDEC)
		return(op2->value_o == SPREG);
	    return(0);
	}
	return(1);
    case T_DISPL:
	r1 = op1->reg_o;
	/* fall through */
    case T_ABSS:
    case T_ABSL:
    case T_NORMAL:
	sym1 = op1->sym_o;
	lb1 = op1->value_o;
	ub1 = lb1 + BYTESIZE(subop1)-1;
	break;
    case T_DEFER:
	r1 = op1->value_o;
	ub1 = lb1 + BYTESIZE(subop1)-1;
	break;
    }
    switch(op2->type_o) {
    case T_IMMED:
    case T_REG:
	return(0);
    case T_POSTINC:
    case T_PREDEC:
	return(op2->value_o != SPREG);
    case T_DISPL:
	r2 = op2->reg_o;
	/* fall through */
    case T_ABSS:
    case T_ABSL:
    case T_NORMAL:
	sym2 = op2->sym_o;
	lb2 = op2->value_o;
	ub2 = lb2 + BYTESIZE(subop2)-1;
	goto compute_overlap;
    case T_DEFER:
	r2 = op2->value_o;
	ub2 = lb2 + BYTESIZE(subop2)-1;
	/* fall through */
    compute_overlap:
	if (r1 != r2 || sym1 != sym2)
	    return(1);
	ub1 = lb1 + BYTESIZE(subop1)-1;
	ub2 = lb2 + BYTESIZE(subop2)-1;
	glb = lb1 > lb2 ? lb1 : lb2;
	lub = ub1 < ub2 ? ub1 : ub2;
	return(glb <= lub);
    default:
	return(1);
    }
}

/*
 * returns 1 if two operands are in adjacent *addressable* words
 */
int
adjacent(op1, op2)
    register struct oper *op1,*op2;
{
    int w;
    struct oper temp;

    if (op1->type_o != op2->type_o) {
	/* make DEFER operands look like DISPL operands */
	if (op1->type_o == T_DEFER) {
	    temp.type_o = T_DISPL;
	    temp.reg_o = op1->value_o;
	    temp.value_o = 0;
	    op1 = &temp;
	} else if (op2->type_o == T_DEFER) {
	    temp.type_o = T_DISPL;
	    temp.reg_o = op2->value_o;
	    temp.value_o = 0;
	    op2 = &temp;
	} else {
	    return 0;
	}
    }
    switch (op1->type_o){
    case T_IMMED:
    case T_NORMAL:
    case T_ABSS:
    case T_ABSL:
	if (op1->sym_o != op2->sym_o) return 0; /* oops -- complex*/
	/* fall through */
    case T_REG: 
    case T_DEFER:
    case T_POSTINC:
    case T_PREDEC:
	if (op1->value_o + sizeof(long) != op2->value_o) return 0;
	break;
    case T_INDEX:
	if (op1->disp_o + sizeof(long) != op2->disp_o) return 0;
	/* fall through */
    case T_DISPL:
	if (op1->reg_o != op2->reg_o) return 0;
	if (op1->value_o + sizeof(long) != op2->value_o) return 0;
	break;
    default:
	return 0;
    }
    return 1;
}

/*
 * returns 1 if node (n) may use/modify operand (o)
 */
int
may_touch(n, o, osubop, touchmask)
    register NODE *n;
    register struct oper *o;
    int osubop;	/* subop of instruction using (o) */
    register touchmask;
{
    register int i,v;
    regmask l;

    switch(o->type_o) {
    case T_IMMED:
	return 0;
    case T_REG:
	if (touchmask&RMASK == touchmask)
	    return inmask(o->value_o,n->ruse);
	if (touchmask&WMASK == touchmask)
	    return inmask(o->value_o,n->rset);
	if (touchmask)
	    return inmask(o->value_o,addmask(n->rset,n->ruse));
	return 0;
    default:
	/*
	 * for each operand modified by (n), see if
	 * a potential overlap with (o) exists.
	 */
	if (n->op == OP_CALL)
	    return 1;
	v = make_touchop(n, n->instr->touchop_i);
	for (i = 0; i < n->nref; i++) {
	    if (v&touchmask) {
		if (mayoverlap(o, n->ref[i], osubop, n->subop))
		    return 1;
	    }
	    v >>= TOUCHWIDTH;
	}
    }
    return 0;
}


/*
 * reduce stack traffic exposed by inline expansion
 * of procedures
 */
int
stackops()
{
    register NODE *n;
    register NODE *p,*b;
    register struct ins_bkt *ip;
    register struct oper *o;
    register int regno;
    extern NODE *deletenode();
    int changes;
    regmask l;
    int offset;
    int r;

    changes = 0;
    for (n=first.forw; n != &first; n=n->forw) {

	/*
	 * Pattern #0:
	 *	(0) sub  #X,sp
	 *	(1) add  #Y,sp	-or- (1) lea sp@(Y),sp
	 * Rewrite:
	 *	delete both, or use a single addql, subql, or lea
	 */
	if (n->op == OP_SUB
	  && n->ref[1]->type_o == T_REG && n->ref[1]->value_o == SPREG
	  && n->ref[0]->type_o == T_IMMED) {
	    p = n->forw;
	    switch(p->op) {
	    case OP_ADD:
		o = p->ref[1];
		if (o->type_o == T_REG && o->value_o == SPREG
		  && p->ref[0]->type_o == T_IMMED) {
		    goto delete_adjust;
		}
		break;
	    case OP_LEA:
		o = p->ref[1];
		if (o->type_o == T_REG && o->value_o == SPREG
		  && p->ref[0]->type_o == T_DISPL
		  && p->ref[0]->reg_o == SPREG ) {
		    goto delete_adjust;
		}
		break;
	    delete_adjust:
		offset = n->ref[0]->value_o - p->ref[0]->value_o;
		p = deletenode(p);
		if (offset == 0) {
		    n = deletenode(n);
		} else if (offset >= 1 && offset <= 8) {
		    n->ref[0]->value_o = offset;
		    installinstruct(n, sopcode("addql"));
		} else if (offset <= -8 && offset >= -1) {
		    n->ref[0]->value_o = -offset;
		    installinstruct(n, sopcode("subql"));
		} else {
		    o = n->ref[0];
		    o->type_o = T_DISPL;
		    o->reg_o = SPREG;
		    o->value_o = offset;
		    installinstruct(n, sopcode("lea"));
		}
		continue;
	    }
	}

	/*
	 * Pattern #1:
	 *	(b) movl  X,sp@-
	 *	...
	 *	(n) <op>  sp@+,...
	 *
	 * if possible, rewrite as
	 *	(b) (deleted)
	 *	...
	 *	(n) <op>  X,...
	 *
	 * Requirements:
	 *	X must not be a control register or i/o device.
	 *	X must not be modified in the interim.
	 *      Side effects of X must not be used in the interim.
	 */
	if (ISINSTRUC(n->op) && n->ref[0] != NULL && ispopop(n->ref[0])) {
	    b = n;
	    o = NULL;
	    do {
		previ(b);
		if (b->op == OP_FIRST || b->op == OP_LABEL
		  || ISBRANCH(b->op)) {
		    /* flow of control to reach this point is unknown */
		    break;
		}
		if (b->op == OP_PEA) {
		    switch(b->ref[0]->type_o) {
		    case T_ABSS:
		    case T_ABSL:
		    case T_NORMAL:
			/*
			 * srce is #X
			 */
			o = newoperand(b->ref[0]);
			o->type_o = T_IMMED;
			b->subop = SUBOP_L;
		    }
		    break;
		}
		if (b->op == OP_CLR && ispushop(b->ref[0])) {
		    /*
		     * srce is #0
		     */
		    o = newoperand(b->ref[0]);
		    o->type_o = T_IMMED;
		    o->value_o = 0;
		    break;
		}
		if (b->op == OP_MOVE && ispushop(b->ref[1])) {
		    o = newoperand(b->ref[0]);
		    break;
		}
		if (inmask(SPREG,b->rset) || inmask(SPREG,b->ruse)) {
		    /* unknown use/modification of sp */
		    break;
		}
	    } while(1);
	    /*
	     * If the preceding search yielded a usable operand,
	     * determine whether its use can be moved from node
	     * (b) to (n).
	     */
	    if (o != NULL) {
		l = submask(b->rset, MAKEWMASK(SPREG, LW));
		if ( o->type_o == T_REG && !datareg(o->value_o)
		  || o->type_o != T_IMMED && o->type_o != T_REG && !cancache(o)
		  || !emptymask(andmask(l, b->forw->rlive))) {
		    /*
		     * x is a control register or might be an i/o device,
		     * or has non-trivial side effects: cannot move.
		     */
		    freeoperand(o);
		    o = NULL;
		} else {
		    /* search for defs of X and defs affecting access to X */
		    l = submask(b->ruse, MAKERMASK(SPREG, LR));
		    p = n;
		    previ(p);
		    while (p != b) {
			if (may_touch(p, o, b->subop, WMASK)) {
			    freeoperand(o);	/* bail out */
			    o = NULL;
			    break;
			}
		        if (!emptymask(andmask(l, p->rset))) {
			    /*
			     * (p) defines register used at (b);  if (p)
			     * is a simple register load, and if the lifetime
			     * of the register is restricted to (p)..(n),
			     * we may be able to use a different register,
			     * eliminating the conflict
			     */
			    int r1,r2;
			    if (p->op != OP_MOVE
			      || p->ref[1]->type_o != T_REG ) {
				freeoperand(o);	/* bail out */
				o = NULL;
				break;
			    }
			    r1 = p->ref[1]->value_o;
			    r2 = find_freereg(p, n, reg_access[r1]);
			    if (r2 == -1 || !can_reassign_reg(p, n, r1, r2)) {
				freeoperand(o);	/* bail out */
				o = NULL;
				break;
			    }
			    reassign_reg(p, n, r1, r2);
			}
			previ(p);
		    }
		}
	    }
	    /*
	     * If it is feasible to move the use of the operand X,
	     * find out whether X is a suitable source operand of
	     * the instruction at (n).
	     */
	    if (o != NULL) {
		if (BYTESIZE(b->subop) == BYTESIZE(n->subop)
		  && operand_ok(n->instr, o, n->ref[1], n->ref[2])) {
		    n = moveuse(b, n, o, n->instr);
		    changes++;
		    continue;
		}
		/*
		 * if X is a floating point register, try rewriting
		 * node (n) using an extended floating point opcode
		 */
		if (o->type_o == T_REG && freg(o->value_o)) {
		    ip = extendedop(n->instr);
		    if (ip != NULL
		      && operand_ok(ip, o, n->ref[1], n->ref[2])) {
			n = moveuse(b, n, o, ip);
			changes++;
			continue;
		    }
		}
		/*
		 * if X is an addressable double operand and the
		 * the use of sp@+ occurs in a double precision
		 * floating point operation, use X directly.
		 */
		if (n->subop == SUBOP_D && o->type_o != T_REG
		  && b->back->op == OP_MOVE
		  && b->back->subop == SUBOP_L
		  && ispushop(b->back->ref[1])
		  && adjacent(o, b->back->ref[0])) {
		    (void) deletenode(b->back);
		    n = moveuse(b, n, o, n->instr);
		    changes++;
		    continue;
		}
		freeoperand(o); /* give up */
	    } /* if o != NULL */
	} /* if ISINSTRUCT... */

	/*
	 * Other patterns assume the instruction at (n) is a move.
	 */
	if (n->op != OP_MOVE)
	    continue;

	/*
	 * Pattern #2:
	 *	(b) move X,sp@-
	 *	    ....
	 *	(n) move sp@+,rn
	 * If possible, rewrite as:
	 *	(b) move X,rn
	 *	    ....
	 *	(n) (deleted)
	 * Requirements:
	 *	Rn must neither be used nor set in the interim.
	 *	If rn is an a-register, condition codes must not
	 *	be live immediately following (b).
	 *	If rn is not an a-register, condition codes must not
	 *	be live immediately following (n).
	 */
	o = n->ref[0];
	regno = n->ref[1]->value_o;
	if(ispopop(o) && n->ref[1]->type_o == T_REG
	  && !(dreg(regno) && inmask(CCREG,n->forw->rlive))
	  && !(freg(regno) && inmask(FPCCREG, n->forw->rlive))) {
	    b = n;
	    do {
		previ(b);
		if (b->op == OP_FIRST || b->op == OP_LABEL
		  || ISBRANCH(b->op)) {
		    /* flow of control to reach this point is unknown */
		    break;
		}
		if (b->op == OP_PEA) {
		    /* note: PEA does not set the condition codes */
		    if (areg(regno)) {
			/*
			 * rewrite as: lea <srce>,areg
			 */
			n = movedef(b,n,sopcode("lea"));
			changes++;
		    } else if ((r = dead_areg(b)) != -1
		      && !inmask(CCREG,b->forw->rlive)) {
			/*
			 * rewrite as:	lea <srce>,areg;
			 *		movl areg,rn
			 */
			n = movedef2(b,n,r,sopcode("lea"));
			changes++;
		    } else {
			switch(b->ref[0]->type_o) {
			case T_ABSS:
			case T_ABSL:
			case T_NORMAL:
			    /*
			     * rewrite as: movl #<srce>,reg
			     */
			    b->ref[0]->type_o = T_IMMED;
			    n = movedef(b,n,sopcode("movl"));
			    changes++;
			}
		    }
		    break;
		}
		if (b->op == OP_CLR && ispushop(b->ref[0])
		  && b->subop == SUBOP_L
		  && !(areg(regno) && inmask(CCREG,b->forw->rlive))) {
		    /*
		     * rewrite as: movl #0,reg
		     */
		    b->ref[0]->type_o = T_IMMED;
		    b->ref[0]->value_o = 0;
		    n = movedef(b,n,sopcode("movl"));
		    changes++;
		    break;
		}
		if (b->op == OP_MOVE && ispushop(b->ref[1])
		  && b->subop == SUBOP_L
		  && !(areg(regno) && inmask(CCREG,b->forw->rlive))) {
		    /*
		     * try to change into MOVE <srce>,reg
		     * beware floating point register operands
		     */
		    if (!freg(regno)) {
			/* reg is a-reg or d-reg */
			if (b->subop == SUBOP_D || b->subop == SUBOP_X) {
			    /* can't move doubles to a register pair */
			} else if (areg(regno)
			  && b->ref[0]->type_o == T_REG
			  && freg(b->ref[0]->value_o)) {
			    /* can't move fp reg to a-reg */
			} else if (b->subop == n->subop) {
			    /* easy case */
			    n = movedef(b,n,n->instr);
			    changes++;
			}
		    } else {
			o = b->ref[0];
			if (o->type_o == T_REG && freg(o->value_o)) {
			    /*
			     * freg-freg move
			     */
			    n = movedef(b,n,sopcode("fmovex"));
			} else if (b->subop == n->subop
			  && BYTESIZE(n->subop) <= sizeof(long)
			  && !(o->type_o == T_REG && areg(o->value_o))) {
			    /*
			     * move single word to freg
			     */
			    n = movedef(b,n,n->instr);
			    changes++;
			} else if (n->subop == SUBOP_D
			  && o->type_o != T_REG
			  && b->back->op == OP_MOVE
			  && b->back->subop == SUBOP_L
			  && ispushop(b->back->ref[1])
			  && adjacent(o, b->back->ref[0])) {
			    /*
			     * move addressable double operand to freg
			     */
			    (void) deletenode(b->back);
			    b->subop = SUBOP_D;
			    n = movedef(b,n,n->instr);
			    changes++;
			}
			/* else forget it! */
		    }
		    break;
		}
		if (inmask(regno,b->rset) || inmask(regno,b->ruse)) {
		    /* dest reg was used or modified along the way */
		    break;
		}
		if (inmask(SPREG,b->rset) || inmask(SPREG,b->ruse)) {
		    /* some other use/modification of sp */
		    break;
		}
	    } while(1);
	}

	/*
	 * Pattern #3:
	 *	(b) move sp@+,rn
	 *	    ....
	 *	(n) move rn,sp@-
	 *
	 * if possible, rewrite as
	 *	(b) move sp@,rn
	 *	    ....
	 *	(n) (deleted)
	 *
	 * requirements:
	 *	Rn must not be used or set in the interim.
	 *	Condition codes must not be live after either (b) or (n).
	 */
	o = n->ref[1];
	if (ispushop(o) && n->ref[0]->type_o == T_REG
	  && datareg(n->ref[0]->value_o)
	  && !inmask(CCREG,n->forw->rlive)
	  && !inmask(FPCCREG, n->forw->rlive)) {
	    regno = n->ref[0]->value_o;
	    b = n;
	    do {
		previ(b);
		if (b->op == OP_FIRST || b->op == OP_LABEL
		  || ISBRANCH(b->op)) {
		    /* flow of control to reach this point is unknown */
		    break;
		}
		if (b->op == OP_MOVE && ispopop(b->ref[0])
		  && b->subop == n->subop
		  && sameops(b->ref[1], n->ref[0])
		  && !inmask(CCREG, b->forw->rlive)
		  && !inmask(FPCCREG, b->forw->rlive)) {
		    /* pushing same value that was just popped */
		    n = deletenode(n);
		    b->ref[0]->type_o = T_DEFER;
		    installinstruct(b, b->instr);
		    l = n->forw->rlive;
		    for (p = n; p != b; p = p->back) {
			l = p->rlive = compute_normal(p, l);
		    }
		    changes++;
		    break;
		}
		if (inmask(regno,b->rset) || inmask(regno,b->ruse)) {
		    /* reg was used/modified along the way */
		    break;
		}
		if (inmask(SPREG,b->rset) || inmask(SPREG,b->ruse)) {
		    /* some other use/modification of sp */
		    break;
		}
	    } while(1);
	} /* if */

    } /* for */
    return(changes);
}
