#ifndef lint
static	char sccsid[] = "@(#)optim2.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "cpass2.h"

/* 
 * routines to detect and to deal with doubly-indexed OREGs.
 * Initially, these were all stolen from the vax code.
 */

base( p ) 
    register NODE *p; 
{
    register int o = p->in.op;
    register v,maxoff;
    register NODE *lp,*rp;

    if ( BTYPE(p->in.type) == DOUBLE )
	maxoff = 123;
    else
	maxoff = 127;
    if ( o==REG && isbreg( p->tn.rval ))
	return( p->tn.rval );
    lp = p->in.left;
    rp = p->in.right;
    if ((o==PLUS || o==MINUS) 
	&& lp->in.op==REG  && isbreg( lp->tn.rval )
	&& rp->in.op==ICON && rp->in.name[0] == '\0'
	&& (use68020 || (v=rp->tn.lval) <= maxoff && v >= -128)
    )
	return( lp->tn.rval );
    return( -1 );
}

offset( p, notused ) 
    register NODE *p;
    int notused; 
{
    if ( p->in.op==REG ) {
	if ( p->in.type==INT || p->in.type==UNSIGNED || ISPTR(p->in.type) ) {
	    return( p->tn.rval );
	}
	if ( p->in.type==SHORT ) {
	    /*
	     * result is: short index flag + xreg
	     */
	    return( (1<<8) + p->tn.rval );
	}
    }
    if (use68020) {
	/*
	 * test for scaled indexing
	 */
	int count;
	NODE *lp = p->in.left;
	NODE *rp = p->in.right;
	if ( p->in.op==LS && lp->in.op==REG 
	&& (lp->in.type==INT || lp->in.type==UNSIGNED || lp->in.type == SHORT )
	&& (rp->in.op==ICON && rp->in.name[0]=='\0')
	&& (count = rp->tn.lval) >= 0 && count <= 3 ) {
	    int scale = (1 << count);
	    if ( p->in.type == SHORT ) {
		/*
		 * result is: short index flag + scale factor + xreg
		 */
		return( (1<<8) + (scale<<4) + lp->tn.rval );
	    }
	    /*
	     * result is: scale factor + xreg
	     */
	    return( (scale<<4) + lp->tn.rval );
	}
    }
    return( -1 );
}

makeor2( p, basenode, breg, scale_plus_xreg )
    register NODE *p;		/* node to be rewritten */
    register NODE *basenode;	/* base node (usually a ptr) */
    int breg;			/* base register */
    int scale_plus_xreg;	/* scale factor + index register */
{
    register NODE *t;	/* from which tn.lval, tn.name are obtained */
    register int i;
    NODE *f;

    register flags;
    int scale, xreg, shortx, ibit;

    ibit = 0;
    shortx= (scale_plus_xreg >> 8) & 1;
    scale = (scale_plus_xreg >> 4) & 017;
    xreg  = scale_plus_xreg & 017;

    p->in.op = OREG;
    f = p->in.left; 	/* have to free this subtree later */

    /* init base */
    switch (basenode->in.op) {
	    case ICON:
	    case REG:
	    case OREG:
		    t = basenode;
		    break;

	    case MINUS:
		    basenode->in.right->tn.lval = -basenode->in.right->tn.lval;
	    case PLUS:
		    t = basenode->in.right;
		    break;

	    case INCR:
	    case ASG MINUS:
		    t = basenode->in.left;
		    break;

	    case UNARY MUL:
		    t = basenode->in.left->in.left;
		    break;

	    default:
		    cerror("illegal makeor2");
    }

    p->tn.lval = t->tn.lval;
#ifndef FLEXNAMES
    for(i=0; i<NCHNAM; ++i)
	    p->in.name[i] = t->in.name[i];
#else
    p->in.name = t->in.name;
#endif

    /*
     * For 68020, R2UPK3 encodes the following data:
     *     int shortx:1; short index
     *     int ibit:1; memory indirect mode (unused)
     *     int scale:4; scale factor (1,2,4,8)
     * For 68010, only the short index flag is encoded.
     */
    R2PACKFLGS(flags,shortx,ibit,scale);
    p->tn.rval = R2PACK( breg, xreg, flags );

    tfree(f);
    return;
}


/*
 * Multiply a value by a constant.
 * Multiplies are slow on the 68010, so do some shifting and
 * adding here.
 * For each 1-bit in the constant the value is shifted
 * by n where 2**n equals the bit.
 * The shifted value is then added into an accumulated value.
 * One tricky part is for runs of three or more consecutive
 * bits.  For a number like 7, it is cheaper to calculate
 * (8 - 1), than it is (3 + 2 + 1)
 */
conmul(p, cookie)
	register NODE	*p;
{
	register const;		/* The constant value */
	register int	curpos;	/* Position of last one bit */
	register int	power;	/* The bit being tested */
	int	run;		/* Num of consecutive one bits */
	int	nbits;		/* Num of bits since the last one bit */
	int	negconst;	/* Is the constant <0 ? */
	char *  destreg;	/* name of "AL" register */
	char *  helper;		/* name of "A1" register */
	int	helperreg;
	TWORD	ltype;
	char	typechar;
	int	t;

	/*
	 * first determine how many instructions we will need to do
	 * it in line. Cost estimator is: # single bits + 2* # of multi bit
	 * runs. If this exceeds 5, then it is more compact to use a 
	 * multiply instruction or routine.
	 */
	const = p->in.right->tn.lval;
	if (const<0){
	    negconst = 1;
	    const = - const;
	} else 
	    negconst = 0;
	run = const & (const>>1); /* each run shortened by one bit */
	run = cntbits( run ^ (run>>1) ) + (run&1); /* 2*# multibit runs */
	nbits = cntbits( const ^ (const>>1)) +(const&1); /* 2* #total runs*/
	run += nbits; /* 2* estimator */
	ltype = p->in.left->in.type;
	if ((ttype(ltype,TUCHAR|TCHAR|TUSHORT|TSHORT) && run>6) || (run > 10)){
	    /* old-fashioned way */
	    if (tshape( p->in.right, SSCON ) ){
		if (negconst){
		    expand( p->in.left, INAREG|INTAREG, "	negZB	AL\nZv");
		    p->in.right->tn.lval = const;
		}
		if (ttype(ltype, TSHORT|TCHAR)){
		    expand( p, cookie, "	muls	#CR,AL\nZv");
		} else if (ttype(ltype, TUSHORT|TUCHAR)){
		    expand( p, cookie, "	mulu	#CR,AL\n");
		} else if (use68020) {
		    if (ISUNSIGNED(p->in.type)) {
			expand( p, cookie, "	mulul	#CR,AL\n");
		    } else {
			expand( p, cookie, "	mulsl	#CR,AL\nZv");
		    }
		} else {

		    expand( p, cookie,
			"\tmovl\tAL,A1\n\tswap\tA1\n\tmulu\t#CR,AL\n");
		    fputs( ttype(ltype, TULONG|TUNSIGNED|TUCHAR|TPOINT)
			? "\tmulu" : "\tmuls", stdout);
		    expand( p, cookie, 
			"\t#CR,A1\n\tswap\tA1\n\tclrw\tA1\n\taddl\tA1,AL\nZv");
		}
	    } else if (use68020) {
		if (ISUNSIGNED(p->in.type)) {
		    expand( p, cookie, "	mulul	#CR,AL\n");
		} else {
		    expand( p, cookie, "	mulsl	#CR,AL\nZv");
		}
	    } else {
		order( p->in.right, INTAREG );
		order( p, INAREG|INTAREG );
	    }
	    return;
	}
	typechar = (((t=p->in.type)==CHAR)||t==UCHAR) ? 'b' 
		 :  (t==SHORT||t==USHORT) ? 'w'
		 : 'l';
	/*
	 * if the result type is larger than the operand type, we must expand
	 * the operand before doing the operation.
	 */
	switch( p->in.left->tn.type){
	case UCHAR: 
	case USHORT: 
		if (typechar=='b' || typechar=='w') break;
		expand( p->in.left, INAREG|INTAREG, "	andl	#0xffff,AL\n");
		break;
	case CHAR:
	case SHORT: 
		if (typechar=='b' || typechar=='w') break;
		expand( p->in.left, INAREG|INTAREG, "	extl	AL\n");
		break;
	}
	p->in.left->tn.type = t;
	nbits = ffs( const )-1;	
	power = 0;
	helperreg = resc[0].tn.rval ;
	helper = rnames[ helperreg ];
	destreg  = rnames[ p->in.left->tn.rval];
	if (negconst){
	    expand( p->in.left, INAREG|INTAREG, "	negZB	AL\nZv");
	}
	if (nbits>0){
	    shiftreg( nbits, destreg, helperreg , 1, typechar, t);
	    const >>= nbits;
	}
	if (const==1) return;
	/* 
	 * the first time is special (ask Ann Landers). If it is a run,
	 * then we move, negate, shift add. Otherwise, we don't negate.
	 */
	run = ffs( ~const) -1;
	expand(p, cookie, "	movZB	AL,A1\n");
	switch (run){
	case 2:
		shiftreg( 1, helper, -1, 1, typechar, t);
		expand( p, cookie, "\taddZB\tA1,AL\nZv");
		/* fall through */
	case 1: 
		curpos = 0;
		break;
	default:
		shiftreg( run, helper, -1, 1, typechar, t);
		expand( p->in.left, INAREG|INTAREG, "	negZB	AL\nZv");
		expand( p, cookie, "\taddZB\tA1,AL\nZv");
		curpos = 1;
		break;
	}
	for ( const >>= run; const >>= (power=ffs(const))-1; const >>= run){
		run = ffs(~const)-1;
		switch(run) {
		case 1:
			shiftreg( power-curpos, helper, -1, 1, typechar, t);
			expand( p, cookie, "\taddZB\tA1,AL\nZv");
			curpos = 0;
			break;
		case 2:
			shiftreg( power-curpos, helper, -1, 1, typechar, t);
			expand( p, cookie, "\taddZB\tA1,AL\nZv");
			shiftreg( 1, helper, -1, 1, typechar, t);
			expand( p, cookie, "\taddZB\tA1,AL\nZv");
			curpos = 0;
			break;
		default:
			shiftreg( power-curpos, helper, -1, 1, typechar, t);
			expand( p, cookie, "\tsubZB\tA1,AL\nZv");
			shiftreg( run, helper, -1, 1, typechar, t);
			expand( p, cookie, "\taddZB\tA1,AL\nZv");
			curpos = 1;
			break;
		}
	}
}

/*
 * divide a signed number by a constant.
 * divides are really slow on this machine, so we really want to avoid them.
 * the only interesting case is divide by power-of-two. Others, we really
 * have to do the division.
 */
void
condiv( p, cookie ) register NODE *p;
{
    char sizechar;
    register const;
    register TWORD ltype;
    int lab1f;
    char * lname = rnames[p->in.left->tn.rval];
    const = p->in.right->tn.lval;
    ltype = p->in.left->tn.type;
    if (const & (const-1)){
	/* too bad -- not a positive power of two */
	switch (ltype){
	case CHAR:
		print_str_str_nl( "	extw	", lname );
		/* fall through */
	case SHORT:
		print_str_str_nl( "	extl	", lname );
		printf( "	divs	#%d,%s\n", const, lname);
		break;
	default:
		if (use68020) {
			printf( "	divsl	#%d,%s\n",
				const, lname);
		} else {
			order( p->in.right, INTAREG );
			order( p, INTAREG );
		}
		break;
	}
	return;
    }
    switch (ltype){
    case CHAR: sizechar = 'b'; break;
    case SHORT: sizechar = 'w'; break;
    default: sizechar = 'l';
    }
    lab1f = getlab();
    printf("	tst%c	%s\n	jge	L%d\n", sizechar, lname, lab1f);
    print_str((const >= 1 && const <= 8) ? "	addq" : "	add");
    printf("%c	#%d,%s\n", sizechar, const-1, lname );
    deflab(lab1f);
    const = ffs(const)-1;
    /* like shiftreg(), but for right, arithmetic shifts */
    while (const > 0){
	if (const> 8){
	    printf("	asr%c	#8,%s\n", sizechar, lname );
	    const -= 8;
	} else {
	    printf("	asr%c	#%d,%s\n", sizechar, const, lname );
	    const = 0;
	}
    }
    return;
}


/*
 * remainder a signed number by a constant.
 * remainders are really slow on this machine, so we really want to avoid them.
 * the only interesting case is powers-of-two. Others, we really
 * have to do the division.
 */
void
conrem( p, cookie ) register NODE *p;
{
    char sizechar, opchar;
    register const;
    register TWORD ltype;
    char * lname = rnames[p->in.left->tn.rval];
    char * helper, u;
    int lab1f, lab2f;
    const = p->in.right->tn.lval;
    ltype = p->in.left->tn.type;
    if (const & (const-1)){
	/* too bad -- not a positive power of two */
	switch (ltype){
	case CHAR:
		print_str_str_nl( "	extw	", lname );
		/* fall through */
	case SHORT:
		print_str_str_nl( "	extl	", lname );
		printf( "	divs	#%d,%s\n", const, lname);
		print_str_str_nl( "	swap	", lname );
		break;
	case UCHAR:
		print_str_str_nl("	andl	#0xff,", lname );
		goto divu;
	case USHORT:
		print_str_str_nl("	andl	#0xffff,", lname );
	divu:	printf("	divu	#%d,%s\n", const, lname);
		print_str_str_nl("	swap	", lname);
		break;
	default:
		if (use68020) {
			helper = rnames[resc[0].tn.rval];
			printf("	%s	#%d,%s:%s\n",
				(ISUNSIGNED(ltype)? "divull" : "divsll"),
				const, helper, lname);
			printf("	movl	%s,%s\n", helper, lname);
		} else {
			order( p->in.right, INTAREG );
			order( p, INTAREG );
		}
		break;
	}
	return;
    }
    helper = rnames[resc[0].tn.rval];
    u = 0;
    switch (ltype){
    case UCHAR: u = 1; /* fall through */
    case CHAR: sizechar = 'b'; opchar = 'w'; break;
    case USHORT: u = 1; /* fall through */
    case SHORT: sizechar = 'w'; opchar = 'w'; break;
    case UNSIGNED: u = 1; /* fall through */
    default: sizechar = 'l'; opchar = 'l';
    }
    if (!u){
	lab1f = getlab();
	lab2f = getlab();
    }
    if (const<=128 && const >=-128){
	printf("	moveq	#%d,%s\n", const-1, helper);
	if (!u){
	    printf("	tst%c	%s\n	jge	L%d\n	neg%c	%s\n",
		sizechar, lname, lab1f, sizechar, lname);
	    printf("	and%c	%s,%s\n	neg%c	%s\n	jra	L%d\n",
		opchar, helper, lname, opchar, lname, lab2f );
	    deflab( lab1f );
	}
	printf("	and%c	%s,%s\n", opchar, helper, lname);
	if (!u){ deflab(lab2f);}
    } else {
	if (!u){
	    printf("	mov%c	%s,%s\n	jge	L%d\n	neg%c	%s\n",
		    sizechar, lname, helper, lab1f, sizechar, lname);
	    deflab(lab1f);
	}
	printf("	and%c	#%d,%s\n", opchar, const-1, lname );
	if (!u){
	    printf("	tst%c	%s\n	jge	L%d\n	neg%c	%s\n",
		    sizechar, helper, lab2f, opchar, lname );
	    deflab(lab2f);
	}
    }
    return;
}

optim2( p )
register NODE *p; 
{
	register NODE *q;
	register NODE *r;
	register long v;
	register op;
	int w;
	int lsize, rsize;
	NODE *rl, *rr, *qq;
	TWORD t;

	/*
	 * reduction of strength on integer constant operands
	 * this is a practical, fruitful area of peephole code optimization,
	 * since there are so many easy things that can be done.
	 * we also check for possible single-o OREGs in a place where
	 * the oreg2() should but does not.
	 */
	op = p->in.op;
	switch (optype(op)) {
	case LTYPE:
		break;
	case UTYPE:
		optim2(p->in.left);
		break;
	case BITYPE:
		optim2(p->in.left);
		optim2(p->in.right);
		break;
	}
	switch (op){
	case LS:
	case ASG LS:
	case RS:
	case ASG RS:
		/* if rhs is an extending SCONV, don't bother converting */ 
		r = p->in.right;
		if (isconv(r, SHORT, USHORT) || isconv(r, CHAR, UCHAR)) {
		    q = r->in.left;
		    *r = *q;
		    q->in.op = FREE;
		}
		/* one interesting case: <lhs> <op> 0  */
		if (r->tn.op != ICON ) break;
		if (r->tn.lval == 0 && r->tn.name[0] == '\0' ){
promoteleft:
		    t = p->in.type;
		    q = p->in.left;
		    *p = *q; /* paint over */
		    p->in.type = t;
		    q->in.op = r->in.op = FREE;
		}
		/* could test for count of 1, too, but this is easier to do later on */
		break;

	case UNARY MUL:
		/* in case this wasn't done earlier... */
		if (p->in.left->in.op == ICON) {
			q = p->in.left;
			t = p->in.type;
			*p = *q;
			p->in.op = NAME;
			p->in.type = t;
			q->in.op = FREE;
			break;
		}
		/*
		 * put potential double index OREG trees into a canonical
		 * form: ( <base> + <offset> ) + <index>
		 */
		p = p->in.left;
		if (p->in.op != PLUS)
			break;
		q = p->in.left;
		r = p->in.right;
		if ( ISPTR(r->in.type) ) {
			/*
			 * put the pointer on the left
			 */
			p->in.left = r;
			p->in.right = q;
			q = p->in.left;
			r = p->in.right;
		}
		if ( ISPTR(q->in.type)
		    && (r->in.op == PLUS || r->in.op == MINUS)
		    && !ISPTR(r->in.left->in.type)
		    && r->in.right->in.op == ICON ) {
			/*
			 * <ptr exp> + ( <int exp> [+-] ICON )
			 * 	rewrite as
			 * (<ptr exp> [+-] ICON) + <int exp>
			 */
			p->in.right = r->in.left;
			r->in.left = q;
			p->in.left = r;
			r->in.type = q->in.type;
			break;
		}
		if (r->in.op == LS
		    && ( (rr = r->in.right)->in.op == ICON )
		    && ( (rl = r->in.left)->in.op == PLUS
			|| rl->in.op == MINUS )
		    && !ISPTR(rl->in.left->in.type)
		    && rl->in.right->in.op == ICON ) {
			/*
			 * <ptr exp> + ((<int exp> [+-] ICON) << ICON)
			 *	rewrite as
			 * (<ptr exp> [+-] ICON*) + (<int exp> << ICON)
			 */
			r->in.left = rl->in.left;
			rl->in.left = q;
			p->in.left = rl;
			rl->in.type = q->in.type;
			rl->in.right->tn.lval <<= rr->tn.lval;
			break;
		}
		break;

	case PLUS:
	case ASG PLUS:
	case MINUS:
	case ASG MINUS:
		/*
		 * one interesting cases: <lhs> <op> 0.
		 * <address reg> + SCONV( <short> ) is a good one, too,
		 * but is best done later on.
		 */
		if (((r=p->in.right)->tn.op == ICON
		    && r->tn.lval == 0 && r->tn.name[0] == '\0' )
		    || (r->tn.op == FCON && r->fpn.dval == 0.0 )) {
			goto promoteleft;
		}
		break;

	case MUL:
		/*
		 * put constants on the right
		 */
		if (p->in.left->in.op == ICON && p->in.right->in.op != ICON) {
			r = p->in.right;
			p->in.right = p->in.left;
			p->in.left = r;
		}
		/* fall through */

	case ASG MUL:
	case DIV:
	case ASG DIV:
	case MOD:
	case ASG MOD:
		/*
		 * two interesting case: <lhs> <op> 1
		 * and: small multiplies that can be handled by 
		 *      the feeble instructions.
		 */
		if (((r=p->in.right)->tn.op == ICON
		    && r->tn.lval == 1 && r->tn.name[0] == '\0')
		    || (r->tn.op == FCON && r->fpn.dval == 1.0 )) {
			switch (op){
			case MOD:
			case ASG MOD:
			    /*
			     * x%1 == 0
			     * BUT x might have side effects, so...
			     * x%1 ==> (x,0)
			     */
			    p->in.op = COMOP;
			    if (r->tn.op==FCON)
				r->fpn.dval = 0.0;
			    else
				r->tn.lval = 0;
			    break;
			default:
			    goto promoteleft;
			}
			break;
		}
		if (op == DIV || op == ASG DIV){
		    /*
		     * a very particular case: a difference of two
		     * pointers, divided by a power of two should
		     * always give an integral result (this is
		     * pointer subtraction, with the implied divide
		     * by sizeof( *p ) ). So, we can change this
		     * into a shift, if we ever see it.
		     * AND...
		     * a less peculiar case: unsigned divide by a power of 2.
		     */
		    if (
		       (p->in.type == UNSIGNED && r->in.op ==ICON  )
		    || ((p->in.type == INT || p->in.type == UNSIGNED) 
			&& r->in.op == ICON && (q = p->in.left)->in.op == MINUS
			&& ISPTR(q->in.left->in.type)
			&& ISPTR(q->in.right->in.type))
		    ){
			if (cntbits( v=r->tn.lval )==1 && r->in.name[0]=='\0' ){
			    /* well, looks like we found one */
			    w = ffs( v ) - 1;
			    p->in.op = op += RS - DIV; /* keep ASG if present */
			    r->tn.lval = w;
			    break;
			}
		    }
		}
		/* the other good case is multiply by a power of two,
			and thats already being handled in the front end */
		/*
		 * many multiplies
		 * can be done directly in the hardware:
		 * {SHORT, USHORT, CHAR, UCHAR} x
		 *	{SHORT, USHORT, CHAR, UCHAR, ICON}
		 *
		 * so can the corresponding divides.
		 */
		if (p->in.type != INT && p->in.type != UNSIGNED &&
		    !ISPTR(p->in.type))
			break;
		q = p->in.left;
		if ( isconv(q, SHORT, USHORT ) ) lsize = 2;
		else if (isconv(q, CHAR, UCHAR )) lsize = 1;
		else if (q->in.type == SHORT)     lsize = 0;
		else break;
		if ( isconv(r, SHORT, USHORT ) ||
			    special(r, SSCON) ) rsize = 2;
		else if (isconv(r, CHAR, UCHAR )) rsize = 1;
		else break;
		/* we've looked at both sides and it looks plausable */
		/* rearrange lhs */
		if (lsize == 1){
		    /* diddle SCONV, but leave it in place */
		    q->in.type = (q->in.left->tn.type==UCHAR)?USHORT:SHORT;
		} else if(lsize == 2){
		    p->in.left = q->in.left;
		    q->in.op = FREE;
		}
		/* now rearrange rhs */
		if (rsize == 1){
		    r->in.type = (r->in.left->tn.type==UCHAR)?USHORT:SHORT;
		} else if (r->tn.op == ICON){
		    r->tn.type = SHORT;
		} else {
		    p->in.right = r->in.left;
		    r->in.op = FREE;
		}
		/* 
		 *  The result of the mul[us] instructions are ints
		 *  anyway, so leave the type of the MUL node alone.
		 *  But DIV must be acknowleged as short.
		 */
		if ( (op==DIV || op== ASG DIV || op==MOD || op==ASG MOD)
		&& p->in.type != SHORT && p->in.type != USHORT){
		    w = (p->in.right->tn.type==SHORT && p->in.left->tn.type==SHORT) ? SHORT : USHORT;
		    q = talloc();
		    *q = *p;
		    p->in.op = SCONV;
		    q->in.type = w;
		    p->in.left = q;
		    p->in.right = 0;
		}
		break;
	case SCONV:
		/*
		 * conversions from float to double
		 * in a coprocessor register are vacuous
		 */
		q = p->in.left;
		if (p->in.type == DOUBLE && q->in.type == FLOAT
		  && q->in.op == REG && iscreg(q->tn.rval)) {
		    TWORD t = p->in.type;
		    *p = *q;
		    p->in.type = t;
		    q->in.op = FREE;
		    return;
		}
		/*
		 *           John Gilmore memorial hack
		 *
		 * garbage-can case: 
		 *  if this is a shorten of a simple calculation
		 *  whose (2) operands are no larger than the 
		 *  result type, then we can do the operation more
		 *  cheaply, no?
		 * The architypical case looks like:
		 *  (SCONV<short> (+<int> (SCONV<int> NAME<short> ) ICON ))
		 *  p^                    q^                       r^
		 */
		if ( q->in.type!=INT && q->in.type!=UNSIGNED ) return;
		switch (q->in.op){
		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
		case MOD:
		case LS:
		case RS:
		case AND:
		case OR:
		case ER:
		    break;
		default:
		    return;
		}
		if ( p->in.type==SHORT || p->in.type==USHORT ){
		    w = 2;
		    v = 0x7fff;
		}else if ( p->in.type==CHAR  || p->in.type==UCHAR ){
		    w = 1;
		    v = 0x7f;
		}else break;
		qq = q;
		r = q->in.right;
		q = q->in.left;
		if ( isconv(q, SHORT, USHORT ) ) lsize = 2;
		else if (isconv(q, CHAR, UCHAR )) lsize = 1;
		else if (q->in.op == ICON)
		    if (q->tn.name[0] == '\0' 
		    && ((q->tn.lval | v)==v || (q->tn.lval|v)==-1))
			lsize = 0;
		    else break;
		else if (tlen(q) < sizeof(long)) lsize = -1;
		else break;
		if ( isconv(r, SHORT, USHORT ) ) rsize = 2;
		else if (isconv(r, CHAR, UCHAR )) rsize = 1;
		else if (r->in.op == ICON)
		    if (r->tn.name[0] == '\0' 
		    && ((r->tn.lval | v)==v || (r->tn.lval|v)==-1))
			rsize = 0;
		    else break;
		else if (tlen(r) < sizeof(long)) rsize = -1;
		else break;
		if (lsize == -1) {
		    NODE *t = talloc();
		    t->in.op = SCONV;
		    t->in.type = qq->in.type;
		    t->in.left = q;
		    lsize = tlen(q);
		    q = t;
		    qq->in.left = q;
		}
		if (rsize == -1) {
		    NODE *t = talloc();
		    t->in.op = SCONV;
		    t->in.type = qq->in.type;
		    t->in.left = r;
		    rsize = tlen(r);
		    r = t;
		    qq->in.right = r;
		}
		if (rsize > w || lsize > w) {
		    /* must preserve top SCONV, but weaken operation */
		    p = p->in.left;
		    if (rsize >lsize){
			p->in.type  =  r->in.left->in.type ;
			w = rsize;
		    } else {
			p->in.type  =  q->in.left->in.type;
			w = lsize;
		    }
		} else {
		    /* clobber SCONV */
		    NODE * t = p->in.left;
		    TWORD tt = p->in.type;
		    *p = *t; /* paint over */
		    p->in.type = tt; /* except type */
		    t->in.op = FREE; /* give a node back */
		}
		/* now weaken or elide child convs */
		switch (lsize){
		case 0: /* ICON -- diddle type */
		    q->in.type = p->in.type; break;
		case 1: /* char */
		    if (lsize <w){
			/* retain SCONV but weaken */
			q->in.type = p->in.type; break;
		    }
		    /* else fall through */
		case 2: /* same size -- elide SCONV */
		    { NODE * t = q->in.left;
		    *q = *t;
		    t->in.op = FREE;
		    }
		}
		switch (rsize){
		case 0: /* ICON -- diddle type */
		    r->in.type = p->in.type; break;
		case 1: /* char */
		    if (rsize <w){
			/* retain SCONV but weaken */
			r->in.type = p->in.type; break;
		    }
		    /* else fall through */
		case 2: /* same size -- elide SCONV */
		    { NODE * t = r->in.left;
		    *r = *t;
		    t->in.op = FREE;
		    }
		}
		break;
	case CHK:
		/*
		 * A CHK with constant bounds, of which the left-hand-side
		 * is an SCONV that widens a shorter type can be converted
		 * to an SCONV over a CHK. This lets us use weaker CHK
		 * instructions.
		 */
		if (p->in.type != INT && p->in.type != UNSIGNED) break;
		q = p->in.left;
		if (!isconv(q, CHAR, UCHAR) && !isconv(q, SHORT, USHORT) )break;
		r = p->in.right;
		rr = r->in.right;
		rl = r->in.left;
		if (rr->in.op != ICON || rl->in.op != ICON) break;
		/*
		 * make bounds smaller using the type of the
		 * converted operand
		 */
		t = q->in.left->in.type;
		switch(t) {
		case CHAR:
		    if (reduce_bounds(-128, 127, rl, rr))
			goto delete_check;
		    break;
		case UCHAR:
		    if (reduce_bounds(0, 255, rl, rr))
			goto delete_check;
		    break;
		case SHORT:
		    if (reduce_bounds(-32768, 32767, rl, rr))
			goto delete_check;
		    break;
		case USHORT:
		    if (reduce_bounds(0, 65535, rl, rr))
			goto delete_check;
		    break;
		delete_check:
		    *p = *q;
		    q->in.op = FREE;
		    tfree(r);
		    return;
		}
		/*
		 * weaken the conversion by
		 * exchanging CHK and SCONV nodes
		 */
		*p = *q;	/* p becomes SCONV node */
		p->in.left = q;
		q->in.op = CHK;
		q->in.right = r;
		q->in.type = r->in.type = rl->tn.type = rr->tn.type = t;
		break;

	case INIT:
		/*
		 * not an optimization, but prevents an
		 * infinite loop later on in code generation...
		 */
		q = p->in.left;
		if (q->in.op != ICON && q->in.op != FCON) {
			uerror("Illegal initialization");
			tfree(q);
			q = talloc();
			q->in.type = p->in.type;
			if (ISFLOATING(p->in.type)) {
				q->in.op = FCON;
				q->fpn.dval = 0.0;
			} else {
				q->in.op = ICON;
				q->tn.name = "";
				q->tn.lval = 0;
			}
			p->in.left = q;
		}
		break;

	case FORCE:
		/*
		 * if the type of a FORCE op is {u}char or {u}short,
		 * extend it into an INT.
		 */
		switch(p->in.type){
		case CHAR:
		case UCHAR:
		case SHORT:
		case USHORT:
			q = talloc();
			q->in.op = SCONV;
			q->in.type = INT;
			q->in.left = p->in.left;
			q->in.right = NULL;
			p->in.left = q;
			p->in.type = INT;
			break;
		}
		break;

	} /* switch */
}

/*
 * reduce bounds of a range check using
 * the known bounds of the domain type
 */
int
reduce_bounds(domain_min, domain_max, range_min, range_max)
    register CONSZ domain_min, domain_max;
    register NODE *range_min, *range_max;
{
    /* do the domain and range intersect? */
    if (domain_min > range_max->tn.lval || domain_max < range_min->tn.lval) {
	/* expression value is known to lie outside required range */
	werror("expression value is out of range");
    }
    /* range := intersection(range, domain) */
    if (domain_min > range_min->tn.lval)
	range_min->tn.lval = domain_min;
    if (domain_max < range_max->tn.lval)
	range_max->tn.lval = domain_max;
    /* if domain is a subset of range, range check is unnecessary */
    if (domain_min >= range_min->tn.lval && domain_max <= range_max->tn.lval) {
	return(1);
    }
    return(0);
}
