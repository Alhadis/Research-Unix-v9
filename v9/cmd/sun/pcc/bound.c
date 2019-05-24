#ifndef lint
static	char sccsid[] = "@(#)bound.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "cpass2.h"
#include "ctype.h"

int failsafe; /* very disgusting: see offstar() for use */

# define SAFETY( a, reg,  b ) \
	{if ((reg & MUSTDO) && find_mustdo( a , reg )) {rewrite_rall( b, 1 );} }
# define isareg(r) (rstatus[r]&SAREG)
# define iscnode(p) (p->in.op==REG && iscreg(p->tn.rval))

# define max(x,y) ((x)<(y)?(y):(x))
# define min(x,y) ((x)<(y)?(x):(y))


static char *unsigned_branches[2] = { "jcs", "jls"};
static char   *signed_branches[2] = { "jlt", "jle"};

trapv(type)
{
    if (chk_ovfl){
	if (!ISPTR(type) && !ISUNSIGNED(type)){
	    print_str( "	trapv\n" );
	}
    }
}


#ifdef NOTDEF
chksize(p)
    NODE *p;
{
    /* check size of result after a multiply instruction */
    int chklab = getlab();
    int oklab;
    int type = p->in.type;
    int lb, ub;
    char *regname = rnames[p->in.left->tn.rval];
    if (chk_ovfl){
	if (use68020){
	    switch(type){
	    case CHAR:
		printf("\t.data\nL%d:\t.long\t-0x80,0x7f\n\t.text\n", chklab);
		break;
	    case UCHAR:
		printf("\t.data\nL%d:\t.long\t0,0xff\n\t.text\n", chklab);
		break;
	    case SHORT:
		printf("\t.data\nL%d:\t.long\t-0x8000,0x7fff\n\t.text\n", chklab);
		break;
	    case USHORT:
		printf("\t.data\nL%d:\t.long\t0,0xffff\n\t.text\n", chklab);
		break;
	    default: 
		trapv( type );
		return;
	    }
	    printf("	chk2l	L%d,%s\n", chklab, regname);
	    return;
	}else {
	    oklab = getlab();
	    switch(type){
	    case CHAR:
			lb = -128; ub = 127;
			break;
	    case SHORT:
			lb = -32768; ub = 32767;
	    signed:
			printf("	cmpl	#%#x,%s\n", lb, regname);
			break;
	    case UCHAR:
		        ub = 255;
			break;
	    case USHORT:
		        ub = 0xffff;
			break;
	    default: 
			trapv( type );
			return;
	    }
	    printf("	jlt	L%d\n", chklab);
	    printf("	cmpl	#%#x,%s\n", ub, regname);
	    printf("	jle	L%d\n", oklab);
	    printf("L%d:	chk	#-1,%s\n", chklab, regname);
	    printf("L%d:\n", oklab);
	}
    }
}
#endif

/* generate a copy of p ; p should be simple */

static char *
rcopy(p)
    NODE *p;
{
    NODE *temp;
    int r;

    if (busy[D0] && busy[D1])
	return(NULL);
    temp = tcopy(p);
    order(temp, INTAREG);
    r = temp->tn.rval;
    reclaim(temp, RNULL, FOREFF);
    return(rnames[r]);
}

bound_test( p, cookie )
    register NODE *p;
{
    register NODE *expr, *lb, *ub;
    register t = p->in.type;
    NODE *bounds = p->in.right;
    int chklab, oklab;
    char *size;
    char  **branch;
    NODE *copy;
    char *regname;

    expr = p->in.left;
    lb = bounds->in.left;
    ub = bounds->in.right;
    if (!chk_ovfl){
	order( expr, cookie );
	return;
    }
    regname = rnames[expr->tn.rval];
    /*
     * see if we can use the chk or chkl instructions.
     * For nonzero lower bounds, try to generate a copy and
     * subtract the lower bound.  In array subscripting the
     * lower bound must be subtracted anyway; c2 may be able
     * to identify this as a cse.
     */
    if (lb->in.op == ICON && lb->in.name[0] == '\0' &&
	ub->in.op == ICON && ub->in.name[0] == '\0' ){
	/* constant bounds */
	switch(t){
	case CHAR:
	    printf("	extw	%s\n", regname );
	    p->in.type = expr->in.type = SHORT;
	    goto use_chkw;
	case UCHAR: 
	    printf("	andw	#0xff,%s\n", regname );
	    p->in.type = expr->in.type = SHORT;
	    /* FALL THROUGH */
	case SHORT:
	use_chkw:
	    if (lb->tn.lval) {
		char *regcopy = rcopy(expr);
		if (regcopy == NULL) goto long_case;
		regname = regcopy;
		printf("	subw	#0x%x,%s\n", lb->tn.lval, regname);
	    }
	    if (ub->tn.lval - lb->tn.lval < 0x8000) {
		printf("	chk	#0x%x,%s\n", ub->tn.lval-lb->tn.lval,
		    regname);
	    } else {
		printf("	cmpw	#0x%x,%s\n", ub->tn.lval-lb->tn.lval,
		    regname);
		oklab = getlab();
		printf("	jls	L%d\n", oklab);
		printf("	chk	#-1,%s\n", regname);
		printf("L%d:\n", oklab);
	    }
	    break;
	case USHORT:
	    if (ub->tn.lval <= 0x7fff) goto use_chkw;
	    printf("	andl	#0xffff,%s\n", regname);
	    p->in.type = expr->in.type = INT;
	    /* FALL THROUGH */
	default:
	    if (lb->tn.lval == -32768 && ub->tn.lval == 32767) {
		/* common case, can be done without long literals */
		char *regcopy = rcopy(expr);
		if (regcopy == NULL) goto long_case;
		regname = regcopy;
		printf("	extl	%s\n", regname);
		expand(p, cookie, "	cmpl	AL,");
		printf("%s\n", regname);
		oklab = getlab();
		printf("	jeq	L%d\n", oklab);
		printf("	chk	#-1,%s\n", regname);
		printf("L%d:\n", oklab);
		break;
	    }
	    if (lb->tn.lval) {
		char *regcopy = rcopy(expr);
		if (regcopy == NULL) goto long_case;
		regname = regcopy;
		printf("	subl	#0x%x,%s\n", lb->tn.lval, regname);
	    }
	    if (use68020 && (unsigned)(ub->tn.lval-lb->tn.lval) < 0x80000000) {
		printf("	chkl	#0x%x,%s\n", ub->tn.lval-lb->tn.lval,
		    regname);
	    } else {
		printf("	cmpl	#0x%x,%s\n", ub->tn.lval-lb->tn.lval,
		    regname);
		oklab = getlab();
		printf("	jls	L%d\n", oklab);
		printf("	chk	#-1,%s\n", regname);
		printf("L%d:\n", oklab);
	    }
	    break;
	}
	return;
long_case:
	if (use68020) {
	    /* use chk2l */
	    switch(t){
	    case CHAR:
	    case UCHAR:
		size = "byte"; break;
	    case SHORT:
	    case USHORT:
		size = "word"; break;
	    default:
		size = "long"; break;
	    }
	    chklab = getlab();
	    printf("	.data\nL%d: .%s	%d,%d\n	.text\n",
		chklab, size, lb->tn.lval, ub->tn.lval);
	    printf("	chk2%c	L%d,%s\n", size[0], chklab, regname);
	    return;
	}
    }
    /* can't use any chk instructions -- do cmp & branch */
    branch = unsigned_branches;
    if (ISUNSIGNED(t) || ISPTR(t))
	branch = unsigned_branches;
    else
	branch =   signed_branches;
    chklab = getlab();
    oklab  = getlab();
    if (adjacent(lb, ub)) {
	/*
	 * bounds are in adjacent memory operands
	 */
	if (SUTEST(lb->in.su))
	    order(lb, SOREG);
	p->in.right = lb;
	if (use68020) {
	    expand( p, FORCC, "	chk2ZB	AR,AL\n");
	    p->in.right = bounds;
	    reclaim( lb, RNULL, FOREFF );
	    reclaim( ub, RNULL, FOREFF );
	    return;
	} else {
	    expand( p, FORCC, "	cmpZB	AR,AL\n");
	    printf("	%s	L%d\n", branch[0], chklab);
	    expand( p, FORCC, "	cmpZB	UR,AL\n");
	    printf("	%s	L%d\n", branch[1], oklab);
	    reclaim( lb, RNULL, FOREFF );
	    reclaim( ub, RNULL, FOREFF );
	    /* finish up below */
	}
    } else {
	/* 
	 * lower bound
	 */
	if (SUTEST(lb->in.su)) {
	    order( lb, INTAREG|INTEMP|SOREG );
	}
	p->in.right = lb;
	expand( p, FORCC, "	cmpZB	AR,AL\n");
	printf("	%s	L%d\n", branch[0], chklab);
	reclaim( lb, RNULL, FOREFF );
	/* 
	 * upper bound
	 */
	if (SUTEST(ub->in.su)) {
	    order( ub, INTAREG|INTEMP|SOREG );
	}
	p->in.right = ub;
	expand( p, FORCC, "	cmpZB	AR,AL\n");
	printf("	%s	L%d\n", branch[1], oklab);
	reclaim( ub, RNULL, FOREFF );
    }

    printf("L%d:	chk	#-1,%s\n", chklab, regname);
    printf("L%d:\n", oklab);
    p->in.right = bounds;

}


#define true 1
#define false 0

/*
 * are operands p and q adjacent
 * (in the sense assumed by upput() and adrput())?
 */
int
adjacent(p,q)
    register NODE *p,*q;
{
    register NODE *lp, *lq;
    register o;
    int result;
    CONSZ temp;

    o = p->in.op;
    if (o != q->in.op)
	return false;
    switch(o) {
    case NAME:
    case OREG:
	p->tn.lval+= SZINT/SZCHAR;
	result = equal(p,q);
	p->tn.lval-= SZINT/SZCHAR;
	return result;
    case UNARY MUL:
	lp = p->in.left;
	lq = q->in.left;
	if (lp->in.op == PLUS && lq->in.op == PLUS) {
	    lp = lp->in.right;
	    lq = lq->in.right;
	    if (lp->in.op == ICON && lq->in.op == ICON) {
		if (lp->tn.lval+SZINT/SZCHAR == lq->tn.lval) {
		    lp->tn.lval += SZINT/SZCHAR;
		    result = equal(p,q);
		    lp->tn.lval -= SZINT/SZCHAR;
		    return result;
		}
	    }
	}
	break;
    }
    return false;
}

/*
 * equal(p,q) : returns 1 if expressions p and q are equivalent,
 *	in the sense that they always return the same value given
 *	the same initial values of their operands.  Note that if
 *	either p or q contains operators producing side-effects,
 *	equal(p,q) = 0.
 */

int
equal(p,q)
    register NODE *p,*q;
{
    register char *pn,*qn;
    register o;

    if (p == q)
	return true;
    if (p == NIL || q == NIL)
	return false;
    o = p->in.op;
    if (o != q->in.op)
	return false;
    if (p->in.type != q->in.type)
	return false;
    switch(optype(o)) {
    case UTYPE:
	if (callop(o))
	    return false;
	return equal(p->in.left, q->in.left);
    case BITYPE:
	if (callop(o) || asgop(o))
	    return false;
	if (equal(p->in.left, q->in.left))
	    return equal(p->in.right, q->in.right);
	return false;
    default:
	/* leaf nodes */
	switch (o) {
	case ICON:
	case OREG:
	case NAME:
	    pn = p->tn.name;
	    qn = q->tn.name;
	    if (pn != NULL && qn != NULL) {
		if ( *pn == *qn 
		  && (*pn == '\0' || strcmp(pn,qn) == 0) ) {
		    return (p->tn.lval == q->tn.lval);
		}

	    } else if (pn == qn) {	/* == NULL */
		return ( p->tn.lval == q->tn.lval );
	    }
	    return false;
	case REG:
	    return(p->tn.rval == q->tn.rval);
	case FCON:
	    return(p->fpn.dval == q->fpn.dval);
	}
    }
    return false;
}
