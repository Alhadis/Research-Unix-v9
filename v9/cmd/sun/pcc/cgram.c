
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
extern short  yyexca[];
# define YYNPROD 194
# define YYLAST 1238
extern short  yyact[];
short  yyexca [] ={
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
extern short  yypact[];
short  yyact []={

 217,  98, 269,  20, 137, 238,  94,  92,  93, 215,
  11,  85,  15, 206,  83,  29, 138,  84, 218,  22,
  11,  10,  15,  29,  23,  49,  17, 172,  29,   7,
  39, 310,  23,  87,  86, 101,  17,  23,  63, 227,
 228, 233, 153, 237, 225, 226, 234, 235, 236, 240,
 239,  88,  97,  89,  80, 116, 263, 116, 279, 103,
 229, 326, 278,  24, 162, 213, 238,  94,  92,  93,
 169,  24,  85, 118,  48,  83,  24, 105,  84, 257,
  54, 325, 139, 289,  59, 322, 107, 109,  39,  38,
 154, 158, 318, 300,  87,  86, 273, 272, 113,   4,
 227, 228, 233, 170, 237, 225, 226, 234, 235, 236,
 240, 239,  88, 210,  89, 230, 116, 216, 209, 292,
 106, 229, 174, 175, 176, 178, 180, 182, 184, 186,
 187, 189, 191, 193, 194, 195, 196, 197,  11,  10,
  15, 139,  96, 166, 201, 200,  91,  94,  92,  93,
 199,  71,  85, 154,  17,  83, 162, 116,  84,  91,
  94,  92,  93,  29,  29,  85, 168, 104,  83, 311,
 293,  84,  23,  23,  87,  86, 230,  26, 242, 120,
 243, 161, 244, 101, 245,  75, 246,  87,  86, 247,
  76, 248,  88, 249,  89, 203, 117, 198,  78, 214,
 139,  43,  45, 241,  79,  88,  70,  89, 251, 163,
 261,  24,  24, 262,  41,  74, 252, 295,  42,  19,
 257, 259, 260,  41, 270, 165, 208,  42, 275, 265,
 266, 267, 268,  60, 271, 160,  35,  51,  82, 100,
 287,  52, 281,   5,  33, 290,  91,  94,  92,  93,
 280, 291,  85, 164, 320,  83, 315, 167,  84, 231,
 253, 238,  94,  92,  93, 207, 258,  85,  51, 102,
  83, 298,  52,  84,  87,  86, 255,  28, 204,  50,
  77,  69,  28, 302, 304, 270, 308, 306,  65,  87,
  86, 100,  88, 313,  89, 227, 228, 233, 208, 237,
 225, 226, 234, 235, 236, 240, 239,  88, 314,  89,
  99, 116, 312, 285, 321, 212, 229,  21, 286, 270,
 231, 323, 146, 147, 148, 149, 150, 151,  51,  73,
   9,  29,  52, 284,  44,  46, 124, 207, 125,  30,
  23, 127, 283, 282, 129, 277, 159, 130, 159, 131,
 142, 134, 144, 132, 133, 135, 121, 128, 123, 126,
  55, 114,  56,  18,  61, 297,  91,  94,  92,  93,
 145, 230,  85, 296, 143,  83,  10, 108,  84,  24,
  57, 324,  91,  94,  92,  93, 122,  32, 136, 100,
  11,  10,  15, 288,  87,  86, 301,   8,  37,  11,
 307,  15, 127, 124, 276, 125,  17,  31, 127, 294,
  87,  86,  88, 305,  89,  17, 202, 102,  28, 123,
 155,  64, 192, 121, 128, 123, 126,  61,  88, 124,
  89, 125, 119, 124, 127, 125,  36, 129, 127,  34,
 130, 129, 131, 256, 134,  72, 132, 133, 135, 121,
 128, 123, 126, 121, 128, 123, 126, 232, 115, 112,
 111, 140,  53,  27,  66,  47, 211, 124,  95, 125,
  58, 124, 127, 125, 319, 129, 127,  62, 130, 122,
 131, 136, 134, 155, 132, 133, 135, 121, 128, 123,
 126, 173, 171, 123, 256, 110,  68,  67,  40,   3,
 303,   2, 157, 124,  90, 125,  12,  13, 127,   6,
  25, 129, 317,  14, 130, 231, 131, 122, 134, 136,
 132, 133, 135, 121, 128, 123, 126,  16, 224, 222,
 223, 221, 219, 220,   1,   0, 124,   0, 125,   0,
   0, 127,   0,   0,   0,   0,   0,   0, 316, 124,
   0, 125, 108, 122, 127, 136, 121, 129, 123, 126,
 130,   0, 131,   0, 134, 309, 132, 133, 135, 121,
 128, 123, 126, 124,   0, 125,   0,   0, 127,   0,
   0, 129,   0,   0, 130,   0, 131,   0, 134,   0,
 132, 133, 135, 121, 128, 123, 126,   0, 124, 122,
 125, 136,   0, 127,   0, 124, 129, 125,   0, 130,
 127, 131,   0, 134,   0, 132, 133, 135, 121, 128,
 123, 126,   0, 122, 299, 136,   0, 123, 126,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0, 122, 264,
 136, 124,   0, 125,   0,   0, 127,   0,   0, 129,
   0,   0, 130,   0, 131,   0, 134,   0, 132, 133,
 135, 121, 128, 123, 126, 124,   0, 125,   0,   0,
 127,   0,   0, 129,   0,   0, 130,   0, 131,   0,
 134, 250, 132, 133, 135, 121, 128, 123, 126,   0,
 254, 122,   0, 136,   0,   0,   0,   0,   0,   0,
   0, 124,   0, 125,   0,   0, 127,   0,   0, 129,
   0,   0, 130,   0, 131, 122, 134, 136, 132, 133,
 135, 121, 128, 123, 126,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 124,   0, 125,
   0,   0, 127,   0,   0, 129, 205,   0, 130,   0,
 131, 122, 134, 136, 132, 133, 135, 121, 128, 123,
 126, 124,   0, 125,   0,   0, 127,   0,   0, 129,
   0,   0, 130,   0, 131,   0, 134,   0, 132, 133,
 135, 121, 128, 123, 126,   0,   0, 122,   0, 136,
  91,  94,  92,  93,   0,   0,  85,   0,   0,  83,
   0,   0,  84,  91,  94,  92,  93,   0,   0,  85,
   0,   0,  83, 136,   0,  84,   0,   0,  87,  86,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  87,  86,   0,   0,   0,  88,   0,  89,   0,
   0,  91,  94,  92,  93,   0, 190,  85,   0,  88,
  83,  89,   0,  84,  91,  94,  92,  93,   0, 188,
  85,   0,   0,  83,   0,   0,  84,   0,   0,  87,
  86,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,  87,  86,   0,   0,   0,  88,   0,  89,
   0,   0,  91,  94,  92,  93,   0, 185,  85,   0,
  88,  83,  89,   0,  84,  91,  94,  92,  93,   0,
 183,  85,   0,   0,  83,   0,   0,  84,   0,   0,
  87,  86,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  87,  86,   0,   0,   0,  88,   0,
  89,   0,   0,  91,  94,  92,  93,   0, 181,  85,
   0,  88,  83,  89,   0,  84,  91,  94,  92,  93,
   0, 179,  85,   0,   0,  83,   0,   0,  84,   0,
   0,  87,  86,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,  87,  86,   0,   0,   0,  88,
   0,  89,   0,   0,  91,  94,  92,  93,   0, 177,
  85,   0,  88,  83,  89,   0,  84,   0,   0,   0,
   0, 274,   0,   0,   0,   0,   0,   0,  91,  94,
  92,  93,  87,  86,  85,   0,   0,  83,   0,   0,
  84,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  88,   0,  89,   0, 141,   0,  87,  86,   0,  11,
   0,  15,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,  88,  17,  89,  91,  94,  92,
  93,   0,   0,  85,   0,   0,  83,   0,   0,  84,
  91,  94,  92,  93,   0,   0,  85,   0,   0,  83,
   0,   0,  84,   0,   0,  87,  86,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  87,  86,
   0,   0,   0,  88,   0,  89,   0,  81,  91,  94,
  92,  93,   0,   0,  85,   0,  88,  83,  89, 156,
  84,  91,  94,  92,  93,   0,   0,  85,   0,   0,
  83,   0,   0,  84,   0,   0,  87,  86,   0,   0,
 124,   0, 125,   0,   0, 127,   0,   0, 129,  87,
  86, 130,   0, 131,  88, 134,  89, 132, 133,   0,
 121, 128, 123, 126,   0,   0, 124,  88, 125, 152,
   0, 127,   0,   0, 129,   0,   0, 130,   0, 131,
   0,   0,   0, 132,   0,   0, 121, 128, 123, 126,
 124,   0, 125,   0, 124, 127, 125,   0, 129, 127,
   0, 130, 129, 131,   0,   0,   0, 131,   0,   0,
 121, 128, 123, 126, 121, 128, 123, 126 };
extern short  yypgo[];
short  yypact []={

-1000, -13,-1000,-1000, 313,-1000, 162,-1000, 366, 342,
-1000, 354,-1000,-1000, 192, 437, 184, 434, 395,-1000,
  32, 173,-1000, 329, 329,  23, 218,  22,-1000, 310,
-1000, 366, 347, -23,-1000, 419,-1000, 237,-1000,-1000,
-1000, 230, 151, 218, 173, 278, 164, 134,-1000,-1000,
-1000, 229, 143,1075,-1000,-1000,-1000,-1000,  85,-1000,
-1000, 161, 111,-1000,  19,  63,  21,-1000, 105,-1000,
-1000, 141,1126,-1000,-1000,-1000, 430,-1000,-1000, 124,
 765,1002, 320,1126,1126,1126,1126,1126,1139,1026,
1088, 298,-1000,-1000,-1000, 182, 366, 100,-1000, 173,
 187,-1000,-1000, 172, 419,-1000,-1000,-1000, 173,-1000,
-1000,-1000,-1000,  13,  46,-1000,-1000,-1000, 765,-1000,
-1000,1126,1126, 951, 913, 900, 862, 849,1126, 811,
 798, 364,1126,1126,1126,1126,1126,  94,-1000, 765,
1002,-1000,-1000,1126, 414,-1000, 320, 320, 320, 320,
 320, 320,1026, 227, 705, 287,-1000,  62, 765,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,   8,-1000,
-1000,-1000,  64, 357, 599, 765,-1000,1126, 391,1126,
 391,1126, 465,1126,-1000,1126, 530, 397,1126,1208,
1126, 427,1126,1204,1180, 669, 765, 765, 155,1002,
  94, 645,-1000, 225,1126,-1000,  25, 215, 287,1126,
-1000, 161,-1000,-1000,   3,-1000,-1000, 592,-1000, 259,
 259, 259, 259,1126, 259,  40,  39, 964, 402,-1000,
 295,   5, 259, 293, 292,-1000, 283, 263, 296,1126,
 371,  26, 765, 765, 765, 765, 765, 765, 765, 765,
1126,-1000,-1000,  66,-1000, 380, 320, 115,-1000, 166,
  25, 765,-1000,-1000,-1000, 334,-1000,-1000, 322, 220,
 741,-1000,-1000,-1000,-1000, 567,  36, 393,-1000,-1000,
-1000,-1000, 244, 157, 144,1126,-1000, 543,-1000,-1000,
 -26,1154,-1000,-1000, 114, 262,-1000, 243, 259,-1000,
-1000, 205, 497,-1000, 461,-1000,  35,-1000, 423,-1000,
-1000,-1000, 203,1126,-1000,  28,-1000,-1000,1126,-1000,
-1000, 330,-1000,  24,   4,-1000,-1000 };
extern short  yyr1[];
short  yypgo []={

   0, 534, 151, 533, 532, 531, 530, 529, 528, 527,
 513, 510,   0,   2, 238,  29, 509, 330, 507, 506,
  42,  13, 504,   1, 310, 177, 502, 501, 499,   3,
 498, 497, 496,  18, 495,  27,   9, 492, 491,  52,
 397, 477,  59,  38, 470, 468,  84, 466, 465,  19,
 464, 463, 462,   4,  16, 461, 460, 459, 458, 457,
 445 };
extern short  yyr2[];
short  yyr1 []={

   0,   1,   1,  27,  27,  27,  28,  28,  30,  28,
  31,  32,  32,  35,  35,  37,  37,  38,  38,  38,
  34,  34,  34,  16,  16,  15,  15,  15,  15,  15,
  40,  17,  17,  17,  17,  17,  18,  18,   9,   9,
  41,  41,  43,  43,  19,  19,  10,  10,  44,  44,
  44,  46,  46,  39,  47,  39,  23,  23,  23,  23,
  23,  25,  25,  25,  25,  25,  25,  24,  24,  24,
  24,  24,  24,  24,  11,  48,  48,  48,  29,  50,
  29,  51,  51,  49,  49,  49,  49,  49,  53,  53,
  54,  54,  42,  42,  45,  45,  52,  52,  55,  33,
  33,  56,  57,  58,  36,  36,  36,  36,  36,  36,
  36,  36,  36,  36,  36,  36,  36,  36,  36,  36,
  36,  36,  36,  59,  59,  59,   7,   4,   4,   3,
   5,   5,   6,   6,   8,  60,   2,  13,  13,  26,
  26,  12,  12,  12,  12,  12,  12,  12,  12,  12,
  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,
  12,  12,  12,  12,  12,  12,  14,  14,  14,  14,
  14,  14,  14,  14,  14,  14,  14,  14,  14,  14,
  14,  14,  14,  14,  20,  21,  21,  21,  21,  21,
  21,  21,  22,  22 };
extern short  yychk[];
short  yyr2 []={

   0,   2,   0,   1,   5,   1,   2,   3,   0,   4,
   2,   2,   0,   2,   0,   3,   4,   3,   4,   0,
   3,   2,   2,   1,   0,   2,   2,   1,   1,   3,
   1,   1,   2,   3,   1,   1,   5,   2,   1,   2,
   1,   3,   1,   3,   5,   2,   1,   2,   1,   3,
   1,   2,   1,   1,   0,   4,   1,   1,   3,   2,
   1,   2,   3,   3,   4,   1,   3,   2,   3,   3,
   4,   3,   3,   2,   2,   1,   3,   1,   1,   0,
   4,   1,   1,   1,   1,   3,   6,   1,   1,   3,
   1,   4,   0,   1,   0,   1,   0,   1,   1,   1,
   1,   4,   3,   1,   2,   1,   2,   2,   2,   7,
   4,   2,   2,   2,   2,   3,   3,   1,   5,   2,
   2,   2,   2,   2,   3,   2,   1,   4,   3,   3,
   4,   3,   6,   3,   4,   0,   2,   1,   0,   1,
   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,
   4,   4,   5,   3,   3,   1,   2,   2,   2,   2,
   2,   2,   2,   4,   4,   4,   2,   3,   3,   1,
   1,   1,   1,   3,   2,   0,   2,   5,   2,   3,
   4,   3,   2,   2 };
extern short  yydef[];
short  yychk []={

-1000,  -1, -27, -28, 112, 256, -16, -15, -40, -17,
  34,  33, -19, -18, -10,  35,  -9,  49,  50,  57,
 -29, -24, -49,  11,  50, -11, -25, -51, 256,   2,
 -17, -40,  33,  52,   2,  52,   2,   3,  57,  56,
 -30,  50,  54, -25, -24, -25, -24, -48,  51,   2,
 256,  50,  54, -52,  58,  50, -17,  33, -44, -46,
 256, -17, -41, -43,   2,  51, -50, -31, -32,  51,
  55,  -2, -60,  51,  51,  51,  56,  51,  55,  -2,
 -12,  52, -14,  11,  14,   8,  31,  30,  48,  50,
 -22,   2,   4,   5,   3, -45,  57, -39, -23, -24,
 -25,  22, 256, -42,  56,  58,  57, -49, -24, -33,
 -34, -56, -57, -15, 256, -58,  52,  55, -12,   2,
  55,  26,  56,  28,   6,   8,  29,  11,  27,  14,
  17,  19,  23,  24,  21,  25,  58, -53, -54, -12,
 -55,  52,  30,  54,  32,  50, -14, -14, -14, -14,
 -14, -14,  50, -20, -12, -17,  51, -26, -12,  50,
  53, -46,  56,  22,  -2,  53, -43,  -2, -39,  57,
  57, -37, -35, -38, -12, -12, -12,  58, -12,  58,
 -12,  58, -12,  58, -12,  58, -12, -12,  58, -12,
  58, -12,  58, -12, -12, -12, -12, -12, -42,  56,
 -53, -12,   2, -20,  51,  51, -21,  50,  11,  56,
  51, -47,  -2,  57, -35, -36,  53, -12, -33,  -4,
  -3,  -5,  -7,  -6,  -8,  41,  42,  36,  37,  57,
 112, 256, -59,  38,  43,  44,  45,  40,   2,  47,
  46, -15, -12, -12, -12, -12, -12, -12, -12, -12,
  22,  53, -54, -42,  55,  51, -14,  54,  51, -21,
 -21, -12, -23,  53,  57, -36, -36, -36, -36, -13,
 -12, -36,  57,  57,  57, -12,   2,  50,  57,  53,
 -33, -36,  50,  50,  50,  50,  22, -12,  22,  57,
 -29, -12,  53,  55,  -2,  51,  39,  43,  51,  57,
  57,   3, -12, 256, -12, 256, -13, 256, -12,  22,
  57,  55,  50,  50, -36,  51,  51,  51,  57,  51,
  51, -12,  57, -13,  51,  57,  57 };
# ifdef YYDEBUG
# include "y.debug"
# endif

# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

/*	parser for yacc output	*/

#ifdef YYDEBUG
int yydebug = 0; /* 1 for debugging */
#endif
YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

yyparse()
{	short yys[YYMAXDEPTH];
	int yyj, yym;
	register YYSTYPE *yypvt;
	register int yystate, yyn;
	register short *yyps;
	register YYSTYPE *yypv;
	register short *yyxi;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

yystack:    /* put a state and value onto the stack */
#ifdef YYDEBUG
	if(yydebug >= 3)
		if(yychar < 0 || yytoknames[yychar] == 0)
			printf("char %d in %s", yychar, yystates[yystate]);
		else
			printf("%s in %s", yytoknames[yychar], yystates[yystate]);
#endif
	if( ++yyps >= &yys[YYMAXDEPTH] ) { 
		yyerror( "yacc stack overflow" ); 
		return(1); 
	}
	*yyps = yystate;
	++yypv;
	*yypv = yyval;
yynewstate:
	yyn = yypact[yystate];
	if(yyn <= YYFLAG) goto yydefault; /* simple state */
	if(yychar<0) {
		yychar = yylex();
#ifdef YYDEBUG
		if(yydebug >= 2) {
			if(yychar <= 0)
				printf("lex EOF\n");
			else if(yytoknames[yychar])
				printf("lex %s\n", yytoknames[yychar]);
			else
				printf("lex (%c)\n", yychar);
		}
#endif
		if(yychar < 0)
			yychar = 0;
	}
	if((yyn += yychar) < 0 || yyn >= YYLAST)
		goto yydefault;
	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if( yyerrflag > 0 ) --yyerrflag;
		goto yystack;
	}
yydefault:
	/* default state action */
	if( (yyn=yydef[yystate]) == -2 ) {
		if(yychar < 0) {
			yychar = yylex();
#ifdef YYDEBUG
			if(yydebug >= 2)
				if(yychar < 0)
					printf("lex EOF\n");
				else
					printf("lex %s\n", yytoknames[yychar]);
#endif
			if(yychar < 0)
				yychar = 0;
		}
		/* look through exception table */
		for(yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate);
			yyxi += 2 ) ; /* VOID */
		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
		}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
	}
	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */
		switch( yyerrflag ){
		case 0:   /* brand new error */
#ifdef YYDEBUG
			yyerror("syntax error\n%s", yystates[yystate]);
			if(yytoknames[yychar])
				yyerror("saw %s\n", yytoknames[yychar]);
			else if(yychar >= ' ' && yychar < '\177')
				yyerror("saw `%c'\n", yychar);
			else if(yychar == 0)
				yyerror("saw EOF\n");
			else
				yyerror("saw char 0%o\n", yychar);
#else
			yyerror( "syntax error" );
#endif
yyerrlab:
			++yynerrs;
		case 1:
		case 2: /* incompletely recovered error ... try again */
			yyerrflag = 3;
			/* find a state where "error" is a legal shift action */
			while ( yyps >= yys ) {
				yyn = yypact[*yyps] + YYERRCODE;
				if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE ){
					yystate = yyact[yyn];  /* simulate a shift of "error" */
					goto yystack;
				}
				yyn = yypact[*yyps];
				/* the current yyps has no shift onn "error", pop stack */
#ifdef YYDEBUG
				if( yydebug ) printf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
#endif
				--yyps;
				--yypv;
			}
			/* there is no state on the stack with an error shift ... abort */
yyabort:
			return(1);
		case 3:  /* no shift yet; clobber input char */
#ifdef YYDEBUG
			if( yydebug ) {
				printf("error recovery discards ");
				if(yytoknames[yychar])
					printf("%s\n", yytoknames[yychar]);
				else if(yychar >= ' ' && yychar < '\177')
					printf("`%c'\n", yychar);
				else if(yychar == 0)
					printf("EOF\n");
				else
					printf("char 0%o\n", yychar);
			}
#endif
			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */
		}
	}
	/* reduction by production yyn */
#ifdef YYDEBUG
	if(yydebug) {	char *s;
		printf("reduce %d in:\n\t", yyn);
		for(s = yystates[yystate]; *s; s++) {
			putchar(*s);
			if(*s == '\n' && *(s+1))
				putchar('\t');
		}
	}
#endif
	yyps -= yyr2[yyn];
	yypvt = yypv;
	yypv -= yyr2[yyn];
	yyval = yypv[1];
	yym=yyn;
	/* consult goto table to find next state */
	yyn = yyr1[yyn];
	yyj = yypgo[yyn] + *yyps + 1;
	if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];
	switch(yym){
		
case 2:
# line 148 "../mip/cgram.y"
{
			ftnend();
#ifndef LINT
                        beg_file();
#endif
			} break;
case 3:
# line 156 "../mip/cgram.y"
{ curclass = SNULL;  blevel = 0; } break;
case 4:
# line 158 "../mip/cgram.y"
{  asmout(); curclass = SNULL;  blevel = 0; } break;
case 5:
# line 160 "../mip/cgram.y"
{ curclass = SNULL;  blevel = 0; instruct = 0; } break;
case 6:
# line 164 "../mip/cgram.y"
{  yypvt[-1].nodep->in.op = FREE; } break;
case 7:
# line 166 "../mip/cgram.y"
{  yypvt[-2].nodep->in.op = FREE; } break;
case 8:
# line 167 "../mip/cgram.y"
{
				defid( tymerge(yypvt[-1].nodep,yypvt[-0].nodep), curclass==STATIC?STATIC:EXTDEF );
				} break;
case 9:
# line 170 "../mip/cgram.y"
{  
			    if( blevel ) cerror( "function level error" );
			    if( reached ) retstat |= NRETVAL; 
			    yypvt[-3].nodep->in.op = FREE;
			    ftnend();
			    } break;
case 10:
# line 179 "../mip/cgram.y"
{
#ifndef LINT
			    psline(lineno);
#endif
			    } break;
case 12:
# line 186 "../mip/cgram.y"
{  blevel = 1; } break;
case 14:
# line 191 "../mip/cgram.y"
{  bccode();
			    locctr(PROG);
			    } break;
case 15:
# line 197 "../mip/cgram.y"
{  yypvt[-1].nodep->in.op = FREE; 
#ifndef LINT
			    plcstab(blevel);
#endif
			    } break;
case 16:
# line 203 "../mip/cgram.y"
{  yypvt[-2].nodep->in.op = FREE; 
#ifndef LINT
			    plcstab(blevel);
#endif
			    } break;
case 17:
# line 211 "../mip/cgram.y"
{  yypvt[-1].nodep->in.op = FREE; } break;
case 18:
# line 213 "../mip/cgram.y"
{  yypvt[-2].nodep->in.op = FREE; } break;
case 20:
# line 217 "../mip/cgram.y"
{ curclass = SNULL;  yypvt[-2].nodep->in.op = FREE; } break;
case 21:
# line 219 "../mip/cgram.y"
{ curclass = SNULL;  yypvt[-1].nodep->in.op = FREE; } break;
case 22:
# line 221 "../mip/cgram.y"
{  curclass = SNULL; } break;
case 24:
# line 225 "../mip/cgram.y"
{  yyval.nodep = mkty(INT,0,INT);  curclass = SNULL; } break;
case 25:
# line 228 "../mip/cgram.y"
{  yyval.nodep = yypvt[-0].nodep; } break;
case 27:
# line 231 "../mip/cgram.y"
{  yyval.nodep = mkty(INT,0,INT); } break;
case 28:
# line 233 "../mip/cgram.y"
{ curclass = SNULL ; } break;
case 29:
# line 235 "../mip/cgram.y"
{  yypvt[-2].nodep->in.type = types( yypvt[-2].nodep->in.type, yypvt[-0].nodep->in.type, UNDEF );
			    yypvt[-0].nodep->in.op = FREE;
			    } break;
case 30:
# line 242 "../mip/cgram.y"
{  curclass = yypvt[-0].intval; } break;
case 32:
# line 248 "../mip/cgram.y"
{  yypvt[-1].nodep->in.type = types( yypvt[-1].nodep->in.type, yypvt[-0].nodep->in.type, UNDEF );
			    yypvt[-0].nodep->in.op = FREE;
			    } break;
case 33:
# line 252 "../mip/cgram.y"
{  yypvt[-2].nodep->in.type = types( yypvt[-2].nodep->in.type, yypvt[-1].nodep->in.type, yypvt[-0].nodep->in.type );
			    yypvt[-1].nodep->in.op = yypvt[-0].nodep->in.op = FREE;
			    } break;
case 36:
# line 260 "../mip/cgram.y"
{ yyval.nodep = dclstruct(yypvt[-4].intval); } break;
case 37:
# line 262 "../mip/cgram.y"
{  yyval.nodep = rstruct(yypvt[-0].intval,0);  stwart = instruct; } break;
case 38:
# line 266 "../mip/cgram.y"
{  yyval.intval = bstruct(-1,0); stwart = SEENAME; } break;
case 39:
# line 268 "../mip/cgram.y"
{  yyval.intval = bstruct(yypvt[-0].intval,0); stwart = SEENAME; } break;
case 42:
# line 276 "../mip/cgram.y"
{  moedef( yypvt[-0].intval ); } break;
case 43:
# line 278 "../mip/cgram.y"
{  strucoff = yypvt[-0].intval;  moedef( yypvt[-2].intval ); } break;
case 44:
# line 282 "../mip/cgram.y"
{ yyval.nodep = dclstruct(yypvt[-4].intval);  } break;
case 45:
# line 284 "../mip/cgram.y"
{  yyval.nodep = rstruct(yypvt[-0].intval,yypvt[-1].intval); } break;
case 46:
# line 288 "../mip/cgram.y"
{  yyval.intval = bstruct(-1,yypvt[-0].intval);  stwart=0; } break;
case 47:
# line 290 "../mip/cgram.y"
{  yyval.intval = bstruct(yypvt[-0].intval,yypvt[-1].intval);  stwart=0;  } break;
case 51:
# line 299 "../mip/cgram.y"
{ curclass = SNULL;  stwart=0; yypvt[-1].nodep->in.op = FREE; } break;
case 52:
# line 301 "../mip/cgram.y"
{  if( curclass != MOU ){
				curclass = SNULL;
				}
			    else {
				sprintf( fakename, "$%dFAKE", fake++ );
#ifdef FLEXSTRINGS
				/* No need to hash this, we won't look it up */
				defid( tymerge(yypvt[-0].nodep, bdty(NAME,NIL,lookup( savestr(fakename), SMOS ))), curclass );
#else
				defid( tymerge(yypvt[-0].nodep, bdty(NAME,NIL,lookup( fakename, SMOS ))), curclass );
#endif
				werror("structure typed union member must be named");
				}
			    stwart = 0;
			    yypvt[-0].nodep->in.op = FREE;
			    } break;
case 53:
# line 321 "../mip/cgram.y"
{ defid( tymerge(yypvt[-1].nodep,yypvt[-0].nodep), curclass);  stwart = instruct; } break;
case 54:
# line 322 "../mip/cgram.y"
{yyval.nodep=yypvt[-2].nodep;} break;
case 55:
# line 323 "../mip/cgram.y"
{ defid( tymerge(yypvt[-4].nodep,yypvt[-0].nodep), curclass);  stwart = instruct; } break;
case 56:
# line 325 "../mip/cgram.y"
{ noargs(); } break;
case 58:
# line 329 "../mip/cgram.y"
{  if( !(instruct&INSTRUCT) ) uerror( "field outside of structure" );
			    if( yypvt[-0].intval<0 || yypvt[-0].intval >= FIELD ){
				uerror( "illegal field size" );
				yypvt[-0].intval = 1;
				}
			    defid( tymerge(yypvt[-3].nodep,yypvt[-2].nodep), FIELD|yypvt[-0].intval );
			    yyval.nodep = NIL;
			    } break;
case 59:
# line 339 "../mip/cgram.y"
{  if( !(instruct&INSTRUCT) ) uerror( "field outside of structure" );
			    falloc( stab[0], yypvt[-0].intval, -1, yypvt[-2].nodep );  /* alignment or hole */
			    yyval.nodep = NIL;
			    } break;
case 60:
# line 344 "../mip/cgram.y"
{  yyval.nodep = NIL; } break;
case 61:
# line 349 "../mip/cgram.y"
{  umul:
				yyval.nodep = bdty( UNARY MUL, yypvt[-0].nodep, 0 ); } break;
case 62:
# line 352 "../mip/cgram.y"
{  uftn:
				yyval.nodep = bdty( UNARY CALL, yypvt[-2].nodep, 0 );  } break;
case 63:
# line 355 "../mip/cgram.y"
{  uary:
				yyval.nodep = bdty( LB, yypvt[-2].nodep, 0 );  } break;
case 64:
# line 358 "../mip/cgram.y"
{  bary:
				if( (int)yypvt[-1].intval <= 0 ) werror( "zero or negative subscript" );
				yyval.nodep = bdty( LB, yypvt[-3].nodep, yypvt[-1].intval );  } break;
case 65:
# line 362 "../mip/cgram.y"
{  yyval.nodep = bdty( NAME, NIL, yypvt[-0].intval );  } break;
case 66:
# line 364 "../mip/cgram.y"
{ yyval.nodep=yypvt[-1].nodep; } break;
case 67:
# line 367 "../mip/cgram.y"
{  goto umul; } break;
case 68:
# line 369 "../mip/cgram.y"
{  goto uftn; } break;
case 69:
# line 371 "../mip/cgram.y"
{  goto uary; } break;
case 70:
# line 373 "../mip/cgram.y"
{  goto bary; } break;
case 71:
# line 375 "../mip/cgram.y"
{ yyval.nodep = yypvt[-1].nodep; } break;
case 72:
# line 377 "../mip/cgram.y"
{
				if( blevel!=0 ) uerror("function declaration in bad context");
				yyval.nodep = bdty( UNARY CALL, bdty(NAME,NIL,yypvt[-2].intval), 0 );
				stwart = 0;
				} break;
case 73:
# line 383 "../mip/cgram.y"
{
				yyval.nodep = bdty( UNARY CALL, bdty(NAME,NIL,yypvt[-1].intval), 0 );
				stwart = 0;
				} break;
case 74:
# line 390 "../mip/cgram.y"
{
				/* turn off typedefs for argument names */
				stwart = SEENAME;
				if( STP(yypvt[-1].intval)->sclass == SNULL )
				    STP(yypvt[-1].intval)->stype = FTN;
				} break;
case 75:
# line 399 "../mip/cgram.y"
{ ftnarg( yypvt[-0].intval );  stwart = SEENAME; } break;
case 76:
# line 401 "../mip/cgram.y"
{ ftnarg( yypvt[-0].intval );  stwart = SEENAME; } break;
case 79:
# line 407 "../mip/cgram.y"
{yyval.nodep=yypvt[-2].nodep;} break;
case 81:
# line 411 "../mip/cgram.y"
{  defid( yypvt[-0].nodep = tymerge(yypvt[-1].nodep,yypvt[-0].nodep), curclass);
			    beginit(yypvt[-0].nodep->tn.rval);
			    } break;
case 83:
# line 418 "../mip/cgram.y"
{  nidcl( tymerge(yypvt[-1].nodep,yypvt[-0].nodep) ); } break;
case 84:
# line 420 "../mip/cgram.y"
{  noargs(); defid( tymerge(yypvt[-1].nodep,yypvt[-0].nodep), uclass(curclass) );
			} break;
case 85:
# line 424 "../mip/cgram.y"
{  
#ifndef LINT
                            psline(lineno);
#endif

			    doinit( yypvt[-0].nodep );
			    endinit();
			    if (noassign && errline != lineno)
				    werror( "old-fashioned initialization: use =" );
			} break;
case 86:
# line 435 "../mip/cgram.y"
{  
			    endinit(); 
			    if (noassign && errline != lineno)
				    werror( "old-fashioned initialization: use =" );
			} break;
case 90:
# line 449 "../mip/cgram.y"
{  doinit( yypvt[-0].nodep ); } break;
case 91:
# line 451 "../mip/cgram.y"
{  irbrace(); } break;
case 96:
# line 463 "../mip/cgram.y"
{ noassign = 1; } break;
case 97:
# line 465 "../mip/cgram.y"
{ noassign = 0; } break;
case 98:
# line 470 "../mip/cgram.y"
{  ilbrace(); } break;
case 101:
# line 480 "../mip/cgram.y"
{  
#ifndef LINT
			    prcstab(blevel);
#endif
			    --blevel;
			    if( blevel == 1 ) blevel = 0;
			    clearst( blevel );
			    checkst( blevel );
			    autooff = *--psavbc;
			    regvar = *--psavbc;
			    } break;
case 102:
# line 494 "../mip/cgram.y"
{  --blevel;
			    if( blevel == 1 ) blevel = 0;
			    clearst( blevel );
			    checkst( blevel );
			    autooff = *--psavbc;
			    regvar = *--psavbc;
			    } break;
case 103:
# line 504 "../mip/cgram.y"
{  
			    if( blevel == 1 ) dclargs();
			    ++blevel;
			    if( psavbc > &asavbc[BCSZ-2] ) cerror( "nesting too deep" );
			    *psavbc++ = regvar;
			    *psavbc++ = autooff;
			    } break;
case 104:
# line 514 "../mip/cgram.y"
{ 
#ifndef LINT
			    psline(lineno);
#endif
			    ecomp( yypvt[-1].nodep );
			    } break;
case 106:
# line 522 "../mip/cgram.y"
{ deflab(yypvt[-1].intval);
			   reached = 1;
			   } break;
case 107:
# line 526 "../mip/cgram.y"
{  if( yypvt[-1].intval != NOLAB ){
				deflab( yypvt[-1].intval );
				reached = 1;
				}
			    } break;
case 108:
# line 532 "../mip/cgram.y"
{  branch(  contlab );
			    deflab( brklab );
			    if( (flostat&FBRK) || !(flostat&FLOOP)) reached = 1;
			    else reached = 0;
			    resetbc(0);
			    } break;
case 109:
# line 539 "../mip/cgram.y"
{  
#ifndef LINT
			    psline(lineno);
#endif
			    deflab( contlab );
			    if( flostat & FCONT ) reached = 1;
			    ecomp( buildtree( CBRANCH, buildtree( NOT, yypvt[-2].nodep, NIL ), bcon( yypvt[-6].intval ) ) );
			    deflab( brklab );
			    reached = 1;
			    resetbc(0);
			    } break;
case 110:
# line 551 "../mip/cgram.y"
{
			    deflab( contlab );
			    if( flostat&FCONT ) reached = 1;
#ifndef LINT
			    psline( resetlineno());
#endif
			    if( yypvt[-2].nodep ) ecomp( yypvt[-2].nodep );
			    branch( yypvt[-3].intval );
			    deflab( brklab );
			    if( (flostat&FBRK) || !(flostat&FLOOP) ) reached = 1;
			    else reached = 0;
			    resetbc(0);
			    } break;
case 111:
# line 565 "../mip/cgram.y"
{
			    if( reached ) branch( brklab );
#ifndef LINT
			    psline( resetlineno());
#endif
			    deflab( yypvt[-1].intval );
			    swend();
			    deflab(brklab);
			    if( (flostat&FBRK) || !(flostat&FDEF) ) reached = 1;
			    resetbc(FCONT);
			    } break;
case 112:
# line 577 "../mip/cgram.y"
{  
#ifndef LINT
			    psline(lineno);
#endif
			    if( brklab == NOLAB ) uerror( "illegal break");
			    else if(reached) branch( brklab );
			    flostat |= FBRK;
			    if( brkflag ) goto rch;
			    reached = 0;
			    } break;
case 113:
# line 588 "../mip/cgram.y"
{  
#ifndef LINT
			    psline(lineno);
#endif
			    if( contlab == NOLAB ) uerror( "illegal continue");
			    else branch( contlab );
			    flostat |= FCONT;
			    goto rch;
			    } break;
case 114:
# line 598 "../mip/cgram.y"
{  
#ifndef LINT
			    psline(lineno);
#endif
			    retstat |= NRETVAL;
			    branch( retlab );
			rch:
			    if( !reached ) werror( "statement not reached");
			    reached = 0;
			    } break;
case 115:
# line 609 "../mip/cgram.y"
{  register NODE *temp;
#ifndef LINT
			    psline(lineno);
#endif
			    idname = curftn;
			    temp = buildtree( NAME, NIL, NIL );
			    if(temp->in.type == TVOID)
				uerror("void function %s cannot return value",
					STP(idname)->sname);
			    temp->in.type = DECREF( temp->in.type );
			    temp = buildtree( RETURN, temp, yypvt[-1].nodep );
			    /* now, we have the type of the RHS correct */
			    temp->in.left->in.op = FREE;
			    temp->in.op = FREE;
			    ecomp( buildtree( FORCE, temp->in.right, NIL ) );
			    retstat |= RETVAL;
			    branch( retlab );
			    reached = 0;
			    } break;
case 116:
# line 629 "../mip/cgram.y"
{  register NODE *q;
#ifndef LINT
			    psline(lineno);
#endif
			    q = block( FREE, NIL, NIL, INT|ARY, 0, INT );
			    q->tn.rval = idname = yypvt[-1].intval;
			    defid( q, ULABEL );
			    STP(idname)->suse = -lineno;
			    branch( STP(idname)->offset );
			    goto rch;
			    } break;
case 118:
# line 642 "../mip/cgram.y"
{  
			    asmout();
#ifndef LINT
			    psline(lineno);
#endif
			    } break;
case 123:
# line 654 "../mip/cgram.y"
{  register NODE *q;
			    q = block( FREE, NIL, NIL, INT|ARY, 0, LABEL );
			    q->tn.rval = yypvt[-1].intval;
			    defid( q, LABEL );
			    reached = 1;
			    } break;
case 124:
# line 661 "../mip/cgram.y"
{  addcase(yypvt[-1].nodep);
			    reached = 1;
			    } break;
case 125:
# line 665 "../mip/cgram.y"
{  reached = 1;
			    adddef();
			    flostat |= FDEF;
			    } break;
case 126:
# line 671 "../mip/cgram.y"
{  
#ifndef LINT
			    psline(lineno);
#endif
			    savebc();
			    if( !reached ) werror( "loop not entered at top");
			    brklab = getlab();
			    contlab = getlab();
			    deflab( yyval.intval = getlab() );
			    reached = 1;
			    } break;
case 127:
# line 684 "../mip/cgram.y"
{  
#ifndef LINT
			    psline(lineno);
#endif
			    ecomp( buildtree( CBRANCH, yypvt[-1].nodep, bcon( yyval.intval=getlab()) ) ) ;
			    reached = 1;
			    } break;
case 128:
# line 692 "../mip/cgram.y"
{ /* no use compiling expression */
			   /* but we do need to define a label */
			   yyval.intval = getlab();
			   reached = 1;
			   } break;
case 129:
# line 699 "../mip/cgram.y"
{  if( reached ) branch( yyval.intval = getlab() );
			    else yyval.intval = NOLAB;
			    deflab( yypvt[-2].intval );
			    reached = 1;
			    } break;
case 130:
# line 707 "../mip/cgram.y"
{  
#ifndef LINT
			    psline(lineno);
#endif
			    savebc();
			    if( !reached ) werror( "loop not entered at top");
			    if( yypvt[-1].nodep->in.op == ICON && yypvt[-1].nodep->tn.lval != 0 ) flostat = FLOOP;
			    deflab( contlab = getlab() );
			    reached = 1;
			    brklab = getlab();
			    if( flostat == FLOOP ) tfree( yypvt[-1].nodep );
			    else ecomp( buildtree( CBRANCH, yypvt[-1].nodep, bcon( brklab) ) );
			    } break;
case 131:
# line 721 "../mip/cgram.y"
{ /* don't compile expression, but try other semantics */
			    savebc();
			    if( !reached ) werror( "loop not entered at top");
			    deflab( contlab = getlab() );
			    reached = 1;
			    brklab = getlab();
			    } break;
case 132:
# line 730 "../mip/cgram.y"
{  
#ifndef LINT
			    psline(lineno);
#endif
			    if( yypvt[-3].nodep ) ecomp( yypvt[-3].nodep );
			    else if( !reached ) werror( "loop not entered at top");
			    savebc();
#ifndef LINT
			    savelineno();
#endif
			    contlab = getlab();
			    brklab = getlab();
			    deflab( yyval.intval = getlab() );
			    reached = 1;
			    if( yypvt[-1].nodep ) ecomp( buildtree( CBRANCH, yypvt[-1].nodep, bcon( brklab) ) );
			    else flostat |= FLOOP;
			    } break;
case 133:
# line 748 "../mip/cgram.y"
{ /* do some semantics anyway */
			    savebc();
#ifndef LINT
			    savelineno();
#endif
			    contlab = getlab();
			    brklab = getlab();
			    deflab( yyval.intval = getlab() );
			    reached = 1;
			    } break;
case 134:
# line 760 "../mip/cgram.y"
{  
#ifndef LINT
			    psline(lineno);
#endif
			    savebc();
#ifndef LINT
			    savelineno();
#endif
			    brklab = getlab();
#ifdef VAX
			    ecomp( buildtree( FORCE, yypvt[-1].nodep, NIL ) );
#else
			    ecomp( buildtree( FORCE, makety(yypvt[-1].nodep,INT,0,INT), NIL ) );
#endif
			    branch( yyval.intval = getlab() );
			    swstart();
			    reached = 0;
			    } break;
case 135:
# line 780 "../mip/cgram.y"
{ yyval.intval=instruct; stwart=instruct=0; } break;
case 136:
# line 782 "../mip/cgram.y"
{  yyval.intval = icons( yypvt[-0].nodep );  instruct=yypvt[-1].intval; } break;
case 138:
# line 786 "../mip/cgram.y"
{ yyval.nodep=0; } break;
case 140:
# line 791 "../mip/cgram.y"
{  goto bop; } break;
case 141:
# line 795 "../mip/cgram.y"
{
			preconf:
			    if( yychar==RELOP||yychar==EQUOP||yychar==AND||yychar==OR||yychar==ER ){
			    precplaint:
				if( hflag ) werror( "precedence confusion possible: parenthesize!" );
				}
			bop:
			    yyval.nodep = buildtree( yypvt[-1].intval, yypvt[-2].nodep, yypvt[-0].nodep );
			    } break;
case 142:
# line 805 "../mip/cgram.y"
{  yypvt[-1].intval = COMOP;
			    goto bop;
			    } break;
case 143:
# line 809 "../mip/cgram.y"
{  goto bop; } break;
case 144:
# line 811 "../mip/cgram.y"
{  if(yychar==SHIFTOP) goto precplaint; else goto bop; } break;
case 145:
# line 813 "../mip/cgram.y"
{  if(yychar==SHIFTOP ) goto precplaint; else goto bop; } break;
case 146:
# line 815 "../mip/cgram.y"
{  if(yychar==PLUS||yychar==MINUS) goto precplaint; else goto bop; } break;
case 147:
# line 817 "../mip/cgram.y"
{  goto bop; } break;
case 148:
# line 819 "../mip/cgram.y"
{  goto preconf; } break;
case 149:
# line 821 "../mip/cgram.y"
{  if( yychar==RELOP||yychar==EQUOP ) goto preconf;  else goto bop; } break;
case 150:
# line 823 "../mip/cgram.y"
{  if(yychar==RELOP||yychar==EQUOP) goto preconf; else goto bop; } break;
case 151:
# line 825 "../mip/cgram.y"
{  if(yychar==RELOP||yychar==EQUOP) goto preconf; else goto bop; } break;
case 152:
# line 827 "../mip/cgram.y"
{  goto bop; } break;
case 153:
# line 829 "../mip/cgram.y"
{  goto bop; } break;
case 154:
# line 831 "../mip/cgram.y"
{  abop:
				yyval.nodep = buildtree( ASG yypvt[-2].intval, yypvt[-3].nodep, yypvt[-0].nodep );
				} break;
case 155:
# line 835 "../mip/cgram.y"
{  goto abop; } break;
case 156:
# line 837 "../mip/cgram.y"
{  goto abop; } break;
case 157:
# line 839 "../mip/cgram.y"
{  goto abop; } break;
case 158:
# line 841 "../mip/cgram.y"
{  goto abop; } break;
case 159:
# line 843 "../mip/cgram.y"
{  goto abop; } break;
case 160:
# line 845 "../mip/cgram.y"
{  goto abop; } break;
case 161:
# line 847 "../mip/cgram.y"
{  goto abop; } break;
case 162:
# line 849 "../mip/cgram.y"
{  yyval.nodep=buildtree(QUEST, yypvt[-4].nodep, buildtree( COLON, yypvt[-2].nodep, yypvt[-0].nodep ) );
			    } break;
case 163:
# line 852 "../mip/cgram.y"
{  werror( "old-fashioned assignment operator" );  goto bop; } break;
case 164:
# line 854 "../mip/cgram.y"
{  goto bop; } break;
case 166:
# line 858 "../mip/cgram.y"
{  yyval.nodep = buildtree( yypvt[-0].intval, yypvt[-1].nodep, bcon(1) ); } break;
case 167:
# line 860 "../mip/cgram.y"
{ ubop:
			    yyval.nodep = buildtree( UNARY yypvt[-1].intval, yypvt[-0].nodep, NIL );
			    } break;
case 168:
# line 864 "../mip/cgram.y"
{  if( ISFTN(yypvt[-0].nodep->in.type) || ISARY(yypvt[-0].nodep->in.type) ){
				werror( "& before array or function: ignored" );
				yyval.nodep = yypvt[-0].nodep;
				}
			    else goto ubop;
			    } break;
case 169:
# line 871 "../mip/cgram.y"
{  goto ubop; } break;
case 170:
# line 873 "../mip/cgram.y"
{
			    yyval.nodep = buildtree( yypvt[-1].intval, yypvt[-0].nodep, NIL );
			    } break;
case 171:
# line 877 "../mip/cgram.y"
{  yyval.nodep = buildtree( yypvt[-1].intval==INCR ? ASG PLUS : ASG MINUS,
						yypvt[-0].nodep,
						bcon(1)  );
			    } break;
case 172:
# line 882 "../mip/cgram.y"
{  yyval.nodep = doszof( yypvt[-0].nodep ); } break;
case 173:
# line 884 "../mip/cgram.y"
{  yyval.nodep = buildtree( CAST, yypvt[-2].nodep, yypvt[-0].nodep );
			    yyval.nodep->in.left->in.op = FREE;
			    yyval.nodep->in.op = FREE;
			    yyval.nodep = yyval.nodep->in.right;
			    } break;
case 174:
# line 890 "../mip/cgram.y"
{  yyval.nodep = doszof( yypvt[-1].nodep ); } break;
case 175:
# line 892 "../mip/cgram.y"
{  yyval.nodep = buildtree( UNARY MUL, buildtree( PLUS, yypvt[-3].nodep, yypvt[-1].nodep ), NIL ); } break;
case 176:
# line 894 "../mip/cgram.y"
{  yyval.nodep=buildtree(UNARY CALL,yypvt[-1].nodep,NIL); } break;
case 177:
# line 896 "../mip/cgram.y"
{  yyval.nodep=buildtree(CALL,yypvt[-2].nodep,yypvt[-1].nodep); } break;
case 178:
# line 898 "../mip/cgram.y"
{  if( yypvt[-1].intval == DOT ){
				if( notlval( yypvt[-2].nodep ) ) {
				    uerror("structure reference must be addressable");
				    if (yypvt[-2].nodep == NIL) {
					yypvt[-2].nodep = bcon(0);
					yypvt[-2].nodep->tn.type = (PTR|INT);
					yypvt[-2].nodep = buildtree(UNARY MUL, yypvt[-2].nodep, NIL);
					}
				    }
				yypvt[-2].nodep = buildtree( UNARY AND, yypvt[-2].nodep, NIL );
				}
			    idname = yypvt[-0].intval;
			    yyval.nodep = buildtree( STREF, yypvt[-2].nodep, buildtree( NAME, NIL, NIL ) );
			    } break;
case 179:
# line 913 "../mip/cgram.y"
{  idname = yypvt[-0].intval;
			    /* recognize identifiers in initializations */
			    if( blevel==0 && STP(idname)->stype == UNDEF ) {
				register NODE *q;
#ifndef FLEXNAMES
				werror( "undeclared initializer name %.8s", STP(idname)->sname );
#else
				werror( "undeclared initializer name %s", STP(idname)->sname );
#endif
				q = block( FREE, NIL, NIL, INT, 0, INT );
				q->tn.rval = idname;
				defid( q, EXTERN );
				}
			    yyval.nodep=buildtree(NAME,NIL,NIL);
			    STP(yypvt[-0].intval)->suse = -lineno;
			} break;
case 180:
# line 930 "../mip/cgram.y"
{  yyval.nodep=bcon(0);
			    yyval.nodep->tn.lval = lastcon;
			    yyval.nodep->tn.rval = NONAME;
			    if( yypvt[-0].intval ) yyval.nodep->fn.csiz = yyval.nodep->in.type = ctype(LONG);
			    } break;
case 181:
# line 936 "../mip/cgram.y"
{  yyval.nodep=buildtree(FCON,NIL,NIL);
			    yyval.nodep->fpn.dval = dcon;
			    } break;
case 182:
# line 940 "../mip/cgram.y"
{  yyval.nodep = getstr(); /* get string contents */ } break;
case 183:
# line 942 "../mip/cgram.y"
{ yyval.nodep=yypvt[-1].nodep; } break;
case 184:
# line 946 "../mip/cgram.y"
{
			yyval.nodep = tymerge( yypvt[-1].nodep, yypvt[-0].nodep );
			yyval.nodep->in.op = NAME;
			yypvt[-1].nodep->in.op = FREE;
			} break;
case 185:
# line 954 "../mip/cgram.y"
{ yyval.nodep = bdty( NAME, NIL, -1 ); } break;
case 186:
# line 956 "../mip/cgram.y"
{ yyval.nodep = bdty( UNARY CALL, bdty(NAME,NIL,-1),0); } break;
case 187:
# line 958 "../mip/cgram.y"
{  yyval.nodep = bdty( UNARY CALL, yypvt[-3].nodep, 0 ); } break;
case 188:
# line 960 "../mip/cgram.y"
{  goto umul; } break;
case 189:
# line 962 "../mip/cgram.y"
{  goto uary; } break;
case 190:
# line 964 "../mip/cgram.y"
{  goto bary;  } break;
case 191:
# line 966 "../mip/cgram.y"
{ yyval.nodep = yypvt[-1].nodep; } break;
case 192:
# line 970 "../mip/cgram.y"
{  if( STP(yypvt[-1].intval)->stype == UNDEF ){
				register NODE *q;
				q = block( FREE, NIL, NIL, FTN|INT, 0, INT );
				q->tn.rval = yypvt[-1].intval;
				defid( q, EXTERN );
				}
			    idname = yypvt[-1].intval;
			    yyval.nodep=buildtree(NAME,NIL,NIL);
			    STP(idname)->suse = -lineno;
			} break;
	}
	goto yystack;  /* stack new state and value */
}
