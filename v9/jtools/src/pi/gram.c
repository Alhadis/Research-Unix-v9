
#include "gram.h"
#include "expr.pub"
#include "bpts.pub"
#include <ctype.h>
int LexIndex;
int LexGoal;
struct Expr *CurrentExpr;
char *LexString;
char *yyerr;		/* yacc doesn't use this */
long yyres;
char Token[128];
int DotDot;
Expr *E_IConst(long), *E_DConst(double);

typedef union  {
	char		cc;
	long		ll;
	char		ss[32];
	struct Expr	*ee;
	double		dd;
} YYSTYPE;
# define G_EXPR 257
# define G_DOTEQ_CONEX 258
# define G_DOLEQ_CONEX 259
# define G_CONEX 260
# define G_DOTDOT 261
# define ICONST 262
# define ID 263
# define PCENT 264
# define EQUAL 265
# define SLASH 266
# define DOLLAR 267
# define SIZEOF 268
# define TYPEOF 269
# define QMARK 270
# define SEMI 271
# define UNOP 272
# define STAR 273
# define PLUS 274
# define MINUS 275
# define AMPER 276
# define ARROW 277
# define DOT 278
# define LB 279
# define LP 280
# define COMMA 281
# define ERROR 282
# define RB 283
# define RP 284
# define PLUSPLUS 285
# define MINUSMINUS 286
# define EQUALEQUAL 287
# define GREATER 288
# define LESS 289
# define BAR 290
# define BARBAR 291
# define AMPERAMPER 292
# define HAT 293
# define TILDE 294
# define GREATEREQUAL 295
# define LESSEQUAL 296
# define FABS 297
# define GREATERGREATER 298
# define LESSLESS 299
# define BANG 300
# define BANGEQUAL 301
# define DCONST 302
# define LC 303
# define RC 304
# define DOTDOT 305
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256



#define LOOK (LexString[LexIndex ])
#define TAKE (AddToken(), LexString[LexIndex++])
#define MORE (LexString[LexIndex+1])
#define yc (yylval.cc)
#define yd (yylval.dd)
#define yl (yylval.ll)
#define ys (yylval.ss)
#define ishex(x) (isdigit(x) || (x>='a'&&x<='f') || (x>='A'&&x<='F'))
#define isoct(x) ( x>='0' && x<='7' )
int doyylex();
int yyerror(char*);

yylex()
{
	int token = doyylex();

	return token;
}

void AddToken()
{
	int l = strlen(Token);

	if( l < 64 ){
		Token[l] = LOOK;
		Token[l+1] = '\0';
	}
}

doyylex()
{
	double atof(char*);

	if( LexIndex < 0 ){
		LexIndex = 0;
		return LexGoal;
	}
	while( isspace(LOOK) ) TAKE;
	Token[0] = '\0';
	if( isalpha(LOOK) || LOOK=='_' || LOOK=='$' ){
		TAKE;
		while( isalnum(LOOK) || LOOK=='_' ) TAKE;
		strcpy( ys, Token );
		if( !strcmp(ys,"sizeof") ) return SIZEOF;
		if( !strcmp(ys,"typeof") ) return TYPEOF;
		if( !strcmp(ys,"fabs") ) return FABS;
		if( !strcmp(ys,"$") ) return DOLLAR;
		return ID;
	}
	if( LOOK == '\'' ){
		TAKE;
		if( LOOK == '\\' ){
			TAKE;
			if( MORE != '\'' ) return 0;
			char *trans = "bnftv", *late = "\b\n\f\t\v";
			yl = LOOK;
			for( int i = 0; trans[i]; ++i )
				if( LOOK == trans[i] ) yl = late[i];
			TAKE; TAKE; return ICONST;
		}
		if( MORE != '\'' ) return 0;
		yl = TAKE;
		TAKE;
		return ICONST;
	}
	if( LOOK=='0' && (MORE=='x' || MORE=='X') ){
		TAKE; TAKE;
		if( !ishex(LOOK) ) return 0;
		for( yl = 0; ishex(LOOK); TAKE )
		    yl = (yl<<4) + (isalpha(LOOK) ? (LOOK|' ')+10-'a' : LOOK-'0');
		return ICONST;
	}
	if( LOOK=='0' ){
		for( TAKE, yl = 0; isoct(LOOK); TAKE ) yl = (yl<<3) + LOOK - '0';
		goto IorD;
	}
	if( isdigit(LOOK) ){
		for( yl = 0; isdigit(LOOK); TAKE ) yl = yl*10 + LOOK - '0';
		goto IorD;
	}
	if( LOOK == '.' && isdigit(MORE) ) goto Point;
#define EAT2(x) {TAKE; TAKE; return x;}
	if( LOOK=='.' && MORE=='.' ) EAT2(DOTDOT)
	if( LOOK=='-' && MORE=='>' ) EAT2(ARROW)
	if( LOOK=='-' && MORE=='-' ) EAT2(MINUSMINUS)
	if( LOOK=='+' && MORE=='+' ) EAT2(PLUSPLUS)
	if( LOOK=='=' && MORE=='=' ) EAT2(EQUALEQUAL)
	if( LOOK=='!' && MORE=='=' ) EAT2(BANGEQUAL)
	if( LOOK==':' && MORE=='=' ) EAT2(EQUAL)
	if( LOOK=='>' && MORE=='=' ) EAT2(GREATEREQUAL)
	if( LOOK=='<' && MORE=='=' ) EAT2(LESSEQUAL)
	if( LOOK=='&' && MORE=='&' ) EAT2(AMPERAMPER)
	if( LOOK=='|' && MORE=='|' ) EAT2(BARBAR)
	if( LOOK=='>' && MORE=='>' ) EAT2(GREATERGREATER)
	if( LOOK=='<' && MORE=='<' ) EAT2(LESSLESS)
	switch( TAKE ){
		case '>' : return GREATER;
		case '<' : return LESS;
		case '/' : return SLASH;
		case '*' : return STAR;
		case '+' : return PLUS;
		case '-' : return MINUS;
		case '.' : return DOT;
		case '(' : return LP;
		case ')' : return RP;
		case '[' : return LB;
		case ']' : return RB;
		case '&' : return AMPER;
		case ',' : return COMMA;
		case '%' : return PCENT;
		case '=' : return EQUAL;
		case ';' : return SEMI;
		case '|' : return BAR;
		case '^' : return HAT;
		case '~' : return TILDE;
		case '!' : return BANG;
		case '{' : return LC;
		case '}' : return RC;
		default  : return 0;
	}
IorD:
	if( LOOK=='l' || LOOK=='L' ) return TAKE, ICONST;
	if( LOOK=='.' && MORE=='.' ) return ICONST;
	if( LOOK=='.' ) goto Point;
	if( LOOK=='e' || LOOK=='E' ) goto Exp;
	return ICONST;
Point:
	for( TAKE; isdigit(LOOK); TAKE) {}
	if( LOOK!='e' && LOOK!='E' ) goto Double;
Exp:
	TAKE;
	if( LOOK=='+' || LOOK=='-' ) TAKE;
	if( !isdigit(LOOK) ) return 0;
	while( isdigit(LOOK) ) TAKE;
Double:
	yd = atof(Token);
	return DCONST;	

}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 62
# define YYLAST 800
short yyact[]={

  66,  50,  68, 128,  81,  72, 127,  42,   9,  67,
  69,  64,  54,  65,  48,  70,  10,  49,  35,  34,
 129, 126, 110,  56,  59,  58,  53,  51,  52,  55,
  93,  61,  60,   8,  62,  63,   7,  57,  66,  50,
  68,  47,  65,  48,  70,   1, 117,  67,  69,  64,
  54,  65,  48,  70,   0,  49,   0, 125,   0,   0,
   0,  56,  59,  58,  53,  51,  52,  55,   0,  61,
  60,   0,  62,  63,   0,  57,  66,  50,  68,  47,
   2,   4,   5,   6,   3,  67,  69,  64,  54,  65,
  48,  70,   0,  49,   0,   0, 120,   0,   0,  56,
  59,  58,  53,  51,  52,  55,   0,  61,  60,   0,
  62,  63,   0,  57,  66,  50,  68,  47,   0,   0,
   0,   0,   0,  67,  69,  64,  54,  65,  48,  70,
   0,  49,   0,   0,   0,   0,   0,  56,  59,  58,
  53,  51,  52,  55,   0,  61,  60,   0,  62,  63,
   0,  57,   0,   0, 116,  47,  66,  50,  68,   0,
   0,   0,   0,  82,   0,  67,  69,  64,  54,  65,
  48,  70,   0,  49,   0,   0,   0,   0,   0,  56,
  59,  58,  53,  51,  52,  55,   0,  61,  60,   0,
  62,  63,   0,  57,  66,  50,  68,  47,   0,   0,
   0,  46,   0,  67,  69,  64,  54,  65,  48,  70,
   0,  49,   0,   0,   0,   0,   0,  56,  59,  58,
  53,  51,  52,  55,   0,  61,  60,   0,  62,  63,
   0,  57,  66,  50,  68,  47,   0,   0,   0,   0,
   0,  67,  69,  64,  54,  65,  48,  70,   0,   0,
   0,   0,   0,   0,   0,  56,  59,  58,  53,  51,
  52,  55,   0,  61,  60,   0,  62,  63,   0,  57,
  66,  50,  68,  47,   0,   0,   0,   0,   0,  67,
  69,  64,  54,  65,  48,  70,   0,  49,   0,   0,
   0,   0,   0,  56,  59,  58,  53,  51,  52,  55,
   0,  61,  60,   0,  62,  63,   0,  57,  66,  50,
  68,  14,  13,   0,   0,   0,  12,  67,  69,  64,
  54,  65,  48,  70,  16,  15,   0,   0,   0,  17,
   0,  56,  59,  58,  53,  51,  52,  55,   0,  61,
  60,   0,  62,  63,  66,  57,  68,   0,   0,   0,
   0,   0,   0,  67,  69,  64,  54,  65,  48,  70,
   0,   0,  38,   0,  40,   0,   0,  56,  59,  58,
  53,  39,  52,  55,   0,  61,  60,  42,  62,  63,
  66,  57,  68,   0,   0,   0,   0,   0,   0,  67,
  69,  64,  54,  65,  48,  70,   0,   0,   0,   0,
   0,   0,   0,  56,  59,  58,  53,   0,   0,  55,
   0,  61,  60,   0,  62,  63,  66,  57,  68,   0,
   0,   0,   0,   0,   0,  67,  69,  64,  54,  65,
  48,  70,   0,   0,   0,   0,   0,   0,   0,  56,
  59,  58,   0,   0,   0,  55,   0,  61,  60,   0,
  62,  63,  66,  57,  68,   0,   0,   0,   0,   0,
   0,  67,  69,  64,  54,  65,  48,  70,   0,   0,
   0,   0,   0,   0,   0,  56,  59,  58,   0,  66,
   0,  68,   0,  61,  60,   0,  62,  63,  67,  57,
  23,  21,  65,  48,  70,  19,  26,  27,   0,   0,
  66,  24,  68,  29,  25,   0,   0,   0,  31,  67,
  69,  64, 118,  65,  48,  70,   0,   0,   0,   0,
   0,   0,  30,   0,   0,  32,   0,   0,  28,   0,
  22,  20,  23,  21,  62,  63,   0,  19,  26,  27,
   0,   0,  38,  24,  40,  29,  25,   0,   0,   0,
  31,  39,  41,  37,   0,   0,   0,  42,   0,   0,
   0,   0,  91,   0,  30,   0,  66,  32,  68,   0,
  28,   0,  22,  20,   0,  67,  69,  64,   0,  65,
  48,  70,   0,   0,   0,   0,   0,   0,   0,  56,
  59,  58,   0,   0,   0,   0,   0,  61,  60,   0,
  62,  63,  66,  57,  68,   0,   0,   0,   0,   0,
   0,  67,  69,  64,   0,  65,  48,  70,   0,   0,
  38,   0,  40,   0,   0,   0,  59,  58,   0,  39,
  41,  37,   0,  61,  60,  42,  62,  63,  66, 124,
  68,   0,   0,   0,   0,   0,   0,  67,  69,  64,
   0,  65,  48,  70,  38,   0,  40,   0,   0,   0,
   0, 123,   0,  39,  41,  37,  38,   0,  40,  42,
  18,   0,   0, 122,   0,  39,  41,  37,  38,  33,
  40,  42,   0,   0,   0,  36,   0,  39,  41,  37,
   0,  71,   0,  42,   0,  73,  74,  75,  76,  77,
  78,  79,  80,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  92,   0,
  94,  95,  96,  97,  98,  99, 100, 101, 102, 103,
 104, 105, 106, 107, 108, 109,   0, 111, 112, 113,
 114, 115,  11, 119,   0,   0,   0,   0,   0,   0,
   0,   0, 121,   0,   0,   0,   0,   0,  43,  44,
  45,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  83,  84,   0,
  85,  86,  87,  88,  89,  90,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0, 130 };
short yypact[]={

-177,-1000,-1000,-1000,-270,-251,  49, 270, 270,-246,
-247, 414,-1000,-1000,-1000,  49,  49,  49, -70,-1000,
 270,-275,-1000,-1000, 270, 270, 270, 270, 270, 270,
 270, 270,-276,-108,  49,  49,-1000,  49,  49,  49,
  49,  49,  49,-272,-272, 278,-1000, 270,-233, 270,
 270, 270, 270, 270, 270, 270, 270, 270, 270, 270,
 270, 270, 270, 270, 270,-241, 270, 270, 270, 270,
 270,-150, 228,-235, 302,-235,-235,-235,-235,-235,
-188, 270,-1000, 402, 390,  98,-272,-272,-272,  98,
 356,-1000,   6,-1000,  44,  44,  80, 116, 152, 302,
 188, 338, 338, 236, 236, 236, 236, 374, 374, 215,
-1000,-235,-235,-235, 215,-226,-242,-278,-1000, -32,
-1000,-264,-1000,-1000,-1000,-1000,-1000,-1000, 270,-1000,
 -32 };
short yypgo[]={

   0, 670,  46, 742,  45,  36,  33 };
short yyr1[]={

   0,   5,   4,   6,   4,   4,   4,   4,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   3,   3,
   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
   2,   2 };
short yyr2[]={

   0,   0,   4,   0,   4,   5,   5,   3,   1,   4,
   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
   3,   3,   3,   4,   4,   3,   3,   4,   1,   1,
   1,   2,   2,   3,   3,   3,   3,   3,   4,   3,
   1,   3 };
short yychk[]={

-1000,  -4, 257, 261, 258, 259, 260,  -5,  -6, 278,
 267,  -3, 267, 263, 262, 276, 275, 280,  -1, 267,
 303, 263, 302, 262, 273, 276, 268, 269, 300, 275,
 294, 280, 297,  -1, 265, 265, 271, 275, 264, 273,
 266, 274, 279,  -3,  -3,  -3, 271, 305, 278, 281,
 265, 291, 292, 290, 276, 293, 287, 301, 289, 288,
 296, 295, 298, 299, 275, 277, 264, 273, 266, 274,
 279,  -1, 280,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1, 280, 271,  -3,  -3,  -3,  -3,  -3,  -3,  -3,
  -3, 284,  -1, 263,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
 263,  -1,  -1,  -1,  -1,  -1, 304,  -2, 284,  -1,
 284,  -1, 271, 271, 283, 283, 263, 284, 281, 284,
  -1 };
short yydef[]={

   0,  -2,   1,   3,   0,   0,   0,   0,   0,   0,
   0,   0,  48,  49,  50,   0,   0,   0,   0,   8,
   0,  10,  11,  12,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   7,   0,   0,   0,
   0,   0,   0,  51,  52,   0,   2,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  13,  14,  15,  16,  17,  18,  19,
   0,   0,   4,   0,   0,  53,  54,  55,  56,  57,
   0,  59,  20,  21,  22,  23,  24,  25,  26,  27,
  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,
  38,  39,  40,  41,  42,   0,   0,   0,  45,  60,
  46,   0,   5,   6,  58,  43,   9,  44,   0,  47,
  61 };
#ifndef lint
static	char yaccpar_sccsid[] = "@(#)yaccpar 1.2 86/07/18 SMI"; /* from UCB 4.1 83/02/11 */
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
{ DotDot=0; } break;
case 2:
{ yyres = (long) yypvt[-1].ee;  } break;
case 3:
{ DotDot=1; } break;
case 4:
{ yyres = (long) yypvt[-1].ee;  } break;
case 5:
{ yyres = (long) yypvt[-1].ee; } break;
case 6:
{ yyres = (long) yypvt[-1].ee; } break;
case 7:
{ yyres = (long) yypvt[-1].ee; } break;
case 8:
{ if( !CurrentExpr){
					yyerror("$ cannot be used here");
					YYACCEPT;
				  }
				  yyval.ee = CurrentExpr;
				} break;
case 9:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_ENV, E_Id(yypvt[-0].ss) ); } break;
case 10:
{ yyval.ee = E_Id( yypvt[-0].ss ); } break;
case 11:
{ yyval.ee = E_DConst( yypvt[-0].dd ); } break;
case 12:
{ yyval.ee = E_IConst( yypvt[-0].ll ); } break;
case 13:
{ yyval.ee = E_Unary( O_DEREF, yypvt[-0].ee ); } break;
case 14:
{ yyval.ee = E_Unary( O_REF, yypvt[-0].ee ); } break;
case 15:
{ yyval.ee = E_Unary( O_SIZEOF, yypvt[-0].ee ); } break;
case 16:
{ yyval.ee = E_Unary( O_TYPEOF, yypvt[-0].ee ); } break;
case 17:
{ yyval.ee = E_Unary( O_LOGNOT, yypvt[-0].ee ); } break;
case 18:
{ yyval.ee = E_Unary( O_MINUS, yypvt[-0].ee ); } break;
case 19:
{ yyval.ee = E_Unary( O_1SCOMP, yypvt[-0].ee ); } break;
case 20:
{ if( !DotDot ){
					yyerror(".. cannot be used here");
					YYACCEPT;
				  }
				  yyval.ee = E_Binary( yypvt[-2].ee, O_RANGE, yypvt[-0].ee );} break;
case 21:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_DOT, E_Id(yypvt[-0].ss)); } break;
case 22:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_COMMA, yypvt[-0].ee ); } break;
case 23:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_ASSIGN, yypvt[-0].ee ); } break;
case 24:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_LOGOR, yypvt[-0].ee ); } break;
case 25:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_LOGAND, yypvt[-0].ee ); } break;
case 26:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_BITOR, yypvt[-0].ee ); } break;
case 27:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_BITAND, yypvt[-0].ee ); } break;
case 28:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_BITXOR, yypvt[-0].ee ); } break;
case 29:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_EQ, yypvt[-0].ee ); } break;
case 30:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_NE, yypvt[-0].ee ); } break;
case 31:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_LT, yypvt[-0].ee ); } break;
case 32:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_GT, yypvt[-0].ee ); } break;
case 33:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_LE, yypvt[-0].ee ); } break;
case 34:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_GE, yypvt[-0].ee ); } break;
case 35:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_RSHIFT, yypvt[-0].ee ); } break;
case 36:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_LSHIFT, yypvt[-0].ee ); } break;
case 37:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_MINUS, yypvt[-0].ee ); } break;
case 38:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_ARROW, E_Id(yypvt[-0].ss)); } break;
case 39:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_MOD, yypvt[-0].ee ); } break;
case 40:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_MULT, yypvt[-0].ee ); } break;
case 41:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_DIV, yypvt[-0].ee ); } break;
case 42:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_PLUS, yypvt[-0].ee ); } break;
case 43:
{ yyval.ee = E_Binary( yypvt[-3].ee, O_INDEX, yypvt[-1].ee );} break;
case 44:
{ yyval.ee = E_Binary( E_Id(yypvt[-3].ss), O_CALL, yypvt[-1].ee ); } break;
case 45:
{ yyval.ee = E_Binary( E_Id(yypvt[-2].ss), O_CALL, 0 ); } break;
case 46:
{ yyval.ee = yypvt[-1].ee; } break;
case 47:
{ yyval.ee = E_Unary( O_FABS, yypvt[-1].ee ); } break;
case 48:
{ if( !CurrentExpr){
					yyerror("no current expression for $");
					YYACCEPT;
				  }
				  yyval.ee = CurrentExpr;
				} break;
case 49:
{ yyval.ee = E_Id( yypvt[-0].ss ); } break;
case 50:
{ yyval.ee = E_IConst( yypvt[-0].ll ); } break;
case 51:
{ yyval.ee = E_Unary( O_REF, yypvt[-0].ee ); } break;
case 52:
{ yyval.ee = E_Unary( O_MINUS, yypvt[-0].ee ); } break;
case 53:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_MINUS, yypvt[-0].ee ); } break;
case 54:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_MOD, yypvt[-0].ee ); } break;
case 55:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_MULT, yypvt[-0].ee ); } break;
case 56:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_DIV, yypvt[-0].ee ); } break;
case 57:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_PLUS, yypvt[-0].ee ); } break;
case 58:
{ yyval.ee = E_Binary( yypvt[-3].ee, O_INDEX, yypvt[-1].ee );} break;
case 59:
{ yyval.ee = yypvt[-1].ee; } break;
case 60:
{ yyval.ee = yypvt[-0].ee; } break;
case 61:
{ yyval.ee = E_Binary( yypvt[-2].ee, O_COMMA, yypvt[-0].ee ); } break;
		}
		goto yystack;  /* stack new state and value */

	}
