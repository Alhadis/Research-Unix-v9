# include "cpass2.h"
#ifndef lint
static	char sccsid[] = "@(#)order.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * order.c -- conditionally included 680X0 version
 */


int fltused = 0;
int floatmath = 0;

SUTYPE fregs;

extern int mina, maxa, minb, maxb; /* imported from allo.c */
extern int toff, maxtoff;

int maxargs = { -1 };

static SUTYPE zed = { 0,0,0,0 };

int failsafe; /* very disgusting: see offstar() for use */

# define SAFETY( a, reg,  b ) \
	{if ((reg & MUSTDO) && find_mustdo( a , reg )) {rewrite_rall( b, 1 );} }
# define iscnode(p) ((p)->in.op==REG && iscreg((p)->tn.rval))

# define max(x,y) ((x)<(y)?(y):(x))
# define min(x,y) ((x)<(y)?(x):(y))

# define divmulop(o) (dope[o]&(DIVFLG|MULFLG))


NODE *
double_conv( p ) register NODE *p;
{
	NODE *q    = talloc();
	q->in.op   = SCONV;
	q->in.type = DOUBLE;
	q->in.left = p;
	q->in.rall = p->in.rall;
	if (use68881) {
	    q->in.su.d = max( p->in.su.d, 1 );
	    q->in.su.a = 0;
	    q->in.su.f = max( p->in.su.f, 1 );
	} else {
	    p->in.rall = MUSTDO|D0;
	    q->in.su.d = max( p->in.su.d, fregs.d );
	    q->in.su.a = max( p->in.su.a, fregs.a );
	    q->in.su.f = max( p->in.su.f, fregs.f );
	}
	return q;
}

setincr( p ) register NODE *p; 
{
	return( 0 );
}

niceuty( p ) register NODE *p; 
{
	return( p->in.op == UNARY MUL && p->in.type != FLOAT &&
		shumul( p->in.left) == STARNM );
}

setconv( p, cookie )
    NODE *p;
{
    register NODE *lp;
    TWORD t;
    
    lp = p->in.left;
    switch(p->in.type) {
    case FLOAT:
    case DOUBLE:
	/* convert from something nice to something icky */
	if ( use68881 ) {
	    if (p->in.type == DOUBLE && iscnode(lp)) {
		/*
		 * no code necessary -- floating point
		 * regs are always in extended format
		 */
		TWORD t = p->in.type;
		*p = *lp;
		p->in.type = t;
		lp->in.op = FREE;
		return(1);
	    }
	    if (ISFLOATING(lp->in.type)) {
		/*
		 * float => double, or vice versa
		 */
		order(lp, INCREG|INTCREG|INTEMP);
		return(1);
	    }
	    if (ISUNSIGNED(lp->in.type)) {
		/*
		 * the 68881 doesn't deal with unsigned types;
		 * convert the unsigned operand to INT.
		 */
		register NODE *q;
		q = talloc();
		q->in.op   = SCONV;
		q->in.type = INT;
		q->in.left = lp;
		q->in.rall = lp->in.rall;
		q->in.su.d = 1;
		q->in.su.a = 0;
		q->in.su.f = 0;
		p->in.left = lp = q;
	    }
	    break;
	}
	failsafe = 1;
	break;
    default:
	/* check for inverse (floating type => fixed type) */
	if (lp->in.type == FLOAT || lp->in.type == DOUBLE) {
	    if (use68881) {
		order(lp, INCREG|INTCREG|INTEMP);
		return(1);
	    }
	    failsafe = 1;
	}
	break;
    }
    if (cookie!=(INBREG|INTBREG))
	cookie = (INAREG|INTAREG|INBREG|INTBREG);
    cookie |= INTEMP;
    order( p->in.left, cookie );
    return 1;
}

/*
 * deal properly with the operand of a unary operator.
 */
int
setunary(p, cookie)
	NODE *p;
{
	NODE *lp;

	lp = p->in.left;
	if (use68881 && ISFLOATING(lp->in.type)) {
		switch(p->in.op) {
		case FCOS:
		case FSIN:
		case FTAN:
		case FACOS:
		case FASIN:
		case FATAN:
		case FCOSH:
		case FSINH:
		case FTANH:
		case FEXP:
		case F10TOX:
		case F2TOX:
		case FLOGN:
		case FLOG10:
		case FLOG2:
		case FSQR:
		case FSQRT:
		case FAINT:
		case FANINT:
			floatsrce(lp);
			return(1);
		case FNINT:
			order(lp, INCREG|INTCREG);
			return(1);
		case UNARY MINUS:
		case FABS:
			/* don't use the 81 unless we have to */
			order(lp, INAREG|INCREG);
			return(1);
		default:
			break;
		}
	}
	order(lp, INAREG|INBREG|INCREG);
	return(1);
}

/*
 * put p into a form acceptable as a 68881 source operand.
 */
static
floatsrce(p)
	register NODE *p;
{
	if (p->in.op == SCONV) {
		/*
		 * the 68881 converts signed ints, float, and double
		 * to internal (extended) format automatically.
		 */
		switch(p->in.left->in.type) {
		case CHAR:
		case SHORT:
		case INT:
		case LONG:
		case FLOAT:
		case DOUBLE:
			p = p->in.left;
			break;
		default:
			/* must do the conversion explicitly */
			order(p->in.left, INAREG|INTAREG|INTEMP);
			break;
		}
	}
	switch(p->in.op) {
	case UNARY MUL:
		/* just make it addressable */
		offstar(p->in.left);
		break;
	case REG:
		/* coprocessor cannot access address registers */
		if (isbreg(p->tn.rval)) {
			order(p, INTAREG|INAREG);
		}
		break;
	default:
		/* lacking any better ideas, put it in a register */
		if (SUTEST(p->in.su)) {
			if (ISFLOATING(p->in.type)) {
				order(p, INCREG|INTCREG|INTEMP);
			} else {
				order(p, INAREG|INTAREG|INTEMP);
			}
		}
		break;
	}
}

setbin( p, cook ) register NODE *p; 
{
	register NODE *r, *l;
	NODE *p2, *q, *t;
	SUTYPE sur, sul;
	SUTYPE qcost, pcost;
	extern short revrel[]; /* for logic ops */
	int cookie, i;

	r = p->in.right;
	l = p->in.left;
	sur = r->in.su;
	sul = l->in.su;

	cookie = cook;
	if (cookie!=(INBREG|INTBREG))
	    cookie = (INAREG|INTAREG|INBREG|INTBREG);
	cookie |= INTEMP;
	/* if this is one of the constrained operators, rewrite as op= */
	switch( p->in.op ){
	    case GT:
	    case GE:
	    case LT:
	    case LE:
	    case EQ:
	    case NE:
			if (l->in.type != FLOAT && l->in.type != DOUBLE)
			    break;
			if (use68881) {
			    /*
			     * figure out what, if anything, is
			     * already in a coprocessor reg
			     */
			    int lhs,rhs;
			    lhs = iscnode(l);
			    rhs = iscnode(r);
			    /*
			     * logical ops require one operand to
			     * be in a coprocessor register. table.c
			     * expects the register on the left.
			     */
			    if ( !lhs ) {
				/* lhs is not in a coprocessor reg */
				if ( !rhs ) {
				    /* rhs isn't either; do harder one */
				    if (SUGT(sur, sul)
				      && !tshape(r, SFLOAT_SRCE)) {
					order(r, INCREG|INTCREG);
					rhs = 1;
				    } else {
					order(l, INCREG|INTCREG);
					lhs = 1;
				    }
				} 
				if (rhs) {
				    /* rhs is, lhs isn't */
				    int temp;
				    p->in.op = revrel[p->in.op - EQ];
				    p->in.left = r;
				    p->in.right = l;
				    r = l;
				    l = p->in.left;
				    temp = rhs;
				    rhs = lhs;
				    lhs = temp;
				}
			    }
			    /*
			     * lhs is in a coprocessor reg
			     */
			    if ( !rhs ) {
				/*
				 * rhs isn't, and doesn't need to be
				 */
				floatsrce(r);
			    } /* !rhs */
			    return(1);
			} /* use68881 */

			/*
			 * evaluate most expensive side first. If 
			 * cheaper size has non-zero cost, may have
			 * to put first intermediate result in a temp.
			 * Actually, this should not be a problem, if
			 * the su-stuff worked.
			 */
			/* set variable "i" if we do anything here */
			i = 0;
			if( SUGT( sul, sur ) && !(istnode(l)&&l->tn.rval==D0) ){
			    /* do lhs first if it is the more expensive */
			    l->in.rall = MUSTDO|D0;
			    SAFETY( r , MUSTDO|D0, l);
			    order( l, INTAREG);
			    i++;
			}
			if ( r->in.type == DOUBLE ){
			    /* must be addressable */
			    if ( !tshape( r, SNAME|SOREG|STARNM)){
				r->in.rall = NOPREF;
				order( r, INTEMP );
				i++;
			    }
			} else {
			    if( !(istnode(r) && r->tn.rval == D1) ){
				r->in.rall = MUSTDO|D1;
				SAFETY( l , MUSTDO|D1, r);
				order( r, INTAREG);
				i++;
			    }
			}
			if( !(istnode(l) && l->tn.rval == D0) ){
			    /* do lhs after if it is the less expensive */
			    l->in.rall = MUSTDO|D0;
			    order( l, INTAREG);
			    i++;
			}
			if (i)
			    return 1;
			break ; /* didn't accomplish anything -- fall into general case */
	    case PLUS:
	    case MINUS:
		if( p->in.type == FLOAT || p->in.type == DOUBLE ){
float_ops:
			/*
			 * evaluate most expensive side first. If 
			 * cheaper size has non-zero cost, may have
			 * to put first intermediate result in a temp.
			 * Actually, this should not be a problem, if
			 * the su-stuff worked.
			 */
			if (r->in.type==FLOAT && (!FLOATMATH || p->in.type==DOUBLE)){
			    r = p->in.right = double_conv(r);
			}
			if (l->in.type==FLOAT && (!FLOATMATH || p->in.type==DOUBLE)){
			    l = p->in.left = double_conv(l);
			}
			/*----------------------------*/
			if (use68881) {
			    /*
			     * figure out what, if anything, is
			     * already in a coprocessor reg.  The
			     * lhs must be a temporary.
			     */
			    int lhs,rhs;
			    lhs = (istnode(l) && iscreg(l->tn.rval));
			    rhs = (iscnode(r));
			    /*
			     * dyadic ops require one operand to
			     * be in a coprocessor register. table.c
			     * expects the register on the left.
			     */
			    if ( !lhs ) {
				/* lhs is not in a temp coprocessor reg */
				if ( !rhs ) {
				    /* rhs isn't either; do harder one */
				    if (SUGT(sur, sul)
				      && !tshape(r, SFLOAT_SRCE)) {
					floatsrce(r);
					rhs = iscnode(r);
				    } else {
					order(l, INTCREG);
					lhs = 1;
				    }
				} 
				if ( rhs && !lhs && istnode(r)
				  && (p->in.op == PLUS || p->in.op == MUL) ) {
				    /*
				     * rhs is in a writable coprocessor
				     * reg, lhs isn't, and op is commutable
				     */
				    int temp;
				    p->in.left = r;
				    p->in.right = l;
				    r = l;
				    l = p->in.left;
				    temp = rhs;
				    rhs = lhs;
				    lhs = temp;
				}
				if (!lhs) {
				    order(l, INTCREG);
				    lhs = 1;
				}
			    }
			    /*
			     * lhs is in a writable coprocessor reg
			     */
			    if ( !rhs ) {
				/*
				 * rhs isn't, and doesn't need to be
				 */
				floatsrce(r);
			    } 
			    p->in.op = ASG p->in.op;
			    return(1);
			} /* use68881 */

			if (usesky) {
			    if ( p->in.op == PLUS ) {
				/*
				 * look for expressions of
				 * the form (x + y*z), which the
				 * sky board does in a single
				 * operation.
				 */
				if ( l->in.op == MUL && r->in.op != MUL ) {
				    l = r;
				    r = p->in.left;
				    p->in.left = l;
				    p->in.right = r;
				}
				if (r->in.op == MUL) {
				    /*
				     * Don't even go near it
				     * unless x,y, or z comes for free.
				     */
				    if (!SUTEST(l->in.su)) {
					l = r->in.left;
					r = r->in.right;
				    } else if (!SUTEST(r->in.left->in.su)) {
					r = r->in.right;
				    } else if (!SUTEST(r->in.right->in.su)) {
					r = r->in.left;
				    }
				}
			    }
			    for( i=0; i<2; i++ ){
				if (SUGT( l->in.su, r->in.su )){
				    p2 = l; q = r;
				}else if (SUTEST( r->in.su )){
				    p2 = r; q = l;
				}else break;
				if (p2->in.op==UNARY MUL 
				  && rewrite_b_rall(p2->in.left, 1)){
				    /* try to offstar, but not into a1 */
				    failsafe = find_mustdo( q, MUSTDO|D0 );
				    offstar(p2->in.left);
				} else {
				    if ( !find_mustdo( q, MUSTDO|D0) ||
					rewrite_rall(p2, i) )
					order( p2, INTAREG| INTBREG| INTEMP );
				    else
					order( p2, INTEMP );
				}
				/* set su to zero here */
				p2->in.su = zed;
			    } /* for */
			} else {
			    /*
			     * do it in software
			     */
			    if (p->in.type == FLOAT){
				/* assume rall's set up already */
				for( i=0; i<2; i++ ){
				    if (SUGT( l->in.su, r->in.su )){
					p2 = l; q = r;
				    }else if (SUTEST( r->in.su )){
					p2 = r; q = l;
				    }else break;
				    SAFETY( q, D0|MUSTDO, p2 );
				    failsafe = 1;
				    order( p2, INTAREG|INTBREG );
				    p2->in.su = zed;
				}
				/* are no-cost, but may be wrong */
				if (!istnode(l) || l->tn.rval != D0)
				    order( l, INTAREG );
			    } else {
				/* this case is the least symmetric */
				/* basically, must rewrite lhs as d0/d1 */
				/* but if rhs is hard, do it first */
				if (!istnode(l) || l->tn.rval != D0){
				    if (SUTEST( sur )){
					if (r->in.op == UNARY MUL 
					&& ( !SUTEST(sul) || rewrite_b_rall( r->in.left, 0)) ){
						failsafe = 1;
						offstar( r->in.left );
					} else {
					    order( r, INTEMP );
					}
				    }
				    failsafe = 1;
				    order( l, INTAREG );
				}
			    }
			    p->in.op = ASG p->in.op ; /* gag */
			}
			/*----------------------------*/
#ifdef FLOATMATH
			if (!FLOATMATH)
#endif
			    p->in.type = DOUBLE;
			return 1; /* order(): try again */
 		} else if (ISPTR(p->in.type)){
 		    /*
 		     * cute hack: we can add a short into an address register
 		     * without extending it first.
 		     */
 		    if ( isconv( l, SHORT, -1) && p->in.op==PLUS ){
 			/* un-canonical form -- flip it */
 			p->in.left = r;
 			p->in.right = r =l;
 			l = p->in.left;
		    }
 		    if ( isconv( r, SHORT, -1 ) ){
			/*
			 * if this is being evaluated into an address 
			 * register or for argument, delete the conversion
			 */
			if ( (cook==FORARG || (cook&(INBREG|INTBREG)
				&& !(cook&~(INBREG|INTBREG))))
			&& l->tn.op==REG
			&& tshape( r->in.left, SAREG|STAREG|SBREG|STBREG|SNAME|SOREG|STARREG) ){
			    if (cook==FORARG && r->in.left->tn.op != REG){
				order( r->in.left, INAREG|INTAREG|INBREG|INTBREG );
			    }
			    /* lhs should be in an address register,
			       and should be writeable if SCONV operand
			       is not a register */
			    if (!isbreg(l->tn.rval)
			      || !istreg(l->tn.rval) && r->in.left->in.op != REG) {
				order( l, INBREG|INTBREG );
			    }
 			    p->in.right = r->in.left;
 			    r->in.op = FREE;
			    return 1;
			} else if (p->in.op == PLUS) {
			    /* otherwise, we were better off the other way */
			    /* flip back */
			    p->in.left = r;
			    p->in.right = r =l;
			    l = p->in.left;
			}
 		    }
		}
		break;
	    case MUL:
	    case DIV:
	    case MOD:
		if( p->in.type == FLOAT || p->in.type == DOUBLE ){
			goto float_ops;
		}
		if( p->in.type != INT && p->in.type != UNSIGNED && !ISPTR(p->in.type))

			break;
		if(use68020) 
			break;
d0_d1:
		/* copied from Setasop */
		sul = l->in.su;
		sur = r->in.su;
		/* use q to designate the more expensive side, p2 the cheaper */
		if (!SUGT(sur, sul)){
			q = l; p2 = r; pcost = sur;
		} else {
			q = r; p2 = l; pcost = sul;
		}
		if( SUTEST(sul) && SUTEST(sur) ){
		    SAFETY( p2, q->in.rall, q );
		}
			
		/* do code emission, most expensive side first */
		if (!istnode( q ) ){
		  order( q, INAREG|INTAREG);
		}
		if ( SUTEST( pcost ) && !istnode( p2 )){
		  order( p2 ,INAREG|INTAREG);
		}
		if ( !istnode( l ) || l->tn.rval != D0 ){
		    l->tn.rall = MUSTDO | D0;
		    order( l, INTAREG);
		}
		if (logop(p->in.op)){
		    if ( !istnode( r )||r->tn.rval != D1 ){
			r->tn.rall = MUSTDO | D1;
			order( r, INTAREG);
		    }
		} else {
		    if ( SUTEST( sur ) && (!istnode( r )||r->tn.rval != D1)){
			r->tn.rall = MUSTDO | D1;
			order( r, INTAREG);
		    }
		    p->in.op = ASG p->in.op;
		}
		return 1; /* go try 'gain */
	    case CHK:
		/*
		 * the rhs of a CHK operator is an ordered pair
		 * of integers.  We load the lhs first, regardless
		 * of what the bounds cost, since both the bounds
		 * and the lhs must be in hand simultaneously.
		 */
		if (!istnode(l) || !isareg(l->tn.lval)) {
		    SAFETY(r, MUSTDO|D0, l);
		    order(l, INTAREG|INAREG);
		    return(1);
		}
		return(0);
	} /* switch */

	if( !SUTEST( r->in.su ) ){
		/* rhs is addressable */
		if( logop( p->in.op ) ){
			if ( l->in.op==UNARY MUL 
			&& l->in.type!=FLOAT && shumul(l->in.left)!=STARREG ) {
			    offstar( l->in.left );
			} else  if ( !(l->in.op==REG && isareg(l->tn.rval))) {
			    /* lhs is not in a data register */
			    switch (l->in.type) {
			    case FLOAT:
			    case DOUBLE:
				/* lhs must be in d0/d1 */
				if (r->in.op == REG && r->tn.rval == D0) {
				    goto reverse;
				}
				break;
			    case CHAR:
			    case UCHAR:
				/* bytes cannot be in address registers */
				if (l->in.op == REG && isbreg(l->tn.rval)) {
				    break;
				}
				/* fall through */
			    default:
			    reverse:
				if ( r->in.op == REG && isareg(r->tn.rval)
				  && !SUTEST(l->in.su) ) {
				    /*
				     * rhs is in a data register, lhs
				     * isn't, and doesn't have to be
				     */
				    p->in.left = r;
				    p->in.right = l;
				    p->in.op = revrel[p->in.op - EQ];
				    return(1);
				}
			    }
			    order( l, cookie );
			} else {
			    /*
			     * if here, must be a logical operator for 0-1 value
			     * and we don't have 2 registers, as we would prefer
			     */
			    int m;
			    cbranch( p, -1, m=getlab() );
			    p->in.op = CCODES;
			    p->bn.label = m;
			    order( p, INTAREG );
			} 
			return( 1 );
		}
		if ( !istnode( l ) ){
		    if (commuteop(p->in.op) && istnode(r)) {
			/* commutative op - put temp node on the left */
			p->in.right = l;
			p->in.left = r;
		    } else {
			order( l, cookie&(INTAREG|INTBREG) );
			return( 1 );
		    }
		}
		if ( divmulop(p->in.op) && r->in.op == REG
		  && isbreg(r->tn.rval)) {
		    order( r, INAREG|INTAREG );
		    return( 1 );
		}
		/* rewrite */
		return( 0 );
	}
	/* now, rhs is complicated */
	/* do the harder side first */
	/* be careful of D0, D1 usage */

	if (SUGT(l->in.su, r->in.su)) {
		/* lhs first; put into register */
		SAFETY(r, MUSTDO|D0, l);
		order(l, cookie&(INTAREG|INTBREG));
		return( 1 );
	}
	/* try to make rhs addressable */
	if(r->in.op == UNARY MUL) {
		failsafe = find_mustdo( l, D0|MUSTDO) ;
		if (l->in.su.d >= nfree(STAREG)) {
			/* lhs needs all available d-registers;
			   do not use double indexing */
			failsafe |= 1;
		}
		offstar( r->in.left );
		return(1);
	}
	if (SUGT(r->in.su , l->in.su)){
		/* rhs first; put into register */
		SAFETY( l, MUSTDO|D0, r);
		/* anything goes on rhs */
		order( r, INTAREG|INAREG|INTBREG|INBREG|INTEMP ); 
		return( 1 );
	}
	if (!istnode(l)) {
		SAFETY(r, MUSTDO|D0, l);
		order(l, cookie&(INTAREG|INTBREG));
		return( 1 );
	}
	if (!istnode(r)) {
		SAFETY(l, MUSTDO|D0, r);
		order(r, INTAREG|INAREG|INTBREG|INBREG|INTEMP);
		return( 1 );
	}
	/* rewrite */
	return( 0 );
}

setstr( p, cook ) register NODE *p; 
{ 
	/* structure assignment */
	if (p->in.right->in.op != REG){
		order( p->in.right, INTBREG );
		return(1);
	}
	p = p->in.left;
	if ( p->in.op != NAME && p->in.op != OREG ){
		if ( p->in.op != UNARY MUL ) cerror( "bad setstr");
		order( p->in.left, INTBREG );
		return( 1 );
	}
	if( p->in.op == OREG && !R2TEST(p->tn.rval) && istreg(p->tn.rval)
	    && p->tn.lval == 0){
		/* cheat -- rewrite this as a REG */
		p->in.op = REG;
		return (1);
	}
#ifdef FORT
	/* Pascal does struct assignment of things that ain't structs!! */
	if (ISARY(p->in.type)){
	    p->in.type = STRTY;
	    return(1);
	}
#endif
	return( 0 );
}

setasg( p , cookie ) 
	register NODE *p; 
{
	/* setup for assignment operator */
	register NODE *r, *l;
	int failsave = failsafe;

	r = p->in.right;
	l = p->in.left;

	if ( !SUGT( r->in.su , l->in.su )) goto do_lhs;
/*	if ((p->in.type == DOUBLE || p->in.type == FLOAT) 
/*	&& iscnode(p->in.right)){
/*	    order( p->in.right, INAREG|SOREG|SNAME|SCON );
/*	    return 1;
/*	}
*/
	if( SUTEST( r->in.su ) && r->in.op != REG ){
	    failsafe |=  SUTEST( l->in.su ); /* conservative guess */
	    if( r->in.op == UNARY MUL ){
		offstar( r->in.left );
	    }else{
		if (p->in.type == DOUBLE && r->in.type == FLOAT) 
		    p->in.right = r = double_conv( r );
		if (!use68020 || !use68881) {
		    /*
		     * more goddamn complications from register parameters:
		     * if the lhs needs D0 for a multiply, for instance,
		     * then we must be careful not to
		     * evaluate the rhs into D0. 
		     */
		    SAFETY( l, MUSTDO|D0, r );
		}
	        order( r, INAREG|INBREG|INCREG|SOREG|SNAME|SCON );
	    }
	    failsafe = failsave;
	    return(1);
	}
do_lhs:
	if( l->in.op == UNARY MUL )
		if (!tshape( l, STARREG|STARNM ) 
		    || p->in.type == DOUBLE){
		    /* sorry, we're too feeble to *P++ with double pointers */
			failsafe |=  SUTEST( r->in.su ); /* conservative guess */
			failsafe |=  r->tn.su.d >= fregs.d;
			offstar( l->in.left );
			failsafe = failsave;
			return(1);
		}
	if (l->in.op==FLD && l->in.left->in.op==UNARY MUL ){
		failsafe |=  SUTEST( r->in.su ); /* conservative guess */
		offstar( l->in.left->in.left );
		failsafe = failsave;
		return(1);
	}
	/* if things are really strange, get rhs into a register */
	if( r->in.op != REG ) {
		order( r, INAREG|INBREG|INCREG );
		failsafe = failsave;
		return( 1 );
	}
	/* for fields, rhs must be in a data register */
	if( l->in.op == FLD && r->in.op == REG && isbreg(r->tn.rval) ) {
		order( r, INAREG|INTAREG );
		failsafe = failsave;
		return( 1 );
	}
	return(0);
}

find_mustdo( p, regname ) register NODE *p;
{
    /* if there are any ops here that require use of reg regname,  return
     * 1 else return 0;
     */
redo:
    if (p->in.rall == regname) return 1;
    if (p->in.type == DOUBLE && p->in.rall == regname-1) return 1;
    switch( optype(p->in.op) ){
    case LTYPE: return 0;
    case BITYPE:
		if (find_mustdo(p->in.right, regname)) return 1;
		/* else fall through */
    }
    p = p->in.left;
    goto redo; /* go 'round again */
}

int
rewrite_rall( n, trybreg ) NODE *n;
{
	register int i, reg;
	register TWORD t = n->in.type;

	/* find a temporary home for n */
	reg = -1;
	for( i = D2; i <= maxa; i++){
	    if( istreg(i) && busy[i] == 0){
		if (t==DOUBLE && ((i&1) || !( istreg(i+1) && busy[i+1] == 0)))
		    continue; /* too bad */
		reg = i; break;
	    }
	}
	if (reg < 0 && trybreg && t!=CHAR && t!= UCHAR) {
	    /* we are DESPARATE! */
	    for( i = minb; i <= maxb; i++){
		if( istreg(i) && busy[i] == 0){
		    if (t==DOUBLE && ((i&1) || !( istreg(i+1) && busy[i+1] == 0)))
			continue; /* too bad */
		    reg = i; break;
		}
	    }
	}
	if ( reg < 0 ) return 0;
	n->in.rall = (MUSTDO | reg);
	return 1;
}

int
rewrite_b_rall( n, a0ok )
    NODE *n;
{
	register int i, reg;

	/* find a temporary home for n in the b-registers */
	/* get out of the hair of a0/a1     */
	reg = -1;
	for( i = minb+2; i <= maxb; i++){
	    if( istreg(i) && busy[i] == 0){
		reg = i; break;
	    }
	}
	if (reg<0 && a0ok && busy[minb]==0)
	    reg = minb;
	if ( reg < 0) return 0; /*failure*/
	n->in.rall = MUSTDO | reg;
	/* if this is a prospective oreg -- keep going */
	if (n->in.op == PLUS || n->in.op == MINUS){
	    n = n->in.left;
	    if (n->in.op != REG || !isareg( n->tn.rval ) )
		n->in.rall = MUSTDO | reg;
	}
	return 1;
}

NODE *
hard_rew( p ) register NODE *p;
{
	register NODE *lp, *p2;
	register int i, reg;
	/* do rewriting for non-reg lhs of hard op='s */

	lp = p->in.left;
	switch(lp->in.op){
	    default:
		return p;
	    case REG:
		if(istreg(lp->tn.rval)) return p;
	    case NAME:
	    case OREG:
	    case UNARY MUL:
	    case FLD:
		break;
	    }

	if (odebug){
	    printf("hard_rew( %o ):\n", p);
	    fwalk( p, eprint, 0);
	}

	if( lp->in.op == UNARY MUL ){
		NODE *llp = lp->in.left;
		offstar( llp );
		if ( llp->in.op == PLUS && llp->in.right->in.op != ICON ) {
			/* de-index */
			order(llp, INTBREG|INBREG);
		}
	}
	if( lp->in.op == FLD && lp->in.left->in.op == UNARY MUL ){
		NODE *lllp = lp->in.left->in.left;
		offstar( lllp );
		if ( lllp->in.op == PLUS && lllp->in.right->in.op != ICON ) {
			/* de-index */
			order(lllp, INTBREG|INBREG);
		}
	}
	if( lp->in.op == FLD ) lp = lp->in.left;

	/* mimic code from reader.c */
	p2 = tcopy( p );
	p->in.op = ASSIGN;
	reclaim( p->in.right, RNULL, 0 );
	p->in.right = p2;
	canon(p);
	rallo( p, p->in.rall );
	lp = p2->in.left;
	/* if we are doing a floating op, we'll let the caller finish it off */
	if(p->in.type == DOUBLE || p->in.type == FLOAT)
	    return p2;
	/* if we must, rewrite rhs, too */
	if (SUGT( p2->in.right->in.su , lp->in.su )){
	    SAFETY( lp, p2->in.right->in.rall, p2->in.right );
	    order( p2->in.right, INTAREG|INTBREG );
	}
	SAFETY( p2->in.right, lp->in.rall, lp );
	order( lp, INTBREG|INTAREG );
	
	return p->in.right;
}

setasop( p, cookie ) register NODE *p; 
{
	/* setup for =ops */
	register SUTYPE sul, sur;
	register NODE *r, *l;
	register NODE *q, *p2;
	SUTYPE pcost;

	r = p->in.right;
	l = p->in.left;
	sul = l->in.su;
	sur = r->in.su;

	switch( p->in.op ){
	  case ASG PLUS:
	  case ASG MINUS:
	  case ASG OR:
	  case ASG ER:
	  case ASG AND:
		if (p->in.type == FLOAT || p->in.type == DOUBLE ) {
floatops:
		    /*
		     * lhs in dreg or AWD, rhs in dreg or AWD.
		     */

		    /*
		     * if the lhs is not a temp node, then we need to let
		     * a op= b ; be rewritten as...
		     * a = (a) op= b;
		     */
		    /* HOWEVER!:
		     * if rhs is expensive, we gotta do it first!
		     */
		    if (r->in.type==FLOAT && p->in.type==DOUBLE){
			r = p->in.right = double_conv(r);
		    }

		    if (use68881) {
			/*
			 * if lhs is already a coprocessor reg, all
			 * we have to do is clean up the rhs a little.
			 */
			int rhs = 0;
			if (iscnode(l)) {
			    floatsrce(r);
			    return(1);
			}
			if ( SUGT(sur, sul) ) {
			    /* rhs is hard -- do it first */
			    floatsrce(r);
			    rhs = 1;
			}
			p = hard_rew(p);
			l = p->in.left;
			r = p->in.right;
			if ( istnode(r) && iscreg(r->tn.rval)
			  && (p->in.op == ASG PLUS || p->in.op == ASG MUL) ) {
			    /*
			     * commute -- we can do this because hard_rew
			     * assigns the result of p to the lhs
			     */
			    p->in.left = r;
			    p->in.right = l;
			    l = r;
			    r = p->in.right;
			    rhs = 0;
			} else {
			    /*
			     * put lhs in a coprocessor reg
			     */
			    order(l, INCREG|INTCREG);
			}
			/*
			 * if rhs hasn't been dealt with yet, do it
			 */
			if (!rhs) {
			    floatsrce(r);
			}
			return(1);
		    } /* use68881 */

		    if (usesky){
			/*
			 * check for pivot operation ( x += y * z )
			 * only do if one of x,y,z is free
			 */
			int pivoting = 0;
			if (p->in.op == ASG PLUS && r->in.op == MUL) {
			    if (!SUTEST(sul)) {
				setbin(r, INTAREG|INAREG|INTEMP);
				return(1);
			    }
			    if (!SUTEST(r->in.left->in.su)
			      || !SUTEST(r->in.right->in.su)) {
				pivoting = 1;
			    }
			}
			/*
			 * on the sky board, "op=" ops can be done
			 * with lhs in memory. However, we must stay
			 * away from a0/a1.
			 */
			if (SUGT(sur,sul)){
			    /* must do rhs first */
			    if (pivoting) {
				if (SUTEST(r->in.left->in.su)) {
				    r = r->in.left;
				} else {
				    r = r->in.right;
				}
				pivoting = 0;
			    }
			    if (r->in.op == UNARY MUL
			      && rewrite_b_rall(r->in.left, 0)) {
				/* make addressable, avoiding a1 */
				offstar(r->in.left);
			    } else if (!find_mustdo(l, MUSTDO|D0)
			      || rewrite_rall(r, 0)) {
				order(r, INAREG|INTAREG|INTEMP);
			    } else {
				order(r, INTEMP);
			    }
			    r->in.su = zed;
			}
			/*
			 * deal with lhs
			 */
			if (SUTEST(sul)) {
			    if (l->in.op == UNARY MUL
			      && rewrite_b_rall( l->in.left, 0 )) {
				/* make addressable, avoiding a1 */
				offstar( l->in.left );
			    } else {
				/* failing that, try rewriting */
				p = hard_rew(p);
				l = p->in.left;
				r = p->in.right;
				if (pivoting) {
				    if (SUTEST(r->in.left->in.su)) {
					r = r->in.left;
				    } else {
					r = r->in.right;
				    }
				    pivoting = 0;
				}
				if (!find_mustdo(r, MUSTDO|D0)
				  || rewrite_rall(l, 0)) {
				    order(l, INAREG|INTAREG|INTEMP);
				} else {
				    order(l, INTEMP);
				}
			    }
			}
			/*
			 * deal with rhs, if necessary
			 */
			if (pivoting) {
			    if (SUTEST(r->in.left->in.su)) {
				r = r->in.left;
			    } else {
				r = r->in.right;
			    }
			}
			if (SUTEST(r->in.su)){
			    if (r->in.op == UNARY MUL
			      && rewrite_b_rall(r->in.left, 0)) {
				/* make addressable, avoiding a1 */
				offstar(r->in.left);
			    } else {
				order(r, INAREG|INTAREG|INTEMP);
			    }
			}
			return 1;
		    } else if ( !istnode(l) ){
			if (SUTEST( sur )){
			    if (!SUTEST(sul) && r->in.type == FLOAT){
				r->in.rall = MUSTDO|D1;
				order( r, INAREG );
			    } else {
				/* may not want it in a register, 
				in case lhs needs  the register */
				if (rewrite_rall(r, 0)){
				    order( r, INAREG|INTAREG|INBREG|INTBREG|INCREG|INTCREG );
				} else {
				    r->in.rall = NOPREF;
				    order( r, INTEMP );
				}
			    }
			}
			/* lhs is either easy or is an address */
			/*
			 * if its an address, we must, unhappily,
			 * evaluate it AWAY from a0/a1, which will
			 * be clobbered by the operation.
			 */
			if (l->in.op == UNARY MUL && l->in.left->in.op != REG){
			    failsafe = 1; /* keep away from d0/d1!! */
			    if ( rewrite_b_rall( l->in.left, 0 )  )
				offstar( l->in.left );
			    else 
				order( l->in.left, INTEMP );
			} else 
			    l->in.rall = MUSTDO|D0;
			/* p->in.op = NOASG p->in.op; */
			goto rew_lhs;
		    }
			    
		    if (r->in.type == DOUBLE ){
			if ( !tshape( r, SNAME|SOREG|STARNM)){
			    r->in.rall = NOPREF;
			    order( r, INTEMP );
			}
		    } else {
			if( !(istnode(r) && r->tn.rval == D1) ){
			    r->in.rall = MUSTDO|D1;
			    order( r, INTAREG);
			}
		    }
		    if ( !(istnode(l) && l->tn.rval == D0) ){
			l->in.rall = MUSTDO|D0;
			order( l, INTAREG);
		    }

		    return 1;
		} else if (ISPTR(p->in.type)
		  && (p->in.op==ASG PLUS || p->in.op==ASG MINUS)
		  && isconv(r, SHORT, -1)
		  && l->in.op== REG && isbreg(l->tn.rval)) {
		    p->in.right = r->in.left;
		    r->in.op = FREE;
		    return 1;
		} else {
		    if ((p->in.right->in.op != REG || 
		    isbreg(p->in.right->tn.rval)) && !SUTEST( sul )) {
		        order(p->in.right,INAREG|INTAREG);
		        return(1);
		    } else break;
		}

	  case ASG MUL:
	  case ASG DIV:
	  case ASG MOD:
		/*
		 * This is difficult: These ops require the lhs in d0 and the
		 * rhs in d1. This means that whichever side gets evaluated
		 * first will monopolize one of these strategic registers for
		 * the whole time the other side is being evaluated. This is
		 * not acceptable if the less-expensive side ALSO requires
		 * evaluation of one of these constrained ops. So we must be
		 * very careful here. First, we assume that there are, indeed,
		 * enough registers for us to play with, otherwise off-storing
		 * should already have been taken care of by the wonders of 
		 * portable code generation. So what we'll do is find a free
		 * register NOT in the set {d0,d1}, and force evaluation of the
		 * more expensive side into that register, then rewrite the 
		 * requirements for that side. Hold on.
		 */

		if (p->in.type == FLOAT || p->in.type == DOUBLE)
		    goto floatops;
		if( p->in.type != INT && p->in.type != UNSIGNED && !ISPTR(p->in.type))
			break;
		if(use68020)
			break;

		/* we can only do these into d0 */
d0_d1:
		/* if things already look optimum, we must already have been this way once */
		if (istnode(p->in.left) && !SUTEST( p->in.right->tn.su )){
		    order( p->in.right, INTAREG );
		    return 1;
		}
		p = hard_rew( p );
		sul = p->in.left->in.su;
		/* use q to designate the more expensive side, p2 the cheaper */
		if ( !SUGT( sur, sul )){ q = p->in.left;  p2 = p->in.right; pcost = sur; }
			else { q = p->in.right; p2 = p->in.left;  pcost = sul; }
		if( SUTEST( sul ) && SUTEST( sur )){
		    SAFETY( p2, q->in.rall, q );
		}
			
		/* do code emission, most expensive side first */
		if (!istnode( q ) ){
		  order( q, INAREG|INTAREG);
		}
		if ( SUTEST( pcost ) && !istnode( p2 )){
		  order( p2 ,INAREG|INTAREG);
		}
		if ( !istnode( p->in.left ) || p->in.left->tn.rval != D0 ){
		    p->in.left->tn.rall = MUSTDO | D0;
		    order( p->in.left, INTAREG );
		}
		if ( SUTEST(sur) &&(!istnode(p->in.right)||p->in.right->tn.rval!=D1)){
		    p->in.right->tn.rall = MUSTDO | D0;
		    order( p->in.right, INTAREG );
		}

	        return(1);

	  case ASG LS:
	  case ASG RS:
		if (l->in.op != REG){
			/* must rewrite as lhs = ((lhs) op= rhs) */
			/* if we must, rewrite rhs, too */
			if (SUGT( sur , sul)){
			    SAFETY( l, MUSTDO|D0, r );
			    order( r, INAREG|INBREG );
			}
			/* lhs is either easy or is an address */
			if (SUTEST( sul )){
			    if (l->in.op == UNARY MUL)
				offstar( l->in.left );
			    else if (l->in.op == FLD &&
				l->in.left->in.op == UNARY MUL)
				offstar( l->in.left->in.left);
			}
			goto rew_lhs;
		}
		if (r->in.op == REG && isareg(r->tn.rval)
		  || (r->in.op==ICON && r->tn.lval>=1 && r->tn.lval<=8))
		    break;
		order(r,INAREG|INTAREG);
		return(1);

	}

	/*
	 * What we have here is an asop which could POSSIBLY be done to memory
	 * if we desire it. Luckily we have the cookie as a parameter to make
	 * this decision. If we do it to memory, then we just want to evaluate
	 * the lhs to an address. If we do it for a value, we have to evaluate
	 * the lhs for a value, then store afterwards.
	 */
	if (SUTEST( sul ) && !SUGT( sur,sul )){
	    /* since an lhs can only be an address ... */
	    if (l->in.op == UNARY MUL){
		failsafe |= SUTEST( sur ); /* conservative guess */
		offstar( l->in.left );
	    } else if (l->in.op == FLD){
		if (l->in.left->in.op == UNARY MUL)
		    offstar( l->in.left->in.left);
		return 0; /* rewrite as something sane */
	    }
	    if (cookie&FOREFF){
		/* for effect only! */
		return 1;
	    }
	    /* otherwise, rewrite as (lhs1) = ((lhs2) op= rhs) */
rew_lhs:
	    ncopy( p2=talloc(), p);
	    p->in.op    = ASSIGN;
	    p->in.right = p2;
	    l = p->in.left  = tcopy( p2->in.left );
	    SAFETY( p2->in.right, MUSTDO|D0, p2->in.left );
	    if (l->in.op == UNARY MUL && shumul(l->in.left) == STARREG) {
		/*
		 * Beware side effects lurking in lhs.  We now
		 * have two copies -- side effects of one copy must
		 * be eliminated.  Note that (lhs2) will be evaluated first.
		 */
		if (l->in.left->in.op == ASG MINUS) {
		    /* predecrement -- keep side effect in (lhs2),
		       delete side effect in (lhs1) */
		    q = l->in.left;
		} else {
		    /* postincrement -- keep side effect in (lhs1),
		       delete side effect in (lhs2) */
		    q = p2->in.left->in.left;
		}
		l = q->in.left;  /* = REG */
		r = q->in.right; /* = ICON */
		*q = *l;
		l->in.op = FREE;
		r->in.op = FREE;
	    }
	    order( p2->in.left, INAREG|INBREG );
	    return 1;
	}
	if( SUTEST( sur )){
	    /* evaluate rhs into a register, then go try again */
	    SAFETY( l, MUSTDO|D0, r );
	    order( r, INAREG|INBREG );
	    return 1;
	}
	if (divmulop(p->in.op) && r->in.op == REG && isbreg(r->tn.rval)) {
	    order(r, INAREG|INTAREG);
	    return(1);
	}
	if (l->in.op == UNARY MUL && shumul(l->in.left) == STARREG) {
	    goto rew_lhs;
	}
	return 0;
}

/*
 * returns number of free registers of a given type.  This wouldn't
 * be necessary if the SU-numbers were strictly followed; however,
 * double indexing ties up more registers than sucomp assumes, and
 * should not be attempted unless a spare register exists.
 */
int
nfree(cookie)
{
    register r;
    register n;
    n = 0;
    for (r = 0; r < REGSZ; r++) {
	if ((rstatus[r]&cookie) && !busy[r])
	    n++;
    }
    return n;
}
