/*	@(#)cpass1.h 1.1 86/02/03 SMI	*/


# include "machdep.h"
# include "mip.h"

/*	storage classes  */
# define SNULL 0
# define AUTO 1
# define EXTERN 2
# define STATIC 3
# define REGISTER 4
# define EXTDEF 5
# define LABEL 6
# define ULABEL 7
# define MOS 8
# define PARAM 9
# define STNAME 10
# define MOU 11
# define UNAME 12
# define TYPEDEF 13
# define FORTRAN 14
# define ENAME 15
# define MOE 16
# define UFORTRAN 17
# define USTATIC 18
	/* field size is ORed in */
# define FIELD 0100
# define FLDSIZ 077
# ifndef BUG1
extern char *scnames();
# endif

/*	location counters */
# define PROG 0
# define DATA 1
# define ADATA 2
# define STRNG 3
# define ISTRNG 4
# define STAB 5


/* symbol table flags */
# define SMOS 01
# define SHIDDEN 02
# define SHIDES 04
# define SSET 010
# define SREF 020
# define SNONUNIQ 040
# define STAG 0100
# define SPRFORWARD 0200

# ifndef FIXDEF
# define FIXDEF(p)
#endif
# ifndef FIXARG
# define FIXARG(p)
# endif
# ifndef FIXSTRUCT
# define FIXSTRUCT(a,b)
# endif

	/* alignment of initialized quantities */
# ifndef AL_INIT
#	define	AL_INIT ALINT
# endif

struct symtab {
#ifndef FLEXNAMES
	char sname[NCHNAM];
#else
	char *sname;
#endif
	TWORD stype;  /* type word */

	char sclass;  /* storage class */
	char slevel;  /* scope level */
	char sflags;  /* flags for set, use, hidden, mos, etc. */
	int offset;  /* offset or value */
	short dimoff; /* offset into the dimension table */
	short sizoff; /* offset into the size table */
	short suse;  /* line number of last use of the variable */
        int hashVal;
        struct symtab *next;
	};

# define STP(x)  ((struct symtab *)(x)) /* casting */

# ifdef ONEPASS
/* NOPREF must be defined for use in first pass tree machine */
# define NOPREF 020000  /* no preference for register assignment */
#else

union ndu {
	struct {
		int op;
		TWORD type;
		int cdim, csiz;
		}fn; /* front node */
	struct {
		int op;
		TWORD type;
		int cdim, csiz;
		NODE *left;
		NODE * right;
		}in; /* interior node */
	
	struct {
		/* this structure is the same as above,
		   but is used when a value, rather than
		   address, is kept in +left */
		int op;
		TWORD type;
		int cdim, csiz;
		CONSZ lval;
		int rval;
		}tn; /* terminal node */
	
	struct {
		/* this structure is used when a floating point constant
		   is being computed */
		int op;
		TWORD type;
		int cdim, csiz;
		double dval;
		}fpn; /* floating point node */

	};
# endif

struct sw {
	CONSZ sval;
	int slab;
	};

extern struct sw swtab[];
extern struct sw *swp;
extern int swx;

extern int ftnno;
extern int blevel;
extern int instruct, stwart;

extern int lineno, nerrors;
typedef union {
	int intval;
	NODE * nodep;
	} YYSTYPE;
extern YYSTYPE yylval;

extern CONSZ lastcon;
extern double dcon;

extern char ftitle[];
extern char ititle[];
extern struct symtab *stab[];
extern int curftn;
extern int curclass;
extern int curdim;
extern int *dimtab;
extern int paramstk[];
extern int paramno;
extern int autooff, argoff, strucoff;
extern int regvar;
extern int minrvar;
extern int brkflag;
extern char yytext[];

extern int strflg;

extern OFFSZ inoff;

extern int reached;

/*	tunnel to buildtree for name id's */

extern int idname;

extern int cflag, hflag, pflag;

/* various labels */
extern int brklab;
extern int contlab;
extern int flostat;
extern int retlab;
extern int retstat;
extern int asavbc[], *psavbc;

/*	flags used in structures/unions */

# define SEENAME 01
# define INSTRUCT 02
# define INUNION 04
# define FUNNYNAME 010
# define TAGNAME 020

/*	flags used in the (elementary) flow analysis ... */

# define FBRK 02
# define FCONT 04
# define FDEF 010
# define FLOOP 020

/*	flags used for return status */

# define RETVAL 1
# define NRETVAL 2

/*	used to mark a constant with no name field */

# define NONAME 040000

	/* mark an offset which is undefined */

# define NOOFFSET (-10201)

/*	declarations of various functions */

extern NODE
	*buildtree(),
	*bdty(),
	*mkty(),
	*rstruct(),
	*dclstruct(),
	*getstr(),
	*tymerge(),
	*stref(),
	*offcon(),
	*bcon(),
	*bpsize(),
	*convert(),
	*pconvert(),
	*oconvert(),
	*ptmatch(),
	*tymatch(),
	*makety(),
	*block(),
	*doszof(),
	*talloc(),
	*optim(),
	*strargs(),
	*clocal();

OFFSZ	tsize(),
	psize();

TWORD	types();


double atof();

char *exname(), *exdcon();

# define checkst(x)

# ifndef CHARCAST
/* to make character constants into character connstants */
/* this is a macro to defend against cross-compilers, etc. */
# define CHARCAST(x) (char)(x)
# endif

# define BCSZ 100 /* size of the table to save break and continue labels */
# define SYMTSZ 307 /* size of the symbol table (was 1300) */
# define DIMTABSZ 2048 /* size of increments of dimension/size table */
# define PARAMSZ 500 /* size of the parameter stack */
# define SWITSZ 500 /* size of switch table */
/*	special interfaces for yacc alone */
/*	These serve as abbreviations of 2 or more ops:
	ASOP	=, = ops
	RELOP	LE,LT,GE,GT
	EQUOP	EQ,NE
	DIVOP	DIV,MOD
	SHIFTOP	LS,RS
	ICOP	ICR,DECR
	UNOP	NOT,COMPL
	STROP	DOT,STREF

	*/
# define ASOP 25
# define RELOP 26
# define EQUOP 27
# define DIVOP 28
# define SHIFTOP 29
# define INCOP 30
# define UNOP 31
# define STROP 32

# define LP 50
# define RP 51
# define LC 52
# define RC 53
