#ifndef lint
static	char sccsid[] = "@(#)quicken.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

extern struct ins_bkt *moveq, *subql, *addql;
#ifdef TRACKSP
int spoffset = 0; /* track a6-a7 distance */
#define SPUNK 999999; /* sp unknown here */
#endif TRACKSP

void
quicken()
{
    register NODE *n;
    register struct oper *o;
    register long v;
    register struct ins_bkt *ip;
    register int regno;

    for (n=first.forw; n!=&first; n=n->forw){
	switch (n->op){
	case OP_MOVE:
	    if (n->instr==moveq || (o=n->ref[0])->type_o!=T_IMMED || o->sym_o!=NULL) continue;
	    v = o->value_o;
	    o = n->ref[1];
	    if (!datareg_addr(o))continue;
	    regno = o->value_o;
	    if ( dreg(regno) && v >= -128 && v<= 127) {
		n->instr = moveq;
	    } else if (freg( regno ) ){
		if (use_fmovecr( n ))
		    meter.nusecr++;
		    break;
	    }
	    continue;
	case OP_ADD:
	    ip = addql;
	    goto xsub;
	case OP_SUB:
	    ip = subql;
	xsub:
	    if ( n->subop==SUBOP_B ) ip-=2;
	    else if (n->subop==SUBOP_W) ip-=1;
	    if (n->instr==ip || (o=n->ref[0])->type_o!=T_IMMED || o->sym_o!=NULL) continue;
	    v = o->value_o;
	    if ( v>=1 && v<=8)
		n->instr = ip;
	    else if ( v<=-1 && v>= -8){
		o->value_o = -v;
		if (n->op==OP_ADD){
		    n->op=OP_SUB;
		    ip = subql;
		} else {
		    n->op=OP_ADD;
		    ip = addql;
		}
		goto xsub;
	    }
	    continue;
	}
    }
}

#ifdef TRACKSP
void
track_sp( n )
    NODE *n;
{
    /* try to keep track of distance from a6 to a7 */
    /* keep global variable spoffset up-to-date    */

    register struct oper *o;
    int i;

    switch (n->op){
    case OP_LINK:    
	   if ((o=n->ref[1])->type_o==T_IMMED)
		spoffset = - o->value_o;
	    else
		spoffset = SPUNK;
	    break;
    case OP_UNLK:    
	    spoffset = 0;  break;
    case OP_LEA:     
	    if (istmp(o=n->ref[0]))
		spoffset += o->value_o;
	    else 
		spoffset = SPUNK;
	    break;
    case OP_PEA:
	    spoffset -= 4; break;
    case OP_ADD:
	    if ((o=n->ref[0])->type_o==T_IMMED)
		spoffset +=  o->value_o;
	    else
		spoffset = SPUNK;
	    break;
    case OP_SUB:
	    if ((o=n->ref[0])->type_o==T_IMMED)
		spoffset -=  o->value_o;
	    else
		spoffset = SPUNK;
	    break;
    default: 
	    for (i=0; i<n->nref; i++){
		o = n->ref[i];
		if (o->type_o == T_REG && o->value_o == SPREG)
		    spoffset = SPUNK;
		else if (o->type_o == T_POSTINC)
		    spoffset += BYTESIZE(n->subop);
		else if (o->type_o == T_PREDEC)
		    spoffset -= BYTESIZE(n->subop);
	    }
    }
}
#endif TRACKSP

/*
 * list of floating-point constants in the 68881 constant ROM.
 * Note that the ROM supports more constants, and they are
 * actually documented; but they aren't representable in IEEE
 * double precision.
 */
struct romlist { 
    int	    address;
    double  value;
} romlist[] = {
    0xf, 0.0,
    0x32, 1.0,
    0x33, 1.0e1,
    0x34, 1.0e2,
    0x35, 1.0e4,
    0x36, 1.0e8,
    0x37, 1.0e16,
    0x38, 1.0e32,
    0x39, 1.0e64,
    0x3a, 1.0e128,
    0x3b, 1.0e256,
    -1,   0.0
};

/*
 * determine if the fmove instruction n can be replaced by a fmovecr
 * instruction. If so, then make the substitution.
 */
static int
use_fmovecr( n )
    NODE *n;
{
    register struct oper *o;
    register struct romlist *rp;
    register double v;

    o = n->ref[0];
    if (o->flags_o&O_FLOAT) {
	v = o->fval_o;
    } else {
	/* integer conversion */
	v = (double)o->value_o;
    }
    rp = romlist;
    while ( rp->address >= 0 && rp->value != v )
	rp++;
    if ( rp->address  < 0 ) return 0; /* sorry */
    o->value_o = rp->address;
    o->flags_o = 0;
    cannibalize( n, "fmovecr" );
    return 1;
}
