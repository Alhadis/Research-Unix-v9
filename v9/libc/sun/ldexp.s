	.data
/*	.asciz	"@(#)ldexp.s 1.1 86/02/03 SMI"	*/
	.text

#include "fpcrtdefs.h"

/*
 * double
 * ldexp( value, exp)
 *      double value;
 *      int exp;
 *
 * return a value v s.t. v == value * (2**exp)
 */

ENTER(_ldexp)
	moveml	sp@( 4),d0/d1	| suck parameters into registers
	lea	sp@(12),a0	| a0 gets address of i.
	jmp	Fscaleid
	
