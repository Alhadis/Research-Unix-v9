
# line 1 "config.y"
typedef union  {
	int i;
	char *cp;
	struct idlst *idlst;
} YYSTYPE;
# define CPU 257
# define IDENT 258
# define CONFIG 259
# define ANY 260
# define DEVICE 261
# define UBA 262
# define MBA 263
# define NEXUS 264
# define CSR 265
# define DRIVE 266
# define VECTOR 267
# define OPTIONS 268
# define CONTROLLER 269
# define PSEUDO_DEVICE 270
# define FLAGS 271
# define ID 272
# define SEMICOLON 273
# define NUMBER 274
# define FPNUMBER 275
# define TRACE 276
# define DISK 277
# define SLAVE 278
# define AT 279
# define HZ 280
# define TIMEZONE 281
# define DST 282
# define MAXUSERS 283
# define MASTER 284
# define MAKEFILE 285
# define COMMA 286
# define MINUS 287

# line 13 "config.y"
/*	config.y	1.11	81/05/22	*/
#include "config.h"
#include <stdio.h>
	struct device cur;
	struct device *curp = NULL;
	char *temp_id;
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 169 "config.y"


yyerror(s)
char *s;
{
	fprintf(stderr, "config: %s at line %d\n", s, yyline);
}

/*
 * ns:
 *	Return the passed string in a new space
 */

char *
ns(str)
register char *str;
{
	register char *cp;

	cp = malloc(strlen(str)+1);
	strcpy(cp, str);
	return cp;
}

/*
 * newdev
 *	Add a device to the list
 */

newdev(dp)
register struct device *dp;
{
	register struct device *np;

	np = (struct device *) malloc(sizeof *np);
	*np = *dp;
	if (curp == NULL)
		dtab = np;
	else
		curp->d_next = np;
	curp = np;
}

/*
 * mkconf
 *	Note that a configuration should be made
 */

mkconf(dev, sysname)
char *dev, *sysname;
{
	register struct file_list *fl;

	fl = (struct file_list *) malloc(sizeof *fl);
	fl->f_fn = ns(dev);
	fl->f_needs = ns(sysname);
	if (confp == NULL)
	    conf_list = fl;
	else
	    confp->f_next = fl;
	confp = fl;
}

/*
 * Connect:
 *	Find the pointer to connect to the given device and number.
 *	returns NULL if no such device and prints an error message
 */

struct device *connect(dev, num)
register char *dev;
register int num;
{
	register struct device *dp;
	struct device *huhcon();

	if (num == QUES)
	    return huhcon(dev);
	for (dp = dtab; dp != NULL; dp = dp->d_next)
		if ((num == dp->d_unit) && eq(dev, dp->d_name))
		    if (dp->d_type != CONTROLLER && dp->d_type != MASTER)
		    {
			sprintf(errbuf, "%s connected to non-controller", dev);
			yyerror(errbuf);
			return NULL;
		    }
		    else
			return dp;
	sprintf(errbuf, "%s %d not defined", dev, num);
	yyerror(errbuf);
	return NULL;
}

/*
 * huhcon
 *	Connect to an unspecific thing
 */

struct device *huhcon(dev)
register char *dev;
{
    register struct device *dp, *dcp;
    struct device rdev;
    int oldtype;

    /*
     * First make certain that there are some of these to wildcard on
     */
    for (dp = dtab; dp != NULL; dp = dp->d_next)
	if (eq(dp->d_name, dev))
	    break;
    if (dp == NULL)
    {
	sprintf(errbuf, "no %s's to wildcard", dev);
	yyerror(errbuf);
	return NULL;
    }
    oldtype = dp->d_type;
    dcp = dp->d_conn;
    /*
     * Now see if there is already a wildcard entry for this device
     * (e.g. Search for a "uba ?")
     */
    for (; dp != NULL; dp = dp->d_next)
	if (eq(dev, dp->d_name) && dp->d_unit == -1)
	    break;
    /*
     * If there isn't, make one becuase everything needs to be connected
     * to something.
     */
    if (dp == NULL)
    {
	dp = &rdev;
	init_dev(dp);
	dp->d_unit = QUES;
	dp->d_name = ns(dev);
	dp->d_type = oldtype;
	newdev(dp);
	dp = curp;
	/*
	 * Connect it to the same thing that other similar things are
	 * connected to, but make sure it is a wildcard unit
	 * (e.g. up connected to sc ?, here we make connect sc? to a uba?)
	 * If other things like this are on the NEXUS or if the aren't
	 * connected to anything, then make the same connection, else
	 * call ourself to connect to another unspecific device.
	 */
	if (dcp == TO_NEXUS || dcp == NULL)
	    dp->d_conn = dcp;
	else
	    dp->d_conn = connect(dcp->d_name, QUES);
    }
    return dp;
}

/*
 * init_dev:
 *	Set up the fields in the current device to their
 *	default values.
 */

init_dev(dp)
register struct device *dp;
{
    dp->d_name = "OHNO!!!";
    dp->d_type = DEVICE;
    dp->d_conn = NULL;
    dp->d_vec = NULL;
    dp->d_addr = dp->d_flags = dp->d_dk = 0;
    dp->d_slave = dp->d_drive = dp->d_unit = UNKNOWN;
    dp->d_count = 0;
}

/*
 * Check_nexus:
 *	Make certain that this is a reasonable type of thing to put
 *	on the nexus.
 */

check_nexus(dev, num)
register struct device *dev;
int num;
{
    if (!eq(dev->d_name, "uba") && !eq(dev->d_name, "mba"))
	yyerror("only uba's and mba's should be connected to the nexus");
    if (num != QUES)
	yyerror("can't give specific nexus numbers");
}

/*
 * Check the timezone to make certain it is sensible
 */

check_tz()
{
	if (timezone > 24 * 60)
		yyerror("timezone is unreasonable");
	else
		hadtz = TRUE;
}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 2,
	0, 1,
	-2, 0,
	};
# define YYNPROD 53
# define YYLAST 138
short yyact[]={

   8,  14,  16,  17,  57,   9,  75,  74,  60,  59,
  79,  80,  15,  12,  13,  43,  82,   7,  48,  26,
   6,  11,  89,  81,  18,  19,  88,  22,  10,  21,
  87,  20,  61,  62,  41,  42,  50,  51,  67,  86,
  84,  83,  72,  68,  50,  51,  52,  45,  34,  40,
  25,  24,  23,  58,  52,  44,  38,  64,  37,  76,
  63,  36,  49,  28,  78,  65,  46,  27,  47,  35,
   5,   4,   3,  33,   2,   1,  39,  32,  29,  30,
  31,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  56,  53,  54,  55,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  66,   0,   0,  69,  70,  71,   0,   0,  73,
   0,   0,   0,  77,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  77,  85 };
short yypact[]={

-1000,-1000,-256,-1000,-221,-222,-223,-1000,-254,-1000,
-1000,-1000,-1000,-1000,-224,-224,-216,-224,-225,-240,
-266,-217,-227,-1000,-1000,-1000,-1000,-261,-218,-261,
-261,-261,-218,-1000,-1000,-282,-1000,-1000,-1000,-219,
-1000,-273,-274,-242,-1000,-1000,-210,-1000,-226,-231,
-1000,-1000,-1000,-210,-210,-210,-232,-224,-1000,-1000,
-1000,-275,-276,-1000,-224,-255,-233,-234,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-1000,-224,-1000,-235,
-244,-248,-252,-1000,-1000,-1000,-1000,-1000,-1000,-1000 };
short yypgo[]={

   0,  58,  62,  59,  75,  74,  72,  71,  70,  69,
  61,  67,  66,  60,  63,  68,  65,  64 };
short yyr1[]={

   0,   4,   5,   5,   6,   6,   6,   6,   6,   8,
   8,   8,   8,   8,   8,   8,   8,   8,   8,   8,
   8,   8,   8,   8,   9,   9,  10,   1,   2,   2,
   2,   7,   7,   7,   7,   7,   7,  11,  14,  12,
  12,  15,  15,  16,  16,  17,  17,  17,  17,  13,
  13,   3,   3 };
short yyr2[]={

   0,   1,   2,   0,   2,   2,   2,   1,   2,   2,
   2,   2,   3,   2,   2,   3,   2,   3,   3,   4,
   3,   4,   2,   2,   3,   1,   1,   1,   1,   1,
   1,   4,   4,   4,   4,   3,   4,   3,   0,   2,
   0,   3,   3,   2,   0,   2,   2,   2,   2,   2,
   0,   1,   2 };
short yychk[]={

-1000,  -4,  -5,  -6,  -7,  -8, 276, 273, 256, 261,
 284, 277, 269, 270, 257, 268, 258, 259, 280, 281,
 287, 285, 283, 273, 273, 273, 273, -11, -14, -11,
 -11, -11, -14,  -1, 272,  -9, -10,  -1, 272,  -1,
 274, 274, 275, 281, 272, 274, -12, -15, 279,  -2,
 262, 263, 272, -12, -12, -12,  -2, 286, 272, 282,
 282, 274, 275, -13, 267, -16,  -2, 264, 274, -13,
 -13, -13, 274, -10, 282, 282,  -3,  -1, -17, 265,
 266, 278, 271, 274, 274,  -3, 274, 274, 274, 274 };
short yydef[]={

   3,  -2,  -2,   2,   0,   0,   0,   7,   0,  38,
  38,  38,  38,  38,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   4,   5,   6,   8,  40,   0,  40,
  40,  40,   0,   9,  27,  10,  25,  26,  11,   0,
  13,  14,  16,   0,  22,  23,  50,  44,   0,   0,
  28,  29,  30,  50,  50,  50,  35,   0,  12,  15,
  17,  18,  20,  31,   0,  39,   0,   0,  37,  32,
  33,  34,  36,  24,  19,  21,  49,  51,  43,   0,
   0,   0,   0,  41,  42,  52,  45,  46,  47,  48 };
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
		
case 4:
# line 31 "config.y"
 { newdev(&cur); } break;
case 6:
# line 33 "config.y"
 { do_trace = ! do_trace; } break;
case 9:
# line 39 "config.y"
 {
		    struct cputype *cp = (struct cputype *)malloc(sizeof (struct cputype));
		    cp->cpu_name = ns(yypvt[-0].cp);
		    cp->cpu_next = cputype;
		    cputype = cp;
		    free(temp_id);
		    } break;
case 11:
# line 47 "config.y"
{ ident = ns(yypvt[-0].cp); } break;
case 12:
# line 48 "config.y"
 { mkconf(temp_id, yypvt[-0].cp); free(temp_id); } break;
case 13:
# line 49 "config.y"
 {
		yyerror("HZ specification obsolete; delete");
		hz = 60;
		} break;
case 14:
# line 53 "config.y"
 { timezone = 60 * yypvt[-0].i; check_tz(); } break;
case 15:
# line 54 "config.y"
 { timezone = 60 * yypvt[-1].i; dst = 1; check_tz(); } break;
case 16:
# line 55 "config.y"
 { timezone = yypvt[-0].i; check_tz(); } break;
case 17:
# line 56 "config.y"
 { timezone = yypvt[-1].i; dst = 1; check_tz(); } break;
case 18:
# line 57 "config.y"

	    { timezone = -60 * yypvt[-0].i; check_tz(); } break;
case 19:
# line 59 "config.y"

	    { timezone = -60 * yypvt[-1].i; dst = 1; check_tz(); } break;
case 20:
# line 61 "config.y"

	    { timezone = -yypvt[-0].i; check_tz(); } break;
case 21:
# line 63 "config.y"

	    { timezone = -yypvt[-1].i; dst = 1; check_tz(); } break;
case 22:
# line 65 "config.y"

	    { mkfile = ns(yypvt[-0].cp); } break;
case 23:
# line 67 "config.y"
 { maxusers = yypvt[-0].i; } break;
case 26:
# line 76 "config.y"
 {
		    struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		    op->op_name = ns(yypvt[-0].cp);
		    op->op_next = opt;
		    opt = op;
		    free(temp_id);
	} break;
case 27:
# line 86 "config.y"
 { yyval.cp = temp_id = ns(yypvt[-0].cp); } break;
case 28:
# line 90 "config.y"
 { yyval.cp = ns("uba"); } break;
case 29:
# line 91 "config.y"
 { yyval.cp = ns("mba"); } break;
case 30:
# line 92 "config.y"
 { yyval.cp = ns(yypvt[-0].cp); } break;
case 31:
# line 96 "config.y"
 {  cur.d_type = DEVICE; } break;
case 32:
# line 97 "config.y"
 {  cur.d_type = MASTER; } break;
case 33:
# line 98 "config.y"

				{  cur.d_dk = 1; cur.d_type = DEVICE; } break;
case 34:
# line 100 "config.y"
 {  cur.d_type = CONTROLLER; } break;
case 35:
# line 101 "config.y"

			{ cur.d_name = yypvt[-0].cp; cur.d_type = PSEUDO_DEVICE; } break;
case 36:
# line 103 "config.y"

			{ cur.d_name = yypvt[-1].cp; cur.d_type = PSEUDO_DEVICE;
			  cur.d_count = yypvt[-0].i; } break;
case 37:
# line 109 "config.y"
	{
			cur.d_name = yypvt[-1].cp;
			if (eq(yypvt[-1].cp, "mba"))
			    seen_mba = TRUE;
			else if (eq(yypvt[-1].cp, "uba"))
			    seen_uba = TRUE;
			cur.d_unit = yypvt[-0].i;
		} break;
case 38:
# line 120 "config.y"
 { init_dev(&cur); } break;
case 41:
# line 129 "config.y"
 {
		if (eq(cur.d_name, "mba") || eq(cur.d_name, "uba")) {
			sprintf(errbuf,
				"%s must be connected to a nexus", cur.d_name);
			yyerror(errbuf);
		}
		cur.d_conn = connect(yypvt[-1].cp, yypvt[-0].i);
	} break;
case 42:
# line 137 "config.y"
 { check_nexus(&cur, yypvt[-0].i); cur.d_conn = TO_NEXUS; } break;
case 45:
# line 146 "config.y"
 { cur.d_addr = yypvt[-0].i; } break;
case 46:
# line 147 "config.y"
 { cur.d_drive = yypvt[-0].i; } break;
case 47:
# line 148 "config.y"

	{
		if (cur.d_conn != NULL && cur.d_conn != TO_NEXUS
		    && cur.d_conn->d_type == MASTER)
			cur.d_slave = yypvt[-0].i;
		else
			yyerror("can't specify slave--not to master");
	} break;
case 48:
# line 156 "config.y"
 { cur.d_flags = yypvt[-0].i; } break;
case 49:
# line 160 "config.y"
 { cur.d_vec = yypvt[-0].idlst; } break;
case 51:
# line 163 "config.y"

	    { struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	      a->id = yypvt[-0].cp; a->id_next = 0; yyval.idlst = a; } break;
case 52:
# line 166 "config.y"

	    { struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	      a->id = yypvt[-1].cp; a->id_next = yypvt[-0].idlst; yyval.idlst = a; } break;
	}
	goto yystack;  /* stack new state and value */
}
