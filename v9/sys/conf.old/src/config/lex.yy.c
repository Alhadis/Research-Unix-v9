# include "stdio.h"
# define U(x) x
# define NLSTATE yyprevious=YYNEWLINE
# define BEGIN yybgin = yysvec + 1 +
# define INITIAL 0
# define YYLERR yysvec
# define YYSTATE (yyestate-yysvec-1)
# define YYOPTIM 1
# define YYLMAX 200
# define output(c) putc(c,yyout)
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
# define unput(c) {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;}
# define yymore() (yymorfg=1)
# define ECHO fprintf(yyout, "%s",yytext)
# define REJECT { nstr = yyreject(); goto yyfussy;}
int yyleng; extern char yytext[];
int yymorfg;
extern char *yysptr, yysbuf[];
int yytchar;
FILE *yyin = {stdin}, *yyout = {stdout};
extern int yylineno;
struct yysvf { 
	struct yywork *yystoff;
	struct yysvf *yyother;
	int *yystops;};
struct yysvf *yyestate;
extern struct yysvf yysvec[], *yybgin;
/*	config.l	1.8	81/05/18	*/

#include <ctype.h>
#include "y.tab.h"
#include "config.h"

#define tprintf if (do_trace) printf

YYSTYPE yylval;

/*
 * Key word table
 */

struct kt {
	char *kt_name;
	int kt_val;
} key_words[] = {
	"cpu", CPU, "ident", IDENT, "config", CONFIG, "options", OPTIONS,
	"device", DEVICE, "controller", CONTROLLER, "uba", UBA, "mba", MBA,
	"csr", CSR, "nexus", NEXUS, "drive", DRIVE, "vector", VECTOR,
	"pseudo-device", PSEUDO_DEVICE, "flags", FLAGS, "trace", TRACE,
	"disk", DISK, "tape", DEVICE, "slave", SLAVE, "at", AT,
	"hz", HZ, "timezone", TIMEZONE, "dst", DST, "maxusers", MAXUSERS,
	"master", MASTER, "mkfile", MAKEFILE,
	0,0,
};
# define YYNEWLINE 10
yylex(){
int nstr; extern int yyprevious;
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
	{
			int i;

			if ((i = kw_lookup(yytext)) == -1)
			{
				yylval.cp = yytext;
				tprintf("id(%s) ", yytext);
				return ID;
			}
			tprintf("(%s) ", yytext);
			return i;
		}
break;
case 2:
{
			yytext[strlen(yytext)-1] = '\0';
			yylval.cp = yytext + 1;
			return ID;
		}
break;
case 3:
	{
			yylval.i = octal(yytext);
			tprintf("#O:%o ", yylval.i);
			return NUMBER;
		}
break;
case 4:
{
			yylval.i = hex(yytext);
			tprintf("#X:%x ", yylval.i);
			return NUMBER;
		}
break;
case 5:
{
			yylval.i = atoi(yytext);
			tprintf("#D:%d ", yylval.i);
			return NUMBER;
		}
break;
case 6:
{
			double atof();
			yylval.i = (int) (60 * atof(yytext) + 0.5);
			return FPNUMBER;
		}
break;
case 7:
	{
			return MINUS;
		}
break;
case 8:
	{
			yylval.i = QUES;
			tprintf("? ");
			return NUMBER;
		}
break;
case 9:
{
			yyline++;
			tprintf("\n... ");
		}
break;
case 10:
	{
			yyline++;
			tprintf("\n");
			return SEMICOLON;
		}
break;
case 11:
	{	/* Ignored (comment) */;	}
break;
case 12:
	{	/* Ignored (white space) */;	}
break;
case 13:
	{	return SEMICOLON;		}
break;
case 14:
	{	return COMMA;			}
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */
/*
 * kw_lookup
 *	Look up a string in the keyword table.  Returns a -1 if the
 *	string is not a keyword otherwise it returns the keyword number
 */

kw_lookup(word)
register char *word;
{
	register struct kt *kp;

	for (kp = key_words; kp->kt_name != 0; kp++)
		if (eq(word, kp->kt_name))
			return kp->kt_val;
	return -1;
}

/*
 * Number conversion routines
 */

octal(str)
char *str;
{
	int num;

	sscanf(str, "%o", &num);
	return num;
}

hex(str)
char *str;
{
	int num;

	sscanf(str+2, "%x", &num);
	return num;
}
int yyvstop[] = {
0,

12,
0,

12,
0,

12,
0,

10,
-9,
0,

14,
0,

7,
0,

3,
0,

5,
0,

13,
0,

8,
0,

1,
0,

11,
0,

9,
0,

6,
0,

3,
0,

5,
0,

2,
0,

4,
0,
0};
# define YYTYPE char
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,3,	1,4,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	3,3,	1,0,	1,5,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	1,6,	1,7,	1,0,	
1,0,	1,8,	1,9,	4,14,	
15,20,	0,0,	0,0,	0,0,	
3,3,	1,9,	0,0,	1,0,	
1,10,	1,0,	1,0,	1,0,	
1,11,	1,0,	1,12,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	9,16,	4,14,	9,19,	
9,19,	9,19,	9,19,	9,19,	
9,19,	9,19,	9,19,	9,19,	
9,19,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
1,0,	1,0,	1,0,	1,0,	
0,0,	1,0,	1,12,	8,16,	
0,0,	8,17,	8,17,	8,17,	
8,17,	8,17,	8,17,	8,17,	
8,17,	16,16,	16,16,	16,16,	
16,16,	16,16,	16,16,	16,16,	
16,16,	16,16,	16,16,	0,0,	
0,0,	0,0,	0,0,	0,0,	
1,0,	1,0,	1,0,	1,0,	
1,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,0,	0,0,	0,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
0,0,	2,0,	0,0,	2,13,	
2,0,	2,0,	2,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
2,6,	8,18,	2,0,	2,0,	
17,17,	17,17,	17,17,	17,17,	
17,17,	17,17,	17,17,	17,17,	
0,0,	5,15,	2,0,	2,10,	
2,0,	2,0,	2,0,	2,11,	
2,0,	5,15,	5,15,	19,19,	
19,19,	19,19,	19,19,	19,19,	
19,19,	19,19,	19,19,	19,19,	
19,19,	0,0,	0,0,	18,21,	
18,21,	18,21,	18,21,	18,21,	
18,21,	18,21,	18,21,	18,21,	
18,21,	0,0,	5,0,	2,0,	
2,0,	2,0,	2,0,	0,0,	
2,0,	0,0,	0,0,	0,0,	
0,0,	5,15,	0,0,	0,0,	
5,15,	5,15,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
5,15,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	5,15,	0,0,	2,0,	
2,0,	2,0,	2,0,	2,0,	
18,21,	18,21,	18,21,	18,21,	
18,21,	18,21,	12,12,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	5,15,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
0,0,	0,0,	0,0,	0,0,	
12,12,	0,0,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
12,12,	12,12,	12,12,	12,12,	
13,13,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
13,13,	13,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	13,13,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
13,13,	0,0,	0,0,	13,13,	
13,13,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	13,13,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
13,13,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
13,13,	0,0,	0,0,	0,0,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
yycrank+-1,	0,		yyvstop+1,
yycrank+-128,	yysvec+1,	yyvstop+3,
yycrank+24,	0,		yyvstop+5,
yycrank+42,	0,		yyvstop+7,
yycrank+-184,	0,		0,	
yycrank+0,	0,		yyvstop+10,
yycrank+0,	0,		yyvstop+12,
yycrank+53,	0,		yyvstop+14,
yycrank+27,	0,		yyvstop+16,
yycrank+0,	0,		yyvstop+18,
yycrank+0,	0,		yyvstop+20,
yycrank+217,	0,		yyvstop+22,
yycrank+-339,	0,		yyvstop+24,
yycrank+0,	0,		yyvstop+26,
yycrank+-18,	yysvec+5,	0,	
yycrank+61,	0,		yyvstop+28,
yycrank+128,	0,		yyvstop+30,
yycrank+159,	0,		0,	
yycrank+147,	0,		yyvstop+32,
yycrank+0,	0,		yyvstop+34,
yycrank+0,	yysvec+18,	yyvstop+36,
0,	0,	0};
struct yywork *yytop = yycrank+436;
struct yysvf *yybgin = yysvec+1;
char yymatch[] = {
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,012 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,'"' ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,'-' ,01  ,01  ,
'0' ,'1' ,'1' ,'1' ,'1' ,'1' ,'1' ,'1' ,
'8' ,'8' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,01  ,01  ,01  ,01  ,'A' ,
01  ,'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,
'A' ,'A' ,'A' ,01  ,01  ,01  ,01  ,01  ,
0};
char yyextra[] = {
0,0,0,0,0,0,0,0,
0,1,0,0,0,0,0,0,
0};
int yylineno =1;
# define YYU(x) x
# define NLSTATE yyprevious=YYNEWLINE
char yytext[YYLMAX];
struct yysvf *yylstate [YYLMAX], **yylsp, **yyolsp;
char yysbuf[YYLMAX];
char *yysptr = yysbuf;
int *yyfnd;
extern struct yysvf *yyestate;
int yyprevious = YYNEWLINE;
yylook(){
	register struct yysvf *yystate, **lsp;
	register struct yywork *yyt;
	struct yysvf *yyz;
	int yych;
	struct yywork *yyr;
# ifdef LEXDEBUG
	int debug;
# endif
	char *yylastch;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	if (!yymorfg)
		yylastch = yytext;
	else {
		yymorfg=0;
		yylastch = yytext+yyleng;
		}
	for(;;){
		lsp = yylstate;
		yyestate = yystate = yybgin;
		if (yyprevious==YYNEWLINE) yystate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(yyout,"state %d\n",yystate-yysvec-1);
# endif
			yyt = yystate->yystoff;
			if(yyt == yycrank){		/* may not be any transitions */
				yyz = yystate->yyother;
				if(yyz == 0)break;
				if(yyz->yystoff == yycrank)break;
				}
			*yylastch++ = yych = input();
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"char ");
				allprint(yych);
				putchar('\n');
				}
# endif
			yyr = yyt;
			if ( (int)yyt > (int)yycrank){
				yyt = yyr + yych;
				if (yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
# ifdef YYOPTIM
			else if((int)yyt < (int)yycrank) {		/* r < yycrank */
				yyt = yyr = yycrank+(yycrank-yyt);
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"compressed state\n");
# endif
				yyt = yyt + yych;
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				yyt = yyr + YYU(yymatch[yych]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"try fall back character ");
					allprint(YYU(yymatch[yych]));
					putchar('\n');
					}
# endif
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transition */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
			if ((yystate = yystate->yyother) && (yyt= yystate->yystoff) != yycrank){
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"fall back to state %d\n",yystate-yysvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--yylastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"state %d char ",yystate-yysvec-1);
				allprint(yych);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(yyout,"stopped at %d with ",*(lsp-1)-yysvec-1);
			allprint(yych);
			putchar('\n');
			}
# endif
		while (lsp-- > yylstate){
			*yylastch-- = 0;
			if (*lsp != 0 && (yyfnd= (*lsp)->yystops) && *yyfnd > 0){
				yyolsp = lsp;
				if(yyextra[*yyfnd]){		/* must backup */
					while(yyback((*lsp)->yystops,-*yyfnd) != 1 && lsp > yylstate){
						lsp--;
						unput(*yylastch--);
						}
					}
				yyprevious = YYU(*yylastch);
				yylsp = lsp;
				yyleng = yylastch-yytext+1;
				yytext[yyleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"\nmatch ");
					sprint(yytext);
					fprintf(yyout," action %d\n",*yyfnd);
					}
# endif
				return(*yyfnd++);
				}
			unput(*yylastch);
			}
		if (yytext[0] == 0  /* && feof(yyin) */)
			{
			yysptr=yysbuf;
			return(0);
			}
		yyprevious = yytext[0] = input();
		if (yyprevious>0)
			output(yyprevious);
		yylastch=yytext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	}
yyback(p, m)
	int *p;
{
if (p==0) return(0);
while (*p)
	{
	if (*p++ == m)
		return(1);
	}
return(0);
}
	/* the following are only used in the lex library */
yyinput(){
	return(input());
	}
yyoutput(c)
  int c; {
	output(c);
	}
yyunput(c)
   int c; {
	unput(c);
	}
