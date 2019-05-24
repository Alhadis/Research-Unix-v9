
# line 2 "ld.yac"
static char ID[] = "@(#) ld.yac: 1.8 12/1/83";
#include "system.h"
#include <stdio.h>
#include "bool.h"
#include "attributes.h"
#include "list.h"
#include "structs.h"

#if TRVEC
#include "tv.h"
#include "ldtv.h"
#endif

#include "extrns.h"
#include "sgs.h"
#include "sgsmacros.h"
#include "ldmacros.h"



extern char inline[];


/*
 *
 *	There is a very important variable, called
 *
 *		in_y_exp
 *
 *	that is used to help recognize filenames.
 *
 *	The problems is this:  a UNIX filename can contain
 *	virtually any character, and the nodes in the pathname
 *	are separated by slashes.  Unfortunately, slashes also
 *	occur in expressions, comments, and the /= assignment
 *	operator.  Moreover, the LEX rules ignore white space,
 *	which is important to knowing where one filename stops
 *	and the next begins.
 *
 *	The resolution is this.  Lex doesn't know enough alone to
 *	recognize a filename; so we give it some help.  Whenever we
 *	are in an expression, we set in_y_exp to TRUE, and then lex knows
 *	that any slash is a slash, and should not be kept as part of
 *	a filename.  a/b is a divided by b, not the file a/b.
 *
 *	Consequently, whenever a slash must be kept as a slash,
 *	in_y_exp will be TRUE.  Otherwise, it will be FALSE, and the
 *	lexical analyzer will treat a/b as a filename.
 */

int in_y_exp;

static secnum;					/* number of sections */
static char *fnamptr;
static int nsecspcs, fillflag;			/* parsing status flags */
static ACTITEM *aiptr, *afaiptr, *grptr;	/* pointers to action items */
extern char *curfilnm;				/* name of current ifile */

#if TRVEC
static TVASSIGN *tvslotn,		/* ptr to last  in list of tv slots */
		*slotptr;		/* temp for traversing list	    */
#endif

static int tempi;				/* temporary int */
/*eject*/

# line 167 "ld.yac"
typedef union  {
	int ivalue;	/* yylval values */
	char *sptr;
	long *lptr;

	ACTITEM *aitem;	/* nonterminal values */
	ENODE	*enode;
	} YYSTYPE;
# define NAME 2
# define LONGINT 3
# define INT 4
# define ALIGN 5
# define DOT 6
# define LEN 7
# define MEMORY 8
# define ORG 9
# define REGIONS 10
# define SECTIONS 11
# define PHY 12
# define AND 13
# define ANDAND 14
# define BNOT 15
# define COLON 16
# define COMMA 17
# define DIV 18
# define EQ 19
# define EQEQ 20
# define GE 21
# define GT 23
# define LBRACE 24
# define LE 26
# define LPAREN 27
# define LSHIFT 28
# define LT 29
# define MINUS 30
# define MULT 31
# define NE 32
# define NOT 33
# define OR 34
# define OROR 35
# define PC 36
# define PLUS 37
# define RBRACE 38
# define RPAREN 39
# define RSHIFT 40
# define SEMICOL 41
# define DIVEQ 42
# define MINUSEQ 43
# define MULTEQ 44
# define PLUSEQ 45
# define FILENAME 46
# define TV 47
# define SPARE 48
# define DSECT 49
# define NOLOAD 50
# define COPY 51
# define INFO 52
# define BLOCK 53
# define UMINUS 54
# define GROUP 55
# define RANGE 56
# define ASSIGN 57
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 737 "ld.yac"

/*eject*/
/*VARARGS*/
yyerror(format, a1, a2, a3, a4)
char *format;
{

/*
 * Issue a parsing error message
 */

	char *p;

	p = sname(curfilnm);	/* strip off directories from path name */

/*
 * For any purely YACC-generated error, also print out the current
 * line, up to the point of the error
 */

	if( strcmp(format, "syntax error") == 0 )
		lderror(1, lineno, p, "%s : scanned line = (%s)", format, inline, a1, a2 );
	else
		lderror(1, lineno, p, format, a1, a2, a3, a4);
}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 17,
	19, 123,
	42, 123,
	43, 123,
	44, 123,
	45, 123,
	-2, 125,
-1, 147,
	20, 0,
	21, 0,
	23, 0,
	26, 0,
	29, 0,
	32, 0,
	-2, 103,
-1, 148,
	20, 0,
	21, 0,
	23, 0,
	26, 0,
	29, 0,
	32, 0,
	-2, 104,
-1, 149,
	20, 0,
	21, 0,
	23, 0,
	26, 0,
	29, 0,
	32, 0,
	-2, 105,
-1, 150,
	20, 0,
	21, 0,
	23, 0,
	26, 0,
	29, 0,
	32, 0,
	-2, 106,
-1, 151,
	20, 0,
	21, 0,
	23, 0,
	26, 0,
	29, 0,
	32, 0,
	-2, 107,
-1, 152,
	20, 0,
	21, 0,
	23, 0,
	26, 0,
	29, 0,
	32, 0,
	-2, 108,
	};
# define YYNPROD 128
# define YYLAST 404
short yyact[]={

  97, 105,  17,  40, 108,  93,  20, 104, 101,  52,
  99,  36,  52, 102, 207,  96, 100,  91,  92, 103,
  40,  98, 106,  94,  90,  17,  36,  95, 107,  20,
  51,  13, 188,  12,  15,  81,  71,  51, 167,  68,
 194,  45, 127,  47, 175,  57,  18,  65,  16,  54,
 135,  44,  77,  19,  75,  55,  77, 214, 215, 216,
 217, 229,  53,  86, 228, 176, 227, 226,  33,  18,
  14, 225, 222,  97, 105, 219,  82, 180,  93, 160,
 104, 101,  39,  99,  28, 119, 102,  35,  96, 100,
  91,  92, 103,  74,  98, 106,  94,  90, 209, 179,
  95,  78,  79,  82, 198,  78,  79,  27,  29,  30,
  31,  82,  18, 115, 110, 111, 112,  88, 197, 177,
 172,  69,  66, 124, 123, 185, 134, 129, 114,  16,
 113, 121, 174, 125, 211,  72,  82,  82,  84, 137,
 138, 139, 140, 141, 142, 143, 144, 145, 146, 147,
 148, 149, 150, 151, 152, 153, 154, 169,  80,  72,
 156,  93, 170, 155,  83,  42,  25, 213,  22, 173,
  97, 105,  21, 195,  92,  93,  82, 104, 101,  94,
  99, 132, 178, 102,   9,  96, 100,  91,  92, 103,
 239,  98, 106,  94,  90, 233, 157,  95, 230, 189,
 187,  32, 186, 182, 162, 158, 122, 120, 184, 200,
 117,  67, 159, 190, 129, 193,  16, 201,  93, 116,
 231, 220,  64, 237, 136, 204,  20, 210,  96, 206,
  91,  92, 212,  93, 240, 235,  94,  90, 221, 232,
  95, 218, 223, 109, 205,  91,  92, 203, 202, 199,
 181,  94,  90, 165, 164, 163, 131,  41,  87,  24,
 224, 130, 168,  49,   8,  37,  49, 208, 234, 192,
 183, 238, 118,  85,  41,  89,  97, 105, 191,  11,
  37,  93, 171, 104, 101, 128,  99, 126,  48, 102,
  50,  96, 100,  91,  92, 103, 133,  98,  46,  94,
  90,  97,  43,  95, 166,  76,  93, 161, 104, 101,
  73,  99,  23,  38, 102,  34,  96, 100,  91,  92,
 103,  10,  98,   7,  94,  90,  97,   6,  95,   5,
   4,  93,   3, 104, 101,   2,  99,   1, 236, 102,
 196,  96, 100,  91,  92, 103,  70,  26,   0,  94,
  90,   0,  93,  95, 104, 101,   0,  99,   0,   0,
 102,   0,  96, 100,  91,  92, 103,   0,   0,   0,
  94,  90,  64,  56,  95,  61,  20,   0,   0,   0,
   0,   0,  62,   0,   0,  60,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  63,   0,   0,
  58,   0,   0,  59 };
short yypact[]={

-1000,-1000,  23,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000, 148, 144, 256, 142,  65,-1000,-1000,  66,
-1000,  24,  18, 141,-1000,   7, 370,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,   9,-1000, 195,-1000,   1,-1000,
 108,-1000,  45, 120,-1000,-1000,-1000,-1000, 140,-1000,
 114, 271, 255, 255, -13,-1000,-1000,-1000, 370, 370,
 370, 103, 101, 370,-1000,-1000,-1000, 210,-1000,-1000,
 194,-1000, 270,  47, 188,-1000,  49, 187,  97,  96,
-1000,   7,-1000,   0,  10,-1000, 219,-1000, 219,-1000,
 370, 370, 370, 370, 370, 370, 370, 370, 370, 370,
 370, 370, 370, 370, 370, 370, 370,-1000,-1000,-1000,
-1000,-1000,-1000, 370, 220, 157, 186, 203,  40, 185,
 252,-1000, 251, 250, 260,-1000, 119,-1000,  93,-1000,
-1000,-1000,-1000,  94,-1000,  12,  92,  12, 143, 143,
-1000,-1000,-1000, 215, 215, 334, 313, 200, 200, 200,
 200, 200, 200, 288, 263,  60,  38,-1000, 247, 184,
-1000,-1000, 268,-1000,-1000, 191,  86,-1000, 181, 180,
   0,-1000, 267,  10, 150,  91,  77, 246, 193,-1000,
-1000, 159, 245,-1000, 244,-1000, 260, 241, 150,  11,
-1000,  59,-1000,-1000,-1000, 132, 151,   8, 238,  36,
-1000, 214, 159,  33,-1000,-1000,-1000,-1000,-1000, 180,
 258,-1000,-1000,-1000,  32,  28,  27,  25,  22,-1000,
 179, 213,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
 236, 176, 159, 232, 217, 159,-1000, 171,-1000, 231,
-1000 };
short yypgo[]={

   0, 261, 347,  36,  45,  49, 181,  32,  43,  40,
 346,  44,  63,  50, 340,  55, 338, 337, 335, 332,
 330, 329, 327, 323, 321, 315,  87,  35, 313,  82,
 312, 310, 307,  54, 305, 304,  38, 302,  51,  41,
 298, 296, 290, 288, 287,  42, 285, 282, 278, 275 };
short yyr1[]={

   0,  17,  18,  18,  19,  19,  19,  19,  19,  19,
  19,  19,  20,  25,  25,  26,  26,  27,  27,  16,
  16,  21,  28,  28,  29,  29,  10,  10,   3,  22,
  31,  31,  33,  33,  34,  34,  34,  32,  32,  35,
  35,  35,  36,  30,  30,  23,  37,  37,  38,  38,
  38,  41,  41,  40,  42,   8,  39,  39,  43,  11,
  11,  12,  12,  13,  13,  14,  14,  14,  14,  14,
  44,  44,  44,  45,  45,  45,  45,  46,  47,  47,
  48,  48,   9,   9,   9,   7,   7,   7,   1,   2,
   2,   2,   2,   2,   5,   5,   5,   5,   5,   5,
   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,
   5,   5,  49,  49,  49,  15,  15,  15,  15,  15,
  15,  15,  15,   4,   4,   6,   6,  24 };
short yyr2[]={

   0,   1,   2,   0,   1,   1,   1,   1,   1,   1,
   1,   1,   4,   1,   2,  11,   1,   1,   0,   3,
   0,   4,   1,   2,  11,   1,   1,   0,   3,   6,
   3,   1,   2,   0,   3,   6,   4,   2,   0,   1,
   3,   0,   3,   1,   0,   4,   1,   3,   1,   1,
   1,   1,   3,   5,   5,   2,   6,   1,   6,   4,
   0,   1,   0,   4,   0,   3,   3,   3,   3,   0,
   1,   3,   0,   2,   1,   1,   1,   1,   4,   0,
   1,   3,   2,   2,   0,   2,   0,   2,   4,   1,
   1,   1,   1,   1,   3,   3,   3,   3,   3,   3,
   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
   3,   1,   1,   1,   1,   1,   1,   2,   2,   2,
   4,   4,   3,   1,   1,   1,   1,   2 };
short yychk[]={

-1000, -17, -18, -19, -20, -21, -22, -23,  -1,  -6,
 -24, 256,  10,   8,  47,  11,  -4,   2,  46,  30,
   6,  24,  24, -30,   3,  24,  -2,  42,  19,  43,
  44,  45,  -6,   2, -25, -26,   2, 256, -28, -29,
   2, 256,  24, -37, -38, -39, -40,  -8, -43, 256,
 -42,  30,   2,  55,  -5, -15,   3,  -4,  30,  33,
  15,   5,  12,  27,   2,  38, -26,  16,  38, -29,
 -10,  -3,  27, -31,  48, -33, -34,   7,  56,  57,
  38, -27,  17,  24,  24,   2, -12,   3, -12, -49,
  37,  30,  31,  18,  36,  40,  28,  13,  34,  23,
  29,  21,  26,  32,  20,  14,  35,  41,  17, 256,
 -15, -15, -15,  27,  27,  -5,   9,  16,   2,  38,
  19, -33,  19,  27,  27, -38, -44, -45, -46,  -8,
  -1, 256,  -6, -41, -39, -13,   5, -13,  -5,  -5,
  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,
  -5,  -5,  -5,  -5,  -5,  -5,  -4,  39,  19,   9,
  39, -32,  19,   3,   3,   3, -35, -36,   2,  38,
 -27, -47,  27, -27,  38, -11,  53,  27, -11,  39,
  39,   3,  19,   2,  17,  39, -27,  19,  -7,  19,
 -45, -48,   2, -39,  -9,  23, -14,  27,  27,   3,
  16, -27,   3,   3, -36,   3,  -9,   3, 256,  39,
 -27,   2,  -3,  16,  49,  50,  51,  52,   3,  39,
   7, -27,  39,  -7,   2,  39,  39,  39,  39,  39,
  19,   7,   3,  19, -27,   3, -16,   6, -27,  19,
   3 };
short yydef[]={

   3,  -2,   1,   2,   4,   5,   6,   7,   8,   9,
  10,  11,   0,   0,  44,   0,   0,  -2, 126,   0,
 124,   0,   0,   0,  43,   0,   0,  89,  90,  91,
  92,  93, 127, 125,   0,  13,   0,  16,   0,  22,
  27,  25,  33,  18,  46,  48,  49,  50,   0,  57,
   0,   0,  62,  62,   0, 111, 115, 116,   0,   0,
   0,   0,   0,   0, 123,  12,  14,   0,  21,  23,
   0,  26,   0,   0,   0,  31,  33,   0,   0,   0,
  45,   0,  17,  72,   0,  55,  64,  61,  64,  88,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0, 112, 113, 114,
 117, 118, 119,   0,   0,   0,   0,   0,   0,  38,
   0,  32,   0,   0,  41,  47,  18,  70,  79,  74,
  75,  76,  77,  18,  51,  60,   0,  60,  94,  95,
  96,  97,  98,  99, 100, 101, 102,  -2,  -2,  -2,
  -2,  -2,  -2, 109, 110,   0,   0, 122,   0,   0,
  28,  29,   0,  30,  34,   0,  18,  39,   0,  86,
   0,  73,   0,   0,  84,  69,   0,   0,   0, 120,
 121,  18,   0,  37,   0,  36,   0,   0,  84,   0,
  71,  18,  80,  52,  53,   0,   0,   0,   0,   0,
  54,   0,  18,   0,  40,  42,  56,  85,  87,  86,
   0,  82,  83,  58,   0,   0,   0,   0,   0,  63,
   0,   0,  35,  78,  81,  65,  66,  67,  68,  59,
   0,   0,  18,   0,  20,  18,  15,   0,  24,   0,
  19 };
#ifndef lint
static	char yaccpar_sccsid[] = "@(#)yaccpar 1.6 88/02/08 SMI"; /* from UCB 4.1 83/02/11 */
#endif

#
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

yyparse() {

	short yys[YYMAXDEPTH];
	short yyj, yym;
	register YYSTYPE *yypvt;
	register short yystate, *yyps, yyn;
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
	if( yydebug  ) printf( "state %d, char 0%o\n", yystate, yychar );
#endif
		if( ++yyps>= &yys[YYMAXDEPTH] ) { yyerror( "yacc stack overflow" ); return(1); }
		*yyps = yystate;
		++yypv;
		*yypv = yyval;

 yynewstate:

	yyn = yypact[yystate];

	if( yyn<= YYFLAG ) goto yydefault; /* simple state */

	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
	if( (yyn += yychar)<0 || yyn >= YYLAST ) goto yydefault;

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
		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
		/* look through exception table */

		for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */

		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
			}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:   /* brand new error */

			yyerror( "syntax error" );
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
			if( yydebug ) printf( "error recovery discards char %d\n", yychar );
#endif

			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */

			}

		}

	/* reduction by production yyn */

#ifdef YYDEBUG
		if( yydebug ) printf("reduce %d\n",yyn);
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
			
case 1:
# line 178 "ld.yac"
 ; break;
case 3:
# line 182 "ld.yac"
 ; break;
case 8:
# line 188 "ld.yac"
 {
			bldexp(yypvt[-0].enode, &explist);
			} break;
case 9:
# line 191 "ld.yac"
 {
			filespec(yypvt[-0].sptr);
			} break;
case 11:
# line 195 "ld.yac"
 {
			for( tempi = lineno; tempi == lineno; )
				yychar = yylex();
			yyerrok;
			yyclearin;
			} break;
case 12:
# line 203 "ld.yac"
 {
#if USEREGIONS == 0
			yyerror("REGIONS command not allowed in a %s load", SGS);
#endif
			} break;
case 14:
# line 211 "ld.yac"
 ; break;
case 15:
# line 213 "ld.yac"
 {
			long *orgp, *lenp, *vaddrp, vzero;
			REGION *rgnp;
			orgp = yypvt[-6].lptr;
			lenp = yypvt[-2].lptr;
			if (*lenp <= 0)
				yyerror("region %s has invalid length",yypvt[-10].sptr);
			if ( (vaddrp = yypvt[-0].lptr) == NULL )
				vzero = *orgp;
			else
				vzero = *orgp - *vaddrp;
			if ( (vzero & 0xfL) != 0 ) {
				yyerror("virtual 0 (paddr %.1lx) of region %s is not a multiple of 16",
					vzero, yypvt[-10].sptr);
				vzero &= ~0xfL;
				}
			rgnp = (REGION *) myalloc(sizeof(REGION));
			rgnp->rgorig = *orgp;
			rgnp->rglength = *lenp;
			rgnp->rgvaddr = (vaddrp == NULL) ? 0L : *vaddrp;
			if( (rgnp->rgvaddr + rgnp->rglength) > 0x10000L ) {
				yyerror("addresses of region %s (vaddr=%.1lx  len=%.1lx) exceed 64K",
					yypvt[-10].sptr, rgnp->rgvaddr, *lenp);
				rgnp->rglength = 0x10000L - rgnp->rgvaddr;
				}
			strncpy( rgnp->rgname, yypvt[-10].sptr, 8 );
			listadd(l_REG, &reglist, rgnp);
			} break;
case 16:
# line 241 "ld.yac"
 {
			yyerror("REGIONS specification ignored");
			} break;
case 18:
# line 248 "ld.yac"
 ; break;
case 19:
# line 250 "ld.yac"
 {
			if ( *(long *) yypvt[-0].lptr > 0xffffL ) {
				yyerror("virtual address %.1lx exceeds 0xffff: truncated to %.1lx",
					*(long *) yypvt[-0].lptr, *(long *) yypvt[-0].lptr & 0xffffL);
				*(long *) yypvt[-0].lptr &= 0xffffL;
				}
			yyval.lptr = yypvt[-0].lptr;
			} break;
case 20:
# line 259 "ld.yac"
 {
			yyval.lptr = NULL;
			} break;
case 21:
# line 264 "ld.yac"
 
		; break;
case 23:
# line 268 "ld.yac"
 ; break;
case 24:
# line 270 "ld.yac"
 {
			long *orgp, *lenp;
			MEMTYPE *memp;
			orgp = yypvt[-5].lptr;
			lenp = yypvt[-1].lptr;
			memp = (MEMTYPE *) myalloc(sizeof(MEMTYPE));
			memp->mtorig = *orgp;
			memp->mtlength = *lenp;
			memp->mtattr = yypvt[-9].ivalue;
			strncpy( memp->mtname, yypvt[-10].sptr, 8 );
			listadd(l_MEM, &memlist, memp);
			} break;
case 25:
# line 282 "ld.yac"
 {
			yyerror("MEMORY specification ignored");
			} break;
case 26:
# line 287 "ld.yac"
 {
			yyval.ivalue = yypvt[-0].ivalue;
			} break;
case 27:
# line 290 "ld.yac"
 {			/* empty */
			yyval.ivalue = att_R | att_W | att_X | att_I;
			} break;
case 28:
# line 295 "ld.yac"
  { 
			int attflgs;
			char *p;
			attflgs = 0;
			for( p = yypvt[-1].sptr; *p; p++ )
				switch (*p) {
				case 'R': attflgs |= att_R; break;
				case 'W': attflgs |= att_W; break;
				case 'X': attflgs |= att_X; break;
				case 'I': attflgs |= att_I; break;
				default:  yyerror("bad attribute value in MEMORY directive: %c", *p);
				}
			yyval.ivalue = attflgs;
			} break;
case 29:
# line 311 "ld.yac"
 {
#if TRVEC
			int length;

			tvspec.tvinflnm = curfilnm;
			tvspec.tvinlnno = lineno;

			length = 0;
			for( slotptr = tvslot1; slotptr != NULL; slotptr = slotptr->nxtslot ) {
				TVASSIGN *slotptr2;
				length = max(length, slotptr->slot);
				for (slotptr2 = tvslot1; slotptr2 != slotptr; slotptr2 = slotptr2->nxtslot )
					if (slotptr2->slot == slotptr->slot) {
						lderror(1,0,NULL,
						"function %s assigned to tv slot %d which is already in use", slotptr->funname, slotptr->slot);
						break;
						}
				}
			if( tvspec.tvlength > 0 ) {
				if( length > tvspec.tvlength ) {
					yyerror("ASSIGN slot %d exceeds total TV size of %d",
						length, tvspec.tvlength);
					tvspec.tvlength = length;
					}
				if( tvspec.tvrange[1] > tvspec.tvlength ) {
					yyerror("RANGE value %d exceeds total TV size of %d",
						tvspec.tvrange[1], tvspec.tvlength);
					tvspec.tvlength = tvspec.tvrange[1];
					}
				}
			else
				tvspec.tvlength = max(length, tvspec.tvrange[1]);
#else
			yyerror("usage of unimplemented syntax");
#endif
			} break;
case 30:
# line 349 "ld.yac"
 {
#if TRVEC
			tvspec.tvosptr = (OUTSECT *) ((unsigned) *(long *) yypvt[-0].lptr);
#else
			yyerror("usage of unimplemented syntax");
#endif
			} break;
case 34:
# line 363 "ld.yac"
 {
#if TRVEC
			if( tvspec.tvlength > 0 )
				yyerror("illegal multiple LENGTH fields in the TV directive");
			tvspec.tvlength = (int) (*(long *) yypvt[-0].lptr);
#else
			yyerror("usage of unimplemented syntax");
#endif
			} break;
case 35:
# line 372 "ld.yac"
 {
#if TRVEC
			if( tvspec.tvrange[1] > 0 )
				yyerror("illegal multiple RANGE fields in the TV directive");
			if( tvspec.tvrange[0] < 0 || tvspec.tvrange[0] > tvspec.tvrange[1] )
				yyerror("illegal RANGE syntax: r[0]<0 or r[0]>r[1]");
			else {
				tvspec.tvrange[0] = (int) (*(long *) yypvt[-3].lptr);
				tvspec.tvrange[1] = (int) (*(long *) yypvt[-1].lptr);
				}
#else
			yyerror("usage of unimplemented syntax");
#endif
			} break;
case 37:
# line 389 "ld.yac"
 {
#if TRVEC
#if FLEXNAMES
			tvspec.tvfnfill = savefn(yypvt[-0].sptr);
#else
			strncpy(tvspec.tvfnfill, yypvt[-0].sptr, 8 );
#endif
#else
			yyerror("usage of unimplemented syntax");
#endif
			} break;
case 42:
# line 407 "ld.yac"
 {
#if TRVEC
			if( (int) (*(long *) yypvt[-0].lptr) == 0 )
				yyerror("illegal ASSIGN slot number (0)");
			if( tvslot1 == NULL )
				tvslot1 = tvslotn = (TVASSIGN *) myalloc(sizeof(TVASSIGN));
			else
				tvslotn = tvslotn->nxtslot = (TVASSIGN *) myalloc(sizeof(TVASSIGN));
#if FLEXNAMES
			tvslotn->funname = savefn(yypvt[-2].sptr);
#else
			strncpy(tvslotn->funname, yypvt[-2].sptr, 8);
#endif
			tvslotn->slot = (int) (*(long *) yypvt[-0].lptr);
			tvslotn->nxtslot = NULL;
#else
			yyerror("usage of unimplemented syntax");
#endif
		} break;
case 43:
# line 428 "ld.yac"
 {
			long org;
			org = *(long *) yypvt[-0].lptr;
#if TRVEC
			chktvorg(org, &(tvspec.tvbndadr));
#endif
			} break;
case 45:
# line 438 "ld.yac"
 
		; break;
case 53:
# line 454 "ld.yac"
 {
			listadd(l_AI,&bldoutsc,grptr);
			grptr = NULL;
			} break;
case 54:
# line 460 "ld.yac"
 {
			if ( yypvt[-3].lptr != NULL && yypvt[-2].lptr != NULL )
				yyerror("bonding excludes alignment");
			grptr = dfnscngrp(AIDFNGRP, (long *)yypvt[-3].lptr, (long *)yypvt[-2].lptr, (long *)yypvt[-1].lptr);
			strncpy( grptr->dfnscn.ainame, "*group*", 8 );
			} break;
case 55:
# line 468 "ld.yac"
 {
			char *fp;
			fp = yypvt[-0].sptr;
			if (fp[0] == 'l')	/* library flag */
				library(fp);
			else
				yyerror("bad flag value in SECTIONS directive: -%s", yypvt[-0].sptr);
			} break;
case 56:
# line 478 "ld.yac"
 {
			if (aiptr == NULL)
				goto scnerr;
			aiptr->dfnscn.aifill = yypvt[-1].ivalue;
			aiptr->dfnscn.aifillfg = fillflag;
			listadd(l_AI,grptr ? &grptr->dfnscn.sectspec : &bldoutsc,aiptr);
			if (grptr && yypvt[-0].ivalue)
				yyerror("can not specify an owner for section within a group");
			aiptr = NULL;
		} break;
case 57:
# line 488 "ld.yac"
 {
			if (aiptr)
				yyerror("section %s not built", aiptr->dfnscn.ainame);
		scnerr:
			aiptr = NULL;
			} break;
case 58:
# line 496 "ld.yac"
 {
			secnum++;
			if ( OKSCNNAME(yypvt[-5].sptr) == 0 )
				yyerror("%s is a reserved section name", yypvt[-5].sptr);
			if ( yypvt[-4].lptr != NULL && yypvt[-3].lptr != NULL )
				yyerror("bonding excludes alignment");
			aiptr = dfnscngrp(AIDFNSCN, (long *)yypvt[-4].lptr, (long *)yypvt[-3].lptr, (long *)yypvt[-2].lptr);
			if (grptr && aiptr->dfnscn.aibndadr != -1L)
				yyerror("can not bond a section within a group");
			if (grptr && aiptr->dfnscn.aialign != 0L)
				yyerror("can not align a section within a group");
			aiptr->dfnscn.aisctype = yypvt[-1].ivalue;
			strncpy( aiptr->dfnscn.ainame, yypvt[-5].sptr, 8 );
			} break;
case 59:
# line 512 "ld.yac"
 {
			yyval.lptr = yypvt[-1].lptr;
			} break;
case 60:
# line 515 "ld.yac"
 {			/* empty */
			yyval.lptr = NULL;
			} break;
case 61:
# line 520 "ld.yac"
 {
			yyval.lptr = yypvt[-0].lptr;
			} break;
case 62:
# line 523 "ld.yac"
 {			/* empty */
			yyval.lptr = NULL;
			} break;
case 63:
# line 528 "ld.yac"
 {
			yyval.lptr = yypvt[-1].lptr;
			} break;
case 64:
# line 531 "ld.yac"
 {			/* empty */
			yyval.lptr = NULL;
			} break;
case 65:
# line 536 "ld.yac"
 {
			yyval.ivalue = STYP_DSECT;
			} break;
case 66:
# line 539 "ld.yac"
 {
			yyval.ivalue = STYP_NOLOAD;
			} break;
case 67:
# line 542 "ld.yac"
 {
			yyval.ivalue = STYP_COPY | STYP_DSECT;
			} break;
case 68:
# line 545 "ld.yac"
 {
			yyval.ivalue = STYP_INFO | STYP_DSECT;
			} break;
case 69:
# line 548 "ld.yac"
 {			/* empty */
			yyval.ivalue = STYP_REG;
			} break;
case 74:
# line 559 "ld.yac"
 {
			bldadfil( ((ACTITEM *) ldfilist.tail)->ldlbry.aifilnam, aiptr );
			} break;
case 75:
# line 562 "ld.yac"
 {
			bldexp(yypvt[-0].enode,&aiptr->dfnscn.sectspec);
			} break;
case 76:
# line 565 "ld.yac"
 {
			in_y_exp = 0;
			yyerror("statement ignored");
			for( tempi = lineno; ((int)yychar > 0) && (tempi == lineno) ; )
				yychar = yylex();
			if ((int) yychar <= 0)
				lderror(2,0,NULL, "unexpected EOF");
			yyerrok;
			yyclearin;
			} break;
case 77:
# line 577 "ld.yac"
 {
			fnamptr = savefn(yypvt[-0].sptr);
			nsecspcs = 0;
			bldldfil(fnamptr,0);
			afaiptr = bldadfil(fnamptr,aiptr);
			} break;
case 78:
# line 584 "ld.yac"
 {
			afaiptr->adfile.ainadscs = nsecspcs;
			afaiptr->adfile.aifilflg = fillflag;
			afaiptr->adfile.aifill2 = yypvt[-0].ivalue;
			} break;
case 80:
# line 592 "ld.yac"
 {
			nsecspcs++;
			bldadscn(yypvt[-0].sptr,fnamptr,aiptr);
			} break;
case 81:
# line 596 "ld.yac"
 {
			nsecspcs++;
			bldadscn(yypvt[-0].sptr,fnamptr,aiptr);
			} break;
case 82:
# line 602 "ld.yac"
 {
			if (grptr)
				strncpy( grptr->dfnscn.aiowname, yypvt[-0].sptr, 8 );
			else
				strncpy( aiptr->dfnscn.aiowname, yypvt[-0].sptr, 8 );
			yyval.ivalue = 1;
			} break;
case 83:
# line 609 "ld.yac"
 {
			if (grptr)
				grptr->dfnscn.aiattown = yypvt[-0].ivalue;
			else
				aiptr->dfnscn.aiattown = yypvt[-0].ivalue;
			yyval.ivalue = 1;
			} break;
case 84:
# line 616 "ld.yac"
 {				/* empty */ 
			yyval.ivalue = 0;
			} break;
case 85:
# line 621 "ld.yac"
 {
			fillflag = 1;
			yyval.ivalue = (int) (*(long *)yypvt[-0].lptr);
			} break;
case 86:
# line 625 "ld.yac"
 {				/* empty */
			fillflag = 0;
			yyval.ivalue = 0;
			} break;
case 87:
# line 629 "ld.yac"
 {
			yyerror("bad fill value");
			fillflag = 0;
			yyerrok;
			yyclearin;
			yyval.ivalue = 0;
			} break;
case 88:
# line 638 "ld.yac"
 {
			in_y_exp = 0;
			if ( yypvt[-2].ivalue == EQ )
				yyval.enode = buildtree(EQ, yypvt[-3].enode, yypvt[-1].enode);
			else {
				ENODE *p,*ndp;
				ndp = buildtree(yypvt[-2].ivalue,yypvt[-3].enode,yypvt[-1].enode);
				p = (ENODE *) myalloc(sizeof(ENODE));
				*p = *yypvt[-3].enode;
				yyval.enode = buildtree(EQ,p,ndp);
				}
			} break;
case 89:
# line 652 "ld.yac"
		{ yyval.ivalue = DIV;	in_y_exp = TRUE; } break;
case 90:
# line 653 "ld.yac"
		{ yyval.ivalue = EQ;	in_y_exp = TRUE; } break;
case 91:
# line 654 "ld.yac"
	{ yyval.ivalue = MINUS;	in_y_exp = TRUE; } break;
case 92:
# line 655 "ld.yac"
	{ yyval.ivalue = MULT;	in_y_exp = TRUE; } break;
case 93:
# line 656 "ld.yac"
	{ yyval.ivalue = PLUS;	in_y_exp = TRUE; } break;
case 94:
# line 659 "ld.yac"
 {
			bop:
				yyval.enode = buildtree(yypvt[-1].ivalue,yypvt[-2].enode,yypvt[-0].enode);
			} break;
case 95:
# line 663 "ld.yac"
 { goto bop; } break;
case 96:
# line 664 "ld.yac"
 { goto bop; } break;
case 97:
# line 665 "ld.yac"
 { goto bop; } break;
case 98:
# line 666 "ld.yac"
 { goto bop; } break;
case 99:
# line 667 "ld.yac"
 { goto bop; } break;
case 100:
# line 668 "ld.yac"
 { goto bop; } break;
case 101:
# line 669 "ld.yac"
 { goto bop; } break;
case 102:
# line 670 "ld.yac"
 { goto bop; } break;
case 103:
# line 671 "ld.yac"
 { goto bop; } break;
case 104:
# line 672 "ld.yac"
 { goto bop; } break;
case 105:
# line 673 "ld.yac"
 { goto bop; } break;
case 106:
# line 674 "ld.yac"
 { goto bop; } break;
case 107:
# line 675 "ld.yac"
 { goto bop; } break;
case 108:
# line 676 "ld.yac"
 { goto bop; } break;
case 109:
# line 677 "ld.yac"
 { goto bop; } break;
case 110:
# line 678 "ld.yac"
 { goto bop; } break;
case 111:
# line 679 "ld.yac"
 {
			yyval.enode = yypvt[-0].enode;
			} break;
case 114:
# line 686 "ld.yac"
 {
			yyerror ("semicolon required after expression");
			} break;
case 115:
# line 691 "ld.yac"
 {
			yyval.enode = cnstnode(*(long *) yypvt[-0].lptr);
			} break;
case 116:
# line 694 "ld.yac"
 {
			yyval.enode = yypvt[-0].enode;
			} break;
case 117:
# line 697 "ld.yac"
 {
			yyval.enode = buildtree(UMINUS,yypvt[-0].enode,NULL);
			} break;
case 118:
# line 700 "ld.yac"
 {
			yyval.enode = buildtree(yypvt[-1].ivalue,yypvt[-0].enode,NULL);
			} break;
case 119:
# line 703 "ld.yac"
 {
			yyval.enode = buildtree(BNOT,yypvt[-1].ivalue,NULL);
			} break;
case 120:
# line 706 "ld.yac"
 {
			yyval.enode = buildtree(yypvt[-3].ivalue,yypvt[-1].enode,NULL);
			} break;
case 121:
# line 709 "ld.yac"
 {
			yyval.enode = buildtree(yypvt[-3].ivalue,yypvt[-1].enode,NULL);
			} break;
case 122:
# line 712 "ld.yac"
 {
			yyval.enode = yypvt[-1].enode;
			} break;
case 123:
# line 717 "ld.yac"
	{
			yyval.enode = symnode(yypvt[-0].sptr);
			} break;
case 124:
# line 720 "ld.yac"
	{
			yyval.enode = symnode(NULL);
			} break;
case 125:
# line 725 "ld.yac"
 {
			yyval.sptr = yypvt[-0].sptr;
			} break;
case 126:
# line 728 "ld.yac"
 {
			yyval.sptr = yypvt[-0].sptr;
			} break;
case 127:
# line 733 "ld.yac"
 {
			pflags(yypvt[-0].sptr, TRUE);
			} break;
		}
		goto yystack;  /* stack new state and value */

	}
