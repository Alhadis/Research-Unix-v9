
# line 2 "../mip/cgram.y"
#ifndef lint
static	char sccsid[] = "@(#)cgram.y 1.1 86/02/03 SMI"; /* from UCB X.X XX/XX/XX */
#endif
# define NAME 2
# define STRING 3
# define ICON 4
# define FCON 5
# define PLUS 6
# define MINUS 8
# define MUL 11
# define AND 14
# define OR 17
# define ER 19
# define QUEST 21
# define COLON 22
# define ANDAND 23
# define OROR 24
# define ASOP 25
# define RELOP 26
# define EQUOP 27
# define DIVOP 28
# define SHIFTOP 29
# define INCOP 30
# define UNOP 31
# define STROP 32
# define TYPE 33
# define CLASS 34
# define STRUCT 35
# define RETURN 36
# define GOTO 37
# define IF 38
# define ELSE 39
# define SWITCH 40
# define BREAK 41
# define CONTINUE 42
# define WHILE 43
# define DO 44
# define FOR 45
# define DEFAULT 46
# define CASE 47
# define SIZEOF 48
# define ENUM 49
# define LP 50
# define RP 51
# define LC 52
# define RC 53
# define LB 54
# define RB 55
# define CM 56
# define SM 57
# define ASSIGN 58
# define ASM 112

# line 116 "../mip/cgram.y"
# include "cpass1.h"
# define yyerror( x ) ccerror( x, yychar )
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;

# line 136 "../mip/cgram.y"
	static int fake = 0;
#ifndef FLEXNAMES
	static char fakename[NCHNAM+1];
#else
	static char fakename[24];
#endif
	static int noassign = 0;
	extern int errline, lineno;
# define YYERRCODE 256

# line 982 "../mip/cgram.y"


NODE *
mkty( t, d, s ) unsigned t; {
	return( block( TYPE, NIL, NIL, t, d, s ) );
	}

NODE *
bdty( op, p, v ) NODE *p; {
	register NODE *q;

	q = block( op, p, NIL, INT, 0, INT );

	switch( op ){

	case UNARY MUL:
	case UNARY CALL:
		break;

	case LB:
		q->in.right = bcon(v);
		break;

	case NAME:
		q->tn.rval = v;
		break;

	default:
		cerror( "bad bdty" );
		}

	return( q );
	}

dstash( n )
{
    /* put n into the dimension table */
    extern char *realloc();

    if( (unsigned)curdim % DIMTABSZ == 0 ) {
	dimtab = (int*)realloc(dimtab, (curdim+DIMTABSZ)*sizeof(dimtab[0]));
	if (dimtab == NULL) {
	    cerror("dimension table overflow");
	}
    }
    dimtab[ curdim++ ] = n;
}

savebc() {
	if( psavbc > & asavbc[BCSZ-4 ] ){
		cerror( "whiles, fors, etc. too deeply nested");
		}
	*psavbc++ = brklab;
	*psavbc++ = contlab;
	*psavbc++ = flostat;
	*psavbc++ = swx;
	flostat = 0;
	}

resetbc(mask){

	swx = *--psavbc;
	flostat = *--psavbc | (flostat&mask);
	contlab = *--psavbc;
	brklab = *--psavbc;

	}

#ifndef LINT
savelineno(){
	if( psavbc > & asavbc[BCSZ-1 ] ){
		cerror( "whiles, fors, etc. too deeply nested");
		}
	*psavbc++ = lineno;
	}

resetlineno(){
	return  *--psavbc;
	}
#endif

addcase(p) NODE *p; { /* add case to switch */

	p = optim( p );  /* change enum to ints */
	if( p->in.op != ICON ){
		uerror( "non-constant case expression");
		return;
		}
	if( swp == swtab ){
		uerror( "case not in switch");
		return;
		}
	if( swp >= &swtab[SWITSZ] ){
		cerror( "switch table overflow");
		}
	swp->sval = p->tn.lval;
	deflab( swp->slab = getlab() );
	++swp;
	tfree(p);
	}

adddef(){ /* add default case to switch */
	if( swtab[swx].slab >= 0 ){
		uerror( "duplicate default in switch");
		return;
		}
	if( swp == swtab ){
		uerror( "default not inside switch");
		return;
		}
	deflab( swtab[swx].slab = getlab() );
	}

swstart(){
	/* begin a switch block */
	if( swp >= &swtab[SWITSZ] ){
		cerror( "switch table overflow");
		}
	swx = swp - swtab;
	swp->slab = -1;
	++swp;
	}

swend(){ /* end a switch block */

	register struct sw *swbeg, *p, *q, *r, *r1;
	CONSZ temp;
	int tempi;

	swbeg = &swtab[swx+1];

	/* sort */

	r1 = swbeg;
	r = swp-1;

	while( swbeg < r ){
		/* bubble largest to end */
		for( q=swbeg; q<r; ++q ){
			if( q->sval > (q+1)->sval ){
				/* swap */
				r1 = q+1;
				temp = q->sval;
				q->sval = r1->sval;
				r1->sval = temp;
				tempi = q->slab;
				q->slab = r1->slab;
				r1->slab = tempi;
				}
			}
		r = r1;
		r1 = swbeg;
		}

	/* it is now sorted */

	for( p = swbeg+1; p<swp; ++p ){
		if( p->sval == (p-1)->sval ){
			uerror( "duplicate case in switch, %d", tempi=p->sval );
			return;
			}
		}

	genswitch( swbeg-1, swp-swbeg );
	swp = swbeg-1;
	}
short yyexca[] ={
-1, 1,
	0, -1,
	2, 24,
	11, 24,
	50, 24,
	57, 24,
	-2, 0,
-1, 21,
	56, 84,
	57, 84,
	-2, 8,
-1, 26,
	56, 83,
	57, 83,
	-2, 81,
-1, 28,
	56, 87,
	57, 87,
	-2, 82,
-1, 34,
	52, 47,
	-2, 45,
-1, 36,
	52, 39,
	-2, 37,
-1, 61,
	53, 52,
	57, 52,
	-2, 0,
-1, 115,
	33, 19,
	34, 19,
	35, 19,
	49, 19,
	-2, 14,
-1, 284,
	57, 138,
	-2, 0,
-1, 289,
	33, 17,
	34, 17,
	35, 17,
	49, 17,
	-2, 15,
-1, 310,
	33, 18,
	34, 18,
	35, 18,
	49, 18,
	-2, 16,
	};
# define YYNPROD 194
