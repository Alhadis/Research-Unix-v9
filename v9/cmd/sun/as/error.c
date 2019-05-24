#ifndef lint
static	char sccsid[] = "@(#)error.c 1.1 86/02/03 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "as.h"

/*
 *  routines to write error messages and warnings.
 */


char *e_messages[] = {
/* 0 */		"<unused>",
/* 1 */		"<unused>",
/* 2 */		"Invalid character",
/* 3 */		"Multiply defined symbol",
/* 4 */		"Symbol storage exceeded",
/* 5 */		"Offset too large",
/* 6 */		"Symbol too long",
/* 7 */		"Undefined symbol",
/* 8 */		"Invalid constant",
/* 9 */		"Invalid term",
/* 10 */	"Invalid operator",
/* 11 */	"Non-relocatable expression",
/* 12 */	"Wrong type for instruction",
/* 13 */	"Invalid operand",
/* 14 */	"Invalid symbol",
/* 15 */	"Invalid assignment",
/* 16 */	"Too many labels",
/* 17 */	"Invalid op-code",
/* 18 */	"Invalid entry point",
/* 19 */	"Invalid string",
/* 20 */	"Multiply defined symbol (phase error)",
/* 21 */	"Illegal .align",
/* 22 */	".Error statement",
/* 23 */	"<unused>",
/* 24 */	"<unused>",
/* 25 */	"Wrong number of operands",
/* 26 */	"Line too long",
/* 27 */	"Invalid register expression",
/* 28 */	"Invalid machine address",
/* 29 */	"<unused>",
/* 30 */	"<unused>",
/* 31 */	"Missing close-paren `)'",
/* 32 */	"<unused>",
/* 33 */	"<unused>",
/* 34 */	"<unused>",
/* 35 */	"<unused>",
/* 36 */	"<unused>",
/* 37 */	"<unused>",
/* 38 */	"<unused>",
/* 39 */	"<unused>",
/* 40 */	"<unused>",
/* 41 */	"Obsolete floating point syntax",
/* 42 */	"Bad csect",
/* 43 */	"Odd address",
/* 44 */	"Undefined L-symbol",
/* 45 */	"Invalid register list",
/* 46 */	"Unqualified forward reference",
 		0
} ;

extern char * asm_name; /* from init.c			 */

/* Sys_Error is called when a System Error occurs, that is, something is wrong
   with the assembler itself.  Explanation is a string suitable for a printf
   control string which explains the error, and Number is the value of the
   offending parameter.  This routine will not return.
*/
sys_error(Explanation,Number)
char *Explanation;
{
	fprintf(stderr, "%s: Assembler Error-- ", asm_name);
	fprintf(stderr, Explanation,Number);
	exit(-1);
}

/* This is called whenever the assembler recognizes an error in the current
statement. It registers the error, so that an error code will be listed with
the statement, and a description of the error will be printed at the end of
the listing */

prog_error(code)
register int code;
{
	errors++;			/* increment error count */
	fprintf(stderr,"%s: error (%s:%d): %s\n",asm_name,
	    source_name[current_file], line_no,e_messages[code]);
}

/* Prog_Warning registers a warning on a statement. A warning is like an error,
	in that something is probably amiss, but the assembler will still try 
	to generate the .o file. 
*/

prog_warning(code){

	if (pass != 2) return;
	fprintf(stderr,"%s: warning (%s:%d): %s\n",asm_name, source_name[current_file],
		line_no,e_messages[code]);
}
