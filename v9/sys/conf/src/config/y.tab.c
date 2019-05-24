
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
# define MACHINE 288
# define PRIORITY 289
# define VME16D16 290
# define VME24D16 291
# define VME32D16 292
# define VME16D32 293
# define VME24D32 294
# define VME32D32 295

# line 15 "config.y"
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

# line 255 "config.y"


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
    dp->d_addr = UNKNOWN;
    dp->d_flags = dp->d_dk = 0;
    dp->d_slave = dp->d_drive = dp->d_unit = UNKNOWN;
    dp->d_count = 0;
    dp->d_mach = dp->d_bus = 0;
    dp->d_pri = 0;
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
	switch (machine) {

	case MACHINE_VAX:
		if (!eq(dev->d_name, "uba") && !eq(dev->d_name, "mba"))
			yyerror("only uba's and mba's should be connected to the nexus");
		if (num != QUES)
			yyerror("can't give specific nexus numbers");
		break;

	case MACHINE_SUN2:
		if (!eq(dev->d_name, "virtual") &&
		    !eq(dev->d_name, "obmem") &&
		    !eq(dev->d_name, "obio") &&
		    !eq(dev->d_name, "mbmem") &&
		    !eq(dev->d_name, "mbio") &&
		    !eq(dev->d_name, "vme16d16") &&
		    !eq(dev->d_name, "vme24d16")) {
			(void)sprintf(errbuf,
			    "unknown bus type `%s' for nexus connection on %s",
			    dev->d_name, machinename);
			yyerror(errbuf);
		}
		break;

	case MACHINE_SUN3:
		if (!eq(dev->d_name, "virtual") &&
		    !eq(dev->d_name, "obmem") &&
		    !eq(dev->d_name, "obio") &&
		    !eq(dev->d_name, "vme16d16") &&
		    !eq(dev->d_name, "vme24d16") &&
		    !eq(dev->d_name, "vme32d16") &&
		    !eq(dev->d_name, "vme16d32") &&
		    !eq(dev->d_name, "vme24d32") &&
		    !eq(dev->d_name, "vme32d32")) {
			(void)sprintf(errbuf,
			    "unknown bus type `%s' for nexus connection on %s",
			    dev->d_name, machinename);
			yyerror(errbuf);
		}
		break;
	}
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

/*
 * bi_info gives the magic number used to construct the token for
 * the autoconf code.  bi_max is the maximum value (across all
 * machine types for a given architecture) that a given "bus
 * type" can legally have.
 */
struct bus_info {
	char	*bi_name;
	u_short	bi_info;
	u_int	bi_max;
};

struct bus_info sun2_info[] = {
	{ "virtual",	0x0001,	(1<<24)-1 },
	{ "obmem",	0x0002,	(1<<23)-1 },
	{ "obio",	0x0004,	(1<<23)-1 },
	{ "mbmem",	0x0010,	(1<<20)-1 },
	{ "mbio",	0x0020,	(1<<16)-1 },
	{ "vme16d16",	0x0100,	(1<<16)-1 },
	{ "vme24d16",	0x0200,	(1<<24)-(1<<16)-1 },
	{ (char *)0,	0,	0 }
};

struct bus_info sun3_info[] = {
	{ "virtual",	0x0001,	(1<<32)-1 },
	{ "obmem",	0x0002,	(1<<32)-1 },
	{ "obio",	0x0004,	(1<<21)-1 },
	{ "vme16d16",	0x0100,	(1<<16)-1 },
	{ "vme24d16",	0x0200,	(1<<24)-(1<<16)-1 },
	{ "vme32d16",	0x0400,	(1<<32)-(1<<24)-1 },
	{ "vme16d32",	0x1000,	(1<<16) },
	{ "vme24d32",	0x2000,	(1<<24)-(1<<16)-1 },
	{ "vme32d32",	0x4000,	(1<<32)-(1<<24)-1 },
	{ (char *)0,	0,	0 }
};

bus_encode(addr, dp)
	u_int addr;
	register struct device *dp;
{
	register char *busname;
	register struct bus_info *bip;
	register int num;

	if (machine == MACHINE_SUN2)
		bip = sun2_info;
	else if (machine == MACHINE_SUN3)
		bip = sun3_info;
	else {
		yyerror("bad machine type for bus_encode");
		exit(1);
	}

	if (dp->d_conn == TO_NEXUS || dp->d_conn == 0) {
		yyerror("bad connection");
		exit(1);
	}

	busname = dp->d_conn->d_name;
	num = dp->d_conn->d_unit;

	for (; bip->bi_name != 0; bip++)
		if (eq(busname, bip->bi_name))
			break;

	if (bip->bi_name == 0) {
		(void)sprintf(errbuf, "bad bus type '%s' for machine %s",
			busname, machinename);
		yyerror(errbuf);
	} else if (addr > bip->bi_max) {
		(void)sprintf(errbuf,
			"0x%x exceeds maximum address 0x%x allowed for %s",
			addr, bip->bi_max, busname);
		yyerror(errbuf);
	} else {
		dp->d_bus = bip->bi_info;	/* set up bus type info */
		if (num != QUES)
			/*
			 * Set up cpu type since the connecting
			 * bus type is not wildcarded.
			 */
			dp->d_mach = num;
	}
}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 2,
	0, 1,
	-2, 0,
	};
# define YYNPROD 66
# define YYLAST 168
short yyact[]={

   8,  15,  17,  18,  74,   9,  86,  65,  52,  53,
  77,  85,  16,  12,  13,  84,  68,   7,  60,  45,
   6,  11,  67, 104,  19,  20,  73,  23,  10,  22,
  50,  21,  14,  52,  53, 103,  54,  55,  56,  57,
  58,  59, 102,  60,  91,  92,  69,  70,  43,  44,
  94, 101,  35,  88,  99,  97,  96,  93,  95,  87,
  82,  54,  55,  56,  57,  58,  59,  78,  47,  42,
  27,  26,  25,  24,  35,  66,  46,  40,  74,  72,
  89,  51,  90,  75,  38,  71,  48,  49,  37,  28,
  29,   5,   4,   3,   2,  34,  36,  39,   1,  41,
  30,  31,  32,   0,  33,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  64,   0,  61,  62,  63,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,  76,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0, 100,   0,   0,  39,  79,  80,  81,
  83,   0,   0, 105,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  98 };
short yypact[]={

-1000,-1000,-256,-1000,-200,-201,-202,-1000,-203,-1000,
-1000,-1000,-1000,-1000,-198,-198,-198,-195,-198,-205,
-226,-262,-196,-206,-1000,-1000,-1000,-1000,-249,-229,
-249,-249,-249,-229,-1000,-1000,-1000,-279,-1000,-1000,
-1000,-197,-1000,-260,-266,-228,-1000,-1000,-263,-1000,
-254,-207,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-263,-263,-263,-214,-198,-1000,-1000,-1000,-267,
-271,-1000,-283,-215,-198,-221,-216,-218,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,-219,-189,-1000,-220,
-1000,-223,-232,-239,-251,-1000,-1000,-1000,-1000,-198,
-1000,-1000,-1000,-1000,-1000,-1000 };
short yypgo[]={

   0,  80,  81,  53,  98,  94,  93,  92,  91,  88,
  84,  89,  86,  85,  90,  87,  83,  82,  79 };
short yyr1[]={

   0,   4,   5,   5,   6,   6,   6,   6,   6,   8,
   8,   8,   8,   8,   8,   8,   8,   8,   8,   8,
   8,   8,   8,   8,   8,   9,   9,  10,   1,   2,
   2,   2,   2,   2,   2,   2,   2,   2,   7,   7,
   7,   7,   7,   7,  11,  14,  12,  12,  15,  15,
  16,  16,  17,  17,  17,  17,  13,  13,  13,  13,
  13,  18,   3,   3,   3,   3 };
short yyr2[]={

   0,   1,   2,   0,   2,   2,   2,   1,   2,   2,
   2,   2,   2,   3,   2,   2,   3,   2,   3,   3,
   4,   3,   4,   2,   2,   3,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   4,   4,
   4,   4,   3,   4,   3,   0,   2,   0,   3,   3,
   2,   0,   2,   2,   2,   2,   1,   2,   3,   3,
   0,   2,   1,   2,   2,   3 };
short yychk[]={

-1000,  -4,  -5,  -6,  -7,  -8, 276, 273, 256, 261,
 284, 277, 269, 270, 288, 257, 268, 258, 259, 280,
 281, 287, 285, 283, 273, 273, 273, 273, -11, -14,
 -11, -11, -11, -14,  -1, 272,  -1,  -9, -10,  -1,
 272,  -1, 274, 274, 275, 281, 272, 274, -12, -15,
 279,  -2, 262, 263, 290, 291, 292, 293, 294, 295,
 272, -12, -12, -12,  -2, 286, 272, 282, 282, 274,
 275, -13, -18, 289, 267, -16,  -2, 264, 274, -13,
 -13, -13, 274, -10, 282, 282, 289, 274,  -3,  -1,
 -17, 265, 266, 278, 271, 274, 274, 274, -18, 274,
  -3, 274, 274, 274, 274,  -3 };
short yydef[]={

   3,  -2,  -2,   2,   0,   0,   0,   7,   0,  45,
  45,  45,  45,  45,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   4,   5,   6,   8,  47,   0,
  47,  47,  47,   0,   9,  28,  10,  11,  26,  27,
  12,   0,  14,  15,  17,   0,  23,  24,  60,  51,
   0,   0,  29,  30,  31,  32,  33,  34,  35,  36,
  37,  60,  60,  60,  42,   0,  13,  16,  18,  19,
  21,  38,  56,   0,   0,  46,   0,   0,  44,  39,
  40,  41,  43,  25,  20,  22,   0,  57,  61,  62,
  50,   0,   0,   0,   0,  48,  49,  58,  59,  63,
  64,  52,  53,  54,  55,  65 };
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
# line 33 "config.y"
 { newdev(&cur); } break;
case 6:
# line 35 "config.y"
 { do_trace = ! do_trace; } break;
case 9:
# line 42 "config.y"
 {
		if (eq(yypvt[-0].cp, "vax")) {
			machine = MACHINE_VAX;
			machinename = "vax";
		} else if (eq(yypvt[-0].cp, "sun2")) {
			machine = MACHINE_SUN2;
			machinename = "sun2";
		} else if (eq(yypvt[-0].cp, "sun3")) {
			machine = MACHINE_SUN3;
			machinename = "sun3";
		} else
			yyerror("Unknown machine type");
	      } break;
case 10:
# line 55 "config.y"
 {
		    struct cputype *cp = (struct cputype *)malloc(sizeof (struct cputype));
		    cp->cpu_name = ns(yypvt[-0].cp);
		    cp->cpu_next = cputype;
		    cputype = cp;
		    free(temp_id);
		    } break;
case 12:
# line 63 "config.y"
{ ident = ns(yypvt[-0].cp); } break;
case 13:
# line 64 "config.y"
 { mkconf(temp_id, yypvt[-0].cp); free(temp_id); } break;
case 14:
# line 65 "config.y"
 {
		yyerror("HZ specification obsolete; delete");
		hz = 60;
		} break;
case 15:
# line 69 "config.y"
 { timezone = 60 * yypvt[-0].i; check_tz(); } break;
case 16:
# line 70 "config.y"
 { timezone = 60 * yypvt[-1].i; dst = 1; check_tz(); } break;
case 17:
# line 71 "config.y"
 { timezone = yypvt[-0].i; check_tz(); } break;
case 18:
# line 72 "config.y"
 { timezone = yypvt[-1].i; dst = 1; check_tz(); } break;
case 19:
# line 73 "config.y"

	    { timezone = -60 * yypvt[-0].i; check_tz(); } break;
case 20:
# line 75 "config.y"

	    { timezone = -60 * yypvt[-1].i; dst = 1; check_tz(); } break;
case 21:
# line 77 "config.y"

	    { timezone = -yypvt[-0].i; check_tz(); } break;
case 22:
# line 79 "config.y"

	    { timezone = -yypvt[-1].i; dst = 1; check_tz(); } break;
case 23:
# line 81 "config.y"

	    { mkfile = ns(yypvt[-0].cp); } break;
case 24:
# line 83 "config.y"
 { maxusers = yypvt[-0].i; } break;
case 27:
# line 92 "config.y"
 {
		    struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		    op->op_name = ns(yypvt[-0].cp);
		    op->op_next = opt;
		    opt = op;
		    free(temp_id);
	} break;
case 28:
# line 102 "config.y"
 { yyval.cp = temp_id = ns(yypvt[-0].cp); } break;
case 29:
# line 107 "config.y"
 {
		if (machine != MACHINE_VAX)
			yyerror("wrong machine type for uba");
		yyval.cp = ns("uba");
		} break;
case 30:
# line 113 "config.y"
 {
		if (machine != MACHINE_VAX)
			yyerror("wrong machine type for mba");
		yyval.cp = ns("mba");
		} break;
case 31:
# line 119 "config.y"
 {
		if (machine != MACHINE_SUN2 && machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme16d16");
		yyval.cp = ns("vme16d16");
		} break;
case 32:
# line 125 "config.y"
 {
		if (machine != MACHINE_SUN2 && machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme24d16");
		yyval.cp = ns("vme24d16");
		} break;
case 33:
# line 131 "config.y"
 {
		if (machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme32d16");
		yyval.cp = ns("vme32d16");
		} break;
case 34:
# line 137 "config.y"
 {
		if (machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme16d32");
		yyval.cp = ns("vme16d32");
		} break;
case 35:
# line 143 "config.y"
 {
		if (machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme24d32");
		yyval.cp = ns("vme24d32");
		} break;
case 36:
# line 149 "config.y"
 {
		if (machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme32d32");
		yyval.cp = ns("vme32d32");
		} break;
case 37:
# line 154 "config.y"
 { yyval.cp = ns(yypvt[-0].cp); } break;
case 38:
# line 158 "config.y"
 {  cur.d_type = DEVICE; } break;
case 39:
# line 159 "config.y"
 {  cur.d_type = MASTER; } break;
case 40:
# line 160 "config.y"

				{  cur.d_dk = 1; cur.d_type = DEVICE; } break;
case 41:
# line 162 "config.y"
 {  cur.d_type = CONTROLLER; } break;
case 42:
# line 163 "config.y"

			{ cur.d_name = yypvt[-0].cp; cur.d_type = PSEUDO_DEVICE; } break;
case 43:
# line 165 "config.y"

			{ cur.d_name = yypvt[-1].cp; cur.d_type = PSEUDO_DEVICE;
			  cur.d_count = yypvt[-0].i; } break;
case 44:
# line 171 "config.y"
	{
			cur.d_name = yypvt[-1].cp;
			if (eq(yypvt[-1].cp, "mba"))
			    seen_mba = TRUE;
			else if (eq(yypvt[-1].cp, "uba"))
			    seen_uba = TRUE;
			cur.d_unit = yypvt[-0].i;
		} break;
case 45:
# line 182 "config.y"
 { init_dev(&cur); } break;
case 48:
# line 191 "config.y"
 {
		if (eq(cur.d_name, "mba") || eq(cur.d_name, "uba")) {
			sprintf(errbuf,
				"%s must be connected to a nexus", cur.d_name);
			yyerror(errbuf);
		}
		cur.d_conn = connect(yypvt[-1].cp, yypvt[-0].i);
	} break;
case 49:
# line 199 "config.y"
 { check_nexus(&cur, yypvt[-0].i); cur.d_conn = TO_NEXUS; } break;
case 52:
# line 209 "config.y"
{
		cur.d_addr = yypvt[-0].i;
		if (machine == MACHINE_SUN2 || machine == MACHINE_SUN3)
			bus_encode(yypvt[-0].i, &cur);
		} break;
case 53:
# line 214 "config.y"
 { cur.d_drive = yypvt[-0].i; } break;
case 54:
# line 215 "config.y"

	{
		if (cur.d_conn != NULL && cur.d_conn != TO_NEXUS
		    && cur.d_conn->d_type == MASTER)
			cur.d_slave = yypvt[-0].i;
		else
			yyerror("can't specify slave--not to master");
	} break;
case 55:
# line 223 "config.y"
 { cur.d_flags = yypvt[-0].i; } break;
case 56:
# line 228 "config.y"
 { cur.d_pri = 0; } break;
case 57:
# line 230 "config.y"
 { cur.d_pri = yypvt[-0].i; } break;
case 58:
# line 232 "config.y"
 { cur.d_pri = yypvt[-0].i; } break;
case 59:
# line 234 "config.y"
 { cur.d_pri = yypvt[-1].i; } break;
case 61:
# line 239 "config.y"
 { cur.d_vec = yypvt[-0].idlst; } break;
case 62:
# line 242 "config.y"

	    { struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	      a->id = yypvt[-0].cp; a->id_next = 0; a->vec = 0; yyval.idlst = a; } break;
case 63:
# line 245 "config.y"

	    { struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	      a->id = yypvt[-1].cp; a->id_next = 0; a->vec = yypvt[-0].i; yyval.idlst = a; } break;
case 64:
# line 248 "config.y"

	    { struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	      a->id = yypvt[-1].cp; a->id_next = yypvt[-0].idlst; a->vec = 0; yyval.idlst = a; } break;
case 65:
# line 252 "config.y"
{ struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	      a->id = yypvt[-2].cp; a->id_next = yypvt[-0].idlst; a->vec = yypvt[-1].i; yyval.idlst = a; } break;
	}
	goto yystack;  /* stack new state and value */
}
