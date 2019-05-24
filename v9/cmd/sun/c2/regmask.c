#ifndef lint
static	char sccsid[] = "@(#)regmask.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

regmask RegMasks[PCREG+1][LR+1] = {
	/* d0 */	{ {0}, {01}, {03}, {07} },
	/* d1 */	{ {0}, {010}, {030}, {070} },
	/* d2 */	{ {0}, {0100}, {0300}, {0700} },
	/* d3 */	{ {0}, {01000}, {03000}, {07000} },
	/* d4 */	{ {0}, {010000}, {030000}, {070000} },
	/* d5 */	{ {0}, {0100000}, {0300000}, {0700000} },
	/* d6 */	{ {0}, {01000000}, {03000000}, {07000000} },
	/* d7 */	{ {0}, {010000000}, {030000000}, {070000000} },
	/* a0 */	{ {0}, {0100000000}, {0100000000}, {0100000000} },
	/* a1 */	{ {0}, {0200000000}, {0200000000}, {0200000000} },
	/* a2 */	{ {0}, {0400000000}, {0400000000}, {0400000000} },
	/* a3 */	{ {0}, {01000000000}, {01000000000}, {01000000000} },
	/* a4 */	{ {0}, {02000000000}, {02000000000}, {02000000000} },
	/* a5 */	{ {0}, {04000000000}, {04000000000}, {04000000000} },
	/* a6 */	{ {0}, {010000000000}, {010000000000}, {010000000000} },
	/* a7 */	{ {0}, {020000000000}, {020000000000}, {020000000000} },
	/* f0 */	{ {0}, {0,1}, {0,1}, {0,1} },
	/* f1 */	{ {0}, {0,2}, {0,2}, {0,2} },
	/* f2 */	{ {0}, {0,4}, {0,4}, {0,4} },
	/* f3 */	{ {0}, {0,010}, {0,010}, {0,010} },
	/* f4 */	{ {0}, {0,020}, {0,020}, {0,020} },
	/* f5 */	{ {0}, {0,040}, {0,040}, {0,040} },
	/* f6 */	{ {0}, {0,0100}, {0,0100}, {0,0100} },
	/* f7 */	{ {0}, {0,0200}, {0,0200}, {0,0200} },
	/* cc */	{ {0}, {0,0400}, {0,0400}, {0,0400} },
	/* fcc*/	{ {0}, {0,01000}, {0,01000}, {0,01000} },
	/* pc */	{ {0}, {0}, {0}, {0} },
};

regmask exitmask; 
regmask regmask0 = {0,0};
regmask regmask_all = { -1, 0x3ff };
regmask regmask_nontemp = {0xfcffffc0, 0x0fc }; /* all registers except d0/d1, a0,a1, f0,f1 */

int
inmask( a, b )
    int a;
    regmask b;
{
    if (a < A0REG)
	return( (RegMasks[a][LR].da & b.da)>>(3*a));
    else if (a< FP0REG)
	return( (RegMasks[a][LR].da & b.da)>>(a+24-A0REG));
    else
	return( (RegMasks[a][LR].f & b.f) >> (a-FP0REG));
}

regmask
addmask( a, b )
    regmask a, b;
{
    a.da |= b.da;
    a.f  |= b.f;
    return a;
}

regmask
submask( a, b )
    regmask a, b;
{
    a.da &= ~b.da;
    a.f  &= ~b.f & 01777;
    return a;
}

regmask
andmask( a, b )
    regmask a, b;
{
    a.da &= b.da;
    a.f  &= b.f;
    return a;
}

regmask 
notmask( a )
    regmask a;
{
    a.da = ~a.da;
    a.f  = ~a.f & 01777;
    return a;
}

int
emptymask( a )
    regmask a;
{
    return a.da == 0 && a.f == 0;
}

int
samemask( a, b)
    regmask a, b;
{
    return ((a.da==b.da) && (a.f==b.f));
}

regmask
movemmask( so, rw, operands )
    subop_t so;
    struct oper *operands[];
{
    register short imask, regbit, regno;
    regmask r;
    int sl;
    operand_t t;
    r = regmask0;
    switch (so){
    case SUBOP_W: sl = WR+LW; break; /* movemw */
    case SUBOP_X:
    case SUBOP_L: sl = LR+LW; break; /* moveml */
    }
    sl &= rw;
    if (rw==RMASK){
	/* which registers are read by this operation? */
	if (operands[0]->type_o != T_IMMED) return regmask0; /* this instruction writes */
	if (operands[0]->sym_o) return regmask0 ; /* lord knows */
	imask = operands[0]->value_o;
	t = operands[1]->type_o;
    } else {
	/* which registers are written by this operation */
	sl >>= RWWIDTH;
	if (operands[1]->type_o != T_IMMED) return regmask0; /* this instructino reads */
	if (operands[1]->sym_o) return regmask0 ; /* lord knows */
	imask = operands[1]->value_o;
	t = operands[0]->type_o;
    }
    if (so == SUBOP_X){
	for (regbit=(1<<7), regno=FP0REG; regno<=FP0REG+7; regno++, regbit>>=1)
	    if (imask&regbit)
		r = addmask( r, MAKERMASK( regno, sl ));
    } else if (t == T_PREDEC){
	for (regbit=(1<<15), regno=0; regno<=15; regno++, regbit>>=1)
	    if (imask&regbit)
		r = addmask( r, MAKERMASK( regno, sl ));
    } else {
	for (regbit= 1     , regno=0; regno<=15; regno++, regbit<<=1)
	    if (imask&regbit)
		r = addmask( r, MAKERMASK( regno, sl ));
    }
    return r;
}

void
printmask( mask )
    regmask mask;
{
    register m;
    register regno;
    m = mask.da;
    /* D registers first */
    for (regno=0; regno<=7; regno ++){
	if      (m&4) printf(" L%d", regno);
	else if (m&2) printf(" W%d", regno);
	else if (m&1) printf(" B%d", regno);
	m >>= 3;
    }
    /* now A registers */
    for (regno=8; regno <= 15; regno++){
	if (m&1) printf(" A%d", regno-8);
	m >>= 1;
    }
    /* finally F registers and ccs */
    m = mask.f;
    for (regno=0; regno <=7; regno++){
	if (m&1) printf(" F%d", regno);
	m >>= 1;
    }
    if (m&1) printf(" CC");
    if (m&2) printf(" FCC");
}
