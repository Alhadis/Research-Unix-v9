# include "cpass1.h"
#ifndef lint
static	char sccsid[] = "@(#)local.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif


/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


/*
 * local.68 -- sym linked from local.c -- 68000 version
 */

/*	this file contains code which is dependent on the target machine */

NODE *
cast( p, t ) register NODE *p; TWORD t; 
{
	/* cast node p to type t */

	p = buildtree( CAST, block( NAME, NIL, NIL, t, 0, (int)t ), p );
	p->in.left->in.op = FREE;
	p->in.op = FREE;
	return( p->in.right );
}

NODE *
clocal(p) NODE *p; 
{

	/* this is called to do local transformations on
	   an expression tree preparitory to its being
	   written out in intermediate code.
	*/

	/* the major essential job is rewriting the
	   automatic variables and arguments in terms of
	   REG and OREG nodes */
	/* conversion ops which are not necessary are also clobbered here */
	/* in addition, any special features (such as rewriting
	   exclusive or) are easily handled here as well */

	register struct symtab *q;
	register NODE *r;
	register o;
	register m, ml;
	int fldsize, fldoff;
	TWORD fldtype;

	switch( o = p->in.op ){

	case NAME:
		if( p->tn.rval<0 || p->tn.rval==NONAME ) { /* already processed; ignore... */
			return(p);
			}
		if (BTYPE(p->in.type) == TERROR)
			return(p);
		q = STP(p->tn.rval);
		switch( q->sclass ){

		case AUTO:
		case PARAM:
			/* fake up a structure reference */
			r = block( REG, NIL, NIL, PTR+STRTY, 0, 0 );
			r->tn.lval = 0;
			r->tn.rval = (q->sclass==AUTO?STKREG:ARGREG);
			p = stref( block( STREF, r, p, 0, 0, 0 ) );
			break;

		case ULABEL:
		case LABEL:
		case STATIC:
			if( q->slevel == 0 ) break;
			p->tn.lval = 0;
			p->tn.rval = -q->offset;
			break;

		case REGISTER:
			p->in.op = REG;
			p->tn.lval = 0;
			p->tn.rval = q->offset;
			break;

			}
		break;

	case PCONV:
		/* do pointer conversions for char and short */
		ml = p->in.left->in.type;
		if( ( ml==CHAR || ml==UCHAR || ml==SHORT || ml==USHORT ) && p->in.left->in.op != ICON ){
			p->in.op = SCONV;
			break;
		}

		/* pointers all have the same representation; the type is inherited */

	inherit:
		p->in.left->in.type = p->in.type;
		p->in.left->fn.cdim = p->fn.cdim;
		p->in.left->fn.csiz = p->fn.csiz;
		p->in.op = FREE;
		return( p->in.left );

	case SCONV:
		/* now, look for conversions downwards */

		m = p->in.type;
		ml = p->in.left->in.type;
		if( p->in.left->in.op == ICON ){
			/* simulate the conversion here */
			CONSZ val;
			if ((val=p->in.left->tn.rval) != NONAME
			&& dimtab[p->fn.csiz] < SZPOINT ){
			    /*  a relocatable icon is being shortened */
			    /* DO NOT SHORTEN IT!!		      */
			    werror("shortening &%s may loose significance",
				val<=0?"(constant)":STP(val)->sname);
			    break;
			}
		shorten_int:
			val = p->in.left->tn.lval;
			switch( m ){
			case CHAR:
				p->in.left->tn.lval = (char) val;
				break;
			case UCHAR:
				p->in.left->tn.lval = val & 0XFF;
				break;
			case USHORT:
				p->in.left->tn.lval = val & 0XFFFFL;
				break;
			case SHORT:
				p->in.left->tn.lval = (short)val;
				break;
			case UNSIGNED:
				p->in.left->tn.lval = val & 0xFFFFFFFFL;
				break;
			case INT:
				p->in.left->tn.lval = (int)val;
				break;
			case DOUBLE:
			case FLOAT:
				p->in.left->fpn.dval = (double)val;
				p->in.left->in.op = FCON;
				break;
			}
			p->in.left->in.type = m;
		} else if( p->in.left->in.op == FCON ){
			/* simulate the conversion here */
			if (m!=DOUBLE && m!=FLOAT){
				/* conversion to int-like things */
				/* convert to int first, then to actual thing */
				p->in.left->tn.lval = (CONSZ)p->in.left->fpn.dval;
				p->in.left->tn.rval = NONAME;
				p->in.left->tn.name = (char*)0;
				p->in.left->tn.op = ICON;
				p->in.left->tn.type = INT;
				if (m != INT)
				    goto shorten_int;
			} else
				p->in.left->in.type = m;
		} else {
		    m  = (m ==FLOAT || m ==DOUBLE);
		    ml = (ml==FLOAT || ml==DOUBLE);
		    if (m != ml ) break;
		    else if( tlen(p) == tlen(p->in.left) )
			goto inherit;
		    else if (p->in.type != 0)
			break;
		    /* fall through and clobber conversion to void */
		}
		p->in.op = FREE;
		return( p->in.left );  /* conversion gets clobbered */

	case PVCONV:
	case PMCONV:
		if( p->in.right->in.op != ICON ) cerror( "bad conversion", 0);
		p->in.op = FREE;
		return( buildtree( o==PMCONV?MUL:DIV, p->in.left, p->in.right ) );

	case CALL:
	case UNARY CALL:
		if (p->in.type == FLOAT)
#ifdef FLOATMATH
		    if (FLOATMATH<=1)
#endif
			p->in.type = DOUBLE; /* no such thing as a float fn */
		break;
	
#ifdef FLOATMATH
	case PLUS:
	case MINUS:
	case DIV:
	case MUL:
	case ASG PLUS:
	case ASG MINUS:
	case ASG DIV:
	case ASG MUL:
		if (!floatmath) break;
#endif FLOATMATH
	case ASSIGN:
		if (p->in.type == FLOAT && p->in.right->tn.op == FCON)
		    p->in.right->tn.type = p->in.right->fn.csiz = FLOAT;
		break;

	case FORCE:
		/* we do not FORCE little things. Ints or better only */
		switch ( p->in.type ){
		case CHAR:
		case SHORT:
			p->in.left = makety(p->in.left,INT,0,INT);
			p->in.type = INT;
			break;
		case UCHAR:
		case USHORT:
			p->in.left = makety(p->in.left,UNSIGNED,0,UNSIGNED);
			p->in.type = UNSIGNED;
			break;
		}
		break;


	} /* switch */

	return(p);
}

andable( p ) NODE *p; 
{
	return(1);  /* all names can have & taken on them */
}

cendarg()
{ /* at the end of the arguments of a ftn, set the automatic offset */
	autooff = AUTOINIT;
}

cisreg( t )
	TWORD t; 
{
	/* is an automatic variable of type t OK for a register variable */
	switch (t) {
	case DOUBLE:
		return( use68881 );
	case FLOAT:
	case INT:
	case UNSIGNED:
	case SHORT:
	case USHORT:
	case CHAR:
	case UCHAR:	
		return(1);
	default:
		return( ISPTR(t) );
	}
}

NODE *
offcon( off, t, d, s ) OFFSZ off; TWORD t; 
{

	/* return a node, for structure references, which is suitable for
	   being added to a pointer of type t, in order to be off bits offset
	   into a structure */

	register NODE *p;

	/* t, d, and s are the type, dimension offset, and sizeoffset */
	/* in general they  are necessary for offcon, but not on H'well */

	p = bcon(0);
	p->tn.lval = off/SZCHAR;
	return(p);

}


static inwd;	/* current bit offsed in word */
static word;	/* word being built from fields */

incode( p, sz ) register NODE *p; 
{

	/* generate initialization code for assigning a constant c
		to a field of width sz */
	/* we assume that the proper alignment has been obtained */
	/* inoff is updated to have the proper final value */
	/* we also assume sz  < SZINT */

	if((sz+inwd) > SZINT) cerror("incode: field > int");
	word |= (p->tn.lval & ((1 << sz) -1)) << (SZINT - sz - inwd);
	inwd += sz;
	inoff += sz;
	while (inwd >= 16) {
	  printf( "	.word	%ld\n", (word>>16)&0xFFFFL );
	  word <<= 16;
	  inwd -= 16;
	}
}

cinit( p, sz ) NODE *p; 
{
	/* arrange for the initialization of p into a space of
	size sz */
	/* the proper alignment has been opbtained */
	/* inoff is updated to have the proper final value */
	/*
	  as a favor (?) to people who want to write 
	      int i = 9600/134.5;
	  we will, under the proper circumstances, do
	  a coersion here.
	*/
	NODE *l;

	switch (p->in.type) {
	case INT:
	case UNSIGNED:
		l = p->in.left;
		if (l->in.op != SCONV || l->in.left->tn.op != FCON) break;
		l->in.op = FREE;
		l = l->in.left;
		l->tn.lval = (long)(l->fpn.dval);
		l->tn.rval = NONAME;
		l->tn.op = ICON;
		l->tn.type = INT;
		p->in.left = l;
		break;
	}
	ecode( p );
	inoff += sz;
}

vfdzero( n )
{ /* define n bits of zeros in a vfd */

	if( n <= 0 ) return;

	inwd += n;
	inoff += n;
	while (inwd >= 16) {
	  printf( "	.word	%ld\n", (word>>16)&0xFFFFL );
	  word <<= 16;
	  inwd -= 16;
	}
}

char *
exname( p ) char *p; 
{
	/* make a name look like an external name in the local machine */

#ifndef FLEXNAMES
	static char text[NCHNAM+1];
#else
	static char text[BUFSIZ+1];
#endif

	register i;

	text[0] = '_';
#ifndef FLEXNAMES
	for( i=1; *p&&i<NCHNAM; ++i ){
#else
	for( i=1; *p; ++i ){
#endif
		text[i] = *p++;
		}

	text[i] = '\0';
#ifndef FLEXNAMES
	text[NCHNAM] = '\0';  /* truncate */
#endif

	return( text );
}

ctype( type )
{ /* map types which are not defined on the local machine */
	switch( BTYPE(type) ){

	case LONG:
		MODTYPE(type,INT);
		break;

	case ULONG:
		MODTYPE(type,UNSIGNED);
		}
	return( type );
}

noinit( t ) 
{ /* curid is a variable which is defined but
	is not initialized (and not a function );
	This routine returns the stroage class for an uninitialized declaration */

	return(EXTERN);

}

commdec( id )
{ /* make a common declaration for id, if reasonable */
	register struct symtab *q;
	OFFSZ off, tsize();

	q = STP(id);
	printf( "	.comm	%s,", exname( q->sname ) );
	off = tsize( q->stype, q->dimoff, q->sizoff );
	printf( CONFMT, off/SZCHAR );
	printf( "\n" );
}

isitlong( cb, ce )
{ /* is lastcon to be long or short */
	/* cb is the first character of the representation, ce the last */

	if( ce == 'l' || ce == 'L' ||
		lastcon >= (1L << (SZINT-1) ) ) return (1);
	return(0);
}


isitfloat( s ) char *s; 
{
	double atof();
	dcon = atof(s);
	return( FCON );
}

ecode( p ) NODE *p; {

	/* walk the tree and write out the nodes.. */

	if( nerrors ) return;
	p2tree( p );
	p2compile( p );
}

#ifndef ONEPASS
tlen(p) NODE *p; 
{
	switch(p->in.type) {
		case CHAR:
		case UCHAR:
			return(1);
			
		case SHORT:
		case USHORT:
			return(2);
			
		case DOUBLE:
			return(8);
			
		default:
			return(4);
		}
}
#endif
