#include "cpass2.h"
#include "ctype.h"
#ifndef lint
static	char sccsid[] = "@(#)local2.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

# ifdef FORT
int ftlab1, ftlab2;
# endif
/* a lot of the machine dependent parts of the second pass */
#ifdef FORT
#   define fltfun 0
#else
    extern int fltfun;
#endif
# define BITMASK(n) ((1L<<(n))-1)


int toff = 0;    /* number of stack locations used for args */
int maxtoff;
void stmove();
void eval_field();
void shiftreg();
void incraddr();

/* everything you never wanted to know about condition codes */

char *
ccodes[] =	{ "eq", "ne", "le", "lt", "ge", "gt", "ls", "cs", "cc", "hi" };

char *
fccodes[] =	{ "eq", "neq", "le", "lt", "ge", "gt" };

char *
fnegccodes[] =	{ "neq", "eq", "nle", "nlt", "nge", "ngt" };

/* logical relations when compared in reverse order (cmp R,L) */

#ifdef FORT
short revrel[] = { EQ, NE, GE, GT, LE, LT, UGE, UGT, ULE, ULT };
#else
extern short revrel[];
#endif

/* negated logical relations -- integer comparisons only */

int negrel[] =	{ NE, EQ, GT, GE, LT, LE, UGT, UGE, ULT, ULE } ;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

zzzcode( p, c, cookie ) NODE *p; 
{

	register m,temp;
	NODE *s;

	switch( c ){
	/* BCDEFGHIJKLMNOPRSTVWXabcdfglmrtv` '0-~ */

	case 'C':
		switch (p->in.left->in.op) {

		  case ICON:	print_str("\tjbsr\t");
				acon(p->in.left);
				return;

		  case REG:
				if (indexreg(p->in.left)){
					/* already in an address register */
					print_str("\tjsr\t");
					adrput(p->in.left);
					putchar('@');
					return;
				} /* else fall through */

		  case NAME:
		  case OREG:	print_str("\tmovl\t");
				adrput(p->in.left);
				print_str(",a0\n\tjsr\ta0@");
				return;

		  default:	cerror("bad subroutine name");
		}

	case 'E': /* load doubles with overly complex addressing */
	    {
		/*
		 * accomplished by simplifying addressing, then
		 * falling through to case 'D'. 
		 */
		expand(p, cookie, "	lea	AR,A2\n");
		/* 
		 * we would like, at this point, just to paint over
		 * p with  the place where we have the address calculated
		 * and be done with it. too bad we can't: reclaim
		 * needs to look at it. So we must RECUR here
		 */
		resc[1].tn.op = OREG;
		resc[1].tn.lval = 0;
		resc[1].tn.type = INCREF(DOUBLE);
		zzzcode( &resc[1],  'D', cookie );
		return;
	    }
		

	case 'D': /* load-store doubles with auto-incr/decr addressing */
		{
		/* cases are: ASSIGN, or rewrite LEAF-node */
#		define FORW	0
#		define REVR	1
		static char * lrewrite[] = { "\tmovl\tA.,A1\n\tmovl\tU.,U1\n",
					     "\tmovl\tU.,U1\n\tmovl\tA.,A1\n"};
		static char * asreg[]    = { "\tmovl\tAR,AL\n\tmovl\tUR,UL\n",
					     "\tmovl\tUR,UL\n\tmovl\tAR,AL\n"};
		char **rstring;
		NODE * q;
		if (p->in.op == ASSIGN){
		    rstring = asreg; q = p->in.left;
		} else {
		    rstring = lrewrite; q = p;
		}
		if (q->in.op == UNARY MUL && q->in.left->in.op == ASG MINUS)
		    expand( p, cookie, rstring[REVR] );
		else
		    expand( p, cookie, rstring[FORW] );
#		undef FORW
#		undef REVR
		}
		return;

	case 'K':
		/*
		 * 68881 floating point source operand
		 * This is essentially the same as AR, but
		 * in addition we check for the opportunity
		 * to use floating-point immediate mode.
		 * Only supported by the assembler in
		 * coprocessor instructions.
		 */
		switch(optype(p->in.op)) {
		case LTYPE:
			break;
		case UTYPE:
			if (p->in.op != UNARY MUL)
				p = p->in.left;
			break;
		case BITYPE:
			p = p->in.right;
			break;
		}
		if (p->in.op == FCON) {
			floatimmed(p);
		} else {
			adrput(p);
		}
		return;
		
	case 'L':
		m = p->in.left->in.type;
		goto suffix;

	case 'R':
		m = p->in.right->in.type;
		goto suffix;

	case 'B':
		m = p->in.type;		/* fall into suffix: */

	suffix:	
		switch(m) {
		case CHAR:
		case UCHAR:
			c = 'b';
			break;
		case SHORT:
		case USHORT:
			c = 'w';
			break;
		default:
			c = 'l';
			break;
		}
		putchar(c);
		return;

	case 'F':
		/*
		 * print type characters for the source operand of
		 * a 68881 floating point instruction.
		 */
		switch(optype(p->in.op)) {
		case LTYPE:
			break;
		case UTYPE:
			if (p->in.op != UNARY MUL)
				p = p->in.left;
			break;
		case BITYPE:
			p = p->in.right;
			break;
		}
		/*
		 * at this point, p is assumed to have matched
		 * the shape SFLOAT_SRCE (cf. special())
		 */
		if (p->in.op == SCONV)
			p = p->in.left;
		goto float_suffix;

	case 'G':
		/*
		 * print type character for the left subtree
		 */
		p = p->in.left;
		if (p->in.op == SCONV)
			p = p->in.left;
		goto float_suffix;

	case '.':
	float_suffix:
		if (p->in.op == REG && iscreg(p->tn.rval)) {
			/*
			 * source is coprocessor reg; data
			 * is in extended precision
			 */
			putchar('x');
			return;
		}
		/*
		 * print type character for the root node
		 */
		switch(BTYPE(p->in.type)) {
		case CHAR:
			c = 'b';
			break;
		case SHORT:
			c = 'w';
			break;
		case FLOAT:
			c = 's';
			break;
		case DOUBLE:
			c = 'd';
			break;
		case UCHAR:
		case USHORT:
			/*
			 * the 68881 sign-extends integer operands
			 */
			cerror("compiler botched unsigned operand");
			/*NOTREACHED*/
		default:
			c = 'l';
			break;
		}
		putchar(c);
		return;
		
	case 'N':  /* logical ops, turned into 0-1 */
		/* use register given by RESC1 */
		branch(m=getlab());
		/* FALL THROUGH */

	make_boolean:
		deflab( p->bn.label );
		temp = getlr( p, '1' )->tn.rval;
		printf( "	clrl	%s\n", rnames[temp]);
		markused(temp);
		deflab( m );
		return;

	case 'H':
		cbgen(p->in.op, p->in.left->in.type, p->bn.label,
			p->bn.reversed, FCCODES);
		return;

	case 'I':
		cbgen(p->in.op, p->in.left->in.type, p->bn.label,
			p->bn.reversed, CCODES);
		return;

		/* stack management macros */
	case '-':
		print_str( "sp@-" );
	case 'P':
		toff += 4;
		if (toff > maxtoff) maxtoff = toff;
		return;

	case '0':
		toff = 0; return;

	case '~':
		/* complimented CR */
		p->in.right->tn.lval = ~p->in.right->tn.lval;
		conput( getlr( p, 'R' ) );
		p->in.right->tn.lval = ~p->in.right->tn.lval;
		return;

	case 'M':
		/* negated CR */
		p->in.right->tn.lval = -p->in.right->tn.lval;
	case 'O':
		conput( getlr( p, 'R' ) );
		p->in.right->tn.lval = -p->in.right->tn.lval;
		return;

	case 'T':
		/* Truncate longs for type conversions:
		    INT|UNSIGNED -> CHAR|UCHAR|SHORT|USHORT
		   increment offset to second word */

		m = p->in.type;
		p = p->in.left;
		switch( p->in.op ){
		case NAME:
		case OREG:
			if (p->in.type==SHORT || p->in.type==USHORT)
			  p->tn.lval += (m==CHAR || m==UCHAR) ? 1 : 0;
			else p->tn.lval += (m==CHAR || m==UCHAR) ? 3 : 2;
			return;
		case REG:
			return;
		default:
			cerror( "Illegal ZT type conversion" );
			return;

			}

	case 't':
		/*
		 * Lengthen little (unsigned) things. Try to be smart
		 * about instruction sequences.
		 */
		s = p->in.left;
		m = s->in.type;
		if (ISUNSIGNED(m)){
		    /* zero-extending into temp register */
		    if ( istnode(s) && s->tn.rval == resc[0].tn.rval ){
			/* must use andl instructions */
			print_str_str( "	andl	#0x", (m==UCHAR)?"ff":"ffff");
		    } else {
			print_str_str_nl( "	moveq	#0,", rnames[ resc[0].tn.rval ] );
			printf( "	mov%c	", (m==UCHAR)?'b':'w');
			adrput( s );
		    }
		    putchar(','); print_str_nl(rnames[ resc[0].tn.rval ] );
		} else {
		    /* sign-extending into temp register */
		    if (!(istnode(s) && s->tn.rval == resc[0].tn.rval) ){
			/* not already in its register */
			printf( "	mov%c	", (m==CHAR)?'b':'w');
			adrput( s );
			putchar(','); print_str_nl(rnames[ resc[0].tn.rval ] );
		    }
		    /* sign-extend */
		    if (m == CHAR)
			print_str_str_nl( "	extw	", rnames[resc[0].tn.rval]);
		    if (p->in.type != SHORT && p->in.type != USHORT)
			print_str_str_nl( "	extl	", rnames[resc[0].tn.rval]);
		}
		return;

	case 'W':	/* structure size */
		if( p->in.op == STASG )
			print_d(p->stn.stsize);
		else	cerror( "Not a structure" );
		return;

	case 'S':  /* structure assignment */
		stmove( p, cookie );
		return;

	case 'X':{	/* indexed effective address */
		NODE fake,*lp,*rp;
		int flags;
		lp = p->in.left;
		rp = p->in.right;
		R2PACKFLGS(flags, rp->in.type == SHORT, 0, 0);
		fake.in.op = OREG;
		fake.tn.rval = R2PACK(lp->in.left->tn.rval, rp->tn.rval, flags);
		fake.tn.name = "\0";
		fake.tn.lval = lp->in.right->tn.lval;
		oregput(&fake);
		return;
		}

	case 'a':
		/* assign something to a bit field */
	case 'b':
		/* extract a bit field */
	case 'c':
		/* compare bit field with constant for CC only */
		eval_field( p, c, cookie);
		return;

	case 'f': /* floating-point operation */
	        /* since the floating-point format may be changed with a
	         * compile-time switch, we need to go think about this
		 */
		floatcode( p, cookie );
		fltused++;
		return;
	case 'g': /* floating-point conversion to normal register */
		/* anything that looks like a floating type conversion
		 * or a floating-register load or store comes here.
		 */
		floatconv( p, c, cookie );
		fltused++;
		return;

	case 'm': /* multiplication by a constant */
		conmul( p, cookie );
		return;

	case 'd': /* division by a constant */
		condiv( p, cookie );
		return;

	case 'r': /* remainder by a constant */
		conrem( p, cookie );
		return;

	case ' ': /* leaf-type for effect only */
		/*
		 * usually this will generate NO code.
		 * the exception is INCR or ASG MINUS, as in
		 * *p++, *--p;
		 * these are recognized as side effects of addressing 
		 * modes, so have not been dealt with by higher functions.
		 */
		if (p->in.op == UNARY MUL){
		    if (p->in.left->in.op == INCR){
			printf("	addql	#%d, %s\n",
			tlen(p), rnames[p->in.left->in.left->tn.rval]);
		    } else if (p->in.left->in.op == ASG MINUS){
			printf("	subql	#%d, %s\n",
			tlen(p), rnames[p->in.left->in.left->tn.rval]);
		    }
		}
		return;

	case 'l': /* lea or addl instruction, as appropriate */
		if (p->in.left->tn.rval == resc[0].tn.rval){
		    /* really a += */
		    expand( p, cookie, "	addZR	AR,A1\n");
		} else {
		    /* really is a 3-address lea */
		    expand( p, cookie, "	lea	AL@(0,AR:ZR),A1\n");
		}
		return;

	case 'v': /* generate trapv for signed arithmetic */
		trapv(p->in.type);
		return;

	case 'V': /* generate bound-testing code */
		bound_test(p, cookie);
		return;

	default:
		cerror( "illegal zzzcode" );
	}
}

#define checkout if (resc[opno].tn.op != REG) cerror("struct-assign botch")

NODE *
cpytmp( l, opno ) NODE *l;
{
	NODE *new;
	print_str( (l->in.op == REG)||(ISPTR(l->in.type))? "	movl	" : "	lea	");
	adrput( l );
	checkout;
	new = &resc[opno];
	new->tn.type = ISPTR(l->in.type)?l->in.type:INCREF(l->in.type);
	markused(new->tn.rval);
	putchar(','); print_str_nl(rnames[new->tn.rval] );
	return new;
}


void
stmove( p , cookie ) register NODE *p;
{
	register NODE *l, *r;
	register size, i;
	extern NODE resc[];
	int opno = 1;
	int rcopy;
	int xsize;
	int labl;
	int loopcode;
	register char *cnt, *lhs, *rhs;

	char **moves;
#	define MOVBSTRNG	0
#	define MOVWSTRNG	1
#	define MOVLSTRNG	2
	static char * asgstrings[] = {
		"	movb	AR,AL\n",
		"	movw	AR,AL\n",
		"	movl	AR,AL\n",
	};
	static char * argstrings[] = {
		"	movb	AR,sp@-\n",
		"	movw	AR,sp@-\n",
		"	movl	AR,sp@-\n",
	};

	if( p->in.op == STASG ){
	    l = p->in.left;
	    r = p->in.right;
	    moves = asgstrings;
	} else if( p->in.op == STARG ){  /* store an arg onto the stack */
	    /*
	     * "r" and "l" are obviously misnomers here.
	     * they should be "from" and, perhaps, "to".
	     */
	    r = p->in.left;
	    checkout;
	    l = &resc[opno++];
	    markused(l->tn.rval);
	    moves = argstrings;
	} else 
	    cerror( "STASG bad" );

	if ( r->in.op == ICON )
	    r->in.op = NAME;
	/*
	 * avoid complicated memory operands
	 */
	if (l->in.op == OREG && R2TEST(l->tn.rval)) {
	    l = cpytmp( l, opno++ );
	}
	if (r->in.op == OREG && R2TEST(r->tn.rval)) {
	    r = cpytmp( r, opno++ );
	}
	size = p->stn.stsize;
	xsize = size + (size&1); /* must be even */

	/*
	 * now we make a totally arbitrary decision about
	 * when to loop, and when to emit straight-line code.
	 */

	if (size <= 8){
	    /* straight-line */
	    NODE xxx;

	    /* the MIT strategy -- gross, no? */
	    if( l->in.op != REG && ISPTR(l->in.type))
		l = cpytmp( l, opno++ );
	    if( r->in.op != REG && ISPTR(r->in.type))
		r = cpytmp( r, opno++ );
	    xxx.in.op = STASG;
	    xxx.in.left = l;
	    xxx.in.right = r;
	    if( r->tn.op == REG ){
		r->tn.op = OREG;
		r->tn.type = DECREF(r->tn.type);
	    }
	    if( l->in.op == REG ) l->in.op = OREG;
	    r->tn.lval += size;
	    l->tn.lval += size;

	    switch (size&03){
		/* do special cases */
	    case 1:
	    case 3:
		r->tn.lval -= 1;
		l->tn.lval -= 1;
		expand( &xxx, cookie, moves[MOVBSTRNG] );
		if ((size&03) == 1) break;
		/* case 3 falls through */
	    case 2:
		r->tn.lval -= 2;
		l->tn.lval -= 2;
		expand( &xxx, cookie, moves[MOVWSTRNG] );
	    }
	    size -= size&03;
           /* assert( size%4 == 0 ) */
	    while( size ){ /* simple load/store loop */
		r->tn.lval -= 4;
		l->tn.lval -= 4;
		size -= 4;
		expand( &xxx, cookie, moves[MOVLSTRNG] );
	    }
	} else {
	    /* loop code */
	    loopcode = size >= 20;
	    if (loopcode) {
		cnt = rnames[resc[0].tn.rval];
		printf("	mov%c	#%d,%s\n", (size<(1<<16))?'w':'l',
		    size/4-1, cnt);
	    }
	    if ( !istnode(l)){
		if (l->tn.op==OREG && !R2TEST(l->tn.rval)
		  && istreg(l->tn.rval) && l->tn.lval==0)
		    l->tn.op=REG;
		else
		    l = cpytmp( l, opno++ );
	    }
	    if ( !istnode(r) ){
		r = cpytmp( r, opno++ );
		rcopy = 1; /* this is not our real rhs */
	    } else
		rcopy = 0;
	    lhs = rnames[l->tn.rval];
	    rhs = rnames[r->tn.rval];
	    if (p->in.op==STARG){
		/*
		 * structure argument:
		 *    push space on stack (lea sp@(size),sp)
		 *    copy sp to a temp register we can bomb
		 *    go for it.
		 */
		if (xsize <(1<<16))
		    printf(  "	lea	sp@(-%d),sp\n", xsize);
		else
		    printf( "	subl	#%d,sp\n", xsize);
		printf("	movl	sp,%s\n", lhs);
	    }
	    if (loopcode) {
		print_label(labl= getlab());
		/* now emit the 2-instruction loop */
		printf("	movl	%s@+,%s@+\n", rhs, lhs);
		printf("	dbra	%s,L%d\n", cnt, labl);
		if (size>=(1<<16)){
		    /* "long dbra" */
		    printf("	clrw	%s\n", cnt);
		    printf("	subql	#1,%s\n", cnt);
		    printf("	jcc	L%d\n", labl);
		}
	    } else {
		/* use autoincrement mode in an unrolled loop */
		int n;
		for (n = 0; n < size/4; n++)
		    printf("	movl	%s@+,%s@+\n", rhs, lhs);
	    }
	    switch (size&03) {
		/* oops -- stuff leftover */
	    case 3:
		printf("	movw	%s@+,%s@+\n", rhs, lhs);
		/* fall thorugh */
	    case 1:
		printf("	movb	%s@,%s@\n", rhs, lhs);
		break;
	    case 2:
		printf("	movw	%s@,%s@\n", rhs, lhs);
		break;
	    }
	    if (cookie&(INBREG|INTBREG) && !rcopy){
		/* oh damn, must repair damage */
		int temp;
		temp = p->stn.stsize;
		p->stn.stsize &= ~0x3;
		if (size <(1<<16))
		    expand( p, FOREFF, "	lea	AR@(-ZW),AR\n");
		else
		    expand( p, FOREFF, "	subl	#ZW,AR\n");
		p->stn.stsize = temp;
	    }
	}
	if (p->in.op==STARG){
	    /* keep toff up-to-date */
	    if (xsize<3){
		/* must pad */
		printf("	subqw	#2,sp\n");
		xsize = 4;
	    }
	    toff += xsize;
	    if (toff>maxtoff) maxtoff= toff;
	} else {
	    r = p->in.right;
	    if ( r->in.op == NAME )
		r->in.op = ICON;
	    else if (!(cookie&SANY) && r->tn.op==OREG && !ISPTR(r->tn.type)){
		/* we get to make up a (choke) address here */
		if (r->tn.lval == 0){
		    r->in.op = REG;
		    r->tn.type = INCREF(r->tn.type);
		} else 
		    cerror("trapped in a multiple-structue-assigment");
	    }
	}
}

#undef checkout

/*
 * alignfield(p, byteoff)
 *	Align an operand for a 68020 bit field operation.  If byteoff
 *	is nonzero, subtract it (it was previously added to the offset
 *	part of p->in.left, possibly resulting in an odd offset). Then
 *	longword-align the offset, assuming that names and activation
 *	records are longword-aligned.
 */
static
alignfield(p, byteoff)
	register NODE *p;	/* assumed to be FLD op */
	int byteoff;
{
	register NODE *lp;	/* assumed to be operand address */
	int fieldoff, fieldsize, residue;

	lp = p->in.left;
	switch( lp->in.op ){
	case NAME:
	case ICON:
	case OREG:
		if (byteoff) {
			/* undo it */
			lp->tn.lval -= byteoff;
		}
		residue = lp->tn.lval % (SZLONG/SZCHAR);
		if (residue < 0) {
			/*
			 * careful about AUTOs --
			 * the offsets are negative
			 */
			residue += (SZLONG/SZCHAR);
		}
		fieldoff = UPKFOFF(p->tn.rval) + residue*SZCHAR;
		fieldsize = UPKFSZ(p->tn.rval);
		if (fieldoff + fieldsize <= SZLONG) {
			/*
			 * take bytes away from the operand address,
			 * and put bits back in the field offset.
			 */
			lp->tn.lval -= residue;
			p->tn.rval = PKFIELD(fieldsize, fieldoff);
		}
		break;
	default:
		cerror( "illegal address in alignfield" );
		break;
	}
}

expandfield(p)
    register NODE *p;
{
    printf( "{#%d:#%d}", UPKFOFF(p->tn.rval), UPKFSZ(p->tn.rval) );
}

void
eval_field( p, c, cookie )
    NODE *p;
    char c;
{
    register temp, m;
    register NODE *l, *r;
    int lobyte, hibyte;
    char *regname;
    NODE *fieldnode;
    int conval;

    l = p->in.left;
    r = p->in.right;
    fieldnode = (c == 'b') ? p : p->in.left;
    lobyte = (SZINT - fldshf - 1)/SZCHAR;  /* byte containing low-order bit  */
    hibyte = (SZINT -fldshf -fldsz)/SZCHAR;/* byte containing high-order bit */
    if (fieldnode->in.left->tn.op == REG){
	if ( ISPTR(fieldnode->in.left->tn.type)){
	    fieldnode->in.left->tn.op = OREG;
	} else {
	    /* always use longword accesses to registers */
	    hibyte = 0;
	    temp = SZINT;
	    p->in.type = UNSIGNED;
	    goto longonly;
	}
    } 
	
    if (lobyte==hibyte){
	/* ok to do byte accesses to memory */
	temp = SZCHAR;
	p->in.type = UCHAR;
    } else if ((hibyte&1) || (hibyte < (lobyte-1))){
	/* must do longword access */
	hibyte = 0;
	temp = SZINT; 
	p->in.type = UNSIGNED;
    } else {
	/* word access */
	temp = SZSHORT;
	p->in.type = USHORT;
    }

    /* adjust shift value, paint over left type and address */
    fldshf = temp - (SZINT-fldshf -(hibyte*SZCHAR));
    if (hibyte){
	incraddr( fieldnode, hibyte);
    }
    
longonly:

    switch (c){
    case 'a':
	/* assignment of a value to a bit field */
	/* the two cases are: constant r-h-s, and variable r-h-s */
	/* the constant case has several interesting sub-cases   */
	if (r->tn.op == ICON){
	    /* easy case */
	    conval = r->tn.lval;
	    m = r->tn.lval;
	    if (fldsz == 1) {
		/* very easy case */
		print_str_d( (m&1)?"	bset	#":"	bclr	#", fldshf); putchar(',');
		adrput( l );
	    } else {
		/* 
		 * wide field:
		 * depending on field size and alignment, we may
		 * have to do word or longword accesses
		 */
		if (m==0){
		    /* clearing entire field */
		    if (fldshf==0 && fldsz == temp){
			/* no masking involved -- whole unit */
			expand( p, cookie, "	clrZB	AL");
		    } else {
			expand( p, cookie, "	andZB	#N,AL");
		    }
		} else if ( m==-1 || m==BITMASK(fldsz)){
		    /* setting entire field */
		    if (fldshf==0 && fldsz == temp){
			/* no masking involved -- whole unit */
			if (fldsz == SZCHAR) {
			    expand( p, cookie, "	st	AL");
			} else {
			    expand( p, cookie, "	movZB	#CR,AL");
			}
		    } else {
			expand( p, cookie, "	orZB	#M,AL" );
		    }
		} else {
		    /* -- inserting value -- */
		    if (fldshf==0 && fldsz == temp){
			/* no masking involved -- whole unit */
			expand( p, cookie, "	movZB	#CR,AL");
		    } else {			
			m &= BITMASK(fldsz);
			r->tn.lval = m<<fldshf;
			expand(p, cookie, "	andZB	#N,AL\n	orZB	#CR,AL");
		    }
		}
	    }
	    /* restore old ICON value in case we want to use it as
	       the result of the assignment */
	    r->tn.lval = conval;
	} else {
	    /* non-constant */
	    /* there are only two cases here : 
	       shifting and masking, or
	       not shifting or masking
	    */
	    if (fldsz == temp && fldshf == 0){
		/* direct move */
		expand( p, cookie, "	movZB	AR,AL");
	    } else if (use68020) {
		/* the normal, ugly case, with 68020 instructions */
		alignfield(fieldnode, hibyte);
		expand( p, cookie, "	bfins	AR,AL");
		expandfield(fieldnode);
	    } else {
		/* the normal, ugly case, with 68010 instructions */
		/* clear field in destination */
		expand( p, cookie, "	andZB	#N,AL\n");
		/* move value to temp and adjust */
		temp = fldshf;
		fldshf = 0;
		if (r->tn.op == REG && istreg(r->tn.rval) && (cookie&FOREFF)){
		    /* do it in place */
		    regname = rnames[r->tn.rval];
		    /* mask source to the right number of bits, and */
		    expand( p, cookie, "	andZB	#M,AR\n");
		    /* shift into place */
		    shiftreg( temp, regname , -1, 0, 'l', UNSIGNED);
		    /* combine source into destination */
		    expand( p, cookie, "	orZB	AR,AL");
		} else {
		    /* must copy over */
		    rmove( resc[0].tn.rval, r->tn.rval, UNSIGNED );
		    expand( p, cookie, "	andZB	#M,A1\n");
		    regname = rnames[ resc[0].tn.rval ];
		    shiftreg( temp, regname , -1, 0, 'l', UNSIGNED);
		    expand( p, cookie, "	orZB	A1,AL");
		}
	    }
	}
	p->in.type = UNSIGNED;
	return;

    case 'b':
	/* extracting field value -- either for CC or for value */
	if (cookie & FORCC){
compare_with_zero:
	    /* only extracting value to compare it with zero */
	    if (fldsz == 1){
		/* snap case -- so long as fields are unsigned! */
		print_str_d( "	btst	#", fldshf); putchar(',');
		adrput( l );
	    } else if (fldsz == temp && fldshf == 0){
		/* direct memory test */
		expand( p, cookie, "	tstZB	AL");
	    } else {
		/*
		 * move value into a register -- and with mask to set CC.
		 * We would have to worry about setting the N condition
		 * here, but our unsigned-ness prevents this from being
		 * a problem.
		 */
		if (l->tn.op == REG && istreg(l->tn.rval)){
		    expand( p, cookie, "	andl	#M,AL");
		} else {
		    expand( p, cookie, 
			"	movZB	AL,A1\n	andZB	#M,A1");
		}
	    }
	} else {
	    /* extracting value for value's sake */
	    if (l->tn.op == REG && istreg(l->tn.rval)){
		regname = rnames[ l->tn.rval ];
		if (use68020 && fldshf != 0) {
		    alignfield(fieldnode, hibyte);
		    expand( p, cookie, "	bfextu	AL");
		    expandfield(fieldnode);
		    putchar(','); print_str(regname);
		} else {
		    shiftreg( -fldshf, regname , resc[0].tn.rval, 0, 'l', UNSIGNED);
		    fldshf = 0;
		    expand( p, cookie,	"	andl	#M,AL");
		}
	    } else {
		regname = rnames[ resc[0].tn.rval ];
		if (use68020 && fldshf != 0) {
		    alignfield(fieldnode, hibyte);
		    expand( p, cookie, "	bfextu	AL");
		    expandfield(fieldnode);
		    putchar(','); print_str(regname);
		} else {
		    if (temp != SZINT){
			print_str_str_nl("	moveq	#0,", regname );
		    }
		    expand( p, cookie, "	movZB	AL,A1");
		    if (fldsz != temp || fldshf != 0){
			putchar('\n');
			shiftreg( -fldshf, regname, -1, 0, 'l', UNSIGNED);
			fldshf = 0;
			expand( p, cookie,	"	andZB	#M,A1");
		    }
		}
	    }
	}

	fieldnode->in.type = UNSIGNED;
	return;

    case 'c':
	/* comparison of field value with constant value for CC only */
	m = r->tn.lval;
	if (m==0) goto compare_with_zero;
	r->tn.lval = m << fldshf;
	if (temp == fldsz && fldshf == 0 ){
	    /* compare directly to memory */
	    expand(p, cookie, "	cmpZB	#CR,AL");
	} else if (l->tn.op == REG && istreg(l->tn.rval) && cookie&FOREFF){
	    /* munge in place */
	    expand( p, cookie,"	andZB	#M,AL\n	cmpZB	#CR,AL");
	} else {
	    /* move to register, mask, compare */
	    expand( p, cookie,
		"	movZB	AL,A1\n	andZB	#M,A1\n	cmpZB	#CR,A1");
	}
    }

}

/* routine used by the field-evaluation code to emit shifting instructions.
   "val" is the amount to shift, s is the name of the register to shift,
   and "reg" is the register NUMBER of a helper, if one is available.
   If val<0, we're doing right shifts
   If needclr != 0, must clear lower word after swap.
*/
void
shiftreg( val, s , reg, needclr, typechar, type )
    char *s;
    char typechar;
{
    char *t;
    char *shift = "lsll";
    int lshift;
    if (val < 0 ){
	shift = "lsrl";
	val =   -val;
	lshift = 0;
    } else{
	lshift = 1;
	shift[0] = (ISUNSIGNED(type) || ISPTR(type))? 'l' : 'a';
    }
    shift[3] = typechar; /* ugly hack */
    if (val > 16 ){
	if (reg >= 0){
	    printf("	movl	#%d,%s\n", val,	t=rnames[ reg ]);
	    printf("	%s	%s,%s\n", shift, t, s);
	    trapv(type);
	    return;
	}
	if (!(chk_ovfl && shift[0] == 'a')){
	    print_str_str_nl("	swap	", s);
	    if (needclr&&lshift)
		print_str_str_nl("	clrw	", s);
	    val -= 16;
	}
    }
    while (val > 0){
	if (val> 8){
	    printf("	%s	#8,%s\n", shift, s);
	    val -= 8;
	} else {
	    if (val==1 && lshift)
		printf("	add%c	%s,%s\n", typechar, s,s);
	    else
		printf("	%s	#%d,%s\n", shift, val, s);
	    val = 0;
	}
	trapv(type);
    }
}

void
incraddr( p , v)
	register NODE *p; 
	long v;
{
	/* bump the address p by the value b. */
	if( p->in.op == FLD ){
		p = p->in.left;
	}
	switch( p->in.op ){
	case NAME:
	case ICON:
	case OREG:
		p->tn.lval += v;
		break;
	default:
		cerror( "illegal incraddr address" );
		break;

	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

insput( p ) register NODE *p; 
{
#ifndef IEEECCODES
	printf(ccodes[p->in.op-EQ]);
#else   IEEECCODES
	if (ISFLOATING(p->in.left->in.type)) {
		printf(fccodes[p->in.op-EQ]);
	} else {
		printf(ccodes[p->in.op-EQ]);
	}
#endif  IEEECCODES
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * Generate conditional branches from a relational operator.
 * The exact form of the branch depends on:
 * 1. whether or not the compared operands have floating point types
 * 2. whether or not the coprocessor condition code is used
 * 3. whether the operator has been negated.
 */

cbgen( o, type, label, reversed, mode )
	int o;		/* relational op {EQ, NE, LT, LE, ...} */
	TWORD type;	/* operand type - determines opcode */
	int label;	/* destination label */
	int reversed;	/* =1 if op is to be negated */
	int mode;	/* if FCCODES, use coprocessor condition codes */
{
	char *prefix;
	char *opname;

#ifndef IEEECCODES
	if (reversed) {
		/* treat !(a < b) as (a >= b) */
		o = negrel[o-EQ];
	}
	if (mode == FCCODES) {
		prefix = "fj";
		opname = fccodes[o-EQ];
	} else {
		prefix = "j";
		opname = ccodes[o-EQ];
	}
#else	IEEECCODES
	if (ISFLOATING(type)) {
		prefix = (mode == FCCODES ? "fj" : "jf");
		opname = (reversed ? fnegccodes[o-EQ] : fccodes[o-EQ]);
	} else {
		prefix = "j";
		if (reversed)
			o = negrel[o-EQ];
		opname = ccodes[o-EQ];
	}
#endif	IEEECCODES
	printf("	%s%s	L%d\n", prefix, opname, label);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef FORT
branch(lab)
	int lab;
{
	printf("	jra	L%d\n", lab);
}
#endif FORT
