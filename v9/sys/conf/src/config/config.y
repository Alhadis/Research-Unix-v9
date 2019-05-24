%union {
	int i;
	char *cp;
	struct idlst *idlst;
}
%token CPU IDENT CONFIG ANY DEVICE UBA MBA NEXUS CSR DRIVE VECTOR OPTIONS
%token CONTROLLER PSEUDO_DEVICE FLAGS ID SEMICOLON NUMBER FPNUMBER TRACE
%token DISK SLAVE AT HZ TIMEZONE DST MAXUSERS MASTER MAKEFILE COMMA MINUS
%token MACHINE PRIORITY
%token VME16D16 VME24D16 VME32D16 VME16D32 VME24D32 VME32D32
%type <cp> Save_id ID Dev
%type <i> NUMBER FPNUMBER
%type <idlst> Id_list
%{
/*	config.y	1.11	81/05/22	*/
#include "config.h"
#include <stdio.h>
	struct device cur;
	struct device *curp = NULL;
	char *temp_id;
%}
%%
Configuration:
	Many_specs
	;

Many_specs:
	Many_specs Spec
	|
	;

Spec:
	Device_spec SEMICOLON  = { newdev(&cur); } |
	Config_spec SEMICOLON |
	TRACE SEMICOLON = { do_trace = ! do_trace; } |
	SEMICOLON |
	error SEMICOLON
	;

Config_spec:
	MACHINE Save_id
	    = {
		if (eq($2, "vax")) {
			machine = MACHINE_VAX;
			machinename = "vax";
		} else if (eq($2, "sun2")) {
			machine = MACHINE_SUN2;
			machinename = "sun2";
		} else if (eq($2, "sun3")) {
			machine = MACHINE_SUN3;
			machinename = "sun3";
		} else
			yyerror("Unknown machine type");
	      } |
	CPU Save_id = {
		    struct cputype *cp = (struct cputype *)malloc(sizeof (struct cputype));
		    cp->cpu_name = ns($2);
		    cp->cpu_next = cputype;
		    cputype = cp;
		    free(temp_id);
		    } |
	OPTIONS Opt_list |
	IDENT ID { ident = ns($2); } |
	CONFIG Save_id ID = { mkconf(temp_id, $3); free(temp_id); } |
	HZ NUMBER = {
		yyerror("HZ specification obsolete; delete");
		hz = 60;
		} |
	TIMEZONE NUMBER = { timezone = 60 * $2; check_tz(); } |
	TIMEZONE NUMBER DST = { timezone = 60 * $2; dst = 1; check_tz(); } |
	TIMEZONE FPNUMBER = { timezone = $2; check_tz(); } |
	TIMEZONE FPNUMBER DST = { timezone = $2; dst = 1; check_tz(); } |
	MINUS TIMEZONE NUMBER =
	    { timezone = -60 * $3; check_tz(); } |
	MINUS TIMEZONE NUMBER DST =
	    { timezone = -60 * $3; dst = 1; check_tz(); } |
	MINUS TIMEZONE FPNUMBER =
	    { timezone = -$3; check_tz(); } |
	MINUS TIMEZONE FPNUMBER DST =
	    { timezone = -$3; dst = 1; check_tz(); } |
	MAKEFILE ID =
	    { mkfile = ns($2); } |
	MAXUSERS NUMBER = { maxusers = $2; }
	;

Opt_list:
	Opt_list COMMA Option |
	Option
	;

Option:
	Save_id = {
		    struct opt *op = (struct opt *)malloc(sizeof (struct opt));
		    op->op_name = ns($1);
		    op->op_next = opt;
		    opt = op;
		    free(temp_id);
	}
	;

Save_id:
	ID = { $$ = temp_id = ns($1); }
	;

Dev:
	UBA
	      = {
		if (machine != MACHINE_VAX)
			yyerror("wrong machine type for uba");
		$$ = ns("uba");
		} |
	MBA
	      = {
		if (machine != MACHINE_VAX)
			yyerror("wrong machine type for mba");
		$$ = ns("mba");
		} |
	VME16D16
	      = {
		if (machine != MACHINE_SUN2 && machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme16d16");
		$$ = ns("vme16d16");
		} |
	VME24D16
	      = {
		if (machine != MACHINE_SUN2 && machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme24d16");
		$$ = ns("vme24d16");
		} |
	VME32D16
	      = {
		if (machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme32d16");
		$$ = ns("vme32d16");
		} |
	VME16D32
	      = {
		if (machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme16d32");
		$$ = ns("vme16d32");
		} |
	VME24D32
	      = {
		if (machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme24d32");
		$$ = ns("vme24d32");
		} |
	VME32D32
	      = {
		if (machine != MACHINE_SUN3)
			yyerror("wrong machine type for vme32d32");
		$$ = ns("vme32d32");
		} |
	ID = { $$ = ns($1); }
	;

Device_spec:
	DEVICE Dev_name Dev_info Int_spec = {  cur.d_type = DEVICE; } |
	MASTER Dev_name Dev_info Int_spec = {  cur.d_type = MASTER; } |
	DISK Dev_name Dev_info Int_spec =
				{  cur.d_dk = 1; cur.d_type = DEVICE; } |
	CONTROLLER Dev_name Dev_info Int_spec = {  cur.d_type = CONTROLLER; } |
	PSEUDO_DEVICE Init_dev Dev =
			{ cur.d_name = $3; cur.d_type = PSEUDO_DEVICE; } |
	PSEUDO_DEVICE Init_dev Dev NUMBER =
			{ cur.d_name = $3; cur.d_type = PSEUDO_DEVICE;
			  cur.d_count = $4; }
	;

Dev_name:
	Init_dev Dev NUMBER =	{
			cur.d_name = $2;
			if (eq($2, "mba"))
			    seen_mba = TRUE;
			else if (eq($2, "uba"))
			    seen_uba = TRUE;
			cur.d_unit = $3;
		}
	;

Init_dev:
	= { init_dev(&cur); }
	;

Dev_info:
	Con_info Info_list
	|
	;

Con_info:
	AT Dev NUMBER = {
		if (eq(cur.d_name, "mba") || eq(cur.d_name, "uba")) {
			sprintf(errbuf,
				"%s must be connected to a nexus", cur.d_name);
			yyerror(errbuf);
		}
		cur.d_conn = connect($2, $3);
	} |
	AT NEXUS NUMBER = { check_nexus(&cur, $3); cur.d_conn = TO_NEXUS; }
	;
    
Info_list:
	Info_list Info
	|
	;

Info:
	CSR NUMBER
		{
		cur.d_addr = $2;
		if (machine == MACHINE_SUN2 || machine == MACHINE_SUN3)
			bus_encode($2, &cur);
		} |
	DRIVE NUMBER = { cur.d_drive = $2; } |
	SLAVE NUMBER =
	{
		if (cur.d_conn != NULL && cur.d_conn != TO_NEXUS
		    && cur.d_conn->d_type == MASTER)
			cur.d_slave = $2;
		else
			yyerror("can't specify slave--not to master");
	} |
	FLAGS NUMBER = { cur.d_flags = $2; }
	;

Int_spec:
	Vec_spec
	      = { cur.d_pri = 0; } |
	PRIORITY NUMBER
	      = { cur.d_pri = $2; } |
	Vec_spec PRIORITY NUMBER
	      = { cur.d_pri = $3; } |
	PRIORITY NUMBER Vec_spec
	      = { cur.d_pri = $2; } |
	/* lambda */
		;

Vec_spec:
	VECTOR Id_list = { cur.d_vec = $2; };

Id_list:
	Save_id =
	    { struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	      a->id = $1; a->id_next = 0; a->vec = 0; $$ = a; } |
	Save_id NUMBER =
	    { struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	      a->id = $1; a->id_next = 0; a->vec = $2; $$ = a; } |
	Save_id Id_list =
	    { struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	      a->id = $1; a->id_next = $2; a->vec = 0; $$ = a; } |
	Save_id NUMBER Id_list
	    { struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
	      a->id = $1; a->id_next = $3; a->vec = $2; $$ = a; };

%%

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
