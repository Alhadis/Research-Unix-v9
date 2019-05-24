/*
 * $Header: cppsetup.c,v 1.2 87/08/07 00:23:56 toddb Exp $
 *
 * $Log:	cppsetup.c,v $
 * Revision 1.2  87/08/07  00:23:56  toddb
 * Added ibm032 to #if line for COFF.
 * 
 * Revision 1.1  87/04/08  16:40:33  rich
 * Initial revision
 * 
 * Revision 1.3  86/09/15  17:34:42  toddb
 * Added mc68000 to the list of machines with #define COFF 128.
 * 
 * Revision 1.2  86/09/04  09:54:10  toddb
 * lookup() returned NULL if a symbol was not defined.  However, yylex()
 * expected a pointer but with a null value.  The effect was that vaxes
 * didn't work because referece through a null pointer gave a "defined"
 * result; merlins and stratos worked just fine.
 * 
 * Revision 1.1  86/04/15  08:34:15  toddb
 * Initial revision
 * 
 */
#include "def.h"

#ifdef	CPP
/*
 * This file is strictly for the sake of cpy.y and yylex.c (if
 * you indeed have the source for cpp).
 */
#define IB 1
#define SB 2
#define NB 4
#define CB 8
#define QB 16
#define WB 32
#define SALT '#'
#if pdp11 | vax | ns16000 | mc68000 | ibm032
#define COFF 128
#else
#define COFF 0
#endif
/*
 * These variables used by cpy.y and yylex.c
 */
extern char	*outp, *inp, *newp, *pend;
extern char	*ptrtab;
extern char	fastab[];
extern char	slotab[];

/*
 * cppsetup
 */
struct filepointer	*currentfile;
struct inclist		*currentinc;

cppsetup(line, filep, inc)
	register char	*line;
	register struct filepointer	*filep;
	register struct inclist		*inc;
{
	register char *p, savec;
	static boolean setupdone = FALSE;
	boolean	value;

	if (!setupdone) {
		cpp_varsetup();
		setupdone = TRUE;
	}

	currentfile = filep;
	currentinc = inc;
	inp = newp = line;
	for (p=newp; *p; p++)
		;

	/*
	 * put a newline back on the end, and set up pend, etc.
	 */
	*p++ = '\n';
	savec = *p;
	*p = '\0';
	pend = p;

	ptrtab = slotab+COFF;
	*--inp = SALT; 
	outp=inp; 
	value = yyparse();
	*p = savec;
	return(value);
}

struct symtab *lookup(symbol)
	char	*symbol;
{
	static struct symtab    undefined;
	struct symtab   *sp;

	sp = defined(symbol, currentinc);
	if (sp == NULL) {
		sp = &undefined;
		sp->s_value = NULL;
	}
	return (sp);
}

pperror(tag, x0,x1,x2,x3,x4)
	int	tag,x0,x1,x2,x3,x4;
{
	log("\"%s\", line %d: ", currentinc->i_file, currentfile->f_line);
	log(x0,x1,x2,x3,x4);
}


yyerror(s)
	register char	*s;
{
	log_fatal("Fatal error: %s\n", s);
}
#endif	CPP
