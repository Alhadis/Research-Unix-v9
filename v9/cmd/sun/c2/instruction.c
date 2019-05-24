#ifndef lint
static	char sccsid[] = "@(#)instruction.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

extern char *malloc();
struct oper *newoperand();
struct oper  operands[OPERANDS_MAX];
int numops;
void new_csect(), save_stabs();

NODE first = { OP_FIRST, SUBOP_Z, &first, &first };

#define nexti( p ) { do p=p->forw; while( ISDIRECTIVE(p->op) ); }
#define previ( p ) { do p=p->back; while( ISDIRECTIVE(p->op) ); }

extern int bytesize[];
#define BYTESIZE( s ) (bytesize[ (int)(s)])

/* does this branch read the c or v bit of the condition code? */
char read_cc_cv[]={ 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0 };
static struct ins_bkt * moveq = NULL;
static err_code operand_error = E_NOERROR;

extern struct ins_bkt *tstw, *tstl;
extern int ext_instruction_set;

static int
numeric_immediate( op )
    struct oper *op;
{
    if (op->type_o != T_IMMED || op->sym_o != NULL )
	return 0;
    return 1;
}

static int
immediate_value( op, value )
    struct oper *op;
    int value;
{
    if (op->type_o != T_IMMED || op->sym_o != NULL || op->flags_o&O_FLOAT || op->value_o != value)
	return 0;
    return 1;
}

instruction( ip )
    struct ins_bkt *ip;
{
    register NODE *np;
    register i;

    if (ip->noper_i != numops && ip->noper_i <= OPERANDS_MAX){
	/* shifts are a special case...*/
	if ((ip->touchop_i&TOUCHMASK) == SPEC(2) || numops==1 || ip->subop_i==SUBOP_W)
	    i = 0;
	else if ( (ip->touchop_i&TOUCH1(TOUCHMASK)) == TOUCH1(SPEC(7)) && numops==0 )
	    /* so is rts */
	    i = 0;
	else
	    prog_error(E_NUMOPS);
    }
    if ( ISINSTRUC(ip->op_i) ){
	if (cur_csect_name != C_TEXT){
	    /* non-text is not delt with -- echo it out directly */
	    putinstr( ip );
	    /* instruction in data space -- punt addressing      */
	    bc = 0x80000000; /* magic, negative number */
	    return;
	}
	switch (numops){
	case 0: i = 1; break;
	case 1: i = operand_ok( ip, &operands[0], (struct oper *)NULL, (struct oper *)NULL  ); break;
	case 2: i = operand_ok( ip, &operands[0], &operands[1], (struct oper *)NULL  );break; 
	case 3: i = operand_ok( ip, &operands[0], &operands[1], &operands[2]  );break; 
	}
	if (!i)
	    prog_warning(operand_error);
    } else {
	switch ( ip->op_i ){ 
	case OP_TEXT:  cur_csect_name = C_TEXT;  new_csect(); return;
	case OP_DATA:  cur_csect_name = C_DATA;  new_csect(); break;
	case OP_DATA1: cur_csect_name = C_DATA1; new_csect(); break;
	case OP_DATA2: cur_csect_name = C_DATA2; new_csect(); break;
	case OP_BSS:   cur_csect_name = C_BSS;   new_csect(); break;
	case OP_ASCII:
	case OP_ASCIZ: bc += ascii(cur_csect_name)+(ip->op_i==OP_ASCIZ); break;
	case OP_STABS: save_stabs(cur_csect_name); break;
	default:       bc += pseudo_size( ip ); break;
	}
	if (cur_csect_name != C_TEXT){
	    /* non-text is not delt with -- echo it out directly */
	    putinstr( ip );
	    return;
	}
    }
    np = new();
    np->nref = numops;
    for (i=0; i<numops; i++){
	np->ref[i] = newoperand( &operands[i] );
    }
    installinstruct( np, ip );
    addnode( np );
}

unsigned short
make_touchop( np, to )
    NODE *np;
    unsigned short to;
{
    /* ripped off from installinstruct, but not yet used by it */
    int i;
    unsigned short tfield, tretval;
    struct oper *o;

    tretval = 0;
    for (i=np->nref-1; i >= 0; i--){
	o = np->ref[i];
	switch(tfield = (to>>(TOUCHWIDTH*i))&TOUCHMASK){
	case SPEC(0): /* bset, bclr, bcng */
		if (o->type_o==T_REG){
		    tfield = touchselect(np->ref[0], BR+BW, WR+WW, LR+LW);
		} else{
		    tfield = (BR+BW);
		}
		break;
	case SPEC(1): /* btst */
		if (o->type_o==T_REG){
		    tfield = touchselect(np->ref[0], BR, WR, LR);
		} else {
		    tfield = (BR);
		}
		break;
	case SPEC(2): /* shifts */
		if (np->nref==1)
		    tfield = (WR+WW);
		else
		    tfield = (WR);
		break;
	case SPEC(3): /* pea, link, unlk    */
		tfield = 0;
		break;
	case SPEC(4): /* bfset  bfins */
		tfield = (LW);
		break;
	case SPEC(5): /* bfext? bftst */
		tfield = (LR);
		break;
	case SPEC(6): /* mulsl, divsll */
		tfield = (LR+LW);
		break;
	}
	tretval <<= TOUCHWIDTH;
	tretval |= tfield;
    }
    return tretval;
}

static int
touchselect( operand, b, w, l )
    struct oper *operand;
{
    register v;
    if (!numeric_immediate( operand ))  return l;
    v = operand->value_o;
    if ( v >=0 && v < 16 )
	if (v < 8 ) return b;
	else return w;
    else return l;
}

static subop_t
subopselect( touchword )
{
    switch (touchword&TOUCHMASK) {
    case BR:
    case BW:
    case BR+BW:
	return SUBOP_B;
    case WR:
    case WW:
    case WR+WW:
	return SUBOP_W;
    }
    return SUBOP_L;
}

void
cannibalize( np, sp )
    NODE *np;
    char *sp;
{
    installinstruct( np, sopcode(sp) );
}

installinstruct( np, ip )
    register NODE *np;
    register struct ins_bkt *ip;
{
    register struct oper *o;
    register unsigned short touchop;
    regmask smask, umask;
    register i;
    static int regnames[] = {
	D0REG, D0REG+1,
	A0REG, A0REG+1,
	FP0REG, FP0REG+1
    };

    np->instr = ip;
    np->op = ip->op_i;
    np->subop = ip->subop_i;
    umask = MAKERMASK( CCREG, (ip->cc_i&CCR)?BR:0);
    smask = MAKEWMASK( CCREG, (ip->cc_i&CCX)?BW:0);
    if (ip->cc_i&FCCR) umask = addmask( umask, MAKERMASK( FPCCREG, BR));
    if (ip->cc_i&FCC) smask = addmask( smask, MAKEWMASK( FPCCREG, BW));
    switch (np->op){
    case OP_CALL: /* writes d0,d1,a0,a1 except with exceptions */
	o = np->ref[0];
	if ( o->type_o == T_NORMAL && o->value_o==0 ){
	    struct sym_bkt *s;
	    s = o->sym_o;
	    if (s->attr_s & S_CRT){
		/*
		 * c- runtime support routine -- some of these may touch
		 * the floating point registers, but as of 3.0 they are
		 * not called by programs that use them.
		 */
		for (i=0; i<4; i++ ){
		    umask = addmask( umask, MAKERMASK( regnames[i], s->builtin_s[i] ));
		    smask = addmask( smask, MAKEWMASK( regnames[i], s->builtin_s[i] ));
		}
		break;
	    }
	}
	/* normal routine -- write d0,d1,a0,a1,fp0,fp1 */
	for (i=0; i<6; i++){
	    smask = addmask(smask,MAKEWMASK( regnames[i], LW ));
	}
	break;
    case OP_EXIT: /* reads some registers, writes all */
	umask  = addmask(exitmask, regmask_nontemp);
	smask = regmask_all;
	break;
    }
    touchop = ip->touchop_i;
    for (i=0; i<np->nref; i++, touchop >>= TOUCHWIDTH ){
	o = np->ref[i];
	switch(touchop&TOUCHMASK){
	case SPEC(0): /* bset, bclr, bcng */
		if (o->type_o==T_REG){
		    touchop = (touchop&~TOUCHMASK)|touchselect(np->ref[0], BR+BW, WR+WW, LR+LW);
		} else{
		    touchop = (touchop&~TOUCHMASK)|(BR+BW);
		}
		np->subop = subopselect( touchop );
		break;
	case SPEC(1): /* btst */
		if (o->type_o==T_REG){
		    touchop = (touchop&~TOUCHMASK)|touchselect(np->ref[0], BR, WR, LR);
		} else {
		    touchop = (touchop&~TOUCHMASK)|(BR);
		}
		np->subop = subopselect( touchop );
		break;
	case SPEC(2): /* shifts */
		if (np->nref==1)
		    touchop = (touchop&~TOUCHMASK)|(WR+WW);
		else
		    touchop = (touchop&~TOUCHMASK)|(WR);
		break;
	case SPEC(3): /* pea, link, unlk    */
		smask = MAKEWMASK( SPREG, LW );
		/* umask = MAKERMASK( SPREG, LR ); */
		/* the unlk instruction does not read sp before setting it */
		umask = (ip->op_i == OP_UNLK) ? regmask0 : smask;
		touchop = LR+LW; /* link and unlk use this */
		break;
	case SPEC(4): /* bfset  bfins */
		touchop = (touchop&~TOUCHMASK)|(LW);
		break;
	case SPEC(5): /* bfext? bftst */
		touchop = (touchop&~TOUCHMASK)|(LR);
		break;
	case SPEC(6): /* mulsl, divsll */
		if (o->type_o != T_REGPAIR){
		    touchop = (touchop&~TOUCHMASK)|(LR+LW);
		    break;
		} else {
		    /* 1st register of pair written only */
		    smask = addmask( smask, MAKEWMASK( o->value_o, LW ));
		    /* 2nd register of pair read+written */
		    umask = addmask( umask, MAKERMASK( o->reg_o,   LR ));
		    smask = addmask( smask, MAKEWMASK( o->reg_o,   LW ));
		    continue;
		}
	}
	switch (o->type_o ){
	case T_REGPAIR:
	    umask = addmask( umask, MAKERMASK( o->reg_o,   touchop ));
	    smask = addmask( smask, MAKEWMASK( o->reg_o,   touchop ));
	    /* FALL THROUGH */
	case T_REG:
	    if (dareg( o->value_o ) || srreg( o->value_o) || freg( o->value_o)){
		umask = addmask( umask, MAKERMASK( o->value_o, touchop ));
		smask = addmask( smask, MAKEWMASK( o->value_o, touchop ));
	    }
	    break;
	case T_POSTINC:
	case T_PREDEC:
	    smask = addmask( smask, MAKEWMASK( o->value_o, LW ));
	    /* FALL THROUGH */
	case T_DEFER:
	    umask = addmask( umask, MAKERMASK( o->value_o, LR ));
	    break;
	case T_DISPL:
	    umask = addmask( umask, MAKERMASK( o->reg_o, LR ));
	    break;
	case T_INDEX:
	    if (! (o->flags_o & O_BSUPRESS))
		umask = addmask( umask, MAKERMASK( o->reg_o, LR ));
	    if ( o->flags_o & (O_PREINDEX|O_POSTINDEX)) 
		umask = addmask( umask, MAKERMASK( o->value_o, 
		    o->flags_o&O_LINDEX ? LR : WR ));
	    break;
	}
	if (o->flags_o & O_BFLD){
	    if (o->flags_o & O_BFOREG)
		umask = addmask( umask, MAKERMASK( o->bfoffset_o, BR ));
	    if (o->flags_o & O_BFWREG)
		umask = addmask( umask, MAKERMASK( o->bfwidth_o, BR ));
	}
    }
    np->ruse = umask;
    np->rset = smask;
}

static unsigned
op_to_bits( op )
    register struct oper *op;
{
    /* return the address-mode bits for operand *op */
    register unsigned s; 
    static unsigned bits[] = {
	0,
	AM_REG, AM_DEFER, AM_POSTINC, AM_PREDEC, AM_DISPL,
	AM_INDEX, AM_ABSS, AM_ABSL, AM_IMMED, AM_NORMAL, 
	AM_REGPAIR, -1, -1
    };
    switch ( s = bits[(int)op->type_o]){
    case AM_REG:
	s = reg_access[op->value_o]; break;
    case AM_DISPL:
	if (op->reg_o == PCREG) s = AM_PCDISPL; break;
    case AM_INDEX:
	if (op->reg_o == PCREG) s = AM_PCINDEX; break;
    }
    return s;
}

int
operand_ok( ip, op1, op2, op3 )
    struct ins_bkt *ip;
    struct oper    *op1, *op2, *op3;
{
    /*
     * this routine answers the eternal question:
     *  are operands *op1 and *op2 ok to use as the operands of instruction *ip?
     * it does it by looking at the optype_i bit fields in the instruction
     * description.
     */
    register i;
    register unsigned opbits1, opbits2, opbits3;
    register noperands = ip->noper_i;

    opbits1 = op_to_bits( op1 );
    if (op2)
	opbits2 = op_to_bits( op2 );
    else
	opbits2 = 0;
    if (op3)
	opbits3 = op_to_bits( op3 );
    else
	opbits3 = 0;
    switch (ip->touchop_i&TOUCH2(TOUCHMASK)){
    case TOUCH2(SPEC(0)):
    case TOUCH2(SPEC(1)):
	/* special hackery for bit ops */
	if (opbits1 == AM_IMMED){
	    i = op1->value_o;
	    if (op1->sym_o) { operand_error =  E_CONSTANT; return 0;}
	    if (opbits2 == AM_DREG){
		if (i < 0 || i > 31 ) { operand_error =  E_CONSTANT; return 0;}
	    } else 
		if (i < 0 || i > 7 ) { operand_error =  E_CONSTANT; return 0;}
	}
	break;
    }
    switch (ip->touchop_i&TOUCH1(TOUCHMASK)){
    case TOUCH1(SPEC(2)):
	/* special hackery for shifts */
	if (opbits1 == AM_IMMED && ((i=op1->value_o)<1 || i>8 || op1->sym_o)){
	    operand_error = E_CONSTANT;
	    return 0;
	}
	break;
    }
    switch (noperands){
    case 0: return 1;
    case 1:
	if ((ip == tstw || ip == tstl) && areg_addr(op1)
	  && !ext_instruction_set)
	    break;
	for (i=0; i<N_OPTYPES; i+=1)
	    if ((opbits1&ip->optype_i[i])==opbits1) return 1;
	break;
    case 2:
	for (i=0; i<N_OPTYPES; i+=2)
	    if (((opbits1&ip->optype_i[i])==opbits1) && ((opbits2&ip->optype_i[i+1])==opbits2) ) return 1;
	break;
    case 3:
	for (i=0; i<N_OPTYPES; i+=3)
	    if (((opbits1&ip->optype_i[i])==opbits1) && ((opbits2&ip->optype_i[i+1])==opbits2) && ((opbits3&ip->optype_i[i+2])==opbits3) ) return 1;
	break;
    }
    operand_error =  E_OPERAND;
    return 0;
}
onceonly()
{
    /*
     * A collection of context-free, compiler specific hacks 
     * we need only do once, as circumstances will not change
     * during the course of our work. These include:
     *  - moveml's: deleting those that do nothing, replacing the degenerate
     *             cases with movl's.
     *  - link a6,#0 ... addl #X,sp: combine if X in range of the link.
     *  - pea 0 : becomes clrl sp@-
     *  - cmpX #0,Y : becomes tstX Y, for Y not an address register.
     *  - movX #0,Y : becomes clrX Y, for Y not a  register, or
     *                        moveq #0,Y, for type long data register, or
     *                        subl Y,Y for Y an address register.
     */
    
    register NODE * n;
    register NODE *f;
    register i, v;
    register struct oper *o;
    int regno;
    extern NODE *deletenode();

    for (n=first.forw; n != &first; n=n->forw){
	switch( n->op){
	case OP_MOVEM:
	    if (n->subop==SUBOP_L || n->subop==SUBOP_X) {
		/* i get the index of the immediate operand */
		i = (n->ref[1]->type_o==T_IMMED); 
		/* o get the pointer to the immediate operand */
		o = n->ref[i];
		if (o->sym_o || o->type_o!=T_IMMED) continue;
		/* v gets the register mask -- how many bits set here? */
		switch( cntbits( v=o->value_o ) ){
		case 0: 
		    if (deladdr(n->ref[1-i])){
			n = deletenode( n ); 
			meter.nrmtfr++;
		    }
		    break;
		case 1: /* replace with a moveml, same "other" operand */
		    v = ffs(v); /* find first bit set -- "1" on littleend */
		    /* if mode of "other" operand is predecrement, bits work backwards */
		    o->type_o=T_REG;
		    if (n->subop==SUBOP_L){
			if (n->ref[1-i]->type_o==T_PREDEC)
			    v = 16 - v;
			o->value_o = v-1;
			cannibalize( n, "movl" );
		    }else{
			o->value_o = FP0REG + 8 - v;
			cannibalize( n, "fmovex" );
		    }
		    meter.nmmtmo++;
		    break;
		}
	    }
	    continue;
	case OP_LINK:
	    /* make sure we understand this instruction */
	    o = n->ref[1];
	    if (!numeric_immediate(o)) continue;
	    v = o->value_o;
	    /* see if next instruction is of form: addl #nnn,sp */
	    f = n;
	    nexti(f);
	    if (f->op!=OP_ADD) continue;
	    o = f->ref[0];
	    if (!numeric_immediate(o)) continue;
	    v += o->value_o;
	    o = f->ref[1];
	    if (!(o->type_o==T_REG && o->value_o==SPREG)) continue; 
	    /* make sure sum of immediates is in range */
	    if (v <-32768 || v > 32767) continue;
	    /* got it! */
	    n->ref[1]->value_o = v;
	    n = deletenode( f );
	    meter.namwl++;
	    continue;
	case OP_PEA:
	    /* if operand is a "normal" 0, change the opcode */
	    o = n->ref[0];
	    if (o->type_o==T_NORMAL && o->sym_o==NULL && o->value_o==0){
		o->type_o = T_PREDEC;
		o->value_o = SPREG;
		cannibalize( n, "clrl" );
		meter.nmtoc++;
	    }
	    continue;
	case OP_CMP:
	    /* compares to zero are degenerate */
	    o = n->ref[0];
	    if (immediate_value( o, 0)){
		if (areg_addr(n->ref[1])){
		    /* "A" register
		     * may be able to do something about this later on,
		     * after we gain some context.
		     */
		    continue;
		} else {
		    n->ref[0] = n->ref[1];
		    n->nref--;
		    freeoperand( o );
		    cannibalize( n,  (n->subop==SUBOP_L)?"tstl"
				    :(n->subop==SUBOP_W)?"tstw"
				    :              "tstb");
		    meter.nctot++;
		}
	    }
	    continue;
	case OP_MOVE:
	    /* moves of zero can often be simplified */
	    o = n->ref[0];
	    if (immediate_value(o, 0)){
		switch (n->ref[1]->type_o){
		case T_REG:
		    v = n->ref[1]->value_o;
		    if (dreg(v) ){
			if (n->subop != SUBOP_L) goto mkclr;
			/* use moveq */
			cannibalize( n, "moveq" );
			meter.nwmov++;
		    } else if (areg(v)){
			/* subtract from self */
			o->type_o = T_REG;
			o->value_o = v;
			cannibalize( n , "subl" );
			meter.nmtos++;
		    }
		    break;
		default:
		mkclr:
		    /* use the clear instruction */
		    n->ref[0] = n->ref[1];
		    n->nref--;
		    freeoperand( o );
		    cannibalize( n,  (n->subop==SUBOP_L)?"clrl"
				    :(n->subop==SUBOP_W)?"clrw"
				    :              "clrb");
		    meter.nmtoc++;
		    break;
		}
	    }
	    continue;
	case OP_AND:
	    /*
	     * if this looks like a mask of the lower bits,
	     * then it kill the rest of the register without
	     * reading it: i.e.
	     *	andl	#255,d0
	     * only uses the low-order 8bits anyway, so we can
	     * change our usage mask.
	     */
	    if (numeric_immediate(o=n->ref[0])){
		v = o->value_o;
		if ((o=n->ref[1])->type_o==T_REG){
		    regno = o->value_o;
		    if ( v>= 0 && v <= 0xffff && dreg(regno) ){
			n->ruse = submask( n->ruse, MAKERMASK( regno, LR ));
			n->ruse = addmask( n->ruse, MAKERMASK( regno, v<=0xff ? BR : WR ));
			n->rlive = compute_normal( n, n->forw->rlive);
		    }
		}
	    }
	    continue;
	}
    }
}

int
addr_delt( old, new )
    subop_t old, new;
{
#define NO_WAY -32768
    /*
     * if an operand was addressed using subop "old", and will be
     * addressed by "new", what is the address increment?
     */
    static char deltsize[7][7] = {
    /* new is ... B    W    L    S    D    X	  P       */
    /* old is ... */
    /* B */ {     0,  -1,  -3,  -3,  -7,  -11,   -11    },
    /* W */ {     1,   0,  -2,  -2,  -6,  -10,   -10    },
    /* L */ {     3,   2,   0,   0,  -4,   -8,    -8    },
    /* S */ {     3,   2,   0,   0,  -4,   -8,    -8    },
    /* D */ {     7,   6,   4,   4,   0,   -4,    -4    },
    /* X */ {    11,  10,   8,   8,   4,    0,     0    }, 
    /* P */ {    11,  10,   8,   8,   4,    0,     0    }, 
    };

    switch ( old ){
    case SUBOP_B:
    case SUBOP_W:
    case SUBOP_L:
	break;
    default: return NO_WAY;
    }
    switch ( new ){
    case SUBOP_B:
    case SUBOP_W:
    case SUBOP_L:
	break;
    default: return NO_WAY;
    }
    return deltsize[(int)old - (int)SUBOP_B][(int)new - (int)SUBOP_B];
#undef NO_WAY
}

int
long_immediate( opcode )
    opcode_t opcode;
{
    /*
     * list some of the opcodes in which immediate operands are long (addl),
     * as opposed to those in which this is not the case (asrl).
     */
    switch ( opcode ){
    case OP_ADD:
    case OP_AND:
    case OP_SUB:
    case OP_MOVE:
    case OP_CMP:
    case OP_OR:
	return 1;
    }
    return 0;
}

static int
exception( p, pnext )
    NODE *p, *pnext;
{	
    if (p->instr==moveq && long_immediate(pnext->op )) 
	return 1; /* don't elide moveq's */
    if (pnext->op == OP_BOP){
	if (!numeric_immediate(pnext->ref[0])) return 1; /* be careful here */
	if (p->subop!=SUBOP_B)
	    if (p->ref[0]->type_o != T_REG && !cancache(p->ref[0]))
		return 1; /* don't mess up memory references */
    }
    return 0;
}

/*
 * have the pattern 
 *   movX   Y,d0
 *   opZ    d0,...
 * where the second instruction only reads the operand, and we could 
 * have done
 *   opZ    Y',...
 * instead.
 */
static NODE *
elide_move( p, pnext, opset )
    register NODE *p;
    register NODE *pnext;
    int opset;
{
    register i;

    for ( i=0 ; i<pnext->nref; i++)
	if (opset & (1<<i)) 
	    *pnext->ref[i] = *p->ref[0];
    installinstruct( pnext, pnext->instr );
    pnext->rlive = compute_normal(pnext,pnext->forw->rlive);
    meter.redunm++;
    return( deletenode( p ));
}

/*
 * look for dbra-equivalent sequence. The compiler
 * already makes some assumptions about 16-bit reachability
 * (word offsets in case-jumps), so we don't worry about that
 * here. We're going to be rather strict about what we recognize,
 * because we don't have the information to determine whether,
 * for instance, an index begins positive, in which case we
 * could recognize the condition " > 0 " as being equivalent
 * to " != -1". Someday, perhaps.
 * We will also be just a little adventuresome and try to
 * handle the "long dbra" case, too. (Long is the data type,
 * not the jump offset!)
 */
static NODE *
make_dbra( p, didchange )
    register NODE *p;
    int *didchange;
{
    register NODE *pnext;
    NODE *p3;
    register struct oper *o;
    subop_t length;
    static struct oper immed = { T_IMMED, 0,0,0, NULL, NULL, NULL, 0 };

    /* recognize subqZ #1,dX */
    length = p->subop;
    if (length == SUBOP_B) return p;
    if ( !immediate_value((o=p->ref[0]), 1))
	return p;
    o=p->ref[1];
    if (!dreg_addr(o)) return p;
    pnext = p->forw;
    switch (pnext->op){
    case OP_CMP:
	/* recognize cmpZ #-1,dX */
	if (pnext->subop != length ||
	    !immediate_value( pnext->ref[0], -1) ||
	    !sameops( o, pnext->ref[1]))
		return p;
	/* recognize jne Y */
	pnext = pnext->forw;
	if (pnext->op != OP_JUMP || pnext->subop != JNE ) return p;
	if (inmask( CCREG, pnext->forw->rlive) || inmask( CCREG, pnext->luse->rlive)) return p;
	/*
	 * found the pattern:
	 *    subqX   #1,dY    <== p
	 *    cmpX    #-1,dY
	 *    jne     Z        <== pnext
	 */
	/* save one copy of the operand register, delete much */
	p->nref --;
	p = deletenode( p)->forw;
	p = deletenode( p)->forw;
	/* p == pnext */
	break;
    case OP_TST:
	/* try a different pattern */
	p3 = p->back;
	if (p3->op != OP_MOVE || p3->subop != length ||
	    !sameops( p3->ref[0], o) || !dreg_addr( p3->ref[1]))
	    return p;
	if (pnext->subop != length || !sameops( p3->ref[1], pnext->ref[0]))
	    return p;
	/* recognize jne Y */
	pnext = pnext->forw;
	if (pnext->op != OP_JUMP || pnext->subop != JNE ) return p;
	if (inmask( CCREG, pnext->forw->rlive) || inmask( CCREG, pnext->luse->rlive)) return p;
	if (inmask( p3->ref[1]->value_o, pnext->rlive )) return p;
	/*
	 * found the pattern:
	 *    movX	dY,dW   <== p3
	 *	  subqX	#1,dY   <== p
	 *    tstX	dW
	 *    jne   Y       <== pnext
	 */
	/* save one copy of the operand register, delete much */
	(void) deletenode( p3 );
	p->nref --;
	p = deletenode( p)->forw;
	p = deletenode( p)->forw;
	/* p == pnext */
	break;
    default:
	return p;
    }
    /* 
     * now: two cases here: simple and long:
     *
     * simple case: move operand o to the jump instruction; 
     * cannibalize it as a dbra;
     *
     * long case: as simple case, then add the long-dbra
     * tail: clrw , cmpl, jcc.
     */
    pnext->ref[0] = o;
    pnext->nref++;
    cannibalize( pnext, "dbra" );
    pnext->op = OP_DJMP;
    if (length == SUBOP_L){
	/* add clrw dX instruction after that */
	pnext = new();
	pnext->nref = 1;
	pnext->ref[0] = newoperand( o );
	insert( pnext, p);
	cannibalize( pnext, "clrw" );
	/* add subql #1 instruction next */
	p3 = pnext;
	pnext = new();
	pnext->nref = 2;
	immed.value_o = 1;
	pnext->ref[0] = newoperand( &immed );
	pnext->ref[1] = newoperand( o );
	insert( pnext, p3);
	cannibalize( pnext, "subql" );
	/* finally, add jcc Y instruction */
	p3 = pnext;
	pnext = new();
	insert( pnext, p3);
	newreference( p->luse, pnext );
	cannibalize( pnext, "jcc" );
	pnext->op = OP_JUMP;
    }
    meter.ndbra++;
    (*didchange)++;
    return p;
}

/*
 * find out if operand *o uses the register r 
 */
int
operand_uses( o, r )
    struct oper *o;
    int r;
{
    int t;
    switch (o->type_o){
    case T_REG:
    case T_DEFER:
    case T_POSTINC:
    case T_PREDEC:
	return (o->value_o == r);
    case T_DISPL:
	return (o->reg_o == r);
    case T_INDEX:
	t  = (o->flags_o & O_BSUPRESS)?0 : (o->reg_o == r);
	t |= (o->flags_o & (O_PREINDEX|O_POSTINDEX))? (o->value_o == r) : 0;
	return t;
    case T_REGPAIR:
	return (o->value_o == r || o->reg_o == r);
    }
    return 0; /* others don't use registers */
}

/*
 * Look for arithmetic on an A register. Look for nearby uses of
 * the same register using deferred addressing. See if we can combine
 * them into postincrement or predecrement mode addressing.
 */
NODE *
plus_minus( p, didchange )
    register NODE *p;
    int *didchange;
{
    int incrval;
    register int reg;
    register NODE *q;
    int found;
    int i;
    operand_t substitute;
    register struct oper *o;

    if ( numeric_immediate( p->ref[0] ) && areg_addr( p->ref[1] ) ){
	incrval = p->ref[0]->value_o;
	reg = p->ref[1]->value_o;
	q = p;
	found = 0;
	if ( p->op == OP_ADD ){
	    /* search backwards */
	    substitute = T_POSTINC;
	    while ( (q=q->back)->op != OP_FIRST && q->op != OP_LABEL
	    && !ISBRANCH(q->op) && ! inmask( reg, q->rset)){
		if (inmask( reg, q->ruse)){ 
		    found = 1; 
		    break;
		}
	    }
	} else {
	    /* search forwards */
	    substitute = T_PREDEC;
	    while ( (q=q->forw)->op != OP_FIRST && q->op != OP_LABEL
	    && !ISBRANCH(q->op) && ! inmask( reg, q->rset)){
		if (inmask( reg, q->ruse)){ 
		    found = 1; 
		    break;
		}
	    }
	}
	if (found){
	    /*
	     * we've found a nearby node that reads our register.
	     * it must now meet these criteria:
	     *  -- the size of the access must agree with the size of the
	     *     increment/decrement operation
	     * 	-- must not change the register (already checked)
	     *  -- must use the register exactly once
	     *  -- must use the register in a type T_DEFER operand
	     *  -- must be able to take the substituted operand type
	     */
	    switch (q->subop){
	    case SUBOP_B:  found = incrval==1; break;
	    case SUBOP_W:  found = incrval==2; break;
	    case SUBOP_L:  found = incrval==4; break;
	    case SUBOP_S:  found = incrval==4; break;
	    case SUBOP_D:  found = incrval==8; break;
	    case SUBOP_X:  found = incrval==12; break;
	    default: 	   found = 0; break;
	    }
	    if (found){
		found = 0;
		for (i=0; i < q->nref; i++)
		    if (operand_uses( q->ref[i], reg)){
			found++;
			o = q->ref[i];
		    }
		if (found == 1 && o->type_o == T_DEFER){
		    o->type_o = substitute;
		    if (operand_ok(q->instr, q->ref[0], q->ref[1], q->ref[2] )){
			/* got it */
			p = deletenode( p );
			q->rset = addmask( q->rset, MAKEWMASK( reg, LW ) );
			*didchange++;
			meter.nplusminus++;
		    } else {
			/* blew it */
			o->type_o = T_DEFER; /* restore operand, go home */
		    }
		}
	    }
	}
    }
    return p;
}

int
shorten()
{
    int nchanged = 0;
    register NODE *p, *pnext;
    NODE *pprev, *p3;
    register struct oper *o;
    register regno, t;
    int didchange;
    char *newname;
    int i, newcc, opset;
    int otherreg;
    regmask m;
    struct oper *trialops[OPERANDS_MAX];

    /*
     * delete instructions that do useless things:
     *	moves to registers or parts of registers that are dead;
     *  extends to parts of registers that are dead;
     *  reset the condition codes to its current value.
     * also, anything that depends on context which might
     *  be changed by rearranging branchs:
     *  resetting condition codes, multiple shifts in a row,
     *  changing a "cmpl #0,A0" into a "movl A0,Di", where Di is dead.
     *  and the transformation
     *  - subqw #1,dX; cmpw #-1,dX; jne Y: becomes dbra dX,Y
     *			     (most often requested hack).
     */ 
    if (moveq==NULL) moveq = sopcode( "moveq" ); 
    didchange = 1; 
    while(didchange){ 
	didchange = 0;
	for (p=first.forw; p != &first; p=p->forw){
	    if (!ISINSTRUC(p->op)) continue;
	    if (!emptymask( submask(p->rset , addmask( p->forw->rlive, MAKERMASK( CCREG, LR)))) ){
		switch (p->op){
		case OP_CALL:
		case OP_DBRA:
		case OP_DJMP:
		case OP_BRANCH:
		case OP_JUMP:
		    break;
		case OP_EXT:
		    /* ext's also set condition codes */
		    if (!inmask( CCREG, p->forw->rlive)){
			/* useless extend -- simply delete it */
			p = deletenode( p );
			didchange++;
			meter.nredext++;
		    }
		    break;
		case OP_MOVE:
		    /* be careful here -- useless set must be to a register */
		    /* for instance: movl #0,a0@+ sets a0, but we cannot change it */
		    o = p->ref[1]; /* object of move */
		    if (o->type_o != T_REG) break;
		    if (!datareg(regno = o->value_o)) break;
		    /* TEST FOR CC USAGE */
		    if ( dreg(regno) && inmask( CCREG, p->forw->rlive)) break;
		    if ( freg(regno) && inmask( FPCCREG, p->forw->rlive)) break;
		    switch (inmask(regno,  p->forw->rlive)) {
		    case 0:
			if (!deladdr( p->ref[0])) continue; /* side effects? */
			/* totally useless -- fugg it */
			p = deletenode( p );
			didchange++;
			meter.redunm++;
			continue;
		    case 1: /* BYTE ONLY */
			newname = "movb";
			t = (p->subop ==SUBOP_L )?3 : 1; /* could have been movw */
			break;
		    case 2:
		    case 3: /* WORD ONLY */
			newname = "movw";
			t = 2;
			break;
		    default: continue;
		    } 
		    if ( freg(regno) ) break; /* don't rewrite fp moves */
		    if (incraddr( p->ref[0], t)) break;/* not ok */
		    if (p->instr == moveq) break; /* already moveq */
		    cannibalize( p, newname );
		    p->rlive = compute_normal( p, p->forw->rlive );
		    didchange++;
		    meter.nwmov++;
		    continue;
		case OP_AND:
		    /*
		     * if we're doing an AND to a dead register, it must be
		     * for condition-code only (don't even bother to check).
		     * if its only a single bit, change this to a btst.
		     */
		    if ((o=p->ref[1])->type_o!=T_REG || !dreg(regno=o->value_o))
			break;
		    if (inmask(regno, p->forw->rlive)!=0)
			break;
		    if (!numeric_immediate(o=p->ref[0]))
			break;
		    if (cntbits( t = o->value_o ) == 1){
			o->value_o = ffs(t)-1;
			cannibalize( p, "btst" );
			p->rlive = compute_normal( p, p->forw->rlive );
			didchange++;
			meter.nwop++;
		    }
		    break;
		}
	    }
	    switch (p->op){
	    case OP_MOVE:
		/*
		 * If this is a move to a register which is only read once
		 * by a following instruction, see if we can elide a move.
		 * If this is a move to a register which is changed once
		 * in the next instruction, then written back whence it
		 * came, try doing the modify directly to the source 
		 * of it all.
		 * If this is a move of a register to itself, and condition
		 * codes are dead, elide the move.
		 * SPECIAL HACK: the sequence
		 *	moveq	#n, d0
		 *	movl	d0, XXX
		 * is FASTER and shorter than the instruction
		 *	movl	#n, XXX
		 * DO NOT UNDO THIS OPTIMIZATION!
		 */
		if ((o=p->ref[1])->type_o != T_REG) break;
		regno = o->value_o;
		if ( p->ref[0]->type_o ==T_REG && p->ref[0]->value_o == regno){
		    /* either:
		     * its a Dreg or Freg, and the condition code is useless,
		     * or its an Areg, and we're not doing some funny
		     * sign extension.
		     */
		    if ( ((dreg(regno)||freg(regno))  && !inmask( CCREG, p->forw->rlive))
		    || (areg(regno) && p->subop == SUBOP_L)){
			p = deletenode( p );
			didchange++;
			meter.redunm++;
			break;
		    }
		}
		newcc = 0;
		p3 = p;
		nexti(p3);
		for (pnext=p3; ISINSTRUC(pnext->op)||ISDIRECTIVE(pnext->op); pnext=pnext->forw){
		    if (ISDIRECTIVE(pnext->op)) continue;
		    newcc = inmask( CCREG, pnext->rset );
		    if (inmask( regno, pnext->ruse)) break;
		    if ((!newcc && inmask(CCREG, pnext->ruse )) || inmask( regno,  pnext->rset))
			goto no_cookie;
		    if (!emptymask( andmask( pnext->rset,p->ruse)) || !emptymask( andmask( p->rset,pnext->ruse)) )
			goto no_cookie;
		    switch (pnext->op){
		    case OP_BRANCH:
		    case OP_DBRA:
		    case OP_JUMP:
		    case OP_DJMP:
			goto no_cookie;
		    }
		}
		if (!ISINSTRUC(pnext->op)) break;
		/* see if operands the same */
		/* for each operand #i that is the same, opset gets bit 1<<i set */
		opset = 0;
		for ( i=0 ; i<pnext->nref; i++)
		    if (sameops( o, pnext->ref[i])) opset |= 1<<i;
		if (!opset) break;
		/*
		 * We have only two cases we are smart enough to understand.
		 * Either the two instructions are ajacent, in which case
		 * there can have been no side effects in the intervening
		 * (null) instructions, OR we're looking at this pattern:
		 *	movX Y,d0; ... ; opX d0,dZ
		 * where dZ is not used between the moves. The reason for
		 * this restriction is that it's too easy to break subtile
		 * C constructs like post-incrementing, and too hard to detect
		 * the general case.
		 */
		if (pnext==p3){
		    /*
		     * see if register used after that instruction
		     * make sure its a read-only access, and same size
		     */
		    if (!inmask(regno, pnext->forw->rlive) && !inmask( regno, pnext->rset ) ){
			t = addr_delt(p->subop, pnext->subop);
			for ( i=0 ; i<pnext->nref; i++)
			    trialops[i] = (opset & (1<<i) ) ? p->ref[0] : pnext->ref[i];
			for ( i ; i<OPERANDS_MAX; i++)
				trialops[i]  = NULL;
			if (operand_ok(pnext->instr, trialops[0], trialops[1], trialops[2])){
			    int ok = 0;
			    if (t==0) {
				/* both instructions use same size data */
				ok = 1;
			    } else if (t>0 && !areg(regno)) {
				/*
				 * second inst uses smaller data, and
				 * doesn't depend on sign-extension. The
				 * check on regno prevents us from changing
				 *	movl a6@(8),a0; cmpw #0,a0 | RIGHT
				 * into
				 *	cmpw #0,a6@(10)		   | WRONG
				 */
				ok = !incraddr(p->ref[0],t);
			    }
			    if (ok && !exception( p, pnext)){
				p = elide_move(p, pnext, opset);
				didchange++;
				break;
			    }
			} else if (pnext->op == OP_BOP && 
			numeric_immediate( pnext->ref[0] )){
			    /*
			     * btst  #t,d0
			     * take t modulo 8
			     */
			    t = pnext->ref[0]->value_o;
			    pnext->ref[0]->value_o = t%8;
			    if (operand_ok(pnext->instr, trialops[0], trialops[1], trialops[2]) &&
			    !incraddr( p->ref[0], BYTESIZE(p->subop)-1-t/8 )){
				/* it worked */
				p = elide_move(p, pnext, opset);
				didchange++;
				break;
			    } else {
				/* didn't work; put it back */
				pnext->ref[0]->value_o = t;
			    }
				
			}
		    }
		}else{
		    if (pnext->nref!=2 || !dareg_addr(pnext->ref[1])) break;
		    otherreg =  pnext->ref[1]->value_o;
		    for (p3=p->forw; p3!=pnext; p3=p3->forw){
			if (inmask( otherreg, addmask(p3->ruse,p3->rset) )) break;
		    }
		    if (p3!=pnext ) break;
		    if (!inmask( regno, pnext->forw->rlive ) && !inmask( CCREG, pnext->forw->rlive)
		    && ! (inmask( regno, pnext->rset) || p->subop != pnext->subop )){
			if ( operand_ok(pnext->instr,p->ref[0],pnext->ref[1], 0 )){
			    freeoperand( p->ref[1] );
			    p->ref[1] = pnext->ref[1];
			    installinstruct( p, pnext->instr );
			    pnext->nref=1;
			    m = pnext->forw->rlive;
			    for (p3=pnext->back; p3!=p; p3=p3->back)
				p3->rlive = m = compute_normal( p3, m);
			    p->rlive = compute_normal(p, m );
			    (void) deletenode(pnext);
			    didchange++;
			    meter.redunm++;
			}
		    }
		    break;
		}

		/* see if the operand is written in the next instruction,
		 * then moved back in the following, and dead thereafter 
		 */
    no_cookie:
		if (inmask( regno, pnext->rset ) && (p3=pnext->forw)->op==OP_MOVE 
		&& sameops( o, p3->ref[0] ) 
		&& (pnext->nref== 1 || !sameops( o, pnext->ref[0] ) )
		&& sameops( p->ref[0], p3->ref[1] ) && !inmask(regno , p3->forw->rlive)
		&& p->subop==pnext->subop && pnext->subop==p3->subop )
		{
		    if (pnext->nref==1 && operand_ok( pnext->instr, p->ref[0], NULL, 0 )){
			/* looks like a negX d0 or notX d0 */
			/* move p's operand to pnext */
			freeoperand( pnext->ref[0] );
			pnext->ref[0] = p->ref[0];
			p->ref[0] = NULL;
			/* delete p and p3 */
			p = deletenode( p ); 
			(void )deletenode( p3 );
			/* recompute register info */
			installinstruct( pnext, pnext->instr );
			pnext->rlive = compute_normal(pnext, pnext->forw->rlive );
			didchange++;
			meter.redunm += 2;
			break;
		    } else if (pnext->nref==2 && operand_ok( pnext->instr, pnext->ref[0], p->ref[0], 0 )){
			/* looks like a addX #Y,d0 */
			/* move p's operand to pnext */
			freeoperand( pnext->ref[1] );
			pnext->ref[1] = p->ref[0];
			p->ref[0] = NULL;
			/* delete p and p3 */
			p = deletenode( p ); 
			(void )deletenode( p3 );
			/* recompute register info */
			installinstruct( pnext, pnext->instr );
			pnext->rlive = compute_normal(pnext, pnext->forw->rlive );
			didchange++;
			meter.redunm += 2;
			break;
		    } else if (pnext->nref==2 && !inmask(regno , p3->forw->rlive) 
		    && operand_ok( pnext->instr, o, p->ref[0], 0 )){
			/* 
			 * change:
			 *	movX	A,d0    <== p
			 *	opX	B,d0    <== pnext
			 *	movX	d0,A    <== p3
			 * into:
			 *	movX	B,d0    <== p
			 *	opX	d0,A    <== pnext
			 */
			p->ref[0] = pnext->ref[0];
			installinstruct( p, p->instr );
			pnext->ref[0] = pnext->ref[1];
			pnext->ref[1] = p3->ref[1];
			p3->nref=1;
			(void)deletenode( p3 );
			installinstruct( pnext, pnext->instr );
			pnext->rlive = m = compute_normal( pnext, pnext->forw->rlive);
			p->rlive = compute_normal( p, m );
			didchange++;
			meter.redunm ++;
			break;
		    }
		}
		break;
		    
	    case OP_TST:
		/*
		 * if a previous instruction touched this as an operand
		 * AND set cc on it in a way we appreciate, then delete this
		 * EXTRA HACKERY:
		 *    some instructions (e.g.: lsrl) set the condition codes
		 *    in a way that we can use MOST of the time. others
		 *    (esp moves) set the cond codes in a way that is ok
		 *    all of the time. If the previous instruction is not
		 *    a move, we will try to find the instruction checking
		 *    the cc, and decide if this is an extraordinary case.
		 */
		pprev = p;
		do{
		    previ( pprev );
		} while ( !ISBRANCH(pprev->op) && !(pprev->op==OP_LABEL) && 
		    !inmask(CCREG, p->rset));
		if (!ISINSTRUC(pprev->op) || pprev->nref<1 || pprev->subop != p->subop) break;
		/* integer cc is very funny -- floating cc very regular */
		if (pprev->op!=OP_MOVE){
		    for( p3 = p->forw;  p3->op != OP_FIRST; ){
			switch (p3->op){
			case OP_LABEL:
			    nexti(p3);
			    continue;
			case OP_JUMP:
			case OP_BRANCH:
			case OP_DBRA:
			case OP_DJMP:
			    break;
			default:
			    if (inmask( CCREG, p3->ruse))
				break;
			    nexti(p3);
			    continue;
			}
			break;
		    }
		    if (!inmask( CCREG, p3->ruse)) break;/* cannot find user */
		    if (p3->op == OP_BRANCH || p3->op == OP_DBRA) 
			break;  /* too hard */
		    if (p3->op == OP_JUMP || p3->op == OP_DJMP){
			if (inmask( CCREG, p3->luse->rlive)) break; /* double use */
			if (p3->subop != JALL)
			    if (inmask( CCREG, p3->forw->rlive)) break; /* double use */
		    }
		    if (read_cc_cv[(int)p3->subop - (int)JEQ]) break; /* untrustworthy */
		}
		o = p->ref[0];
		if (!deladdr( o )) break; /* not ok to remove this reference */
		/*
		 * if we set condition codes on this, AND its not an
		 * Areg (on which condition codes are never set, then
		 * its ok to delete the test
		 */
		if (pprev->nref>=1 && sameops( pprev->ref[0], o)){
		    if (((pprev->instr->cc_i&CCX) == CC1) || 
		    ( pprev->op == OP_MOVE  && !areg_addr( pprev->ref[1])) ){
			p = deletenode( p );
			didchange++;
			meter.nrtst++;
		    }
		} else if (pprev->nref==2 && sameops( pprev->ref[1], o)){
		    if (((pprev->instr->cc_i&CCX) == CC2) && !areg_addr(o) ){
			p = deletenode( p );
			didchange++;
			meter.nrtst++;
		    }
		}
		break;
	    case OP_FTST:
		/*
		 * Floating-point instructions always set their condition codes
		 * on the result. The exception is that a move from the 
		 * coprocessor does not set the condition codes at all.
		 */
		/* see if we can find an instruction that sets fpcc */
		pprev = p;
		do{
		    previ( pprev );
		} while ( !ISBRANCH(pprev->op) && !(pprev->op==OP_LABEL) && 
		    !inmask(FPCCREG, p->rset));
		if (!ISINSTRUC(pprev->op) || pprev->nref<1 || pprev->subop != p->subop) break;
		o = p->ref[0];
		if ((pprev->op ==OP_MOVE && sameops( o, pprev->ref[0] ))||
		    (pprev->op !=OP_MOVE && sameops( o, pprev->ref[pprev->nref-1] ))){
		    p = deletenode( p );
		    didchange++;
		    meter.nrtst++;
		}
		break;
	    case OP_OR:
		/*
		 * if what we're or-ing will fit into a byte or word, then
		 * there's no excuse for using a longer immediate.
		 * unless condition codes are live.
		 */
		if ( inmask( CCREG, p->forw->rlive)) break;
		if (numeric_immediate(o=p->ref[0])){
		    t = o->value_o;
		    if (t==0 && deladdr(p->ref[1] )){
			p = deletenode(p);
			didchange++;
			break;
		    }
		    if (t>0 && t<0xffff){
			i = (p->subop==SUBOP_L)?2:0;
			newname = "orw";
			if (t<0xff){
			    i = (p->subop==SUBOP_L)?3:(p->subop==SUBOP_W)?1:0;
			    newname = "orb";
			}
			if (i && !incraddr( p->ref[1], i)){
			    cannibalize( p, newname);
			    meter.nwop++;
			    didchange++;
			}
		    }
			
		}
		break;
	    case OP_ASL:
		/* look for multiple shifts in a row */
		if (!numeric_immediate(o=p->ref[0])) continue;
		t = o->value_o;
		o = p->ref[1];
		while ((pnext=p->forw)->op==OP_ASL){
		    register struct oper *iop;
		    if (!numeric_immediate(iop=pnext->ref[0])
		    || !sameops( pnext->ref[1], o))
			break;
		    if ( t + iop->value_o > 8){
			p->ref[0]->value_o = t;
			t = iop->value_o;
			p = pnext;
			o = p->ref[1];
			continue;
		    }
		    t += iop->value_o;
		    (void)deletenode( pnext );
		    meter.nredshf++;
		    didchange++;
		}
		p->ref[0]->value_o = t;
		break;
	    case OP_CMP:
		/* compares to zero are degenerate */
		o = p->ref[0];
		if (immediate_value(o, 0) && areg_addr(p->ref[1])){
		    /* "A" register */
		    /* look for a dead D register. Make this a movl */
		    if ((i=dead_dreg(p)) >= 0){
			/* change cmpx #0,ay => movl ay,di */
			/* reuse the #0 operand as the di operand */
			p->ref[0] = p->ref[1];
			o->type_o = T_REG;
			o->value_o = i;
			p->ref[1] = o;
			cannibalize( p, "movl" );
			p->rlive = compute_normal( p, p->forw->rlive );
			meter.nttomo++;
			didchange++;
		    }
		}
		break;
	    case OP_LEA:
		/*
		 * this form seems to occur rather often:
		 *	lea	XXX,a0
		 *	movl	a0,aN
		 * with a0 dead beyond this point. Change this into
		 *	lea	XXX,aN
		 * then, look for the case
		 *	lea	aN@(0,dM:Z),aN
		 * and simplify it into
		 *	addZ	dM,aN
		 */
		if ((pnext=p->forw)->op == OP_MOVE
		  && pnext->subop == SUBOP_L 
		  && sameops( p->ref[1], pnext->ref[0])
		  && emptymask( andmask(p->rset,pnext->forw->rlive))) {
		    /* if the move is to an A register, we're set */
		    o=pnext->ref[1];
		    if (areg_addr(o)){
			pnext->ref[1] = p->ref[1]; /* give away our 2nd operand */
			p->ref[1] = o; /* gain a new operand */
			(void) deletenode( pnext ); /* get rid of next, with operands */
			p->rset = MAKEWMASK( o->value_o, LW );
			p->rlive = compute_normal( p, p->forw->rlive);
			didchange++;
			meter.redunm++;
		    }
		}
		if ((o=p->ref[0])->type_o == T_INDEX
		  && o->reg_o == p->ref[1]->value_o
		  && o->scale_o == 1
		  && !(o->flags_o&(O_BSUPRESS|O_INDIRECT|O_POSTINDEX))
		  && o->disp_o == 0){
		    o->type_o = T_REG;
		    newname = (o->flags_o&O_WINDEX) ? "addw" : "addl";
		    o->flags_o &= ~(O_WINDEX|O_LINDEX);
		    cannibalize( p, newname );
		    meter.nsaddr++;
		    didchange++;
		}
		break;

	    case OP_SUB:
		p = make_dbra( p, &didchange );
		if (p->op != OP_SUB) break;
		/* FALL THROUGH */
	    case OP_ADD:
		p = plus_minus( p, &didchange );
		break;
	    }
	}
	nchanged += didchange;
    }
    return nchanged;
}
