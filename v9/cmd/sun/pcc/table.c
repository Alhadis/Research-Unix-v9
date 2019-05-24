# include "cpass2.h"
#ifndef lint
static	char sccsid[] = "@(#)table.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

# define ANYSIGNED TPOINT|TINT|TLONG|TSHORT|TCHAR
# define ANYUSIGNED TUNSIGNED|TULONG|TUSHORT|TUCHAR
# define ANYFIXED ANYSIGNED|ANYUSIGNED
# define TWORD TINT|TUNSIGNED|TPOINT|TLONG|TULONG
# define TSCALAR TCHAR|TUCHAR|TSHORT|TUSHORT|TINT|TUNSIGNED|TPOINT

# define EA SNAME|SOREG|SCON|STARREG|STARNM|SAREG|SBREG
# define EAA SNAME|SOREG|SCON|STARREG|STARNM|SAREG
# define EB SBREG
# define ED SNAME|SOREG|SCON|SAREG 	/* for addressability of DOUBLEs */
# define ES SNAME|SOREG|STARREG|STARNM|SAREG|SBREG
# define EM SNAME|SOREG|STARREG|STARNM|SCON

struct optab table[] = {

/*
 * special-case constant assignments
 */

ASSIGN,	INAREG|FOREFF|FORCC,
	(EAA)&~SAREG,	TSCALAR|TFLOAT,
	SZERO,	TANY,
		0,	RLEFT|RRIGHT|RESCC,
		"	clrZB	AL\n",

ASSIGN,	INAREG|FOREFF|FORCC,
	SAREG|STAREG,	TSCALAR,
	SCCON,	TSCALAR,
		0,	RLEFT|RRIGHT|RESCC,
		"	moveq	AR,AL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* 
 * two's complement assignments, to anything but the address registers
 */
ASSIGN,	INAREG|FOREFF|FORCC,
	EAA,	TSCALAR,
	EA,	TSCALAR,
		0,	RLEFT|RRIGHT|RESCC,
		"	movZB	AR,AL\n",

/*
 * two's complement assignments, to the address registers
 */
ASSIGN,	INBREG|FOREFF,
	SBREG|STBREG,	TWORD|TSHORT,
	EA,		TWORD|TSHORT,
		0,	RLEFT|RRIGHT,
		"	movZB	AR,AL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Floating assignments to anything but the address registers
 * Note that the condition codes are not left in a valid state.
 */
ASSIGN,	INAREG|FOREFF,
	EAA,	TFLOAT,
	EA,	TFLOAT,
		0,	RLEFT|RRIGHT,
		"	movZB	AR,AL\n",

/*
 * Floating assignments to the address registers.
 * Note that the condition codes are not left in a valid state.
 */
ASSIGN,	INBREG|FOREFF,
	SBREG|STBREG,	TFLOAT,
	EA,	TFLOAT,
		0,	RLEFT|RRIGHT,
		"	movZB	AR,AL\n",

ASSIGN|OP68881, INBREG|FOREFF,
	SBREG|STBREG,	TFLOAT,
	SCREG|STCREG,	TFLOAT,
		NAREG,	RLEFT|RRIGHT|RESC1,
		"	fmoves	AR,A1\n	movl	A1,AL\n",

/*
 * assignments from temp aregs to anything but the a-registers
 */
ASSIGN, INTAREG|FOREFF,
	EAA,	TSCALAR|TFLOAT,
	STAREG,	TSCALAR|TFLOAT,
		0,	RRIGHT,
		"	movZB	AR,AL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Floating assignments to the fp-registers, if we have them.
 */

ASSIGN|OP68881,	INCREG|FOREFF|FORCC,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SFLOAT_SRCE,	TFLOAT|TDOUBLE,
		0,	RLEFT|RRIGHT|RESFCC,
		"	fmoveZF	ZK,AL\n",

ASSIGN|OP68881,	INCREG|FOREFF|FORCC,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
		0,	RLEFT|RRIGHT|RESFCC,
		"	fmovex	AR,AL\n",

ASSIGN|OP68881,	INCREG|FOREFF|FORCC,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SAREG|STAREG,	TDOUBLE,
		0,	RLEFT|RRIGHT|RESFCC,
"	movl	UR,sp@-\n\
	movl	AR,sp@-\n\
	fmoved	sp@+,AL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Floating stores from the fp-registers, if we have them.
 * Note: these do not affect the condition codes on either processor.
 */

ASSIGN|OP68881,	INCREG|FOREFF,
	EM,	TFLOAT|TDOUBLE,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
		0,	RLEFT|RRIGHT,
		"	fmoveZG	AR,AL\n",

ASSIGN|OP68881,	INCREG|FOREFF,
	SAREG|STAREG,	TFLOAT,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
		0,	RLEFT|RRIGHT,
		"	fmoveZG	AR,AL\n",

ASSIGN|OP68881,	INCREG|FOREFF,
	SAREG|STAREG,	TDOUBLE,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
		0,	RLEFT|RRIGHT,
"	fmoved	AR,sp@-\n\
	movl	sp@+,AL\n\
	movl	sp@+,UL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * Other floating-point assignments
 */

ASSIGN,	INAREG|FOREFF,
	ED,	TDOUBLE,
	ED,	TDOUBLE,
		0,	RLEFT|RRIGHT,
		"	movl	AR,AL\n	movl	UR,UL\n",

ASSIGN|NO68881,	INAREG|FOREFF,
	EAA,	TFLOAT,
	ED,	TDOUBLE,
		0,	RLEFT|RRIGHT,
		"Zg",

ASSIGN,	INAREG|FOREFF,
	EAA,	TDOUBLE,
	SAREG,	TDOUBLE,
		0,	RLEFT|RRIGHT,
		"ZD",

ASSIGN,	INAREG|FOREFF,
	SAREG,	TDOUBLE,
	EAA,	TDOUBLE,
		0,	RLEFT|RRIGHT,
		"ZD",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* new, fun field ops */
ASSIGN, INAREG|FOREFF,
	SPEC_FLD,		TANY,
	SAREG|STAREG,	TANY,
		NAREG,	RRIGHT,
		"Za\n", /* ie: do the whole show by hand */

ASSIGN, INAREG|FOREFF,
	SPEC_FLD,		TANY,
	SCON,		TANY,
		NAREG,	RRIGHT,
		"Za\n", /* ie: do the whole show by hand */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* put this here so UNARY MUL nodes match OPLTYPE when appropriate */
UNARY MUL,	INTAREG|INAREG|FORCC,
	SBREG,	TSCALAR,
	SANY,	TANY,
		NAREG|NASR,	RESC1|RESCC,
		"	movZB	AL@,A1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifdef FORT
GOTO,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jra	CL\n",

GOTO,	FOREFF,
	SBREG|STBREG,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"	jmp	AL@\n",
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* leaf node for effect -- evaluate and discard */

OPLTYPE,	FOREFF,
	SANY,	TANY,
	EA,	TANY,
		0,	RRIGHT,
		"Z ",   /* note that the "leaf" may have side effects!!*/

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * tests to set condition codes, integral operands
 */

OPLTYPE|OP68020,	FORCC,
	SANY,	TANY,
	(EA)&~SCON,	TWORD|TSHORT|TUSHORT,
		0,	RESCC,
		"	tstZB	AR\n",

OPLTYPE,	FORCC,
	SANY,	TANY,
	(EAA)&~SCON,	TSCALAR,
		0,	RESCC,
		"	tstZB	AR\n",

OPLTYPE,	FORCC,
	SANY,	TANY,
	EB,	TWORD|TSHORT|TUSHORT,
		0,	RESCC,
		"	cmpw	#0,AR\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * (68881) tests to set coprocessor condition codes, floating point operands
 */

OPLTYPE|OP68881,	FORCC, 
	SANY,	TANY,
	SFLOAT_SRCE,	TFLOAT|TDOUBLE,
		0,	RESFCC,
		"	ftestZF	ZK\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * (no 68881) tests to set condtion codes, floating point operands
 */
OPLTYPE|NO68881,	FORCC, 
	SANY,	TANY,
	EA,	TFLOAT,
		0,	RESCC,
		"Zf",

OPLTYPE|NO68881,	FORCC, 
	SANY,	TANY,
	ED,	TDOUBLE,
		0,	RESCC,
		"Zf",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * special-case loads, constant integral operands
 */
OPLTYPE,	INTAREG|INAREG|FORCC,
	SANY,	TANY,
	SZERO,	TSCALAR,
		NAREG|NASR,	RESC1|RESCC,
		"	moveq	#0,A1\n",

OPLTYPE,	INTAREG|INAREG|FORCC,
	SANY,	TANY,
	SCCON,	TSCALAR,
		NAREG|NASR,	RESC1|RESCC,
		"	moveq	AR,A1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * load scalar into temp d-register
 */
OPLTYPE,	INTAREG|INAREG|FORCC,
	SANY,	TANY&~TSTRUCT,
	EA,	TSCALAR,
		NAREG|NASR,	RESC1|RESCC,
		"	movZB	AR,A1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * special-case loads, float and double operands
 */
OPLTYPE,	INTAREG|INAREG,
	SANY,	TANY,
	EAA,	TDOUBLE,
		NAREG,	RESC1,
		"ZD",

OPLTYPE,	INTAREG|INAREG,
	SANY,	TANY,
	SOREG,	TDOUBLE,
		NAREG|NASL|NBREG|NBSL,	RESC1,
		"ZE",

OPLTYPE,	INTAREG|INAREG,
	SANY,	TANY,
	EA,	TFLOAT,
		NAREG|NASR,	RESC1,
		"	movl	AR,A1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * (68881) loads from various places into coprocessor registers
 */

OPLTYPE|OP68881,	INTCREG|INCREG,
	SANY,	TANY,
	SFLOAT_SRCE,	TFLOAT|TDOUBLE,
		NCREG,	RESC1,
		"	fmoveZF	ZK,A1\n",

OPLTYPE|OP68881,	INTCREG|INCREG,
	SANY,	TANY,
	SAREG|STAREG,	TDOUBLE,
		NCREG,	RESC1,
"	movl	UR,sp@-\n\
	movl	AR,sp@-\n\
	fmoved	sp@+,A1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * (68881) loads from coprocessor registers into data registers
 */

OPLTYPE|OP68881,	INTAREG|INAREG,
	SANY,	TANY,
	SCREG|STCREG,	TFLOAT,
		NAREG,	RESC1,
		"	fmoves	AR,A1\n",

OPLTYPE|OP68881,	INTAREG|INAREG,
	SANY,	TANY,
	SCREG|STCREG,	TDOUBLE,
		NAREG,	RESC1,
"	fmoved	AR,sp@-\n\
	movl	sp@+,A1\n\
	movl	sp@+,U1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * loads into address-registers
 */
OPLTYPE,	INTBREG|INBREG,
	SANY,	TANY,
	SCON,	TSCALAR,
		NBREG|NBSR,	RESC1,
		"	lea	CR,A1\n",

OPLTYPE,	INTBREG|INBREG,
	SANY,	TANY,
	EA,	TWORD|TSHORT,
		NBREG|NBSR,	RESC1,
		"	movZB	AR,A1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * stores into temp stack locations
 */
OPLTYPE,	INTEMP|FORCC,
	SANY,	TANY,
	EA,	TSCALAR,
		NTEMP,	RESC1|RESCC,
		"	movZB	AR,A1\n",

OPLTYPE,	INTEMP,
	SANY,	TANY,
	EA,	TFLOAT,
		NTEMP,	RESC1,
		"	movZB	AR,A1\n",

OPLTYPE,	INTEMP,
	SANY,	TANY,
	ED,	TDOUBLE,
		2*NTEMP,	RESC1,
		"	movl	AR,A1\n	movl	UR,U1\n",

OPLTYPE|OP68881,	INTEMP,
	SANY,	TANY,
	SCREG|STCREG,	TFLOAT,
		NTEMP,	RESC1,
		"	fmoves	AR,A1\n",

OPLTYPE|OP68881,	INTEMP,
	SANY,	TANY,
	SCREG|STCREG,	TDOUBLE,
		2*NTEMP,	RESC1,
		"	fmoved	AR,A1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * argument passing, integral types
 */

OPLTYPE,	FORARG,
	SANY,	TANY,
	SBREG,	TINT|TUNSIGNED|TPOINT,
		0,	RNULL,
		"	pea	AR@\nZP",

OPLTYPE,	FORARG,
	SANY,	TANY,
	SCON,	TSCALAR,
		0,	RNULL,
		"	pea	CR\nZP",

OPLTYPE,	FORARG,
	SANY,	TANY,
	EA,	TINT|TUNSIGNED|TPOINT,
		0,	RNULL,
		"	movl	AR,Z-\n",

OPLTYPE,	FORARG,
	SANY,	TANY,
	EA,	TSHORT,
		NBREG|NBSR,	RNULL,
		"	movw	AR,A1\n	movl	A1,Z-\n",

OPLTYPE,	FORARG,
	SANY,	TANY,
	EA,	TUSHORT,
		NAREG,		RNULL,
		"	clrl	A1\n	movw	AR,A1\n	movl	A1,Z-\n",

OPLTYPE|NO68020,	FORARG,
	SANY,	TANY,
	EA,	TCHAR,
		NAREG|NASR,	RNULL,
		"	movb	AR,A1\n	extw	A1\n	extl	A1\n	movl	A1,Z-\n",

OPLTYPE|OP68020,	FORARG,
	SANY,	TANY,
	EA,	TCHAR,
		NAREG|NASR,	RNULL,
		"	movb	AR,A1\n	extbl	A1\n	movl	A1,Z-\n",

OPLTYPE,	FORARG,
	SANY,	TANY,
	EA,	TUCHAR,
		NAREG,		RNULL,
		"	clrl	A1\n	movb	AR,A1\n	movl	A1,Z-\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * argument passing, floating types
 */

OPLTYPE|OP68881,	FORARG,
	SANY,	TANY,
	SCREG|STCREG,	TFLOAT,
		0,	RNULL,
		"Zf",		/* can't tell from here whether to convert */

OPLTYPE|OP68881,	FORARG,
	SANY,	TANY,
	SCREG|STCREG,	TDOUBLE,
		0,	RNULL,
		"	fmoved	AR,Z-\n",

OPLTYPE,	FORARG,
	SANY,	TANY,
	EA,	TFLOAT,
		0,	RNULL,
		"Zf",		/* can't tell from here whether to convert */

OPLTYPE,	FORARG,
	SANY,	TANY,
	ED,	TDOUBLE,
		0,	RNULL,
		"	movl	UR,Z-\n	movl	AR,Z-\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * field extraction, for value or condition codes
 */
FLD,		INAREG|INTAREG|FORCC,
	SANY,	TANY,
	SPEC_FLD,	TSCALAR,
		NAREG,	RESC1|RESCC,
		"Zb\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * compare integral operands, for flow control
 */

OPLOG,	FORCC,
	SAREG|STAREG|SBREG|STBREG,	TSCALAR,
	EA,	TSCALAR,
		0,	RESCC,
		"	cmpZL	AR,AL\nZI",

OPLOG,	FORCC,
	(EA)&~SCON,	TSCALAR,
	SCON,	TSCALAR,
		0,	RESCC,
		"	cmpZL	AR,AL\nZI",

OPLOG,	FORCC,
	SAUTOINC,	TSCALAR,
	SAUTOINC,	TSCALAR,
		0,	RESCC,
		"	cmpmZL	AR,AL\nZI",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * (68881) compare floating operands, for flow control.
 * Note that the 68881 has a separate condition code register and
 * requires a different set of branch instructions.
 */

OPLOG|OP68881,	FORCC,
	SFLOAT_SRCE,	TFLOAT|TDOUBLE,
	SFZERO,		TFLOAT|TDOUBLE,
		0,	RESFCC,
		"	ftestZG	AL\nZH",

OPLOG|OP68881,	FORCC,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SFLOAT_SRCE,	TFLOAT|TDOUBLE,
		0,	RESFCC,
		"	fcmpZF	ZK,AL\nZH",

OPLOG|OP68881,	FORCC,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
		0,	RESFCC,
		"	fcmpx	AR,AL\nZH",

OPLOG|OP68881,	FORCC,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SAREG|STAREG,	TDOUBLE,
		0,	RESFCC,
"	movl	UR,Z-\n\
	movl	AR,Z-\n\
	fcmpd	sp@+,AL\nZH",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * (no 68881) compare floating operands, for flow control.
 */
OPLOG|NO68881,	FORCC,
	SPEC_FLT,	TFLOAT,
	SPEC_FLT,	TFLOAT,
		0,	RESCC,
		"ZfZI",

OPLOG|NO68881,	FORCC,
	SPEC_FLT,	TDOUBLE,
	SPEC_DFLT,	TDOUBLE,
		NBREG,	RESCC,
		"ZfZI",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * compare constant to field, for flow control
 */
OPLOG, FORCC,
	SPEC_FLD,	TSCALAR,
	SCON,	TSCALAR,
		NAREG,	RESCC,
		"Zc\nZI",	/* let ME do it */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * compare integral operands  for boolean result
 */

OPLOG, INTAREG,
	SAREG|STAREG|SBREG|STBREG,	TSCALAR,
	EA,	TSCALAR,
		NAREG, RESC1,
		"	moveq	#0,A1\n	cmpZL	AR,AL\n	sI.	A1\n	negb	A1\n",	

OPLOG, INTAREG,
	(EA)&~SCON,	TSCALAR,
	SCON,	TSCALAR,
		NAREG, RESC1,
		"	moveq	#0,A1\n	cmpZL	AR,AL\n	sI.	A1\n	negb	A1\n",	

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * (68881) compare floating operands for boolean result
 */

OPLOG|OP68881,	INTAREG,
	SFLOAT_SRCE,	TFLOAT|TDOUBLE,
	SFZERO,		TFLOAT|TDOUBLE,
		NAREG,	RESC1,
"	moveq	#0,A1\n\
	ftestZG	AL\n\
	fsI.	A1\n\
	negb	A1\n",

OPLOG|OP68881,	INTAREG,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SFLOAT_SRCE,	TFLOAT|TDOUBLE,
		NAREG,	RESC1,
"	moveq	#0,A1\n\
	fcmpZF	ZK,AL\n\
	fsI.	A1\n\
	negb	A1\n",

OPLOG|OP68881,	INTAREG,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
		NAREG,	RESC1,
"	moveq	#0,A1\n\
	fcmpx	AR,AL\n\
	fsI.	A1\n\
	negb	A1\n",

OPLOG|OP68881,	INTAREG,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SAREG|STAREG,	TDOUBLE,
		NAREG,	RESC1,
"	moveq	#0,A1\n\
	movl	UR,sp@-\n\
	movl	AR,sp@-\n\
	fcmpd	sp@+,AL\n\
	fsI.	A1\n\
	negb	A1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * (no 68881) compare floating operands for boolean result
 */
OPLOG|NO68881,	INTAREG,
	SPEC_FLT,	TFLOAT,
	SPEC_FLT,	TFLOAT,
		NAREG,	RESC1,
		"	moveq	#0,A1\nZf	sfI.	A1\n	negb	A1\n",

OPLOG|NO68881,	INTAREG,
	SPEC_FLT,	TDOUBLE,
	SPEC_DFLT,	TDOUBLE,
		NBREG+NAREG,	RESC1,
		"	moveq	#0,A1\nZf	sfI.	A1\n	negb	A1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * compare constant to field, for boolean result
 */
OPLOG, INTAREG,
	SPEC_FLD,	TSCALAR,
	SCON,	TSCALAR,
		2*NAREG,	RESC2,
		"	moveq	#0,A2\nZc\n	sI.	A2\n	negb	A2\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * convert floating point condition codes into boolean result
 */
FCCODES|OP68881,	INTAREG|INAREG,
	SANY,	TANY,
	SANY,	TANY,
		NAREG,	RESC1,
		"	moveq	#1,A1\nZN",

/*
 * convert integer condition codes into boolean result
 */
CCODES,	INTAREG|INAREG,
	SANY,	TANY,
	SANY,	TANY,
		NAREG,	RESC1,
		"	moveq	#1,A1\nZN",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * negate scalar in temp register
 */
UNARY MINUS,	INAREG|INTAREG,
	STAREG,	TSCALAR,
	SANY,	TANY,
		0,	RLEFT,
		"	negZB	AL\n",

/*
 * negate floating operand in temp data register.
 */
UNARY MINUS,	INAREG|INTAREG,
	STAREG,	TFLOAT|TDOUBLE,
	SANY,	TANY,
		0,	RLEFT,
		"Zf",

UNARY MINUS,	INAREG|INTAREG,
	EA&~STAREG,	TFLOAT,
	SANY,	TANY,
		NAREG|NASL,	RESC1,
		"	movl	AL,A1\n	bchg	#31,A1\n",

UNARY MINUS,	INAREG|INTAREG,
	EA&~STAREG,	TDOUBLE,
	SANY,	TANY,
		NAREG|NASL,	RESC1,
		"	movl	AL,A1\n	movl	UL,U1\n	bchg	#31,A1\n",

/*
 * negate floating operand in floating point register
 */
(UNARY MINUS)|OP68881,	INCREG|INTCREG,
	SFLOAT_SRCE,	TFLOAT|TDOUBLE,
	SANY,	TANY,
		NCREG|NCSL,	RESC1,
		"	fnegZF	ZK,A1\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * absolute value in temp data register
 */
FABS,	INAREG|INTAREG,
	STAREG,	TFLOAT|TDOUBLE,
	SANY,	TANY,
		0,	RLEFT,
		"	bclr	#31,AL\n",

FABS,	INAREG|INTAREG,
	EA&~STAREG,	TFLOAT,
	SANY,	TANY,
		NAREG|NASL,	RESC1,
		"	movl	AL,A1\n	bclr	#31,A1\n",

FABS,	INAREG|INTAREG,
	EA&~STAREG,	TDOUBLE,
	SANY,	TANY,
		NAREG|NASL,	RESC1,
		"	movl	AL,A1\n	movl	UL,U1\n	bclr	#31,A1\n",

/*
 * floating=>integer conversion -- at present, these are
 * only generated by FORTRAN and Pascal -- but see also
 * SCONV below.
 */
FNINT|OP68881,	INAREG|INTAREG,
	STCREG,	TFLOAT|TDOUBLE,
	SANY,	TSCALAR,
		NAREG,	RESC1,
		"Zf",

FNINT|NO68881,	INAREG|INTAREG,
	STAREG,	TFLOAT|TDOUBLE,
	SANY,	TSCALAR,
		0,	RLEFT,
		"Zf",

/*
 * floating point intrinsics (sin, cos, exp, log, ...)
 */
OPINTR|OP68881,	INCREG|INTCREG,
	SFLOAT_SRCE,	TFLOAT|TDOUBLE,
	SANY,	TANY,
		NCREG|NCSL,	RESC1,
		"Zf",

OPINTR|OP68881,	INCREG|INTCREG,
	SAREG|STAREG,	TDOUBLE,
	SANY,	TANY,
		NCREG|NCSL,	RESC1,
		"Zf",

OPINTR|NO68881,	INAREG|INTAREG,
	STAREG,	TFLOAT|TDOUBLE,
	SANY,	TANY,
		0,	RLEFT,
		"Zf",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * one's complement of temp d-register
 */
COMPL,	INTAREG|FORCC,
	STAREG,	TSCALAR,
	SANY,	TANY,
		0,	RLEFT|RESCC,
		"	notZB	AL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * increment/decrement operators
 */

INCR,	INTAREG|INAREG|FOREFF,
	EAA,	TSCALAR,
	S8CON,	TSCALAR,
		NAREG,	RESC1,
		"F	movZB	AL,A1\n	addqZB	AR,AL\nZv",

DECR,	INTAREG|INAREG|FOREFF,
	EAA,	TSCALAR,
	S8CON,	TSCALAR,
		NAREG,	RESC1,
		"F	movZB	AL,A1\n	subqZB	AR,AL\nZv",

INCR,	INTAREG|INAREG|FOREFF,
	EAA,	TSCALAR,
	SCON,	TSCALAR,
		NAREG,	RESC1,
		"F	movZB	AL,A1\n	addZB	AR,AL\nZv",

DECR,	INTAREG|INAREG|FOREFF,
	EAA,	TSCALAR,
	SCON,	TSCALAR,
		NAREG,	RESC1,
		"F	movZB	AL,A1\n	subZB	AR,AL\nZv",

INCR,	INTBREG|INBREG|FOREFF,
	EB,	TSCALAR,
	S8CON,	TSCALAR,
		NBREG,	RESC1,
		"F	movZB	AL,A1\n	addqZB	AR,AL\n",

DECR,	INTBREG|INBREG|FOREFF,
	EB,	TSCALAR,
	S8CON,	TSCALAR,
		NBREG,	RESC1,
		"F	movZB	AL,A1\n	subqZB	AR,AL\n",

INCR,	INTBREG|INBREG|FOREFF,
	EB,	TSCALAR,
	SCON,	TSCALAR,
		NBREG,	RESC1,
		"F	movZB	AL,A1\n	addZB	AR,AL\n",

DECR,	INTBREG|INBREG|FOREFF,
	EB,	TSCALAR,
	SCON,	TSCALAR,
		NBREG,	RESC1,
		"F	movZB	AL,A1\n	subZB	AR,AL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * address expressions ( base + offset [+ index] )
 */
PLUS,		INBREG|INTBREG,
	SBREG,	TPOINT,
	SSCON,	TANY,
		NBREG|NBSL,	RESC1,
		"	lea	AL@(ZO),A1\n",

PLUS,		INBREG|INTBREG,
	SBREG,	TPOINT,
	SAREG|SBREG,	TWORD|TSHORT,
		NBREG|NBSL,	RESC1,
		"Zl",

PLUS,		INBREG|INTBREG,
	SBASE,	TPOINT,
	SXREG,	TWORD|TSHORT,
		NBREG|NBSL,	RESC1,
		"	lea	ZX,A1\n",

PLUS,		FORARG,
	SBREG,	TPOINT,
	SSCON,	TANY,
		0,	RNULL,
		"	pea	AL@(ZO)\nZP",

PLUS,		FORARG,
	SBREG,	TPOINT,
	SAREG,	TWORD|TSHORT,
		0,	RNULL,
		"	pea	AL@(0,AR:ZR)\nZP",

PLUS,		FORARG,
	SBASE,	TPOINT,
	SXREG,	TWORD|TSHORT,
		0,	RNULL,
		"	pea	ZX\nZP",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

MINUS,		INBREG|INTBREG,
	SBREG,	TPOINT,
	SSCON,	TANY,
		NBREG|NBSL,	RESC1,
		"	lea	AL@(ZM),A1\n",

MINUS,		FORARG,
	SBREG,	TPOINT,
	SSCON,	TANY,
		0,	RNULL,
		"	pea	AL@(ZM)\nZP",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ASG PLUS,	INAREG|FORCC,
	EAA,	TSCALAR,
	S8CON,	TSCALAR,
		0,	RLEFT|RESCC,
		"	addqZB	AR,AL\nZv",

ASG PLUS,	INBREG,
	EB,	TSCALAR,
	S8CON,	TSCALAR,
		0,	RLEFT|RESCC,
		"	addqw	AR,AL\n",

ASG PLUS,	INAREG|FORCC,
	SAREG|STAREG,	TSCALAR,
	EAA,	TSCALAR,
		0,	RLEFT|RESCC,
		"	addZB	AR,AL\nZv",

ASG PLUS,	INAREG|FORCC,
	SAREG|STAREG,	TWORD|TSHORT,
	EB,	TWORD|TSHORT,
		0,	RLEFT|RESCC,
		"	addZB	AR,AL\nZv",

ASG PLUS,	INBREG,
	SBREG|STBREG,	TSCALAR,
	SICON,	TANY,
		0,	RLEFT,
		"	lea	AL@(CR),AL\n",

ASG PLUS,	INBREG,
	SBREG|STBREG,	TSCALAR,
	EA,	TWORD|TSHORT,
		0,	RLEFT,
		"	addZR	AR,AL\n",

ASG PLUS,	INAREG|FORCC,
	EAA,	TSCALAR,
	SAREG|STAREG,	TSCALAR,
		0,	RLEFT|RESCC,
		"	addZB	AR,AL\nZv",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ASG MINUS,	INAREG|FORCC,
	EAA,	TSCALAR,
	S8CON,	TSCALAR,
		0,	RLEFT|RESCC,
		"	subqZB	AR,AL\nZv",

ASG MINUS,	INBREG,
	EB,	TSCALAR,
	S8CON,	TSCALAR,
		0,	RLEFT|RESCC,
		"	subqw	AR,AL\n",

ASG MINUS,	INBREG,
	SBREG|STBREG,	TSCALAR,
	SICON,	TANY,
		0,	RLEFT,
		"	lea	AL@(ZM),AL\n",

ASG MINUS,	INAREG|FORCC,
	SAREG|STAREG,	TSCALAR,
	EAA,	TSCALAR,
		0,	RLEFT|RESCC,
		"	subZB	AR,AL\nZv",

ASG MINUS,	INAREG|FORCC,
	SAREG|STAREG,	TWORD|TSHORT,
	EB,	TWORD|TSHORT,
		0,	RLEFT|RESCC,
		"	subZB	AR,AL\nZv",

ASG MINUS,	INBREG,
	SBREG|STBREG,	TSCALAR,
	EA,	TWORD|TSHORT,
		0,	RLEFT,
		"	subZR	AR,AL\n",

ASG MINUS,	INAREG|FORCC,
	EAA,	TSCALAR,
	SAREG|STAREG,	TSCALAR,
		0,	RLEFT|RESCC,
		"	subZB	AR,AL\nZv",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ASG ER, 	INAREG|FORCC,
	EAA,	TSCALAR,
	SCON,	TSCALAR,
		0,	RLEFT|RESCC,
		"	eorZB	AR,AL\n",

ASG ER, 	INAREG|FORCC,
	EAA,	TSCALAR,
	SAREG|STAREG,	TSCALAR,
		0,	RLEFT|RESCC,
		"	eorZB	AR,AL\nZv",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ASG OPSIMP, 	INAREG|FORCC,
	SAREG|STAREG,	TSCALAR,
	EAA,	TSCALAR,
		0,	RLEFT|RESCC,
		"	OIZB	AR,AL\nZv",

ASG OPSIMP, 	INAREG|FORCC,
	EAA,	TSCALAR,
	SCON,	TSCALAR,
		0,	RLEFT|RESCC,
		"	OIZB	AR,AL\nZv",

ASG OPSIMP, 	INAREG|FORCC,
	EAA,	TSCALAR,
	SAREG|STAREG,	TSCALAR,
		0,	RLEFT|RESCC,
		"	OIZB	AR,AL\nZv",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * floating ASG binary operators
 */

(ASG OPFLOAT)|OP68881,	INCREG|INTCREG|FOREFF,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SFLOAT_SRCE,	TFLOAT|TDOUBLE,
		0,	RLEFT,
		"	OFZF	ZK,AL\n",

(ASG OPFLOAT)|OP68881,	INCREG|INTCREG|FOREFF,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
		0,	RLEFT,
		"	OFx	AR,AL\n",

(ASG OPFLOAT)|NO68881,	INAREG|INTAREG|FOREFF,
	SPEC_FLT,	TFLOAT,
	SPEC_FLT,	TFLOAT,
		0,	RLEFT,
		"Zf",

(ASG OPFLOAT)|NO68881,	INAREG|INTAREG|FOREFF,
	SPEC_FLT,	TDOUBLE,
	SPEC_DFLT,	TDOUBLE,
		NBREG,	RLEFT,
		"Zf",

(ASG PLUS)|NO68881,	INAREG|INTAREG|FOREFF,
	SPEC_FLT,	TFLOAT,
	SPEC_PVT,	TFLOAT,
		0,	RLEFT,
		"Zf",

(ASG PLUS)|NO68881,	INAREG|INTAREG|FOREFF,
	SPEC_FLT,	TDOUBLE,
	SPEC_PVT,	TDOUBLE,
		NBREG,	RLEFT,
		"Zf",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * floating binary operators
 */

OPFLOAT|NO68881,	INAREG|INTAREG,
	SPEC_FLT,	TFLOAT,
	SPEC_FLT,	TFLOAT,
		NAREG|NASL|NASR,	RESC1,
		"Zf",

OPFLOAT|NO68881,	INAREG|INTAREG,
	SPEC_FLT,	TDOUBLE,
	SPEC_DFLT,	TDOUBLE,
		NAREG|NBREG|NASL|NASR,	RESC1,
		"Zf",

PLUS|NO68881,	INAREG|INTAREG,
	SPEC_FLT,	TFLOAT,
	SPEC_PVT,	TFLOAT,
		NAREG|NASL|NASR,	RESC1,
		"Zf",

PLUS|NO68881,	INAREG|INTAREG,
	SPEC_FLT,	TDOUBLE,
	SPEC_PVT,	TDOUBLE,
		NAREG|NBREG|NASL|NASR,	RESC1,
		"Zf",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ASG MUL,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	TCHAR,
	SCON,		TANY,
		NAREG,	RLEFT,
		"	extw	AL\nZm",

ASG MUL,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	TUCHAR,
	SCON,		TANY,
		NAREG,	RLEFT,
		"	andw	#255,AL\nZm",

ASG MUL,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	TWORD|TSHORT|TUSHORT,
	SCON,		TANY,
		NAREG,	RLEFT,
		"Zm",

ASG MUL,	INAREG|FORCC|FOREFF,
	SAREG|STAREG,	TSHORT,
	EAA,	TSHORT,
		0,	RLEFT|RESCC,
		"	muls	AR,AL\n",

ASG MUL,	INAREG|FORCC|FOREFF,
	SAREG|STAREG,	TUSHORT,
	EAA,	TUSHORT|TSHORT,
		0,	RLEFT|RESCC,
		"	mulu	AR,AL\n",

ASG MUL,	INAREG|FORCC|FOREFF,
	SAREG|STAREG,	TSHORT,
	EAA,	TUSHORT,
		0,	RLEFT|RESCC,
		"	mulu	AR,AL\n",

ASG MUL,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	TCHAR,
	EAA,	TCHAR,
		NAREG,	RLEFT,
		"\textw	AL\n\tmovb	AR,A1\n\textw	A1\n\tmuls	A1,AL\n",

ASG MUL,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	TUCHAR,
	EAA,	TUCHAR|TCHAR,
		NAREG,	RLEFT,
		"\tandw	#255,AL\n\tclrw	A1\n\tmovb	AR,A1\n\tmuls	A1,AL\n",

(ASG MUL)|NO68020,	INTAREG|FOREFF,
	STAREG,	TPOINT|TLONG|TINT,
	STAREG,	TPOINT|TLONG|TINT,
		0,	RLEFT,
		"	jsr	lmult\nZv",

(ASG MUL)|NO68020,	INTAREG|FOREFF,
	STAREG,	TWORD,
	STAREG,	TWORD,
		0,	RLEFT,
		"	jsr	ulmult\nZv",

(ASG MUL)|OP68020,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	TPOINT|TLONG|TINT,
	EAA,	TPOINT|TLONG|TINT,
		0,	RLEFT,
		"	mulsl	AR,AL\nZv",

(ASG MUL)|OP68020,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	TWORD,
	EAA,	TWORD,
		0,	RLEFT,
		"	mulul	AR,AL\nZv",

(ASG MUL)|NO68881,	INAREG|FOREFF,
	STAREG,	TFLOAT,
	STAREG,	TFLOAT,
		0,	RLEFT,
		"Zf",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ASG DIV,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	ANYSIGNED,
	SCON,	TANY,
		0,	RLEFT|RNULL,
		"Zd",

ASG DIV,	INAREG|INTAREG|FORCC|FOREFF,
	SAREG|STAREG,	TSHORT,
	EAA,	TSHORT,
		0,	RLEFT|RESCC,
		"	extl	AL\n	divs	AR,AL\n",

ASG DIV,	INAREG|INTAREG|FORCC|FOREFF,
	SAREG|STAREG,	TUSHORT,
	EAA,	TUSHORT|TSHORT,
		0,	RLEFT|RESCC,
		"	andl	#0xffff,AL\n	divu	AR,AL\n",

ASG DIV,	INAREG|INTAREG|FORCC|FOREFF,
	SAREG|STAREG,	TSHORT,
	EAA,	TUSHORT,
		0,	RLEFT|RESCC,
		"	andl	#0xffff,AL\n	divu	AR,AL\n",

(ASG DIV)|NO68020,	INTAREG|INAREG|FOREFF,
	SAREG|STAREG,	TCHAR,
	EAA,	TCHAR,
		NAREG,	RLEFT,
"	extw	AL\n\
	extl	AL\n\
	movb	AR,A1\n\
	extw	A1\n\
	divs	A1,AL\n",

(ASG DIV)|OP68020,	INTAREG|INAREG|FOREFF,
	SAREG|STAREG,	TCHAR,
	EAA,	TCHAR,
		NAREG,	RLEFT,
"	extbl	AL\n\
	movb	AR,A1\n\
	extw	A1\n\
	divs	A1,AL\n",

ASG DIV,	INTAREG|INAREG|FOREFF,
	SAREG|STAREG,	TUCHAR,
	EAA,	TUCHAR|TCHAR,
		NAREG,	RLEFT,
		"\tandw	#255,AL\n\tclrw	A1\n\tmovb	AR,A1\n\tdivs	A1,AL\n",

(ASG DIV)|NO68020,	INTAREG|FOREFF,
	STAREG,	ANYSIGNED,
	STAREG,	ANYSIGNED,
		0,	RLEFT,
		"	jsr	ldivt\n",

(ASG DIV)|NO68020,	INTAREG|FOREFF,
	STAREG,	TWORD,
	STAREG,	TWORD,
		0,	RLEFT,
		"	jsr	uldivt\n",

(ASG DIV)|OP68020,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	ANYSIGNED,
	EAA,	ANYSIGNED,
		0,	RLEFT,
		"	divsl	AR,AL\n",

(ASG DIV)|OP68020,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	TWORD,
	EAA,	TWORD,
		0,	RLEFT,
		"	divul	AR,AL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * <areg> %= <constant, not a power of 2>.
 * This case has two templates before the "Zr" case, so that we can
 * generate a div.ll instruction with the remainder returned in RESC1
 * instead of RLEFT, thus saving a register-register move. whoop-te-doo
 */

(ASG MOD)|OP68020,	INAREG|INTAREG,
	STAREG,	TINT|TLONG,
	SNONPOW2,	TANY,
		NAREG,	RESC1,
		"	divsll	AR,A1:AL\n",

(ASG MOD)|OP68020,	INAREG|INTAREG,
	STAREG,	TUNSIGNED|TULONG,
	SNONPOW2,	TANY,
		NAREG,	RESC1,
		"	divull	AR,A1:AL\n",

ASG MOD,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	ANYFIXED,
	SCON,	TANY,
		NAREG,	RLEFT|RNULL,
		"Zr",

ASG MOD,	INTAREG|INAREG|FOREFF,
	SAREG|STAREG,	TSHORT,
	EAA,	TSHORT,
		0,	RLEFT,
		"	extl	AL\n	divs	AR,AL\n	swap	AL\n",

ASG MOD,	INTAREG|INAREG|FOREFF,
	SAREG|STAREG,	TUSHORT,
	EAA,	TUSHORT|TSHORT,
		0,	RLEFT,
		"	andl	#65535,AL\n	divu	AR,AL\n	swap	AL\n",

ASG MOD,	INTAREG|INAREG|FOREFF,
	SAREG|STAREG,	TSHORT,
	EAA,	TUSHORT,
		0,	RLEFT,
		"	andl	#65535,AL\n	divu	AR,AL\n	swap	AL\n",

ASG MOD,	INTAREG|INAREG|FOREFF,
	SAREG|STAREG,	TCHAR,
	EAA,	TCHAR,
		NAREG,	RLEFT,
"	extw	AL\n\
	movb	AR,A1\n\
	extw	A1\n\
	divs	A1,AL\n\
	swap	AL\n",

ASG MOD,	INTAREG|INAREG|FOREFF,
	SAREG|STAREG,	TUCHAR,
	EAA,	TUCHAR|TCHAR,
		NAREG,	RLEFT,
		"\tandw	#255,AL\n\tclrw	A1\n\tmovb	AR,A1\n\tdivs	A1,AL\n	swap	AL\n",

(ASG MOD)|NO68020,	INTAREG|FOREFF,
	STAREG,	ANYSIGNED,
	STAREG,	ANYSIGNED,
		0,	RLEFT,
		"	jsr	lmodt\n",

(ASG MOD)|NO68020,	INTAREG|FOREFF,
	STAREG,	TWORD,
	STAREG,	TWORD,
		0,	RLEFT,
		"	jsr	ulmodt\n",

(ASG MOD)|OP68020,	INAREG|INTAREG,
	STAREG,	ANYSIGNED,
	EAA,	ANYSIGNED,
		NAREG|NASR,	RESC1,
		"	divsll	AR,A1:AL\n",

(ASG MOD)|OP68020,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	ANYSIGNED,
	EAA,	ANYSIGNED,
		NAREG|NASR,	RLEFT,
		"	divsll	AR,A1:AL\n	movl	A1,AL\n",

(ASG MOD)|OP68020,	INAREG|INTAREG,
	STAREG,	TWORD,
	EAA,	TWORD,
		NAREG|NASR,	RESC1,
		"	divull	AR,A1:AL\n",

(ASG MOD)|OP68020,	INAREG|INTAREG|FOREFF,
	SAREG|STAREG,	TWORD,
	EAA,	TWORD,
		NAREG|NASR,	RLEFT,
		"	divull	AR,A1:AL\n	movl	A1,AL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

ASG OPSHFT, 	FOREFF,
	SNAME|SOREG|STARREG,	TSHORT,
	SONE,	TSCALAR,
		0,	RLEFT,
		"	aOIw	AL\n",

ASG OPSHFT, 	INAREG|FOREFF,
	SAREG,	TINT|TSHORT|TCHAR,
	S8CON,	TSCALAR,
		0,	RLEFT,
		"	aOIZB	AR,AL\n",

ASG OPSHFT, 	INAREG|FOREFF,
	SAREG,	TINT|TSHORT|TCHAR,
	SAREG,	TSCALAR,
		0,	RLEFT,
		"	aOIZB	AR,AL\n",

/* ASG OPSHFT, 	INAREG|FOREFF, */
ASG OPSHFT, 	FOREFF,
	SNAME|SOREG|STARREG,	TUSHORT,
	SONE,	TSCALAR,
		0,	RLEFT,
		"	lOIw	AL\n",

ASG OPSHFT, 	INAREG|FOREFF,
	SAREG,	TUNSIGNED|TUSHORT|TUCHAR,
	S8CON,	TSCALAR,
		0,	RLEFT,
		"	lOIZB	AR,AL\n",

ASG OPSHFT, 	INAREG|FOREFF,
	SAREG,	TUNSIGNED|TUSHORT|TUCHAR,
	SAREG,	TSCALAR,
		0,	RLEFT,
		"	lOIZB	AR,AL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

UNARY CALL,	INTAREG,
	SBREG|SNAME|SOREG|SCON|SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1, /* should be register 0 */
		"ZC\n",

CHK,		INAREG,
	SAREG,	TSHORT|TWORD,
	SZEROLB,	TANY,
		0,	RLEFT,
		"ZV",

CHK,		INTAREG,
	STAREG,	TSCALAR,
	SANY,	TANY,
		0,	RLEFT,
		"ZV",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * conversions, from and to integer types
 */
SCONV,	INTAREG,
	STAREG,	TINT|TUNSIGNED|TPOINT,
	SANY,	TINT|TUNSIGNED|TPOINT,
		0,	RLEFT,
		"",

SCONV,	INAREG|INTAREG,
	STAREG,	TINT|TUNSIGNED|TPOINT,
	SANY,	TSHORT|TCHAR|TUSHORT|TUCHAR,
		0,	RLEFT,
		"",

SCONV,	INAREG|INTAREG,
	SAREG,	TINT|TUNSIGNED|TPOINT,
	SANY,	TSHORT|TCHAR|TUSHORT|TUCHAR,
		NAREG|NASL,	RESC1,
		"	movZB	AL,A1\n",

SCONV,	INAREG|INTAREG,
	STAREG,	TCHAR,
	SANY,	TSHORT|TUSHORT,
		0,	RLEFT,
		"	extw	AL\n",

(SCONV)|NO68020,	INAREG|INTAREG,
	STAREG,	TCHAR,
	SANY,	TINT|TUNSIGNED|TPOINT,
		0,	RLEFT,
		"	extw	AL\n	extl	AL\n",

(SCONV)|OP68020,	INAREG|INTAREG,
	STAREG,	TCHAR,
	SANY,	TINT|TUNSIGNED|TPOINT,
		0,	RLEFT,
		"	extbl	AL\n",

SCONV,	INAREG|INTAREG,
	STAREG,	TSHORT,
	SANY,	TINT|TUNSIGNED|TPOINT,
		0,	RLEFT,
		"	extl	AL\n",

SCONV,	INAREG|INTAREG,
	ES,	TUCHAR,
	SANY,	TSCALAR,
		NAREG|NASL,	RESC1,
		"Zt",

SCONV,	INAREG|INTAREG,
	ES,	TUSHORT,
	SANY,	TWORD,
		NAREG|NASL,	RESC1,
		"Zt",

/* icky conversions into B registers -- for immediate use as a pointer */
SCONV,	INBREG|INTBREG,
	STAREG,	TSHORT,
	SANY,	TWORD,
		NBREG,	RESC1,
		"	movw	AL,A1\n",

SCONV,	INBREG|INTBREG,
	STAREG,	TUSHORT,
	SANY,	TWORD,
		NBREG,	RESC1,
		"	andl	#0xffff,AL\n	movl	AL,A1\n",

SCONV,	INBREG|INTBREG,
	STAREG,	TCHAR,
	SANY,	TWORD,
		NBREG,	RESC1,
		"	extw	AL\n	movw	AL,A1\n",

SCONV,	INBREG|INTBREG,
	STAREG,	TUCHAR,
	SANY,	TWORD,
		NBREG,	RESC1,
		"	andl	#0xff,AL\n	movl	AL,A1\n",
/* end icky */

SCONV,	INAREG|INTAREG,
	SNAME|SOREG|SAREG,	TINT|TUNSIGNED|TPOINT|TSHORT|TUSHORT,
	SANY,	TSHORT|TUSHORT|TCHAR|TUCHAR,
		0,	RLEFT,
		"ZT",

SCONV,	INAREG|INTAREG,
	SBREG,	TINT|TUNSIGNED|TPOINT|TSHORT|TUSHORT,
	SANY,	TSHORT|TUSHORT,
		0,	RLEFT,
		"ZT",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * floating point conversions
 */

SCONV|OP68881,	INCREG|INTCREG,
	EAA,	ANYSIGNED|TFLOAT,
	SANY,	TFLOAT|TDOUBLE,
		NCREG,		RESC1,
		"	fmoveZG	AL,A1\n",

SCONV|OP68881,	INCREG|INTCREG,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SANY,	TDOUBLE,
		0,		RLEFT,
		"",

/* conversions into the d-registers, for function results etc */
 
SCONV|OP68881,	INTAREG|INTAREG,
	SCREG|STCREG,	TFLOAT|TDOUBLE,
	SANY,	TFLOAT,
		NAREG,	RESC1,
		"	fmoveZ.	AL,A1\n",

SCONV|OP68881,	INTAREG|INTAREG,
	SCREG|STCREG,	TFLOAT,
	SANY,	TDOUBLE,
		NAREG,	RESC1,
"	fmoveZ.	AL,sp@-\n\
	movl	sp@+,A1\n\
	movl	sp@+,U1\n",

/*
 * conversion from float|double to integer
 */
SCONV|OP68881,	INTAREG,
	STCREG, TDOUBLE|TFLOAT,
	SANY,	TSCALAR,
		NAREG,	RESC1,
"	fintrzx	AL,AL\n\
	fmovel	AL,A1\n",

SCONV|NO68881,	INAREG|INTAREG,
	STAREG,	TDOUBLE,
	SANY,	TFLOAT,
		NAREG|NASL,	RESC1,
		"Zg",

SCONV|NO68881,	INTAREG|INTAREG,
	STAREG,TFLOAT,
	SANY,	TDOUBLE,
		NAREG|NASL,	RESC1,
		"Zg",

SCONV|NO68881,	INTAREG,
	STAREG, TDOUBLE|TFLOAT,
	SANY,	TSCALAR,
		NAREG|NASL,	RESC1,
		"Zg",

SCONV|NO68881,	INTAREG,
	STAREG, TSCALAR,
	SANY,	 TDOUBLE|TFLOAT,
		NAREG|NASL,	RESC1,
		"Zg",

SCONV,	INAREG,
	STAREG|SAREG, TDOUBLE,
	SANY,	 	TDOUBLE,
		0,	RLEFT,
		"",

SCONV,	INAREG,
	STAREG|SAREG, TFLOAT,
	SANY,	 	TFLOAT,
		0,	RLEFT,
		"",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * structure assignments
 */

STASG,	FOREFF,
	STBREG,				TANY,
	STBREG,				TANY,
		NAREG,			RNULL,
		"ZS",

STASG, INBREG|INTBREG|FOREFF,
	SNAME|SOREG|SBREG|STBREG,	TANY,
	STBREG,				TANY,
		NAREG|NBREG|NBSL,	RRIGHT,
		"ZS",

/* never needs a temp register on the left */
STASG, INBREG|INTBREG|FOREFF,
	STBREG,				TANY,
	SBREG|STBREG,			TANY,
		NAREG|NBREG|NBSR,	RRIGHT,
		"ZS",

/* last resort -- uses more registers */
STASG, INBREG|INTBREG|FOREFF,
	SNAME|SOREG|SBREG|STBREG,	TANY,
	SBREG|STBREG,			TANY,
		NAREG|2*NBREG|NBSR,	RRIGHT,
		"ZS",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * structure arguments
 */

STARG,	FORARG,
	STBREG,				TPOINT,
	SANY,				TANY,
		NAREG|NBREG,		RNULL,
		"ZS",

STARG,	FORARG,
	SNAME|SOREG,			TSTRUCT,
	SANY,				TANY,
		NAREG|2*NBREG,		RNULL,
		"ZS",

STARG,	FORARG,
	SBREG|STBREG,			TPOINT,
	SANY,				TANY,
		NAREG|2*NBREG,		RNULL,
		"ZS",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
 * initializations - constants only
 */
INIT,	FOREFF,
	SCON,	TANY,
	SANY,	TINT|TLONG|TUNSIGNED|TULONG|TPOINT,
		0,	RNOP,
		"	.long	CL\n",

INIT,	FOREFF,
	SCON,	TANY,
	SANY,	TSHORT|TUSHORT,
		0,	RNOP,
		"	.word	CL\n",

INIT,	FOREFF,
	SCON,	TANY,
	SANY,	TCHAR|TUCHAR,
		0,	RNOP,
		"	.byte	CL\n",

INIT,	FOREFF,
	SCON,	TANY,
	SANY,	TFLOAT|TDOUBLE,
		0,	RNOP,
		"	.long	CL\n",

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

	/* Default actions for hard trees ... */

# define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,""

UNARY MUL, DF( UNARY MUL ),

INCR, DF(INCR),

DECR, DF(INCR),

ASSIGN, DF(ASSIGN),

STASG, DF(STASG),

OPLEAF, DF(NAME),

OPLOG,	FORCC,
	SANY,	TANY,
	SANY,	TANY,
		REWRITE,	BITYPE,
		"",

/*OPLOG,	DF(NOT),*/
OPLOG,	DF(BITYPE),
COMOP, DF(COMOP),
INIT, DF(INIT),
FLD, DF(FLD),
GOTO,DF(GOTO),
SCONV, DF(SCONV),
OPUNARY, DF(UNARY MINUS),
ASG OPANY, DF(ASG PLUS),
OPANY, DF(BITYPE),
FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	"help; I'm in trouble\n" };
