# include "cpass1.h"
#ifndef lint
static	char sccsid[] = "@(#)regvars.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Register variable allocation:
 *
 * regvar_init()	Initialize register variable allocation
 * regvar_alloc(type)	Allocate register for variable of given type
 * regvar_avail(type)	Is register available for var of given type
 */

long regvar;

#ifdef sun	>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void
regvar_init()
{
	SETD(regvar, MAX_DVAR);
	SETA(regvar, MAX_AVAR);
	SETF(regvar, MAX_FVAR);
	minrvar = regvar;
}

int
regvar_alloc(type)
	TWORD type;
{
	int rvar;
	if ( ISPTR(type) ) {
		/*
		 * allocate address register
		 */
		rvar = NEXTA(regvar);
		SETA(regvar, rvar-1);
		if (NEXTA(regvar) < NEXTA(minrvar)) { 
			SETA(minrvar, NEXTA(regvar));
		}
	} else if ( use68881 && ISFLOATING(type) ) {
		/*
		 * allocate floating point register
		 */
		rvar = NEXTF(regvar);
		SETF(regvar, rvar-1);
		if (NEXTF(regvar) < NEXTF(minrvar)) { 
			SETF(minrvar, NEXTF(regvar));
		}
	} else {
		/*
		 * allocate data register
		 */
		rvar = NEXTD(regvar);
		SETD(regvar, rvar-1);
		if (NEXTD(regvar) < NEXTD(minrvar)) { 
			SETD(minrvar, NEXTD(regvar));
		}
	}
	return(rvar);
}

int
regvar_avail(type)
{
	if ( ISPTR(type) ) {
		return( NEXTA(regvar) >= MIN_AVAR );
	}
	if ( use68881 && ISFLOATING(type) ) {
		return( NEXTF(regvar) >= MIN_FVAR );
	}
	return( NEXTD(regvar) >= MIN_DVAR && cisreg(type) );
}

#else VAX	=========================================

void
regvar_init()
{
	minrvar = regvar = MAXRVAR;
}

int
regvar_alloc(type)
	TWORD type;
{
	int rvar;
	rvar = regvar--;
	if( regvar < minrvar )
		minrvar = regvar;
	return(rvar);
}

int
regvar_avail(type)
{
	return( regvar >= MINRVAR && cisreg( type ) );
}

#endif VAX	<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
