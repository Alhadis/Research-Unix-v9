#ifndef lint
static	char sccsid[] = "@(#)error.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "as.h"

/*
 *  routines to write error messages and warnings.
 */


char *e_messages[] = {
/* E_NOERROR	 */	"<unused>",
/* E_BADCHAR	 */	"Invalid character",
/* E_MULTSYM	 */	"Multiply defined symbol",
/* E_NOSPACE	 */	"Symbol storage exceeded",
/* E_OFFSET	 */	"Offset too large",
/* E_SYMLEN	 */	"Symbol too long",
/* E_SYMDEF	 */	"Undefined symbol",
/* E_CONSTANT	 */	"Invalid constant",
/* E_TERM	 */	"Invalid term",
/* E_OPERATOR	 */	"Invalid operator",
/* E_RELOCATE	 */	"Non-relocatable expression",
/* E_TYPE	 */	"Wrong type for instruction",
/* E_OPERAND	 */	"Invalid operand",
/* E_SYMBOL	 */	"Invalid symbol",
/* E_EQUALS	 */	"Invalid assignment",
/* E_NLABELS	 */	"Too many labels",
/* E_OPCODE	 */	"Invalid op-code",
/* E_STRING	 */	"Invalid string",
/* E_PHASE	 */	"Unacceptable relocatable expression",
/* E_NUMOPS	 */	"Wrong number of operands",
/* E_LINELONG	 */	"Line too long",
/* E_REG	 */	"Invalid register expression",
/* E_IADDR 	 */	"Invalid machine address",
/* E_PAREN	 */	"Missing close-paren `)'",
/* E_ODDADDR     */	"Odd address",
/* E_UNDEFINED_L */	"Undefined L-symbol",
/* E_REGLIST	 */	"Invalid register list",
		0
} ;

int errors = 0;		/* Number of errors in this pass */
extern char * asm_name; /* from init.c			 */

/* Sys_Error is called when a System Error occurs, that is, something is wrong
   with the assembler itself.  Explanation is a string suitable for a printf
   control string which explains the error, and Number is the value of the
   offending parameter.  This routine will not return.
*/
/*VARARGS 1*/
sys_error(Explanation,Number)
char *Explanation;
{
	fflush(stdout);
	fprintf(stderr, "%s: Optimizer Error-- ", asm_name);
	fprintf(stderr, Explanation,Number);
	exit(-1);
}

/* This is called whenever the assembler recognizes an error in the current
statement. It registers the error, so that an error code will be listed with
the statement, and a description of the error will be printed at the end of
the listing */

prog_error(code)
err_code code;
{
	errors++;			/* increment error count */
	fprintf(stderr,"%s: error (%d): %s\n",asm_name, 
		line_no,e_messages[(int)code]);
}

/* Prog_Warning registers a warning on a statement. A warning is like an error,
	in that something is probably amiss, but the assembler will still try 
	to generate the .o file. 
*/

prog_warning(code)
err_code code;
{

#if AS
	if (pass != 2) return;
#endif
	fprintf(stderr,"%s: warning (%d): %s\n",asm_name, 
		line_no,e_messages[(int)code]);
}
