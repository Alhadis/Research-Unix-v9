#ifndef lint
static	char sccsid[] = "@(#)su.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

# include "cpass2.h"

/*
 * from order.c
 */


extern int mina, maxa, minb, maxb; /* imported from allo.c */
int failsafe;
extern toff, maxtoff;

# define iscnode(p) (p->in.op==REG && iscreg(p->tn.rval))

# define max(x,y) ((x)<(y)?(y):(x))
# define min(x,y) ((x)<(y)?(x):(y))


sucomp( p ) register NODE *p; 
{

	/* set the su field in the node to the sethi-ullman
	   number, or local equivalent */

	register o, t;
	register nr;
	register u, tt;
	register NODE *r, *l;
	SUTYPE sul, sur;
	SUTYPE tempsu;
	static SUTYPE zed = { 0,0,0,0 };

	p->in.su = zed;
	switch(t = optype( o=p->in.op)) {
	case LTYPE:
		return;
	case UTYPE:
		sucomp(p->in.left);
		break;
	case BITYPE:
		sucomp(p->in.left);
		sucomp(p->in.right);
		r = p->in.right;
		sur = r->in.su ;
		break;
	}
	l = p->in.left;
	sul = l->in.su;
	nr = szty( tt = p->in.type );

	switch ( o ){
	case ASG ER:
		/* exclusive-or -- not generally computable to-memory */
		if ( r->tn.op != ICON ) goto reg_reg_asg;
		/* fall through */

	case ASG PLUS:
	case ASG MINUS:
	case ASG AND:
	case ASG OR:
		/* these may be done directly at memory -- looks like = */
		if ( tt == FLOAT || tt == DOUBLE ) {
			goto float_asop;
		}
		/* fall through */
	
	case ASSIGN:
		/* usually, compute rhs into a register, compute lhs, store */
		/* could do it backwards if lhs is hairier		    */
		if (SUSUM( sur ) > SUSUM( sul )){
/*		    p->in.su = max( sur, sul+nr); */
		    sur.d = max( sur.d, sul.d+nr );
		    sur.a = max( sur.a, sul.a );
		}else{
/*		    p->in.su = max( sur+1, sul); */
		    if(l->in.op == FLD) {
			sur.d = max(max(sur.d, sul.d),2);
		    } else {
			sur.d = max(sur.d, sul.d);
		    }
		    sur.a = max( sur.a+1, sul.a);
		}
		sur.f = max( sur.f, sul.f);
		p->in.su = sur;
		return;

	case STASG:
		/* compute two addresses, then use up to three more registrers */
/*		p->in.su = max( 3, 1+max(sul,sur)); */
		sur.d = 1+max( sur.d, sul.d );
		t = max( sur.a, sul.a );
		sur.a = max( 2, t );
		p->in.su = sur;
		return;

	case UNARY MUL:
		if (shumul(p->in.left))
			return;
		/* most other unary ops handled in default case */
		sul.a = max( sul.a, 1); /* 1 address register to * through */
		p->in.su = sul;
		return;

	case CALL:
	case UNARY CALL:
	case STCALL:
	case UNARY STCALL:
		p->in.su = fregs;
		return;

	case ASG RS:
	case ASG LS:
		/* if the rhs is not a small contant, this is a reg-reg op */
		if ( r->tn.op == ICON && r->tn.lval >0 && r->tn.lval <= 8 ){
			/* looks like a unary op */
			sul.d += nr;
			p->in.su = sul;
			return;
		}
		/* fall through */
	case ASG MUL:
	case ASG DIV:
	case ASG MOD:
	/* compute rhs, compute lhs (or vice versa), get lhs value, op */
		if ( tt == FLOAT || tt == DOUBLE ) {
			goto float_asop;
		}
reg_reg_asg:
		t = SUSUM(sul)?1:0; /* number regs needed to store lhs address */
		/* want at least 2 regs + lhs address */
		/* if lhs is complicated, will always compute it into a reg FIRST */
		/* p->in.su = max( t+2*nr, min(t+sur, nr+sul)); */
/*		if (sul)
/*		    u = max( sul, t+sur );
/*		else
/*		    u = sur;
/*		p->in.su = max( u, t+2*nr);
*/
		if (SUSUM(sul)){
		    sur.d = max( sur.d, sul.d );
		    sur.a = max( sur.a+t, sul.a );
		} 
		sur.d = max( sur.d , 2*nr );
		sur.a = max( sur.a , t );
		sur.f = 0;
		p->in.su = sur;
		return;

	case PLUS:
	case AND:
	case OR:
	case ER:
		/* commutable memory-register ops */
		if ( tt == FLOAT || tt == DOUBLE ) {
			goto float_asop;
		}
		if (l->in.op != REG && SUSUM(sul) < SUSUM(sur)){
			p->in.right = l;
			p->in.left = r;
			l = r;
			r = p->in.right;
			sul = sur;
			sur = r->in.su ;
		}
		goto reg_mem_op;

	case EQ:
	case NE:
	case LE:
	case GE:
	case GT:
	case LT:
	case UGT:
	case UGE:
	case ULT:
	case ULE:
		    tt = l->in.type;
		    nr = szty( tt );
		    /* fall through */
	case MINUS:
	case INCR:
	case DECR:
		if ( tt == FLOAT || tt == DOUBLE ) {
float_asop:
			if (use68881) {
			    nr = 1;
			    if (!SUTEST(sur)
			      && (r->in.op != REG || !isbreg(r->tn.rval))){
				/* rhs is cheap */
				sul.f = max( sul.f, nr );
				p->in.su = sul;
				return;
			    }
			    /* compute rhs, lhs (or vice versa) do op */
			    if (SUGT(sur, sul)){
				/*
				 * value of rhs, value of lhs,
				 * reg-reg op
				 */
/*				p->in.su = max( sur, sul+nr ); */
				if (commuteop(o) && !SUTEST(sul)
				  && (l->in.op != REG || !isbreg(l->tn.rval))){
				    u = max( sul.f, nr );
				} else {
				    u = nr + max( sul.f, nr );
				}
				sur.d = max( sur.d, sul.d );
				sur.a = max( sur.a, sul.a );
				sur.f = max( sur.f, u );
			    } else {
/*				p->in.su = max( sul, sur+1 ); */
				if (r->in.op == UNARY MUL){
				    /*
				     * address of rhs, value of lhs,
				     * op from memory
				     */
				    sur.d = max( sur.d, sul.d   );
				    sur.a = max( sur.a, sul.a+1 );
				    sur.f = max( sul.f, sur.f+1 );
				} else {
				    /*
				     * value of lhs, value of rhs,
				     * reg-reg-op
				     */
				    sur.d = max( sul.d, sur.d );
				    sur.a = max( sul.a, sur.a );
				    sur.f = max( sul.f+1, sur.f+1 );
				}
			    }
			    p->in.su = sur;
			    return;
			}
			/*
			 * floating operations are calls.
			 * They always take ALL the registers.
			 * SIMPLE FIRST APPROXIMATION HERE.
			 * evaluate rhs first: if lhs 
			 * costs, then may need an extra temp.
			 */
			if (SUTEST(sur) && SUTEST(sul) )
			    u = max( sur.d, sul.d+nr);
			else
			    u = max( sur.d, sul.d );
			tempsu.d = max( u, fregs.d );

			u = max( sur.a, sul.a );
			tempsu.a = max( u, fregs.a );
			tempsu.f = 0;
			p->in.su = tempsu;
			return;
		}
reg_mem_op:
		/* memory-register ops */

		/*
		 * if rhs is direct, then the op is very cheap.
		 * But if rhs is an address register, only integer
		 * addition and subtraction are cheap.
		 */
		if (SUSUM(sur) == 0) {
		    int cheap = 1;
		    if (r->in.op == REG && isbreg(r->tn.rval)) {
			switch(o) {
			case PLUS:
			case MINUS:
			case ASG PLUS:
			case ASG MINUS:
			    if (tt == CHAR || tt == UCHAR) {
				/* can't use addr regs in byte adds and subs */
				cheap = 0;
			    }
			    break;
			default:
			    /* can't use addr regs for other ops, period */
			    cheap = 0;
			    break;
			}
		    }
		    if (cheap) {
			sul.d = max(sul.d, nr);
			p->in.su = sul;
			return;
		    }
		}
		/* else must compute rhs, lhs (or vice versa) do op */
		if (SUGT(sul, sur)) {
			/* lhs, rhs, reg-reg op */
/*			p->in.su = max( sul, sur+nr ); */
			u = nr + max( sur.d , nr );
			sur.d = max( sul.d, u );
			sur.a = max( sur.a, sul.a );
		} else {
/*			p->in.su = max( sul, sur+1 ); */
			if (r->in.op == UNARY MUL){
			    /* address of rhs, value of lhs, op from memory */
			    sur.d = max( sur.d, sul.d   );
			    sur.a = max( sur.a, sul.a+1 );
			} else if (SUGT(sur,sul)) {
			    /* value of rhs, value of lhs, reg-reg-op */
			    sur.d = max( sur.d, sul.d+nr );
			    sur.a = max( sur.a, sul.a    );
			} else {
			    /* value of lhs, value of rhs, reg-reg-op */
			    sur.d = max( sul.d, sur.d+nr );
			    sur.a = max( sul.a, sur.a    );
			}
		}
		sur.f = 0;
		p->in.su = sur;
		/* is there an easier way to do this? */
		return;

	case RS:
	case LS:
		/* looks like a unary op if rhs is small */
		if ( r->tn.op == ICON && r->tn.lval >0 && r->tn.lval <= 8 ){
			/* looks like a unary op */
			sul.d = max( sul.d, nr);
			p->in.su = sul;
			return;
		}
		/* fall through */
	case MUL:
	case DIV:
	case MOD:
		if ( tt == FLOAT || tt == DOUBLE ) {
			goto float_asop;
		}
reg_reg_op:
		/* register-register ops */
		/* must do one and then the other */
		/* commutability not an issue here */
		/* need at least two register sets */
		if ( SUGT( sul, sur ) ){
		    t = max( sul.d, sur.d+nr );
		} else {
		    t = max( sul.d+nr, sur.d );
		}
		sul.d = max( 2*nr, t);
		sul.a = max( sul.a, sur.a );
		sul.f = max( sul.f, sur.f );
		p->in.su = sul;
		return;

	case NAME:
	case REG:
	case OREG:
	case ICON:
	case FCON:
		return; /* su is zero */

	case SCONV:
		if (!ezsconv(p)){
			sul.d = max( sul.d, fregs.d);
			sul.a = max( sul.a, fregs.a);
		} else if (use68881
		    && (ISFLOATING(tt) || ISFLOATING(l->in.type))) {
			sul.f = max( sul.f, 1 );
			sul.d = max( sul.d, 1 );
		} else {
			sul.d = max( sul.d, nr); /* normal unary */
		}
		p->in.su = sul;
		break;
	
	case FAINT:
	case FANINT:
	case FNINT:
		/* float=>integer conversion */
		if (use68881) {
			sul.d = max(sul.d, 1);
			sul.f = max(sul.f, 1);
		} else {
			/* must do subroutine call */
			sul.d = max(sul.d, fregs.d);
			sul.a = max(sul.a, fregs.a);
		}
		p->in.su = sul;
		break;

	case CHK:
		/*
		 * this is actually a ternary operation: 
		 * the expression to be checked,
		 *     the lower bound,
		 *     the upper bound.
		 * if the upper and lower bounds are constant, then
		 * cost is the cost of the expression, which must
		 * always be evaluated into a data register. We
		 * may have to evaluate the bounds, too, and this 
		 * costs more.
		 * we will try to use the 68010's chk instruction.
		 * failing that, we will use the 68020's chk2 instruction.
		 * lacking that, use compare-and-branch code.
		 */
		if (chk_ovfl){
		    /*
		     * assume we have to evaluate each of the bounds &
		     * at the same time hold the expression.
		     */
		    sur.d = max( r->in.left->in.su.d, r->in.right->in.su.d);
		    sur.a = max( r->in.left->in.su.a, r->in.right->in.su.a);
		    sur.f = max( r->in.left->in.su.f, r->in.right->in.su.f);
		    p->in.su.d = max( sul.d, nr+sur.d );
		    p->in.su.a = max( sul.a, sur.a );
		    p->in.su.f = max( sul.f, sur.f );
		} else {
		    p->in.su = sul;
		}
		break;

	default:
		if ( t == BITYPE ){
			/* random binary operators */
			/* choose largest */
			t = max( sur.d, sul.d );
			sur.d = max( t, nr );
			sur.a = max( sur.a, sul.a );
			sur.f = max( sur.f, sul.f );
			p->in.su = sur;
			return;
		}
		/* must be unary */
		if (dope[o]&INTRFLG) {
			/* fortran intrinsics */
			if (use68881) {
				sul.f = max(sul.f, 1);
			} else {
				/* must do subroutine call */
				sul.d = max(sul.d, fregs.d);
				sul.a = max(sul.a, fregs.a);
			}
		} else {
			/* random unary op */
			sul.d = max( sul.d, nr);
			if (use68881 && ISFLOATING(tt)) {
				sul.f = max( sul.f, 1 );
			}
		}
		p->in.su = sul;
	}
}

int radebug = 0;

mkrall( p, r ) register NODE *p; 
{
	/* insure that the use of p gets done with register r; in effect, */
	/* simulate offstar */

	if( p->in.op == FLD ){
		p->in.left->in.rall = p->in.rall;
		p = p->in.left;
	}

	if( p->in.op != UNARY MUL ) return;  /* no more to do */
	p = p->in.left;
	if( p->in.op == UNARY MUL ){
		p->in.rall = r;
		p = p->in.left;
	}
	if( p->in.op == PLUS && p->in.right->in.op == ICON ){
		p->in.rall = r;
		p = p->in.left;
	}
	rallo( p, r );
}

rallo( p, down ) NODE *p; 
{
	/* do register allocation */
	register o, type, down1, down2, ty;
	NODE *p2;

again:
	if( radebug ) printf( "rallo( %o, %d )\n", p, down );

	down2 = NOPREF;
	p->in.rall = down;
	down1 = ( down &= ~MUSTDO );

	ty = optype( o = p->in.op );
	type = p->in.type;


	switch( o ) {
	case UNARY MUL:
		down1 = NOPREF;
		break;

	case ASSIGN:	
		down1 = NOPREF;
		down2 = down;
		break;

	case ASG MUL:
	case ASG DIV:
	case ASG MOD:

	case MUL:
	case DIV:
	case MOD:
		switch(type){
		case FLOAT:
		    if (use68881 || usesky) break;
		    down1 = D0|MUSTDO;
		    down2 = D1|MUSTDO;
		    break;
		case INT:
		case UNSIGNED:
		    if (use68020) break;
		    down1 = D0|MUSTDO;
		    down2 = D1|MUSTDO;
		    break;
		case DOUBLE:
		    if (use68881 || usesky) break;
		    down1 = D0|MUSTDO;
		    break;
		default:
		    if (ISPTR(type)){
			/* this is the result of (char *)(a*b) */
			/* treat as an unsigned or int         */
			if (use68020) break;
			down1 = D0|MUSTDO;
			down2 = D1|MUSTDO;
		    }
		    break;
		}
		break;

	case EQ:
	case NE:
	case GT:
	case GE:
	case LT:
	case LE:
		type = p->in.left->in.type;
		/* fall through */
	case PLUS:
	case MINUS:
	case ASG PLUS:
	case ASG MINUS:
		if (use68881 || usesky) break;
		switch (type){
		case FLOAT:
		    down1 = D0|MUSTDO;
		    down2 = D1|MUSTDO;
		    break;
		case DOUBLE:
		    down1 = D0|MUSTDO;
		    break;
		}
		break;

	case NOT:
	case ANDAND:
	case OROR:
		down1 = NOPREF;
		break;

	case SCONV: /* float, fix, single, or double */
		if (use68881 || usesky) break;
		if (ezsconv(p)) break;
		down1 = D0|MUSTDO;
		break;

	case FORCE:	
		down1 = D0|MUSTDO;
		break;

	case FNINT:	/* float=>int conversion, use biased rounding */
		if (use68881) break;
		down1 = D0|MUSTDO;
		break;

	default:
		if ((dope[o]&INTRFLG) && !use68881) {
			/* fortran intrinsics */
			down1 = D0|MUSTDO;
		}
	}

recur:
	if( ty == BITYPE ) rallo( p->in.right, down2 );
	else if( ty == LTYPE )  return;
	/* else do tail-recursion */
	p =  p->in.left;
	down = down1;
	goto again;

}

stoasg( p, o ) register NODE *p; 
{
	/* should the assignment op p be stored,
	   given that it lies as the right operand of o
	   (or the left, if o==UNARY MUL) */
/*
	if( p->in.op == INCR || p->in.op == DECR ) return;
	if( o==UNARY MUL && p->in.left->in.op == REG && !isbreg(p->in.left->tn.rval) ) SETSTO(p,INAREG);
 */
	return( shltype(p->in.left->in.op, p->in.left ) );
}

deltest( p ) register NODE *p; 
{
	/* should we delay the INCR or DECR operation p */
# ifndef MULTILEVEL
	if( p->in.op == INCR && p->in.left->in.op == REG && spsz( p->in.left->in.type, p->in.right->tn.lval ) ){
		/* STARREG */
		return( 0 );
		}
# else
	if( mlmatch(p,DEFINCDEC,INCR) && spsz( p->in.left->in.type, p->in.right->tn.lval ) ){
		/* STARREG */
		return( 0 );
		}
#endif
	p = p->in.left;
	return( p->in.op == REG || p->in.op == NAME || p->in.op == OREG );
}
mkadrs(p) register NODE *p; 
{
	register o;

	o = p->in.op;

	if( asgop(o) ){
		if( !SUGT( p->in.right->in.su , p->in.left->in.su )){
			if( p->in.left->in.op == UNARY MUL ){
				if( SUTEST( p->in.left->in.su ) )
					SETSTO( p->in.left->in.left, INTEMP );
				else {
					if (SUTEST( p->in.right->in.su ) )
					    SETSTO( p->in.right, INTEMP );
					else 
					    cerror( "store finds both sides trivial" );
				}
			}
			else if( p->in.left->in.op == FLD 
			    && p->in.left->in.left->in.op == UNARY MUL ){
				SETSTO( p->in.left->in.left->in.left, INTEMP );
			} else { 
				/* should be only structure assignment */
				SETSTO( p->in.left, INTEMP );
			}
		}
		else SETSTO( p->in.right, INTEMP );
	} else {
		if( SUGT( p->in.left->in.su , p->in.right->in.su )){
			SETSTO( p->in.left, INTEMP );
		} else {
			SETSTO( p->in.right, INTEMP );
		}
	}
}


notoff( t, r, off, cp) CONSZ off; char *cp; 
{
	/* is it legal to make an OREG or NAME entry which has an
	/* offset of off, (from a register of r), if the
	/* resulting thing had type t */

	/* yes */
	if (use68020 && isbreg(r))
		return(0);
	if ( off>=-32768 && off<=32767
	    && (cp == NULL || *cp=='\0') && r>=A0 && r<=SP )
		return(0); 
	return(1); /* NO */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define nncon(p)\
    ((p)->in.op == ICON && ((p)->tn.name == NULL || (p)->tn.name[0] == '\0'))

/*
 * Compile an array index expression, exploiting
 * scaled indexing if possible.  Assume an index
 * register is known to be available.
 */
do_index(p)
	register NODE *p;
{
	register NODE *rp,*lp;
	int count;

	switch(p->in.op) {
	case REG:
		return;
	case MUL:
		/*
		 * at least one front-end doesn't do this...
		 */
		rp = p->in.right;
		if (nncon(rp)) {
			count = -1;
			switch(rp->tn.lval) {
			case 1:
				count = 0;
				break;
			case 2:
				count = 1;
				break;
			case 4:
				count = 2;
				break;
			case 8:
				count = 3;
				break;
			}
			if (count >= 0) {
				p->in.op = LS;
				rp->tn.lval = count;
				/*
				 * reset type, because MULS
				 * changes shorts to longs, but
				 * ASL doesn't!
				 */
				p->in.type = p->in.left->in.type;
				goto leftshift;
			}
		}
		break;
	case LS:
	leftshift:
		rp = p->in.right;
		lp = p->in.left;
		if ( lp->in.op == SCONV && lp->in.type == INT ) {
			if ( lp->in.left->in.type == CHAR ) {
				/*
				 * we only need convert bytes to shorts.
				 */
				lp->in.type = SHORT;
				p->in.type = SHORT;
			}
			else if ( use68020 && lp->in.left->in.type == SHORT
			    && lp->in.rall == NOPREF ) {
				/*
				 * on the 68020, a conversion from short
				 * to int below a scale by 2,4, or 8 is
				 * subsumed into the addressing modes
				 */
				p->in.left = lp->in.left;
				lp->in.op = FREE;
				lp = p->in.left;
				p->in.type = lp->in.type;
			}
		} /* lp->in.op == SCONV */
		if (use68020 && nncon(rp) && rp->tn.lval >= 0
			&& rp->tn.lval <= 3){
			/*
			 * on the 68020, scaling by 2, 4, or 8 is also free
			 */
			if (lp->in.op != REG || p->in.rall != NOPREF) {
				lp->in.rall = p->in.rall;
				order(lp,INTAREG|INAREG|INTBREG|INBREG);
			}
			return;
		}
		break;
	case SCONV:
		if (p->in.type == INT) {
			lp = p->in.left;
			if (lp->in.type == CHAR) {
				/*
				 * we only need convert bytes to shorts.
				 */
				p->in.type = SHORT;
			} /* char */
			else if (lp->in.type == SHORT && p->in.rall == NOPREF) {
				/*
				 * conversions from short to int
				 * are done by the addressing modes
				 */
				*p = *lp;
				lp->in.op = FREE;
			} /* short */
		} /* type == INT */
		break;
	} /* switch */
	order(p, INTAREG|INAREG|INTBREG|INBREG);
} /* do_index */

/*
 * is this expression a register or a
 * sign-extending conversion of one?
 */
static int
regsconv(p)
	NODE *p;
{
	NODE *lp;

	if (p->in.op == REG)
		return(1);
	if (p->in.op == SCONV && p->in.type == INT) {
		lp = p->in.left;
		if ( (lp->in.type == SHORT || lp->in.type == CHAR)
			&& lp->in.op == REG )
			return(1);
	}
	return(0);
}

/*
 * is this already a legal index?
 */
int
isindex(p)
	register NODE *p;
{
	register rval;
	NODE *rp;

	if (use68020) {
		if (p->in.op == MUL) {
			rp = p->in.right;
			if (nncon(rp)) {
				switch(rp->tn.lval) {
				case 1:
				case 2:
				case 4:
				case 8:
					return(regsconv(p->in.left));
				} /* switch */
			} /* nncon */
			return(0);
		} /* MUL */
		if (p->in.op == LS) {
			rp = p->in.right;
			if (nncon(rp)
			    && (rp->tn.lval >= 0 && rp->tn.lval <= 3))
				return(regsconv(p->in.left));
			return(0);
		} /* LS */
	} /* use68020 */
	return(regsconv(p));
}

/*
 * is p a signed byte constant ? This really asks whether p is
 * permissible in a (base+disp+index) expression.  On the 68020,
 * anything is permissible, including relocatables.
 */

static int
isbcon(p, t)
	register NODE *p;
	TWORD t;
{
	int maxoff;
	if (p == NIL)
		return 0;
	if (p->in.op == ICON) {
		if (use68020)
			return(1);
		maxoff = ((BTYPE(t) == DOUBLE) ? 123 : 127);
		if (nncon(p) && p->tn.lval <= maxoff && p->tn.lval >= -maxoff)
			return(1);
	}
	return(0);
}

/*
 * Is p a signed short constant ? This really asks whether p
 * is permissible in a (base+disp) expression.  On the 68020,
 * anything is permissible, including relocatables.
 */

static int
isscon(p, t)
	register NODE *p;
	TWORD t;
{
	int maxoff;
	if (p == NIL)
		return 0;
	if (p->in.op == ICON) {
		if (use68020)
			return(1);
		maxoff = ((BTYPE(t) == DOUBLE) ? 32763 : 32767);
		if (nncon(p) && p->tn.lval <= maxoff && p->tn.lval >= -maxoff)
			return(1);
	}
	return(0);
}

#define isbregnode(p) ((p)->in.op == REG && isbreg((p)->tn.rval))

/*
 * Is a register available for indexing? This depends
 * on context larger than the immediate expression,
 * represented by the 'failsafe' flag. (cf., order.c)
 * Note that we should not ask this unless it we are
 * fairly sure we WANT to use double indexing.
 */
static int
can_index(p, base, index)
	NODE *p, *base, *index;
{
	if (isindex(index))	/* already got one */
		return(1);
	failsafe |= ((p->in.rall == D0) || (p->in.rall == D1));
	if (failsafe) {
		/*
		 * We need d0, d1, or both for something; don't tie
		 * either one up.
		 */
		if (!isbregnode(base) || istreg(base->tn.rval)) {
			/*
			 * need a scratch register for the base, too.
			 */
			return(0);	/* forget it! */
		}
		if(!rewrite_rall(index,1)) {
			return(0);
		}
	}
	return(1);
}

/*
 * Compile memory reference expressions, attempting to make the best
 * use of the target machine's addressing modes.  The overall plan
 * is to put the expression into a canonical form, then select and
 * compile a suitable subtree based on availability of registers,
 * offset limitations, and support from the architecture.  A goal
 * is not to evaluate any more of the tree than is necessary.
 *
 * On entry, we assume we have been given the left branch of a UNARY MUL,
 * not the UNARY MUL itself.
 *
 * When done, we must guarantee that the resulting tree can be turned
 * into an OREG.
 */

offstar( p )
	register NODE *p; 
{
	register NODE *lp, *rp;

	if ( (p->in.rall&MUSTDO)
	    || (p->in.op != PLUS && p->in.op != MINUS ) ) {
		/* No chance to index */
		if ( p->in.op != REG || !isbreg(p->tn.rval) ) {
			failsafe = 0;
			order(p, INTBREG|INBREG);
		}
		return;
	}
	/*
	 * exp is (<exp> [+-] <exp>)
	 */
	lp = p->in.left;
	rp = p->in.right;
	if (!ISPTR(lp->in.type)) {
		/*
		 * put the pointer on the left
		 */
		p->in.right = lp;
		p->in.left = rp;
		lp = p->in.left;
		rp = p->in.right;
	}
	/*
	 * u-page hack, for the kernel boys: for example,
	 * (*((struct u *) 0x4000)).u_dent.d_name[i++] = *cp;
	 */
	if (nncon(lp) && isscon(lp) && tlen(rp) == (SZINT/SZCHAR)) {
		/*
		 * swap base part and index part,
		 * then diddle the types
		 */
		TWORD temp;
		p->in.right = lp;
		p->in.left = rp;
		lp = p->in.left;
		rp = p->in.right;
		temp = lp->in.type;
		lp->in.type = rp->in.type;
		rp->in.type = temp;
	}
	/*
	 * map (<exp> - <icon>) to (<exp> + <-icon>)
	 */
	if (p->in.op == MINUS && nncon(rp)) {
		p->in.op = PLUS;
		rp->tn.lval = -(rp->tn.lval);
	}
	/*
	 * map ( (<exp> - <icon> ) [+-] <exp> )
	 * to  ( (<exp> + <-icon>) [+-] <exp> )
	 */
	if (lp->in.op == MINUS && nncon(lp->in.right)) {
		lp->in.op = PLUS;
		lp->in.right->tn.lval = -(lp->in.right->tn.lval);
	}
	/*
	 * At this point, any subtractions must be done explicitly.
	 */
	if ( p->in.op == MINUS ) {
		failsafe = 0;
		order(p, INTBREG|INBREG);
		return;
	}
	if ( lp->in.op == PLUS ) {

		/*
		 * map (<index> + <base>) + <exp>
		 * to  (<base> + <index>) + <exp>
		 */
		if ( !ISPTR(lp->in.left->in.type) ) {
			NODE *q = lp->in.left;
			lp->in.left = lp->in.right;
			lp->in.right = q;
		}

		/*
		 * map (<base> + <index>) + <bcon>
		 * to  (<base> + <bcon>) + <index>
		 */
		if ( isbcon(rp) && !isbcon(lp->in.right) ) {
			NODE *q = lp->in.right;
			lp->in.right = rp;
			rp = p->in.right = q;
		}

		/*
		 * Now we evaluate the expression, or enough of it so
		 * that the remainder can be handled by an addressing mode.
		 */
		if (isbcon(lp->in.right, p->in.type)) {
			/*
			 * pattern is ( <base exp> + <bcon> ) + <index>
			 * where <bcon> is in [-127..127] for the 68010,
			 * or any integral constant on the 68020.
			 */
			NODE *llp = lp->in.left;
			if (SUTEST(llp->in.su) && SUTEST(rp->in.su)) {
				/*
				 * don't bother;
				 * both <base> and <index> are messy.
				 */
				goto punt;
			}
			if (SUGT(rp->in.su, llp->in.su)){
				/*
				 * Index is more expensive than base.
				 * Do it first, unless you have to bend
				 * over backwards to do it.
				 */
				if (can_index(p,llp,rp)) {
					do_index(rp);	/* breg@(disp,index) */
					if (!isbregnode(llp)) {
						order(llp, INTBREG|INBREG);
					}
					return;
				}
				goto punt;
			}
			/*
			 * try for index
			 */
			if (can_index(p,llp,rp)) {
				/*
				 * evaluate base into a <breg>
				 */
				if (!isbregnode(llp)) {
					order(llp, INTBREG|INBREG);
				}
				/*
				 * evaluate <index> into a register
				 */
				do_index(rp);	/* breg@(disp, index) */
				return;
			}
			/*
			 * can't index; instead,
			 * rewrite as ( <base>+<index> ) + <bcon>
			 */
		punt:	llp = lp->in.right;
			lp->in.right = rp;
			p->in.right = llp;
			failsafe = 0;
			order(lp, INTBREG|INBREG);  /* breg@(disp) */
			return;
		} /* (<base> + <bcon>) + <index> */
	} /* lp == PLUS */

	/*
	 * At this point, the expression is of the form
	 *	<base part> + <index part>
	 */
	if (SUTEST(lp->in.su) && SUTEST(rp->in.su)) {
		/*
		 * both <base> and <index> are messy.
		 */
		failsafe = 0;
		order(p, INTBREG|INBREG);	/* breg@ */
		return;
	}
	if ( SUGT(rp->in.su, lp->in.su) ) {
		/*
		 * index is more expensive than base.
		 * Do it first, unless you have to bend
		 * over backwards to do it.
		 */
		if (can_index(p,lp,rp)) {
			do_index(rp);
			if (!isbregnode(lp)){
				order(lp, INTBREG|INBREG);
			}
			return;		   /* base@(index)*/
		}
		failsafe = 0;
		order(p, INTBREG|INBREG);	/* base@ */
		return;
	}
	/*
	 * try to make a breg@(disp) operand
	 */
	if ( isscon(rp) ) {
		failsafe = 0;
		if (!isbregnode(lp)) {
			order(lp, INTBREG|INBREG);
		}
		return;				/* breg@(disp) */
	}
	/*
	 * Do base first.
	 */
	if ( !isbregnode(lp) ) {
		order(lp, INTBREG|INBREG);
	}
	if ( istnode(lp) && tshape(rp, SCON|SNAME|SOREG|STARNM) ) {
		/*
		 * pattern is <temp reg> + <mem operand>
		 * indexing doesn't buy us anything.
		 */
		failsafe = 0;
		order(p, INTBREG|INBREG);	/* base@ */
		return;
	}
	if ( can_index(p,lp,rp) ) {
		do_index(rp);			/* base@(index) */
		return;
	}
	/*
	 * "Either this man is dead, or my watch has stopped."
	 */
	failsafe = 0;
	order(p, INTBREG|INBREG);	/* base@ */
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int crslab = 2000000;  /* 68k */

getlab()
{
	return( crslab++ );
}

deflab( l )
{
	print_label(l);
}

#define mkmask(reg,mask) (busy[reg] ? ((mask) | (1<<(reg))) : (mask))

genargs( p ) register NODE *p; 
{
	register NODE *pasg;
	register inc;
	register align;
	register size;
	register TWORD type;
	int padflag;

	/* generate code for the arguments */

	/*  first, do the arguments on the right */
	while( p->in.op == CM ){
		genargs( p->in.right );
		p->in.op = FREE;
		p = p->in.left;
	}

	if( p->in.op == STARG ){ /* structure valued argument */
		/*
		 * We are stacking arguments in anticipation of a 
		 * call.
		 * The strategy will be to push words on the
		 * stack (being careful of alignment problems)
		 * in open code. A dbra loop is too hard here.
		 */
		size = p->stn.stsize;
		if( p->in.left->in.op == ICON ){
			p->in.op = FREE;
			p= p->in.left;
			p->in.op = NAME;
		}
		else {
			/* make it look beautiful... */
			p->in.op = UNARY MUL;
			canon( p );  /* turn it into an oreg */
			if( p->in.op != OREG ){
				offstar( p->in.left );
				canon( p );
				if( p->in.op != OREG ){
					offstar( p->in.left );
					canon( p );
					if( p->in.op != OREG ) cerror( "stuck starg" );
				}
			}
		}

		if (size > 16) {
			/*
			 * big structure argument.  In its infinite wisdom,
			 * Pascal passes array value parameters like this.
			 * Twenty-two on the vomit meter.
			 */
			int count;
			int residue;
			short regmask;
			int savetemp;

			if (size&1) size++; /* assumes ALSTACK = 16 */
			toff += size;
			if (toff > maxtoff) maxtoff = toff;
			count = size / sizeof(long);
			residue = size % sizeof(long);
			/*
			 * a0 = &(actual argument);
			 */
			if (p->in.op == OREG && !R2TEST(p->tn.rval)
			    && (p->tn.name == NULL || p->tn.name[0] == '\0')
			    && p->tn.lval == 0) {
				p->in.op = REG;
			}
			if (p->in.op == REG) {
			    if (p->tn.rval != A0)
				expand(p, RNOP, "	movl	AR,a0\n");
			} else {
				expand(p, RNOP, "	lea	AR,a0\n");
			}
			/*
			 * allocate and copy to top of stack
			 */
			if (size > 0x7fff) {
				printf("	subl	#%d,sp\n", size);
			} else {
				printf("	lea	sp@(-%d),sp\n", size);
			}
			print_str("	movl	sp,a1\n");
			if (count > 0x7fff) {
				printf("	movl	#%d,d0\n", count-1);
				print_str("1:	movl	a0@+,a1@+\n");
				print_str("	dbra	d0,1b\n");
				print_str("	clrw	d0\n");
				print_str("	subql	#1,d0\n");
				print_str("	jcc	1b\n");
			} else {
				printf("	movw	#%d,d0\n", count-1);
				print_str("1:	movl	a0@+,a1@+\n");
				print_str("	dbra	d0,1b\n");
			}
			switch(residue) {
			case 1:
				print_str("	movb	a0@+,a1@+\n");
				break;
			case 2:
				print_str("	movw	a0@+,a1@+\n");
				break;
			case 3:
				print_str("	movw	a0@+,a1@+\n");
				print_str("	movb	a0@+,a1@+\n");
				break;
			default:
				break;
			}
			reclaim(p,RNULL,0);
			return;
		} /* big structure */

		p->tn.lval += size;
		/*
		 * arguments must be at least sizeof(int) big around here.
		 * force size up to it.
		 */
		padflag = (size <= 2 ) ;
#ifdef FORT
		/* PASCAL ONLY */
		if (size & 1){
			/*
			 * size is odd: last byte is peculiar.
			 * movb to stack is magic.
			 */
			size -= 1;
			p->tn.lval -=1;
			expand( p, RNOP,"\tmovb\tAR,sp@-\n" );
			toff += 2;
			if (toff > maxtoff) maxtoff = toff;
		}
#endif FORT
		toff += size;
		if (toff > maxtoff) maxtoff = toff;
		for( ; size>0; size -= inc ){
			inc = (size>2) ? 4 : 2;
			p->tn.lval -= inc;
			expand( p, RNOP,(inc==4)?"\tmovl\tAR,sp@-\n":"\tmovw\tAR,sp@-\n" );
		}
		if (padflag){
		    print_str("	subql	#2,sp\n");
		    toff += 2;
		    if (toff > maxtoff) maxtoff = toff;
		}
		reclaim(p,RNULL,0);
		return;
	}

	/* ordinary case */
	order( p, FORARG );
}

argsize( p ) register NODE *p; 
{
	register t, s;
	t = 0;
	if( p->in.op == CM ){
		t = argsize( p->in.left );
		p = p->in.right;
	}
	switch (p->in.type){
	case FLOAT:
#ifdef FLOATMATH
		if (FLOATMATH>1){
		    SETOFF( t, ALFLOAT/SZCHAR );
		    return( t+(SZFLOAT/SZCHAR) );
		}
#endif FLOATMATH
		/* else fall through */
	case DOUBLE:
		SETOFF( t, ALDOUBLE/SZCHAR );
		return( t+(SZDOUBLE/SZCHAR) );
	}
	if( p->in.op == STARG ){
 		SETOFF( t, ALSTRUCT/SZCHAR );  /* alignment */
		s = p->stn.stsize;            /* size */
		s += (s & 1); /* PASCAL ONLY */
		if (s < SZINT/SZCHAR)
		    s = SZINT/SZCHAR;
		return( s+t );
	} else {
		SETOFF( t, ALSTACK/SZCHAR );
		return( t+(SZINT/SZCHAR) );
	}
}
