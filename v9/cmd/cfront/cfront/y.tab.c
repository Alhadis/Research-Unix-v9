
# line 25 "gram.y"
#include "cfront.h"
#include "size.h"

#define YYMAXDEPTH 300

static cdi = 0;
static Pnlist cd = 0, cd_vec[BLMAX];
static char stmt_seen = 0, stmt_vec[BLMAX];
static Plist tn_vec[BLMAX];

void sig_name(Pname);	// fcts put into norm2.c just to get them out of gram.y
Ptype tok_to_type(TOK);
void memptrdcl(Pname, Pname, Ptype, Pname);

#define lex_unget(x) back = x

#define Ndata(a,b)	b->normalize(Pbase(a),0,0)
#define Ncast(a,b)	b->normalize(Pbase(a),0,1)
#define Nfct(a,b,c)	b->normalize(Pbase(a),Pblock(c),0)
#define Ncopy(n)	(n->base==TNAME)?new name(n->string):n

#define Finit(p)	Pfct(p)->f_init
#define Fargdcl(p,q,r)	Pfct(p)->argdcl(q,r)
#define Freturns(p)	Pfct(p)->returns
#define Vtype(v)	Pvec(v)->typ
#define Ptyp(p)		Pptr(p)->typ

		/* avoid redefinitions */
#undef EOFTOK
#undef ASM
#undef BREAK
#undef CASE
#undef CONTINUE
#undef DEFAULT
#undef DELETE
#undef DO
#undef ELSE
#undef ENUM
#undef FOR
#undef FORTRAN
#undef GOTO
#undef IF
#undef NEW
#undef OPERATOR
#undef RETURN
#undef SIZEOF
#undef SWITCH
#undef THIS
#undef WHILE
#undef LP
#undef RP
#undef LB
#undef RB
#undef REF
#undef DOT	
#undef NOT	
#undef COMPL	
#undef MUL	
#undef AND	
#undef PLUS	
#undef MINUS	
#undef ER	
#undef OR	
#undef ANDAND
#undef OROR
#undef QUEST
#undef COLON
#undef ASSIGN
#undef CM
#undef SM	
#undef LC	
#undef RC
#undef ID
#undef STRING
#undef ICON
#undef FCON	
#undef CCON	
#undef ZERO
#undef ASOP
#undef RELOP
#undef EQUOP
#undef DIVOP
#undef SHIFTOP
#undef ICOP
#undef TYPE
#undef TNAME
#undef EMPTY
#undef NO_ID
#undef NO_EXPR
#undef ELLIPSIS
#undef AGGR
#undef MEM
#undef CAST
#undef ENDCAST
#undef MEMPTR
#undef PR

# line 123 "gram.y"
typedef union  {
	char*	s;
	TOK	t;
	int	i;
	loc	l;
	Pname	pn;
	Ptype	pt;
	Pexpr	pe;
	Pstmt	ps;
	Pbase	pb;
	Pnlist	nl;
	Pslist	sl;
	Pelist	el;
	PP	p;	// fudge: pointer to all class node objects
} YYSTYPE;

# line 139 "gram.y"
extern YYSTYPE yylval, yyval;
extern int yyparse();

Pname syn()
{
ll:
	switch (yyparse()) {
	case 0:		return 0;	// EOF
	case 1:		goto ll;	// no action needed
	default:	return yyval.pn;
	}
}

void look_for_hidden(Pname n, Pname nn)
{
	Pname nx = ktbl->look(n->string,HIDDEN);
	if (nx == 0) error("nonTN%n before ::%n",n,nn);
	nn->n_qualifier = nx;
}
# define EOFTOK 0
# define ASM 1
# define BREAK 3
# define CASE 4
# define CONTINUE 7
# define DEFAULT 8
# define DELETE 9
# define DO 10
# define ELSE 12
# define ENUM 13
# define FOR 16
# define FORTRAN 17
# define GOTO 19
# define IF 20
# define NEW 23
# define OPERATOR 24
# define RETURN 28
# define SIZEOF 30
# define SWITCH 33
# define THIS 34
# define WHILE 39
# define LP 40
# define RP 41
# define LB 42
# define RB 43
# define REF 44
# define DOT 45
# define NOT 46
# define COMPL 47
# define MUL 50
# define AND 52
# define PLUS 54
# define MINUS 55
# define ER 64
# define OR 65
# define ANDAND 66
# define OROR 67
# define QUEST 68
# define COLON 69
# define ASSIGN 70
# define CM 71
# define SM 72
# define LC 73
# define RC 74
# define CAST 113
# define ENDCAST 122
# define MEMPTR 173
# define ID 80
# define STRING 81
# define ICON 82
# define FCON 83
# define CCON 84
# define ZERO 86
# define ASOP 90
# define RELOP 91
# define EQUOP 92
# define DIVOP 93
# define SHIFTOP 94
# define ICOP 95
# define TYPE 97
# define TNAME 123
# define EMPTY 124
# define NO_ID 125
# define NO_EXPR 126
# define ELLIPSIS 155
# define AGGR 156
# define MEM 160
# define PR 175
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 1236 "gram.y"


short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 31,
	71, 21,
	72, 21,
	-2, 20,
-1, 45,
	155, 236,
	-2, 181,
-1, 52,
	155, 236,
	-2, 181,
-1, 229,
	1, 129,
	3, 129,
	4, 129,
	7, 129,
	8, 129,
	9, 129,
	10, 129,
	13, 129,
	16, 129,
	19, 129,
	20, 129,
	23, 129,
	24, 129,
	28, 129,
	30, 129,
	33, 129,
	34, 129,
	39, 129,
	40, 129,
	46, 129,
	47, 129,
	50, 129,
	52, 129,
	54, 129,
	55, 129,
	64, 129,
	65, 129,
	66, 129,
	67, 129,
	68, 129,
	70, 129,
	71, 129,
	72, 129,
	73, 129,
	113, 129,
	80, 129,
	81, 129,
	82, 129,
	83, 129,
	84, 129,
	86, 129,
	90, 129,
	91, 129,
	92, 129,
	93, 129,
	94, 129,
	95, 129,
	97, 129,
	123, 129,
	156, 129,
	160, 129,
	-2, 0,
-1, 256,
	72, 52,
	-2, 102,
-1, 257,
	72, 51,
	-2, 63,
	};
# define YYNPROD 244
# define YYLAST 1539
short yyact[]={

 133, 385,  46,  44, 140,  17, 170, 231, 144,  63,
  64, 108, 299, 350, 296, 240,  53,  51, 308, 143,
 210, 207,  20,  21, 122, 301, 262, 390, 355,  95,
 169,  10, 243,  36,  37,  24, 205,  25,  53,  51,
 206, 106,  24, 243,  25,  24, 223,  25, 149,  14,
 132, 159,  96,  24, 328,  25,  62, 150,  23,  99,
 346,  28,  53,  51,  53,  51,  57,  98, 298, 184,
  28,  28, 241,  39,  96, 138, 135, 146, 263,  92,
  91,  22, 401, 241, 146, 100, 394,  52, 208,  47,
 171, 369,  49,  48, 160,  97,  94,  23, 243, 259,
 306, 174, 326,  96, 152, 173, 180, 235,  24, 377,
  25, 177, 190, 259, 257, 319, 258,  97, 223, 197,
  22, 419,  16, 131, 222, 212, 213, 214, 215, 216,
 217, 218, 219, 138, 327,  28,  28, 237, 241, 227,
  28, 224, 225, 209, 211,  19,  97, 229,   7, 289,
 172, 221, 242, 153,  19,  34, 206, 256,  26, 248,
 247,  36,  37, 301, 228,  26, 160, 383,  26,  36,
  37, 139,  24,  24,  25,  25,  26, 178, 176, 130,
 139, 239, 266, 267, 268, 269, 270, 271, 272, 273,
 274, 275, 276, 277, 278, 279, 280, 281, 255, 282,
 309, 283, 323, 261,  29, 265, 284,  50, 264, 291,
 292, 232, 250,  29,  29, 297, 168, 300,  57,  34,
  34, 236, 290, 253,  19, 252,   8, 285, 288, 305,
 295,  26,  36,  37,  36,  37, 293, 311, 131, 226,
 209, 302, 165, 317,  23, 239, 239, 316, 242, 242,
 139, 233, 211, 304, 310,  28, 238, 315, 320, 321,
  18, 325, 260,  23, 286, 294, 324,  22,  54, 146,
  24, 155,  25, 103, 104, 354,   6, 392,  29,  29,
 254, 146, 314,  29, 411,  96,  22, 353,   5,  40,
 154, 249, 105,  47, 130,  26,  26,  48, 333, 340,
  38, 334, 343, 297, 337, 338, 300, 300, 363,  54,
 342, 137, 341, 344, 345, 347, 348, 372, 370,  56,
 166,  96, 396, 317, 317, 158, 376, 374,  97, 184,
 378, 136, 373, 182, 183, 371, 201, 287, 200,  19,
 202, 203, 379,  61, 340, 339, 336, 343, 343,  47,
 363, 184, 335,  48, 332, 182, 183,  11, 342, 387,
 313, 389,  55, 381,  97, 139, 245, 393,  31, 164,
 191, 189, 190, 188, 234,  58,  60, 318, 398, 397,
 175, 163, 157, 400, 399, 386, 363, 403, 363, 136,
 363, 199, 408,  26, 190, 188, 363, 388,  29, 402,
 184, 404, 384, 406, 182, 183, 363, 249, 363, 410,
 363, 380,  47, 363, 107, 421,  48, 363,  28, 414,
 423, 415, 418, 417, 312, 425, 420, 363, 111, 131,
 422,  28, 184, 204, 185, 120, 182, 183,  30, 129,
 427, 191,  23, 190, 188, 123, 187, 186, 192, 193,
 196, 117, 118, 412, 409, 113, 391, 114,  18, 116,
 115, 179, 184, 407, 405,  22, 182, 183,  24, 395,
  25,  43, 307, 191, 189, 190, 188,  47, 134, 375,
  47,  48, 141, 145,  48, 130, 127, 125, 126, 128,
  88, 124, 161, 142, 351,  45, 179,  47,  15, 349,
 119,  48, 147, 101, 352, 190, 364, 361,  32, 246,
 365, 362, 107, 368, 162,  19,  28,  93, 112, 358,
  27,  12, 367, 356,   1, 102, 111, 131, 148, 230,
   2, 366,  45, 120,  47,   0, 359, 129,  48,   0,
   0,  13, 357, 123,   0,  45, 322,  47,   0, 117,
 118,  48,   0, 113,  45, 114,  47, 116, 115,   0,
  48,  29,  41,   0,  42, 121,  45, 156,  47,   0,
   0,   0,  48,   0,  29,   0, 229, 382, 151,   0,
   0,   0,   0, 360, 127, 125, 126, 128,   0, 124,
   0,  26,   0, 167,   0,   0,   0,   0, 119,   0,
 147,   0, 352,   0, 364, 361,   0,   0, 365, 362,
 107, 368,   0,   0,  28,   0, 112, 358,   0,   0,
 367, 356,   0,   0, 111, 131, 148,   0,   0, 366,
   0, 120,   0,   0, 359, 129,   0,   0,   0,   0,
 357, 123,   0,   0,  23,  23,   0, 117, 118,   0,
   0, 113,   0, 114,   0, 116, 115,   0,   0,  29,
  18,  18,   0, 121,   0,   4,   9,  22,  22,   0,
  24,  24,  25,  25, 229,   0,   0,   0,  28,  28,
   0, 360, 127, 125, 126, 128,   0, 124,   0,  23,
  23,   0,   0,   0,   0,   0, 119,   0, 147,   0,
  15,  15,   0,   0,   0,  18,  18,   0,   0,   0,
   0,   0,  22,  22, 112,  24,  24,  25,  25,   0,
   0,   0,   0,   0, 148,   0,   0, 107,   0,   0,
   0,  28,   0,   0,   0,  40,   0,   3,  33,   0,
   0, 111, 131,  61,  59,  15,  38,   0, 120,   0,
   0,   0, 129,   0,   0,   0,   0,  29, 123,   0,
   0, 121,  19,  34, 117, 118,   0,   0, 113,   0,
 114,   0, 116, 115,   0,   0, 184,   0, 185,   0,
 182, 183,   0,   0,   0,   0,   0,   0,  13,  35,
 187, 134,   0,  26,  26,   0,   0,   0, 130, 127,
 125, 126, 128,   0, 124,   0,   0,   0,   0, 107,
   0,   0,   0, 119,   0, 147,   0, 191, 189, 190,
 188,  29,  29, 111, 131,   0,   0,   0,   0,   0,
 120, 112,   0,   0, 129,   0,   0,   0,  26,  26,
 123, 148,   0,   0,   0,   0, 117, 118,   0,   0,
 113,   0, 114,   0, 116, 115,   0,   0,   0,   0,
   0,   0,   0,   0, 184,   0, 185,   0, 182, 183,
   0,   0,   0, 134,  29,   0,   0,   0, 121, 107,
 130, 127, 125, 126, 128,   0, 124,   0,   0,   0,
   0,   0,   0, 111, 131, 119,   0, 109,   0,   0,
 120,   0,   0,   0, 129, 191, 189, 190, 188,   0,
 123,   0,   0, 112,   0,   0, 117, 118,   0,   0,
 113,   0, 114, 110, 116, 115,   0,   0,   0, 184,
   0, 185,   0, 182, 183,   0,   0,   0,   0,   0,
   0,   0,   0, 187, 186, 192, 193, 196,   0, 181,
 130, 127, 125, 126, 128,   0, 124, 111, 131,   0,
 121,   0,   0,   0, 120, 119,   0, 109, 129, 194,
 191, 189, 190, 188, 123,   0, 198,   0,   0,   0,
 117, 118,   0, 112, 113,   0, 114,   0, 116, 115,
   0,   0, 184, 110, 185,   0, 182, 183, 184,   0,
 185,   0, 182, 183,   0,   0, 187, 186, 192,   0,
   0,   0, 187, 186, 130, 127, 125, 126, 128,   0,
 124, 111, 131,   0,   0,   0,   0,   0, 120, 119,
 121, 109, 129, 191, 189, 190, 188,   0, 123, 191,
 189, 190, 188,   0, 117, 118,   0, 112, 113,   0,
 114,   0, 116, 115,   0,   0,   0, 110,   0,   0,
   0,   0,   0,   0,   0,   0, 111, 131,   0,   0,
   0,   0,   0, 120,   0,   0,   0, 129, 130, 127,
 125, 126, 128, 123, 124, 111, 131,   0,   0, 117,
 118,   0, 120, 119, 121, 109, 129,   0,   0,   0,
   0,   0, 123,   0,   0,   0,   0,   0, 117, 118,
   0, 112, 113,   0, 114,   0, 116, 115,   0,   0,
   0, 110,   0, 130, 127, 125, 126, 128,   0, 124,
   0,   0,   0,   0,   0,   0,   0,   0, 119,   0,
 109,   0, 130, 127, 125, 126, 128,   0, 124,   0,
   0,  85,   0,   0,   0,   0, 112, 119, 121, 109,
   0,   0,   0,   0,   0,  84, 110,   0,   0,   0,
   0,   0,   0,   0,   0, 220,   0,   0,   0,   0,
   0,   0,  77,   0,  78, 110,  86,  87,  79,  80,
   0, 426,  67,   0,  68,   0,  65,  66,   0,   0,
 184,   0, 185, 121, 182, 183,  70,  69,  75,  76,
   0,   0,  83,   0, 187, 186, 192, 193, 196,   0,
 181, 195, 121,   0,   0,   0,   0,   0,   0,   0,
   0,   0,  82,  74,  72,  73,  71,  81,   0,  89,
 194, 191, 189, 190, 188,   0, 184,   0, 185,   0,
 182, 183,   0,   0,   0,   0,   0,   0,   0,   0,
 187, 186, 192, 193, 196,  90, 181, 195, 424,   0,
   0,   0,   0,   0,   0,   0,   0, 184,   0, 185,
   0, 182, 183,   0,   0,   0, 194, 191, 189, 190,
 188, 187, 186, 192, 193, 196, 416, 181, 195, 413,
   0,   0,   0,   0,   0,   0,   0,   0, 184,   0,
 185,   0, 182, 183,   0,   0,   0, 194, 191, 189,
 190, 188, 187, 186, 192, 193, 196,   0, 181, 195,
   0,   0, 331,   0,   0,   0,   0,   0,   0, 184,
   0, 185,   0, 182, 183,   0,   0,   0, 194, 191,
 189, 190, 188, 187, 186, 192, 193, 196,   0, 181,
 195,   0,   0, 330,   0,   0,   0,   0,   0,   0,
 184,   0, 185,   0, 182, 183,   0,   0,   0, 194,
 191, 189, 190, 188, 187, 186, 192, 193, 196,   0,
 181, 195,   0,   0,   0,   0,   0,   0,   0,   0,
   0, 184,   0, 185,   0, 182, 183,   0,   0,   0,
 194, 191, 189, 190, 188, 187, 186, 192, 193, 196,
 329, 181, 195, 303,   0,   0,   0,   0,   0,   0,
   0,   0, 184,   0, 185,   0, 182, 183,   0,   0,
   0, 194, 191, 189, 190, 188, 187, 186, 192, 193,
 196,   0, 181, 195,   0,   0, 244,   0,   0,   0,
   0,   0,   0, 184,   0, 185,   0, 182, 183,   0,
   0,   0, 194, 191, 189, 190, 188, 187, 186, 192,
 193, 196,   0, 181, 195,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 184,   0, 185,   0, 182, 183,
   0,   0,   0, 194, 191, 189, 190, 188, 187, 186,
 192, 193, 196,   0, 181, 195,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0, 194, 191, 189, 190, 188 };
short yypact[]={

 665,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000, 398,
 666, 492,-1000,  47,-1000, 264, 239, 621, 620,-1000,
-1000,-1000, -67,1142, -17, -18,-1000,-1000,  23,  -6,
   4, 455, 202,-1000,-1000,  47,-1000,-1000, 223,-1000,
 870, 800,-1000, 242,-1000, 718,-1000, 870,-1000,-1000,
-1000,-1000, 405,-1000,  73, 245, 226,-1000, 514,  47,
 526,  47,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000, 341, 282,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,  -5,-1000,
-1000,-1000,-1000, 418,  10,  77,-1000,-1000,-1000,  32,
 339, 242, 800, 220,-1000, 870, 879, 934, 296, 393,
  -4,  48,  57, 998, 998, 998, 998, 998, 998, 998,
1062, -28,-1000, 870,-1000,-1000,-1000,-1000,-1000,-1000,
-114,1142, 167, 879, 800,  74,-1000, 171, 666,-1000,
 333,  66, 185,-1000,-1000,-1000,  58, 393,  -4,1413,
 325, 620,-1000,-1000,-1000,-1000, 251,-1000,-1000,-1000,
  -5,-1000,-1000,-1000, 153, 151,-1000, 211,  34,  42,
-1000, 192,  10,-1000, -97, 136,  74,-1000,-1000, 455,
 879, 870, 870, 870, 870, 870, 870, 870, 870, 870,
 870, 870, 870, 870, 870, 870, 870, 296, 870,-1000,
 870, 800, 214,  99, 800, 800, 241,-1000,  57, 122,
 -54, 123, 296, 296, 296, 296, 296, 296, 296, 296,
  57,-1000,1382, 205,-1000,-1000,-1000,  26,-1000, -56,
 183,-1000, 800, 384,-1000,-1000, 319, 127, 800,-1000,
 307,-1000,  -8,   3,-1000,-1000, 505,-1000,-1000,  57,
-1000,-1000,-1000,-1000,-1000, 130, 245, 226,-1000,  10,
 870,  28,  61, -69,-1000,-1000, 879,  19,  19,-1000,
 279, 726, 814, 412, 350,-1000, 301, 948, 942, 879,
 879,1351,1320,1289, 313,-1000, 998,-120,-1000, 998,
-120, 311, 305,-1000,1142, 304, 435,  -5, 998, 435,
 -15, -15, -62,-1000,-1000,1142,-1000, 601,-1000,  17,
 171, 294, 800,-1000, 291,-1000,-1000,-1000, 800,-1000,
 435, 438, 367,-1000,-1000, 879,-1000,-1000,  36, 870,
 998,-1000,-1000, 296, 296,-1000,-1000,-1000,-1000,-1000,
-1000, 435, 296,-1000, 435, 370,1043,-1000,-1000, 503,
-1000,  95, 362,-1000,-1000,-1000, 345, 345, 357, 345,
 -42,-1000, 208,1444,-1000,-1000, 870,   6,-1000,-1000,
-1000,-1000, 281,-1000,-1000, 251,-1000,-1000, 382, 296,
 251,-1000,-1000,-1000,   1, 601, 870, 601,-1000, 601,
-1000, 870,-1000,1444,-1000, 601,-1000,-1000,-1000,-1000,
-1000, 243, 441,1258,-1000, 601,-1000, 601,1227, 601,
 383,  49, 601,-1000, 870,-1000, 601,-1000, 345,-1000,
-1000,1196,-1000,-1000, 870,1150, 601,-1000 };
short yypgo[]={

   0, 530, 226, 148, 275, 471,  76, 529,   7, 287,
 524,   2,   5,  31, 521,  23,  30,   6,  29, 520,
  22, 517, 514,  73, 508,  49, 357,   8, 499,  28,
  13, 494, 493,   4,   0,  11,  24,  12,  20,  51,
  10, 490,  15,  19, 483,   3, 482,  14,  21,   1,
 122,   9, 472, 469, 464, 463, 456, 454 };
short yyr1[]={

   0,  10,  10,  10,   1,   1,   1,   1,   1,   2,
   2,   4,   3,   6,   6,   7,   7,   8,   8,   5,
   5,  23,  23,  23,  23,  24,  24,   9,   9,  14,
  14,  14,  14,  13,  13,  13,  13,  13,  15,  15,
  16,  16,  17,  17,  17,  20,  20,  19,  19,  19,
  19,  18,  18,  21,  21,  22,  22,  22,  22,  22,
  22,  22,  22,  25,  25,  25,  25,  51,  51,  51,
  51,  51,  51,  51,  51,  51,  51,  51,  51,  51,
  51,  51,  51,  51,  51,  51,  51,  51,  51,  51,
  50,  50,  50,  50,  26,  26,  26,  26,  26,  26,
  26,  26,  26,  26,  26,  26,  26,  26,  26,  42,
  42,  42,  42,  42,  42,  42,  47,  47,  47,  37,
  37,  37,  37,  37,  39,  39,  28,  28,  49,  52,
  29,  29,  29,  31,  31,  31,  31,  31,  53,  31,
  30,  30,  30,  30,  30,  30,  30,  30,  54,  30,
  30,  55,  30,  56,  30,  57,  30,  33,  32,  32,
  27,  27,  34,  34,  34,  34,  34,  34,  34,  34,
  34,  34,  34,  34,  34,  34,  34,  34,  34,  34,
  34,  34,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  35,  35,  35,  35,  35,  35,
  35,  35,  35,  35,  36,  36,  36,  36,  36,  36,
  36,  36,  36,  38,  41,  41,  40,  48,  44,  44,
  45,  45,  45,  46,  46,  43,  43,  12,  12,  12,
  12,  12,  11,  11 };
short yyr2[]={

   0,   1,   1,   1,   1,   1,   1,   1,   5,   4,
   2,   5,   4,   2,   0,   1,   3,   3,   4,   2,
   0,   1,   3,   2,   3,   1,   3,   3,   2,   1,
   1,   1,   1,   1,   2,   2,   2,   2,   4,   5,
   1,   3,   1,   3,   0,   3,   4,   2,   3,   5,
   6,   1,   1,   2,   0,   1,   2,   1,   2,   1,
   1,   2,   3,   1,   2,   2,   2,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,
   2,   1,   1,   1,   1,   1,   1,   1,   1,   1,
   2,   2,   3,   3,   2,   2,   4,   4,   6,   1,
   3,   2,   2,   2,   2,   2,   2,   4,   4,   1,
   2,   0,   2,   2,   4,   4,   0,   2,   2,   0,
   2,   2,   4,   4,   0,   2,   2,   1,   3,   0,
   4,   2,   3,   1,   1,   1,   2,   2,   0,   5,
   2,   5,   1,   1,   1,   3,   5,   3,   0,   9,
   3,   0,   4,   0,   5,   0,   4,   1,   1,   3,
   1,   3,   3,   3,   3,   3,   3,   3,   3,   3,
   3,   3,   3,   3,   3,   3,   3,   5,   2,   5,
   1,   0,   4,   4,   2,   4,   2,   4,   2,   2,
   2,   2,   2,   2,   2,   2,   4,   4,   4,   3,
   4,   3,   3,   4,   3,   2,   1,   3,   1,   1,
   1,   1,   1,   1,   1,   3,   3,   2,   4,   4,
   2,   4,   4,   2,   1,   1,   2,   2,   2,   4,
   3,   4,   5,   3,   1,   1,   0,   1,   1,   2,
   2,   1,   3,   1 };
short yychk[]={

-1000, -10,  -1,  72,   0,  -9,  -4,  -3,  -2,   1,
 -13, -26, -14, 123, -25,  80, -50, -12,  40,  97,
 -20, -15,  47,  24,  50,  52, 173, -19,  13, 156,
  40, -26, -24,  72,  97, 123, -20, -15,  80, -23,
  69,  70,  72,  -5, -45,  40, -11,  42,  46,  45,
 160, -45,  40, -11,  45, 123,  80, -25, -26, 123,
 -26, 123, 123, -51, -40,  54,  55,  50,  52,  65,
  64,  94,  92,  93,  91,  66,  67,  40,  42,  46,
  47,  95,  90,  70,  23,   9,  44,  45, -41,  97,
 123,  97,  97, -21,  73, -18,  80, 123,  73, -18,
  81,  -5,  70,  71,  72,  69, -34,   9, -35,  97,
 123,  23, 113,  50,  52,  55,  54,  46,  47,  95,
  30, 160, -36,  40,  86,  82,  83,  81,  84,  34,
  80,  24, -27, -34,  73,  -6,  -9,  69, -13, 123,
 -33, -46, -32, -43, -27, -44, -13,  97, 123, -34,
 -33, 173, -25,  80,  45,  45,  41,  41,  43, -39,
 -12,  74, -22,  -9,  -4,  -3,  -2, 175, -50, -16,
 -17,  80,  73,  73,  69,  41,  -6, -27, -23, -26,
 -34,  70,  54,  55,  50,  52,  65,  64,  94,  92,
  93,  91,  66,  67,  90,  71,  68, -35,  42,  95,
  42,  40,  44,  45,  40,  40, 160, -48,  40, -13,
 -38, -13, -35, -35, -35, -35, -35, -35, -35, -35,
 113, -18, -34, 160, -51, -40,  72, -33, -29,  73,
  -7,  -8,  40,  80,  41,  41, 155,  71,  71, 123,
 -42,  80, -12,  40,  43,  41, -26, -45, -11,  40,
 -39,  97,  72,  72,  69, -18, 123,  80,  74,  71,
  70, -16, 123, 175,  72, -29, -34, -34, -34, -34,
 -34, -34, -34, -34, -34, -34, -34, -34, -34, -34,
 -34, -34, -34, -34, -33, -36,  50, 123, -36,  50,
 123, -33, -33, -18,  24, -48, -47, -12, 122, -37,
 -12,  40, -38,  41, -18,  24,  74, -52,  74, 256,
  71, -33,  40,  41, 155, -43, -27, -11,  70, 123,
 -42, -42,  41,  72, -17, -34,  74,  73, 123,  69,
  43,  43,  41, -35, -35,  41,  41, -51, -40,  41,
 -11, -47, -35, -11, -37, -37, 122, -51, -40, -28,
 -30, -31,   1,  -9,  -4, -29,  20,  39,  16,  33,
  80,   4,   8, -34,   3,   7,  28,  19,  10,  74,
  -8,  41, -33,  41, -27,  41, -45,  73, -34, -35,
  41, -30,  74,  72,  40, -49,  40, -49,  40, -49,
  69, -56,  69, -34,  80, -53,  41, -45, -11, -45,
 -11,  81, -30, -34, -30, -54, -30, -55, -34, -57,
 -30,  41,  12,  41, -30, -30,  69, -30,  39,  72,
 -30, -34, -30, -49,  72, -34,  41, -30 };
short yydef[]={

   0,  -2,   1,   2,   3,   4,   5,   6,   7,   0,
   0,  20,  33,  30,  99,  63,   0,   0,   0,  29,
  31,  32,   0,   0, 237, 238, 241,  54,   0,   0,
   0,  -2,   0,  28,  34,  35,  36,  37,  63,  25,
 181, 181,  10,  14,  94,  -2, 106, 181, 243,  90,
  91,  95,  -2, 105,   0, 102,  63, 101, 103, 104,
   0,   0,  64,  65,  66,  67,  68,  69,  70,  71,
  72,  73,  74,  75,  76,  77,  78,   0,   0,  81,
  82,  83,  84,  85,  86,  87,  88,  89, 124, 224,
 225, 239, 240,   0,  44,   0,  51,  52,  47,   0,
   0,  14, 181,   0,  27, 181,  23,   0, 180,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0, 206, 181, 208, 209, 210, 211, 212, 213,
 214,   0,   0, 160, 181,   0,  19,   0,   0,  30,
   0,   0, 157, 234, 158, 235, 111,  29,  30,   0,
   0,   0, 100,  63,  92,  93,   0,  79,  80, 226,
 124,  45,  53,  55,  57,  59,  60,   0,   0,   0,
  40,  42,  44,  48,   0,   0,   0,  24,  26,  21,
  22, 181, 181, 181, 181, 181, 181, 181, 181, 181,
 181, 181, 181, 181, 181, 181, 181, 178, 181, 186,
 181, 181,   0,   0, 181, 181,   0, 184,   0, 116,
   0, 119, 188, 189, 190, 191, 192, 193, 194, 195,
   0, 205,   0,   0, 217, 220,   9,   0,  12,  -2,
  13,  15, 181,   0,  96, 230,   0, 236, 181,  35,
 228, 109, 111, 111, 242,  97,   0, 107, 108, 236,
 125,  46,  56,  58,  61,   0,  -2,  -2,  38,  44,
 181,   0,   0,   0,   8,  11, 162, 163, 164, 165,
 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
 176,   0,   0,   0,   0, 199,   0, 201, 202,   0,
 204,   0,   0, 215,   0,   0, 227, 116,   0, 223,
 119, 119,   0, 207, 216,   0, 161, 181, 131,   0,
   0,   0, 181, 231,   0, 233, 159, 113, 181, 110,
 112,   0,   0,  62,  41,  43,  39,  49,   0, 181,
   0, 197, 198, 200, 203, 182, 183, 218, 221, 185,
 118, 117, 187, 121, 120,   0, 196, 219, 222, 181,
 127,   0,   0, 142, 143, 144,   0,   0,   0,   0,
 214, 153,   0, 133, 134, 135, 181,   0, 138, 132,
  16,  17,   0, 232, 229,   0,  98,  50, 177, 179,
   0, 126, 130, 140,   0, 181, 181, 181, 148, 181,
 151, 181, 155, 136, 137, 181,  18, 114, 115, 122,
 123,   0, 145,   0, 147, 181, 150, 181,   0, 181,
   0,   0, 181, 128, 181, 152, 181, 156,   0, 141,
 146,   0, 154, 139, 181,   0, 181, 149 };
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
		
case 1:
# line 308 "gram.y"
{	return 2; } break;
case 2:
# line 309 "gram.y"
{	return 1; } break;
case 3:
# line 310 "gram.y"
{	return 0; } break;
case 4:
# line 314 "gram.y"
{	modified_tn = 0; if (yypvt[-0].pn==0) yyval.i = 1; } break;
case 5:
# line 316 "gram.y"
{	goto mod; } break;
case 6:
# line 318 "gram.y"
{	goto mod; } break;
case 7:
# line 320 "gram.y"
{ mod:	if (modified_tn) {
					restore();
					modified_tn = 0;
				}
			} break;
case 8:
# line 326 "gram.y"
{	Pname n = new name(make_name('A'));
				n->tp = new basetype(ASM,0);
				Pbase(n->tp)->b_name = Pname(yypvt[-2].s);
				yyval.p = n;
			} break;
case 9:
# line 334 "gram.y"
{	error('s',"T ofIdE too complicated (useTdef or leave out theIr)");
				goto fix;
			} break;
case 10:
# line 338 "gram.y"
{	Pname n;
				Ptype t;
			fix:
				if ((n=yypvt[-1].pn) == 0) {
					error("syntax error:TX");
					yyval.p = Ndata(defa_type,n);
				}
				else if ((t=n->tp) == 0) {
					error("TX for%n",n);
					yyval.p = Ndata(defa_type,n);
				}
				else if (t->base==FCT) {
					if (Pfct(t)->returns==0)
						yyval.p = Nfct(defa_type,n,0);
					else
						yyval.p = Ndata(0,n);
				}
				else {
					error("syntax error:TX for%k%n",t->base,n);
					yyval.p = Ndata(defa_type,n);
				}
			} break;
case 11:
# line 364 "gram.y"
{	Pname n = Nfct(yypvt[-4].p,yypvt[-3].pn,yypvt[-0].p);
				Fargdcl(n->tp,name_unlist(yypvt[-2].nl),n);
				Finit(n->tp) = yypvt[-1].pn;
				yyval.p = n;
			} break;
case 12:
# line 372 "gram.y"
{	Pname n = Nfct(defa_type,yypvt[-3].pn,yypvt[-0].p);
				Fargdcl(n->tp,name_unlist(yypvt[-2].nl),n);
				Finit(n->tp) = yypvt[-1].pn;
				yyval.p = n;
			} break;
case 13:
# line 380 "gram.y"
{	yyval.p = yypvt[-0].p; } break;
case 14:
# line 382 "gram.y"
{	yyval.p = 0; } break;
case 16:
# line 387 "gram.y"
{	yyval.pn = yypvt[-0].pn;  yyval.pn->n_list = yypvt[-2].pn; } break;
case 17:
# line 391 "gram.y"
{	yyval.pn = new name;
				yyval.pn->n_initializer = yypvt[-1].pe;
			} break;
case 18:
# line 395 "gram.y"
{	yyval.pn = yypvt[-3].pn;
				yyval.pn->n_initializer = yypvt[-1].pe;
			} break;
case 19:
# line 406 "gram.y"
{	if (yypvt[-0].pn == 0)
					error("badAD");
				else if (yypvt[-0].pn->tp->base == FCT)
					error("FD inAL (%n)",yypvt[-0].pn);
				else if (yypvt[-1].p)
					yypvt[-1].nl->add(yypvt[-0].pn);
				else
					yyval.nl = new nlist(yypvt[-0].pn);
			} break;
case 20:
# line 416 "gram.y"
{	yyval.p = 0; } break;
case 22:
# line 421 "gram.y"
{	yyval.p = yypvt[-2].pn;
				yyval.pn->tp = new basetype(FIELD,yypvt[-0].pn);
		 	} break;
case 23:
# line 425 "gram.y"
{	yyval.p = new name;
				yyval.pn->tp = new basetype(FIELD,yypvt[-0].pn);
			} break;
case 24:
# line 429 "gram.y"
{	Pexpr e = yypvt[-0].pe; 
				if (e == dummy) error("emptyIr");
				yypvt[-2].pn->n_initializer = e; 
			} break;
case 25:
# line 436 "gram.y"
{	if (yypvt[-0].p) yyval.nl = new nlist(yypvt[-0].pn); } break;
case 26:
# line 438 "gram.y"
{	if (yypvt[-2].p)
					if (yypvt[-0].p)
						yypvt[-2].nl->add(yypvt[-0].pn);
					else
						error("DL syntax");
				else {
					if (yypvt[-0].p) yyval.nl = new nlist(yypvt[-0].pn);
					error("DL syntax");
				}
			} break;
case 27:
# line 450 "gram.y"
{ yyval.p = Ndata(yypvt[-2].p,name_unlist(yypvt[-1].nl)); } break;
case 28:
# line 451 "gram.y"
{ yyval.p = yypvt[-1].pb->aggr(); } break;
case 29:
# line 455 "gram.y"
{ yyval.p = new basetype(yypvt[-0].t,0); } break;
case 30:
# line 456 "gram.y"
{ yyval.p = new basetype(TYPE,yypvt[-0].pn); } break;
case 34:
# line 462 "gram.y"
{ yyval.p = yypvt[-1].pb->type_adj(yypvt[-0].t); } break;
case 35:
# line 463 "gram.y"
{ yyval.p = yypvt[-1].pb->name_adj(yypvt[-0].pn); } break;
case 36:
# line 464 "gram.y"
{ yyval.p = yypvt[-1].pb->base_adj(yypvt[-0].pb); } break;
case 37:
# line 465 "gram.y"
{ yyval.p = yypvt[-1].pb->base_adj(yypvt[-0].pb); } break;
case 38:
# line 474 "gram.y"
{ yyval.p = end_enum(0,yypvt[-1].pn); } break;
case 39:
# line 475 "gram.y"
{ yyval.p = end_enum(yypvt[-3].pn,yypvt[-1].pn); } break;
case 40:
# line 479 "gram.y"
{	if (yypvt[-0].p) yyval.nl = new nlist(yypvt[-0].pn); } break;
case 41:
# line 481 "gram.y"
{	if( yypvt[-0].p)
					if (yypvt[-2].p)
						yypvt[-2].nl->add(yypvt[-0].pn);
					else
						yyval.nl = new nlist(yypvt[-0].pn);
			} break;
case 42:
# line 490 "gram.y"
{	yyval.p = yypvt[-0].pn; yyval.pn->tp = moe_type; } break;
case 43:
# line 492 "gram.y"
{	yyval.p = yypvt[-2].pn;
				yyval.pn->tp = moe_type;
				yyval.pn->n_initializer = yypvt[-0].pe;
			} break;
case 44:
# line 497 "gram.y"
{	yyval.p = 0; } break;
case 45:
# line 502 "gram.y"
{	
				ccl->mem_list = name_unlist(yypvt[-1].nl);
				end_cl();
			} break;
case 46:
# line 507 "gram.y"
{	
				ccl->mem_list = name_unlist(yypvt[-2].nl);
				end_cl();
				error("`;' or declaratorX afterCD");
				lex_unget(yypvt[-0].t);
				/* lex_unget($4); but only one unget, sorry */
			} break;
case 47:
# line 517 "gram.y"
{	yyval.p = start_cl(yypvt[-1].t,0,0); } break;
case 48:
# line 519 "gram.y"
{	yyval.p = start_cl(yypvt[-2].t,yypvt[-1].pn,0); } break;
case 49:
# line 521 "gram.y"
{	yyval.p = start_cl(yypvt[-4].t,yypvt[-3].pn,yypvt[-1].pn);
				if (yypvt[-4].t == STRUCT) ccl->pubbase = 1;
			} break;
case 50:
# line 525 "gram.y"
{	
				yyval.p = start_cl(yypvt[-5].t,yypvt[-4].pn,yypvt[-1].pn);
				ccl->pubbase = 1;
			} break;
case 51:
# line 550 "gram.y"
{ yyval.p = yypvt[-0].pn; } break;
case 53:
# line 555 "gram.y"
{	if (yypvt[-0].p) {
					if (yypvt[-1].p)
						yypvt[-1].nl->add_list(yypvt[-0].pn);
					else
						yyval.nl = new nlist(yypvt[-0].pn);
				}
			} break;
case 54:
# line 563 "gram.y"
{	yyval.p = 0; } break;
case 61:
# line 572 "gram.y"
{	yyval.p = new name; yyval.pn->base = yypvt[-1].t; } break;
case 62:
# line 574 "gram.y"
{	Pname n = Ncopy(yypvt[-1].pn);
				n->n_qualifier = yypvt[-2].pn;
				n->base = PUBLIC;
				yyval.p = n;
			} break;
case 63:
# line 597 "gram.y"
{	yyval.p = yypvt[-0].pn; } break;
case 64:
# line 599 "gram.y"
{	yyval.p = Ncopy(yypvt[-0].pn);
				yyval.pn->n_oper = DTOR;
			} break;
case 65:
# line 603 "gram.y"
{	yyval.p = new name(oper_name(yypvt[-0].t));
				yyval.pn->n_oper = yypvt[-0].t;
			} break;
case 66:
# line 607 "gram.y"
{	Pname n = yypvt[-0].pn;
				n->string = "_type";
				n->n_oper = TYPE;
				n->n_initializer = (Pexpr)n->tp;
				n->tp = 0;
				yyval.p = n;
			} break;
case 79:
# line 628 "gram.y"
{	yyval.t = CALL; } break;
case 80:
# line 629 "gram.y"
{	yyval.t = DEREF; } break;
case 86:
# line 635 "gram.y"
{	yyval.t = NEW; } break;
case 87:
# line 636 "gram.y"
{	yyval.t = DELETE; } break;
case 88:
# line 637 "gram.y"
{	yyval.t = REF; } break;
case 89:
# line 638 "gram.y"
{	yyval.t = DOT; } break;
case 90:
# line 642 "gram.y"
{ 
	             if ( yypvt[-1].pn->tp->base != COBJ )
			error( "T of%n not aC", yypvt[-1].pn );	
		   } break;
case 91:
# line 647 "gram.y"
{ 
	             if ( yypvt[-1].pn->tp->base != COBJ )
			error( "T of%n not aC", yypvt[-1].pn );	
		   } break;
case 92:
# line 651 "gram.y"
{ error("CNs do not nest"); } break;
case 93:
# line 652 "gram.y"
{ error("CNs do not nest"); } break;
case 94:
# line 657 "gram.y"
{	Freturns(yypvt[-0].p) = yypvt[-1].pn->tp;
				yypvt[-1].pn->tp = yypvt[-0].pt;
			} break;
case 95:
# line 661 "gram.y"
{	Pname n = yypvt[-1].pn;
				yyval.p = Ncopy(n);
				if (ccl && strcmp(n->string,ccl->string)) n->hide();
				yyval.pn->n_oper = TNAME;
				Freturns(yypvt[-0].p) = yyval.pn->tp;
				yyval.pn->tp = yypvt[-0].pt;
			} break;
case 96:
# line 673 "gram.y"
{	TOK k = 1;
				Pname l = yypvt[-1].pn;
				if (fct_void && l==0) k = 0;
				yypvt[-3].pn->tp = new fct(yypvt[-3].pn->tp,l,k);
			} break;
case 97:
# line 679 "gram.y"
{	TOK k = 1;
				Pname l = yypvt[-1].pn;
				if (fct_void && l==0) k = 0;
				yyval.p = Ncopy(yypvt[-3].pn);
				yyval.pn->n_oper = TNAME;
				yyval.pn->tp = new fct(0,l,k);
			} break;
case 98:
# line 687 "gram.y"
{	memptrdcl(yypvt[-3].pn,yypvt[-5].pn,yypvt[-0].pt,yypvt[-2].pn);
				yyval.p = yypvt[-2].p;
			} break;
case 100:
# line 692 "gram.y"
{	yyval.p = Ncopy(yypvt[-0].pn);
				yyval.pn->n_qualifier = yypvt[-2].pn;
			} break;
case 101:
# line 696 "gram.y"
{	yyval.p = yypvt[-0].p;
				set_scope(yypvt[-1].pn);
				yyval.pn->n_qualifier = yypvt[-1].pn;
			} break;
case 102:
# line 701 "gram.y"
{	yyval.p = Ncopy(yypvt[-0].pn);
				set_scope(yypvt[-1].pn);
				yyval.pn->n_oper = TNAME;
				yyval.pn->n_qualifier = yypvt[-1].pn;
			} break;
case 103:
# line 707 "gram.y"
{	Ptyp(yypvt[-1].p) = yypvt[-0].pn->tp;
				yypvt[-0].pn->tp = yypvt[-1].pt;
				yyval.p = yypvt[-0].p;
			} break;
case 104:
# line 712 "gram.y"
{	yyval.p = Ncopy(yypvt[-0].pn);
				yyval.pn->n_oper = TNAME;
				yypvt[-0].pn->hide();
				yyval.pn->tp = yypvt[-1].pt;
			} break;
case 105:
# line 718 "gram.y"
{	yyval.p = Ncopy(yypvt[-1].pn);
				yyval.pn->n_oper = TNAME;
				yypvt[-1].pn->hide();
				yyval.pn->tp = yypvt[-0].pt;
			} break;
case 106:
# line 724 "gram.y"
{	Vtype(yypvt[-0].p) = yypvt[-1].pn->tp;
				yypvt[-1].pn->tp = yypvt[-0].pt;
			} break;
case 107:
# line 728 "gram.y"
{	Freturns(yypvt[-0].p) = yypvt[-2].pn->tp;
				yypvt[-2].pn->tp = yypvt[-0].pt;
				yyval.p = yypvt[-2].p;
			} break;
case 108:
# line 733 "gram.y"
{	Vtype(yypvt[-0].p) = yypvt[-2].pn->tp;
				yypvt[-2].pn->tp = yypvt[-0].pt;
				yyval.p = yypvt[-2].p;
			} break;
case 109:
# line 740 "gram.y"
{	yyval.p = yypvt[-0].pn; } break;
case 110:
# line 742 "gram.y"
{	yyval.p = Ncopy(yypvt[-0].pn);
				yyval.pn->n_oper = TNAME;
				yypvt[-0].pn->hide();
				yyval.pn->tp = yypvt[-1].pt;
			} break;
case 111:
# line 748 "gram.y"
{	yyval.p = new name; } break;
case 112:
# line 750 "gram.y"
{	Ptyp(yypvt[-1].p) = yypvt[-0].pn->tp;
				yypvt[-0].pn->tp = (Ptype)yypvt[-1].p;
				yyval.p = yypvt[-0].p;
			} break;
case 113:
# line 755 "gram.y"
{	Vtype(yypvt[-0].p) = yypvt[-1].pn->tp;
				yypvt[-1].pn->tp = (Ptype)yypvt[-0].p;
			} break;
case 114:
# line 759 "gram.y"
{	Freturns(yypvt[-0].p) = yypvt[-2].pn->tp;
				yypvt[-2].pn->tp = (Ptype)yypvt[-0].p;
				yyval.p = yypvt[-2].p;
			} break;
case 115:
# line 764 "gram.y"
{	Vtype(yypvt[-0].p) = yypvt[-2].pn->tp;
				yypvt[-2].pn->tp = (Ptype)yypvt[-0].p;
				yyval.p = yypvt[-2].p;
			} break;
case 116:
# line 771 "gram.y"
{	yyval.p = new name; } break;
case 117:
# line 773 "gram.y"
{	Ptyp(yypvt[-1].p) = yypvt[-0].pn->tp;
				yypvt[-0].pn->tp = (Ptype)yypvt[-1].p;
				yyval.p = yypvt[-0].p;
			} break;
case 118:
# line 778 "gram.y"
{	Vtype(yypvt[-0].p) = yypvt[-1].pn->tp;
				yypvt[-1].pn->tp = (Ptype)yypvt[-0].p;
			} break;
case 119:
# line 784 "gram.y"
{	yyval.p = new name; } break;
case 120:
# line 786 "gram.y"
{	Ptyp(yypvt[-1].p) = yypvt[-0].pn->tp;
				yypvt[-0].pn->tp = (Ptype)yypvt[-1].p;
				yyval.p = yypvt[-0].p;
			} break;
case 121:
# line 791 "gram.y"
{	Vtype(yypvt[-0].p) = yypvt[-1].pn->tp;
				yypvt[-1].pn->tp = (Ptype)yypvt[-0].p;
			} break;
case 122:
# line 795 "gram.y"
{	Freturns(yypvt[-0].p) = yypvt[-2].pn->tp;
				yypvt[-2].pn->tp = yypvt[-0].pt;
				yyval.p = yypvt[-2].p;
			} break;
case 123:
# line 800 "gram.y"
{	Vtype(yypvt[-0].p) = yypvt[-2].pn->tp;
				yypvt[-2].pn->tp = yypvt[-0].pt;
				yyval.p = yypvt[-2].p;
			} break;
case 124:
# line 807 "gram.y"
{	yyval.p = new name; } break;
case 125:
# line 809 "gram.y"
{	Ptyp(yypvt[-1].p) = yypvt[-0].pn->tp;
				yypvt[-0].pn->tp = (Ptype)yypvt[-1].p;
				yyval.p = yypvt[-0].p;
			} break;
case 126:
# line 820 "gram.y"
{	if (yypvt[-0].p)
					if (yypvt[-1].p)
						yypvt[-1].sl->add(yypvt[-0].ps);
					else {
						yyval.sl = new slist(yypvt[-0].ps);
						stmt_seen = 1;
					}
			} break;
case 127:
# line 829 "gram.y"
{	if (yypvt[-0].p) {
					yyval.sl = new slist(yypvt[-0].ps);
					stmt_seen = 1;
				}
			} break;
case 128:
# line 837 "gram.y"
{	yyval.p = yypvt[-1].p;
				if (yyval.pe == dummy) error("empty condition");
				stmt_seen = 1;
			} break;
case 129:
# line 844 "gram.y"
{	cd_vec[cdi] = cd;
				stmt_vec[cdi] = stmt_seen;
				tn_vec[cdi++] = modified_tn;
				cd = 0;
				stmt_seen = 0;
				modified_tn = 0;
			} break;
case 130:
# line 852 "gram.y"
{	Pname n = name_unlist(cd);
				Pstmt ss = stmt_unlist(yypvt[-1].sl);
				yyval.p = new block(yypvt[-3].l,n,ss);
				if (modified_tn) restore();
				cd = cd_vec[--cdi];
				stmt_seen = stmt_vec[cdi];
				modified_tn = tn_vec[cdi];
				if (cdi < 0) error('i',"block level(%d)",cdi);
			} break;
case 131:
# line 862 "gram.y"
{	yyval.p = new block(yypvt[-1].l,0,0); } break;
case 132:
# line 864 "gram.y"
{	yyval.p = new block(yypvt[-2].l,0,0); } break;
case 133:
# line 868 "gram.y"
{	yyval.p = new estmt(SM,curloc,yypvt[-0].pe,0);	} break;
case 134:
# line 870 "gram.y"
{	yyval.p = new stmt(BREAK,yypvt[-0].l,0); } break;
case 135:
# line 872 "gram.y"
{	yyval.p = new stmt(CONTINUE,yypvt[-0].l,0); } break;
case 136:
# line 874 "gram.y"
{	yyval.p = new estmt(RETURN,yypvt[-1].l,yypvt[-0].pe,0); } break;
case 137:
# line 876 "gram.y"
{	yyval.p = new lstmt(GOTO,yypvt[-1].l,yypvt[-0].pn,0); } break;
case 138:
# line 877 "gram.y"
{ stmt_seen=1; } break;
case 139:
# line 878 "gram.y"
{	yyval.p = new estmt(DO,yypvt[-4].l,yypvt[-0].pe,yypvt[-2].ps); } break;
case 141:
# line 883 "gram.y"
{	
				if (stmt_seen)
					yyval.p = new estmt(ASM,curloc,(Pexpr)yypvt[-2].s,0);
				else {
					Pname n = new name(make_name('A'));
					n->tp = new basetype(ASM,(Pname)yypvt[-2].s);
					if (cd)
						cd->add_list(n);
					else
						cd = new nlist(n);
					yyval.p = 0;
				}
			} break;
case 142:
# line 897 "gram.y"
{	Pname n = yypvt[-0].pn;
				if (n)
					if (stmt_seen) {
						yyval.p = new block(n->where,n,0);
						yyval.ps->base = DCL;
					}
					else {
						if (cd)
							cd->add_list(n);
						else
							cd = new nlist(n);
						yyval.p = 0;
					}
			} break;
case 143:
# line 912 "gram.y"
{	Pname n = yypvt[-0].pn;
				lex_unget(RC);
				error(&n->where,"%n's definition is nested (did you forget a ``}''?)",n);
				if (cd)
					cd->add_list(n);
				else
					cd = new nlist(n);
				yyval.p = 0;
			} break;
case 145:
# line 923 "gram.y"
{	yyval.p = new ifstmt(yypvt[-2].l,yypvt[-1].pe,yypvt[-0].ps,0); } break;
case 146:
# line 925 "gram.y"
{	yyval.p = new ifstmt(yypvt[-4].l,yypvt[-3].pe,yypvt[-2].ps,yypvt[-0].ps); } break;
case 147:
# line 927 "gram.y"
{	yyval.p = new estmt(WHILE,yypvt[-2].l,yypvt[-1].pe,yypvt[-0].ps); } break;
case 148:
# line 928 "gram.y"
{ stmt_seen=1; } break;
case 149:
# line 929 "gram.y"
{	yyval.p = new forstmt(yypvt[-8].l,yypvt[-5].ps,yypvt[-4].pe,yypvt[-2].pe,yypvt[-0].ps); } break;
case 150:
# line 931 "gram.y"
{	yyval.p = new estmt(SWITCH,yypvt[-2].l,yypvt[-1].pe,yypvt[-0].ps); } break;
case 151:
# line 932 "gram.y"
{ yyval.p = yypvt[-1].pn; stmt_seen=1; } break;
case 152:
# line 933 "gram.y"
{	Pname n = yypvt[-1].pn;
				yyval.p = new lstmt(LABEL,n->where,n,yypvt[-0].ps);
			} break;
case 153:
# line 936 "gram.y"
{ stmt_seen=1; } break;
case 154:
# line 937 "gram.y"
{	if (yypvt[-2].pe == dummy) error("empty case label");
				yyval.p = new estmt(CASE,yypvt[-4].l,yypvt[-2].pe,yypvt[-0].ps);
			} break;
case 155:
# line 940 "gram.y"
{ stmt_seen=1; } break;
case 156:
# line 941 "gram.y"
{	yyval.p = new stmt(DEFAULT,yypvt[-3].l,yypvt[-0].ps); } break;
case 157:
# line 950 "gram.y"
{	Pexpr e = expr_unlist(yypvt[-0].el);
				while (e && e->e1==dummy) {
					register Pexpr ee2 = e->e2;
					if (ee2) error("EX inEL");
					delete e;
					e = ee2;
				}
				yyval.p = e;
			} break;
case 158:
# line 961 "gram.y"
{	yyval.el = new elist(new expr(ELIST,yypvt[-0].pe,0)); } break;
case 159:
# line 963 "gram.y"
{	yypvt[-2].el->add(new expr(ELIST,yypvt[-0].pe,0)); } break;
case 161:
# line 968 "gram.y"
{	Pexpr e;
				if (yypvt[-1].p)
					e = yypvt[-1].pe;
				else
					e = new expr(ELIST,dummy,0);
				yyval.p = new expr(ILIST,e,0);
			} break;
case 162:
# line 980 "gram.y"
{	binop:	yyval.p = new expr(yypvt[-1].t,yypvt[-2].pe,yypvt[-0].pe); } break;
case 163:
# line 981 "gram.y"
{	goto binop; } break;
case 164:
# line 982 "gram.y"
{	goto binop; } break;
case 165:
# line 983 "gram.y"
{	goto binop; } break;
case 166:
# line 984 "gram.y"
{	goto binop; } break;
case 167:
# line 985 "gram.y"
{	goto binop; } break;
case 168:
# line 986 "gram.y"
{	goto binop; } break;
case 169:
# line 987 "gram.y"
{ 	goto binop; } break;
case 170:
# line 988 "gram.y"
{	goto binop; } break;
case 171:
# line 989 "gram.y"
{	goto binop; } break;
case 172:
# line 990 "gram.y"
{	goto binop; } break;
case 173:
# line 991 "gram.y"
{	goto binop; } break;
case 174:
# line 992 "gram.y"
{	goto binop; } break;
case 175:
# line 993 "gram.y"
{	goto binop; } break;
case 176:
# line 994 "gram.y"
{	goto binop; } break;
case 177:
# line 996 "gram.y"
{	yyval.p = new qexpr(yypvt[-4].pe,yypvt[-2].pe,yypvt[-0].pe); } break;
case 178:
# line 998 "gram.y"
{	yyval.p = new expr(DELETE,yypvt[-0].pe,0); } break;
case 179:
# line 1000 "gram.y"
{	yyval.p = new expr(DELETE,yypvt[-0].pe,yypvt[-2].pe); } break;
case 181:
# line 1003 "gram.y"
{	yyval.p = dummy; } break;
case 182:
# line 1007 "gram.y"
{ 	yyval.p = new texpr(VALUE,tok_to_type(yypvt[-3].t),yypvt[-1].pe); } break;
case 183:
# line 1009 "gram.y"
{	yyval.p = new texpr(VALUE,yypvt[-3].pn->tp,yypvt[-1].pe); } break;
case 184:
# line 1011 "gram.y"
{	Ptype t = yypvt[-0].pn->tp; yyval.p = new texpr(NEW,t,0); } break;
case 185:
# line 1013 "gram.y"
{	Ptype t = yypvt[-1].pn->tp; yyval.p = new texpr(NEW,t,0); } break;
case 186:
# line 1015 "gram.y"
{	yyval.p = new expr(yypvt[-0].t,yypvt[-1].pe,0); } break;
case 187:
# line 1017 "gram.y"
{	yyval.p = new texpr(CAST,yypvt[-2].pn->tp,yypvt[-0].pe); } break;
case 188:
# line 1019 "gram.y"
{	yyval.p = new expr(DEREF,yypvt[-0].pe,0); } break;
case 189:
# line 1021 "gram.y"
{	yyval.p = new expr(ADDROF,0,yypvt[-0].pe); } break;
case 190:
# line 1023 "gram.y"
{	yyval.p = new expr(UMINUS,0,yypvt[-0].pe); } break;
case 191:
# line 1025 "gram.y"
{	yyval.p = new expr(UPLUS,0,yypvt[-0].pe); } break;
case 192:
# line 1027 "gram.y"
{	yyval.p = new expr(NOT,0,yypvt[-0].pe); } break;
case 193:
# line 1029 "gram.y"
{	yyval.p = new expr(COMPL,0,yypvt[-0].pe); } break;
case 194:
# line 1031 "gram.y"
{	yyval.p = new expr(yypvt[-1].t,0,yypvt[-0].pe); } break;
case 195:
# line 1033 "gram.y"
{	yyval.p = new texpr(SIZEOF,0,yypvt[-0].pe); } break;
case 196:
# line 1035 "gram.y"
{	yyval.p = new texpr(SIZEOF,yypvt[-1].pn->tp,0); } break;
case 197:
# line 1037 "gram.y"
{	yyval.p = new expr(DEREF,yypvt[-3].pe,yypvt[-1].pe); } break;
case 198:
# line 1042 "gram.y"
{	Pexpr ee = yypvt[-1].pe;
				Pexpr e = yypvt[-3].pe;
				if (e->base == NEW)
					e->e1 = ee;
				else
					yyval.p = new call(e,ee);
			} break;
case 199:
# line 1050 "gram.y"
{	yyval.p = new ref(REF,yypvt[-2].pe,yypvt[-0].pn); } break;
case 200:
# line 1052 "gram.y"
{	yyval.p = new expr(REF,yypvt[-3].pe,yypvt[-0].pe); } break;
case 201:
# line 1054 "gram.y"
{	yyval.p = new ref(REF,yypvt[-2].pe,Ncopy(yypvt[-0].pn)); } break;
case 202:
# line 1056 "gram.y"
{	yyval.p = new ref(DOT,yypvt[-2].pe,yypvt[-0].pn); } break;
case 203:
# line 1058 "gram.y"
{	yyval.p = new expr(DOT,yypvt[-3].pe,yypvt[-0].pe); } break;
case 204:
# line 1060 "gram.y"
{	yyval.p = new ref(DOT,yypvt[-2].pe,Ncopy(yypvt[-0].pn)); } break;
case 205:
# line 1062 "gram.y"
{	yyval.p = Ncopy(yypvt[-0].pn);
				yyval.pn->n_qualifier = sta_name;
			} break;
case 207:
# line 1067 "gram.y"
{	yyval.p = yypvt[-1].p; } break;
case 208:
# line 1069 "gram.y"
{	yyval.p = zero; } break;
case 209:
# line 1071 "gram.y"
{	yyval.p = new expr(ICON,0,0);
				yyval.pe->string = yypvt[-0].s;
			} break;
case 210:
# line 1075 "gram.y"
{	yyval.p = new expr(FCON,0,0);
				yyval.pe->string = yypvt[-0].s;
			} break;
case 211:
# line 1079 "gram.y"
{	yyval.p = new expr(STRING,0,0);
				yyval.pe->string = yypvt[-0].s;
			} break;
case 212:
# line 1083 "gram.y"
{	yyval.p = new expr(CCON,0,0);
				yyval.pe->string = yypvt[-0].s;
			} break;
case 213:
# line 1087 "gram.y"
{	yyval.p = new expr(THIS,0,0); } break;
case 214:
# line 1091 "gram.y"
{	yyval.p = yypvt[-0].pn; } break;
case 215:
# line 1093 "gram.y"
{	yyval.p = Ncopy(yypvt[-0].pn);
				yyval.pn->n_qualifier = yypvt[-2].pn;
			} break;
case 216:
# line 1097 "gram.y"
{	yyval.p = Ncopy(yypvt[-0].pn);
				look_for_hidden(yypvt[-2].pn,yyval.pn);
			} break;
case 217:
# line 1101 "gram.y"
{	yyval.p = new name(oper_name(yypvt[-0].t));
				yyval.pn->n_oper = yypvt[-0].t;
			} break;
case 218:
# line 1105 "gram.y"
{	yyval.p = new name(oper_name(yypvt[-0].t));
				yyval.pn->n_oper = yypvt[-0].t;
				yyval.pn->n_qualifier = yypvt[-3].pn;
			} break;
case 219:
# line 1110 "gram.y"
{	yyval.p = new name(oper_name(yypvt[-0].t));
				yyval.pn->n_oper = yypvt[-0].t;
				look_for_hidden(yypvt[-3].pn,yyval.pn);
			} break;
case 220:
# line 1115 "gram.y"
{	yyval.p = yypvt[-0].p;
				sig_name(yyval.pn);
			} break;
case 221:
# line 1119 "gram.y"
{	yyval.p = yypvt[-0].p;
				sig_name(yyval.pn);
				yyval.pn->n_qualifier = yypvt[-3].pn;
			} break;
case 222:
# line 1124 "gram.y"
{	yyval.p = yypvt[-0].p;
				sig_name(yyval.pn);
				look_for_hidden(yypvt[-3].pn,yyval.pn);
			} break;
case 223:
# line 1135 "gram.y"
{	yyval.p = Ncast(yypvt[-1].p,yypvt[-0].pn); } break;
case 224:
# line 1139 "gram.y"
{	yyval.p = new basetype(yypvt[-0].t,0); } break;
case 225:
# line 1141 "gram.y"
{	yyval.p = new basetype(TYPE,yypvt[-0].pn); } break;
case 226:
# line 1145 "gram.y"
{	yyval.p = Ncast(yypvt[-1].p,yypvt[-0].pn); } break;
case 227:
# line 1149 "gram.y"
{	yyval.p = Ncast(yypvt[-1].p,yypvt[-0].pn); } break;
case 228:
# line 1152 "gram.y"
{	yyval.p = Ndata(yypvt[-1].p,yypvt[-0].pn); } break;
case 229:
# line 1154 "gram.y"
{	yyval.p = Ndata(yypvt[-3].p,yypvt[-2].pn);
				yyval.pn->n_initializer = yypvt[-0].pe;
			} break;
case 230:
# line 1160 "gram.y"
{	yyval.p = new fct(0,name_unlist(yypvt[-1].nl),1); } break;
case 231:
# line 1162 "gram.y"
{	yyval.p = new fct(0,name_unlist(yypvt[-2].nl),ELLIPSIS); } break;
case 232:
# line 1164 "gram.y"
{	yyval.p = new fct(0,name_unlist(yypvt[-3].nl),ELLIPSIS); } break;
case 233:
# line 1168 "gram.y"
{	if (yypvt[-0].p)
					if (yypvt[-2].p)
						yypvt[-2].nl->add(yypvt[-0].pn);
					else {
						error("AD syntax");
						yyval.nl = new nlist(yypvt[-0].pn); 
					}
				else
					error("AD syntax");
			} break;
case 234:
# line 1179 "gram.y"
{	if (yypvt[-0].p) yyval.nl = new nlist(yypvt[-0].pn); } break;
case 236:
# line 1185 "gram.y"
{	yyval.p = 0; } break;
case 237:
# line 1190 "gram.y"
{	yyval.p = new ptr(PTR,0); } break;
case 238:
# line 1192 "gram.y"
{	yyval.p = new ptr(RPTR,0); } break;
case 239:
# line 1194 "gram.y"
{	TOK t = yypvt[-0].t;
				switch (t) {
				case VOLATILE:
					error('w',"\"%k\" not implemented (ignored)",t);
					yyval.p = new ptr(PTR,0);
					break;
				default:
					error("syntax error: *%k",t);
				case CONST:
					yyval.p = new ptr(PTR,0,1);
				}
			} break;
case 240:
# line 1207 "gram.y"
{	TOK t = yypvt[-0].t;
				switch (t) {
				case VOLATILE:
					error('w',"\"%k\" not implemented (ignored)",t);
					yyval.p = new ptr(RPTR,0);
					break;
				default:
					error("syntax error: &%k",t);
				case CONST:
					yyval.p = new ptr(RPTR,0,1);
				}
			} break;
case 241:
# line 1220 "gram.y"
{	Pptr p = new ptr(PTR,0);
				p->memof = Pclass(Pbase(yypvt[-0].pn->tp)->b_name->tp);
				yyval.p = p;
			} break;
case 242:
# line 1228 "gram.y"
{	yyval.p = new vec(0,(yypvt[-1].pe!=dummy)?yypvt[-1].pe:0 ); } break;
case 243:
# line 1233 "gram.y"
{	yyval.p = new vec(0,0); } break;
	}
	goto yystack;  /* stack new state and value */
}
