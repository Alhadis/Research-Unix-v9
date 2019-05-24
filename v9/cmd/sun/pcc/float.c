#ifndef lint
static	char sccsid[] = "@(#)float.c 1.1 86/02/03 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 *	float.c
 */

#include "cpass1.h"

/* routine for decoding from "native", CC-internal floating point format
 * to object format -- in this case MOTOROLA FFP.
 * This gets invoked in local.68 if fpflag is not the same as NATIVEFP.
 */
fpdecoder( p, sz )
    register * p;
{

    struct mit { 
	unsigned
	    sign:1,
	    exp :8,
	    fract:23; /* has hidden high-order bit! */
    };
    struct motor {
	unsigned
	    fract:24, /* no hidden bit here */
	    sign :1,
	    exp  :7;
    };
    struct doubleieee {
	unsigned
	    sign:1,
	    exp: 11,
	    fract: 20,
	    fmore;
    };
    struct doubleint {
	unsigned n,m;
    };
#define MITEXCESS	(1<<7)
#define MOTOREXCESS	(1<<6)
/* 
 * the following two constants appear to be one two low. This is
 * deceiving. The hidden bit in IEEE form is on the left-hand-side
 * of the decimal point. The hidden bit in MIT form is on the 
 * right-hand-side of the decimal point. Thus the discrepiency.
 */
#define IEEEXCESS	(126)
#define DPEXCESS	(1022)
    union {
	struct motor motor;
	struct doubleieee dp;
	struct mit mit;
	struct doubleint i;
    } new;
    union {
	struct mit mit;
	struct doubleint i;
    } old;
    int x;

    if (NATIVEFP == MIT)
    switch( fpflag ){
    case Motorola:
	old.i.n = *p;
	if (old.i.n == 0) return;
	new.motor.sign = old.mit.sign;
	x = old.mit.exp - MITEXCESS;
	if (x >= MOTOREXCESS || x <= -MOTOREXCESS)
	    uerror("floating-point constant out of range");
	new.motor.exp = x + MOTOREXCESS;
	x = old.mit.fract | (1<<23); /* restore hidden bit */
	new.motor.fract = x;
	*p = new.i.n;
	*(p+1) = 0; /* s-p only, thank you */
	return;
    case IEEE:
	if (sz == SZDOUBLE ){
	    old.i.n = *p;
	    old.i.m = *(p+1);
	    if (old.i.n == 0 && old.i.m == 0) return;
	    new.dp.sign = old.mit.sign;
	    new.dp.exp  = old.mit.exp -MITEXCESS + DPEXCESS;
	    new.dp.fract = old.mit.fract >> 3;
	    new.dp.fmore = (old.mit.fract << 29) | (old.i.m >>3);
	    /* not actually a good approximation, but ... */
	    *p     = new.i.n;
	    *(p+1) = new.i.m;
	    return;
	} else {
	    /* trivial conversion */
	    new.i.n = *p;
	    if (new.i.n == 0) return;
	    new.mit.exp += IEEEXCESS - MITEXCESS;
	    *p = new.i.n;
	    return;
	}
    }
    cerror("incomprehensable floating-point conversion");
}
