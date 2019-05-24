#ifndef lint
static	char sccsid[] = "@(#)op.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"

/* move_op - uses two effective addresses */

move_op( ip )
    struct ins_bkt *ip;
{
	register struct oper *op1, *op2;
	int r1, r2;
	/*
	 * opval_i[0] -- movX   eaddr,eaddr
	 * opval_i[1] -- movl   Areg,USP
	 * opval_i[2] -- movw	eaddr,cc
	 * opval_i[3] -- movw	cc,eaddr
	 */

	op1 = operands;
	op2 = &operands[1];
	r1 = (int)op1->value_o;
	r2 = (int)op2->value_o;

	if (sr_addr(op1)){
		wcode[0] = ip->opval_i[3];
		if (r1 != CCREG) wcode[0] ^= 1<<9;
		eaddr(op2, SUBOP_W, 0);
	} else if (sr_addr(op2)){
		wcode[0] = ip->opval_i[2];
		if (r2 != CCREG) wcode[0] ^= 1<<9;
		eaddr(op1, SUBOP_W, 1);
	} else if (usp_addr(op1)) {
		wcode[0] = ip->opval_i[1] | 0x8 | (r2 & 07);
	} else if (usp_addr(op2)) {
		wcode[0] = ip->opval_i[1] | (r1 & 07);
	} else {
		unsigned reg, mode, source;
		wcode[0] = 0;
		/* get source address */
		eaddr(op1, ip->subop_i, 0);
		source = wcode[0];		/* save it */
		wcode[0] = 0;
		/* get destination address */
		eaddr(op2, ip->subop_i, 1);
		/* flip around destination address */
		reg = wcode[0] & 07;
		mode = (wcode[0] >> 3) & 07;
		/* assemble instruction */
		wcode[0] = ip->opval_i[0] | (reg << 9) | (mode << 6) | source;
	}
}
/* two_ops - these are of the forms:
	xxx Dn,<eaddr>
	xxx <eaddr>,Dn
	xxx #yyy,<eaddr>
	xxx <eaddr>,An
 */

two_op( ip )
	register struct ins_bkt *ip;
{
	register struct oper *op1, *op2;
	int r1, r2;
	int wrtflg = ip->touchop_i&TOUCH2((BW|WW|LW)); /* 2nd operand write-to unless compare */
	subop_t size = ip->subop_i;
	op1 = &operands[0];
	op2 = &operands[1];
	r1 = (int)op1->value_o & 07;
	r2 = (int)op2->value_o & 07;

	/*
	 * opval_i[0] -- op  eaddr,dreg
	 * opval_i[1] -- op  eaddr,areg
	 * opval_i[2] -- op  #imm,eaddr   OR  0xffff
	 * opval_i[3] -- op  #imm,cc
	 */

	if (areg_addr(op2)){
	    /* addl  xxx,a0 */
	    wcode[0] = ip->opval_i[1] |(r2<<9);
	    eaddr(op1, size, 0);
	} else if (op1->type_o == T_IMMED && ip->opval_i[2] != 0xffff ) {
	    if (sr_addr(op2)){
		/* andb	#imm,cc	*/
		wcode[0] = ip->opval_i[3];
		/* byte-size to cc only, word-size to sr only */
		if (!((op2->value_o == SRREG) ^ (size == SUBOP_B)))
		    PROG_ERROR( E_OPERAND );
		rel_val(op1, (size==SUBOP_L)?SUBOP_L:SUBOP_W, 0);
	    } else {
		/* andw #imm,eaddr */
		wcode[0] = ip->opval_i[2];
		rel_val(op1, (size==SUBOP_L)?SUBOP_L:SUBOP_W, 0);
		eaddr(op2, size, wrtflg);
	    }
	} else {
	    wcode[0] = ip->opval_i[0];
	    /* notice closely:
	     *   op	d0,d1
	     * appears ambiguous. Actually, it is not.
	     * eor takes register only as source. other ops only 
	     * as destination.
	     */
	    if (!dreg_addr(op2) || ip->op_i == OP_EOR) {
		/* op	dreg,eaddr & eorl dreg,dreg */
		wcode[0] |= 0400|(r1<<9);
		eaddr(op2, size, wrtflg);
	    } else {
		/* op	eaddr,dreg  and op dreg,dreg */
		/* note that mulu #imm,dreg ends up here, too */
		wcode[0] |= (r2 << 9);
		eaddr(op1, size, 0);
	    }
	}
}
/* one_ops - install opr, check for exactly one operand and compute eaddr */

one_op( ip )
    struct ins_bkt *ip;
{
	register struct oper *op = operands;
	wcode[0] = ip->opval_i[0];
	switch (ip->op_i){
	case OP_CALL:
	case OP_BRANCH:
	    if (jsrflag && op->type_o == T_NORMAL && op->sym_o != 0){
		/* make this a pc-relative */
		/*
		op->value_o -= dot+2;
		*/
		op->type_o = T_PCPLUS; /* MAGIC!! */
		op->reg_o  = PCREG;
	    }
	}
	eaddr(op, ip->subop_i, ip->touchop_i&TOUCH1(LW|WW|BW));
}


/* no_op(opr) -- places opr in wcode[0]. */

no_op(ip)
    struct ins_bkt *ip;
{
    wcode[0] = ip->opval_i[0];
};

/* branch_op - process branch offsets */
/* also handle DBcc instructions */
branch_op(ip)
    struct ins_bkt *ip;
{
	long offs = 0;
	register struct oper *opp = operands;

	wcode[0] = ip->opval_i[0];
	if (ip->noper_i == 2){
	    wcode[0] |= opp->value_o;
	    opp  += 1;
	}
	if ( opp->sym_o && (opp->sym_o->attr_s & S_DEF) == 0){
		/* external branch -- good luck */
		;
	} else if (opp->sym_o == 0 || opp->sym_o->csect_s != cur_csect_name){
		PROG_ERROR(E_RELOCATE);
	} else {
		opp->sym_o = 0;	/* mark as non relocateable expression */
	}
	offs = opp->value_o - (dot + 2);
	if (offs > 32767L || offs < -32768L)
		PROG_ERROR(E_OFFSET);
	opp->value_o = (int)offs;
	rel_val(opp, SUBOP_W, opp->sym_o != 0);
}


brnchs_op(ip)
    struct ins_bkt *ip;
{
	long offs = 0;
	register struct oper *opp = operands;

	wcode[0] = ip->opval_i[0];
	if ( opp->sym_o && (opp->sym_o->attr_s & S_DEF) == 0){
		/* external branch -- out of luck */
		PROG_ERROR(E_RELOCATE);
	} else if (opp->sym_o == 0 || opp->sym_o->csect_s != cur_csect_name){
		PROG_ERROR(E_RELOCATE);
	} 
	offs = opp->value_o - (dot + 2);
	if (offs > 127 || offs < -128)
		PROG_ERROR(E_OFFSET);
	if (offs != 0){
	    wcode[0] |= offs & 0377;
	} else if ( ip->op_i != OP_CALL ){
	    /*
	     * this is not a bsr, and we have a PC-relative address of zero
	     * if we just spit this out as usual, it would look just like
	     * the beginning of a longer form of jump. So instead,
	     * we emit a "nop" instruction right here.
	     */
	    wcode[0] = 0x4e71;
	} else {
	    PROG_ERROR(E_OFFSET);
	}
}

/* it is not clear that we can get there from here.
 * If this is a 68020 instruction generation, then we can use
 * the long form of the branch. Otherwise, if this is an
 * unconditional, then we can use a long jump. Otherwise,
 * we must generate a branch-around-a-jump.
 */
brnchl_op(ip)
    struct ins_bkt *ip;
{
    register struct oper *opp = &operands[0];
    int offs;
    if (ext_instruction_set){
	wcode[0] = ip->opval_i[0] | 0377;
	if (opp->sym_o && ((opp->sym_o->attr_s & S_DEF) == 0)){
		/* external branch -- good luck */
		;
	} else if ((opp->sym_o==0) || (opp->sym_o->csect_s!=cur_csect_name)){
		/* PROG_ERROR(E_RELOCATE); */
	} else {
		opp->sym_o = 0;	/* mark as non relocateable expression */
	}
	offs = opp->value_o - (dot + 2);
	opp->value_o = (int)offs;
	rel_val(opp, SUBOP_L, opp->sym_o != 0);
    } else {
	/* this is not an extended-instruction set assembly */
	hard_branch( ip, opp );
    }
}

static
hard_branch( ip, opp )
    register struct ins_bkt *ip;
    register struct oper *opp;
{
    /* choose alternate opcode -- either long form or complement */
    wcode[0] = ip->opval_i[1];
    /*
     * if we are unconditional, just use the long mode.
     * otherwise, do the jump-around-jump
     */
    if (ip->subop_i == JALL){
	if (jsrflag && opp->type_o == T_NORMAL && opp->sym_o != 0){
	    /* make this a pc-relative */
	    /*
		opp->value_o -= dot+2;
	    */
	    opp->type_o = T_PCPLUS; /* MAGIC!! */
	    opp->reg_o  = PCREG;
	}
	eaddr(opp, SUBOP_L, 0);
	return;
    } else if (d2flag && (opp->type_o == T_NORMAL) ){
	/* we're really doing short displacements, anyway */
	branch_op( ip );
	return;
    } else {
	/* use complementary branch over jump */
	/* we'll fool eaddr() by building the jump instruction in wcode[0], then moving it */
	/* its address word, though, must be built in the right place */
	wcode[0] = 0x4EC0;		/* jmp xxxxxx */
	code_length = 4;		/* so far */
	eaddr(opp, SUBOP_L, 0);		/* the address */
	wcode[1] = wcode[0];
	wcode[0] = ip->opval_i[1] + code_length - 2;
    }
}
/*
 * generate a short or a long branch instruction as appropriate
 * note that:
 *		jra	foo
 *	foo:
 * is translated (by brnchs_op) to a nop.  If we attempt to
 * optimize this to generate no code, then after resolving
 * the span-dependent instructions, the value of
 * foo is the same as the address of the jra instruction.
 * Therefore, on pass 2, the jra looks like a jra .
 * Presumably, this could be fixed by keeping more information
 * after the sdi's are resolved
 */
jbrnch_op( ip )
	struct ins_bkt *ip;
{
	long offs = 0;
	register struct oper *opp = operands;
	int longsize, longsdi, useshort;

	if (opp->type_o != T_NORMAL || opp->sym_o == NULL) {
		/* not a relocatable operand */
		hard_branch( ip, opp );
	} else {
		offs = opp->value_o - (dot + 2);
		if (hflag && ip->op_i != OP_CALL)
		    useshort = 1;
		else
		    useshort = d2flag||jsrflag;
		if ((ip->subop_i == JALL) || ext_instruction_set){
		    longsize = 6;
		    longsdi = SDI6;
		} else {
		    longsize = 8;
		    longsdi = SDI8;
		}
		if (opp->flags_o & O_COMPLEX)	/* not a simple address */
			hard_branch( ip, opp );
		else if (pass == 1){
			if (cansdi) {
			    if (ip->op_i == OP_CALL && !Oflag &&
				operands->sym_o->value_s == 0 )
			    /* jbsr w/ forward reference */
				goto no_sdi;
			    code_length = makesdi(opp, dot + 2, 
				useshort ? SDI4 : longsdi);
			}
			else
		no_sdi:
			    code_length = useshort? 4 : longsize;
			put_rel( opp, SUBOP_L, dot+2, 0); /* may be to external */
		} else {
			/* pass 2 */
			if (opp->sym_o == 0
			 || opp->sym_o->csect_s != cur_csect_name
			 || offs < -32768L || offs > 32767L
			 || !cansdi ){
#ifdef EBUG
				if (debflag>1)
				    printf("jbrnch(2): external branch line %d offset 0x%X\n",
					line_no, dot);
#endif
				if (useshort) branch_op(ip);
				else          brnchl_op( ip );
			}else if (ip->op_i == OP_CALL && !Oflag &&
				  (operands->sym_o->value_s == 0 ||
				   operands->sym_o->value_s > dot ) ){
				cansdi = 0;    /* fake temperarily */
				hard_branch(ip, opp);
				cansdi = 1;
			}else if (offs > 127 || offs < -128){
#ifdef EBUG
				if (debflag>1)
				    printf("jbrnch(2): 4 byte branch line %d offset 0x%X\n",
					line_no, dot);
#endif
				branch_op(ip);
			}else{
#ifdef EBUG
				if (debflag>1)
				    printf("jbrnch(2): 2 byte branch line %d offset 0x%X\n",
					line_no, dot);
#endif
				brnchs_op(ip);
			}
		}
	}
	return;
}

 /* 
 * instructions of the form xxx Ax@+,Ay@+
 * and those of the forms   xxx Dx,Dy
 *                     or   xxx Ax@-,Ay@-
 */

regmem_op(ip)
    struct ins_bkt *ip;
{
	wcode[0] = ip->opval_i[0] 
		| (operands[0].value_o&07) | ((operands[1].value_o&07)<<9);
	if (operands[0].type_o == T_PREDEC ) {
		wcode[0] |= 010;
	}
}

/*
 * quick_op -- instructions such as "addqw	#7,d0"
 */

quick_op( ip )
    struct ins_bkt *ip;
{
    register int val = operands[0].value_o;
    if ( val <= 0 || val > 8 )
	PROG_ERROR( E_CONSTANT);
    if (val == 8 ) 
	val = 8;
    wcode[0] = ip->opval_i[0] | (val<<9);
    eaddr( &operands[1], ip->subop_i, 0);
}

/* shift op -	shift either a register or an effective address */

shift_op( ip )
    struct ins_bkt *ip;
{
	register struct oper *op1, *op2;
	op1 = &operands[0];
	op2 = &operands[1];
	/*
	 * opval_i[0] -- register-shift opcode
	 * opval_i[1] -- memory-shift opcode
	 */

	if (numops == 1) {
	    wcode[0] = ip->opval_i[1];
	    eaddr(op1, SUBOP_W, 1);
	} else {
	    int val1, val2;
	    wcode[0] = ip->opval_i[0];
	    val1 = (int)op1->value_o;
	    val2 = (int)op2->value_o;
	    if (op1->type_o==T_IMMED) {
		if (val1 == 8) val1 = 0;
	    } else {
		wcode[0] |=  040;
	    }
	    wcode[0] |= ((val1 & 07) << 9) | (val2 & 07);
	}
}
/* bit_op - of the form xxx Dn,<eaddr> or xxx #nnn,<eaddr> */

bit_op( ip )
	struct ins_bkt *ip;
{
	register struct oper *op1 ;
	/*
	 * opval_i[0] -- bit number dynamic
	 * opval_i[1] -- bit number static
	 */
	op1 = operands;

	wcode[0] = ip->opval_i[0];
	if (op1->type_o == T_REG ) {
	    /* <eaddr> is destination */
	    wcode[0] = ip->opval_i[0] | ((int)op1->value_o << 9);
	    eaddr(&operands[1], SUBOP_W, 1);
	} else {
	    wcode[0] = ip->opval_i[1];
	    rel_val(op1, SUBOP_W, 0);
	    eaddr(&operands[1], SUBOP_W, 1);
	}
}

/*
 * bitf_op -- single-operand bit-field instructions 
 */
bitf_op( ip )
    struct ins_bkt *ip;
{
    register struct oper *o = &operands[0];
    register flags = o->flags_o;
    wcode[0] = ip->opval_i[0];
    if (!(flags&O_BFLD))
	PROG_ERROR(E_OPERAND);
    wcode[1] = 0;
    if (flags&O_BFOREG)
	wcode[1] |= 1<<11;
    wcode[1] |= o->bfoffset_o << 6;
    if (flags&O_BFWREG)
	wcode[1] |= 1<<5;
    wcode[1] |= o->bfwidth_o;
    code_length += 2;
    o->flags_o &= ~O_BFLD;
    eaddr( o, SUBOP_W, 1);
}

/*
 * bitfr_op -- bit-field + register instructions.
 */
bitfr_op( ip )
    struct ins_bkt *ip;
{
    struct oper * r ;
    int reg;
    r = &operands[0];
    if (!(r->flags_o&O_BFLD)){
	/* first operand register */
	/* extract value, slide on down */
	reg = r->value_o;
	*r = operands[1];
    } else {
	/* second operand register */
	/* just extract value */
	reg = operands[1].value_o;
    }
    bitf_op( ip ); /* do most of the work; */
    wcode[1] |= reg << 12; /* finish off */
}

/* exg_op - instructions like exg rx,ry */

exg_op( ip )
	struct ins_bkt *ip;
{
	int r1, r2;
	register struct oper *op1, *op2;
	op1 = operands;
	op2 = &operands[1];
	r1 = (int)op1->value_o;
	r2 = (int)op2->value_o;

	wcode[0] = ip->opval_i[0];
	if (dreg(r1) && dreg(r2))
		wcode[0] |= 0100 | (r1 << 9) | r2;
	else if (areg(r1) && areg(r2))
		wcode[0] |= 0110 | ((r1 & 07) << 9) | (r2 & 07);
	else if (areg(r1) && dreg(r2))
		wcode[0] |= 0210 | (r2 << 9) | (r1 & 07);
	else if (dreg(r1) && areg(r2))
		wcode[0] |= 0210 | (r1 << 9) | (r2 & 07);
	else
		PROG_ERROR( E_OPERAND );
}		
/* reg_op - instructions that take one register operand */

reg_op( ip )
    struct ins_bkt *ip;
{
    wcode[0] = ip->opval_i[0] | (operands[0].value_o & 07);
}


/* link_op - form: link An,#<disp> */

link_op( ip )
    struct ins_bkt *ip;
{
    register int v = operands[1].value_o;
    /* 
     * opval_i[0] -- normal short form
     * opval_i[1] -- extended long form
     */
    if ( (ip->opval_i[0] == 0) || 		/* If linkl requested specifically, */
	v < -32768 || v > 32767 ||	/* or displacement too big for linkw, */ 
	!( operands[1].sym_o == NULL || operands[1].sym_o->attr_s & S_DEF))
					/* or displacement is relocatable !!, */
					{ /* try long form. */
	if (ext_instruction_set 	/* If generating 68020 code, */
	    && (ip->opval_i[1] != 0))	/* and linkw not requested specifically, */
					{ /* try long form. */
            if (!(operands[1].sym_o==NULL || operands[1].sym_o->attr_s&S_DEF)
                && pass==1 && cansdi){
                (void)makeddi( &operands[1], dot+code_length, SDIL, 0);
            }
	    wcode[0] = ip->opval_i[1] | (operands[0].value_o & 07);
	    rel_val(&operands[1], SUBOP_L, 0);
	    return;
	} else if ( v < -32768 || v > 32767)
	    PROG_ERROR(E_CONSTANT);
	/* else is external -- take pot luck */
    }
    wcode[0] = ip->opval_i[0] | (operands[0].value_o & 07);
    rel_val(&operands[1], SUBOP_W, 0);
}


/* movec_op - for the very weird movc instruction: movec rn,rc   */
/* 		where rc is in { usp, sfc, dfc, vbr }		 */

movec_op( ip )
    struct ins_bkt *ip;
{
    register struct oper *op1, *op2;
    register reg;
    register creg;
    static ctrl_reg_values[] = 
    /*  USP   SFC   DFC   VBR  CACR  CAAR   MSP   ISP */
    { 0x800,   0,    1, 0x801,   2, 0x802, 0x803, 0x804 };
    op1 = operands;
    op2 = &operands[1];
    wcode[0] = ip->opval_i[0];
    code_length = 4;
    if (ctrl_reg(op1->value_o)){
	    /* from control reg to user reg */
	    creg = op1->value_o;
	    reg  = op2->value_o;
    }else {
	    /* from user reg to control reg */
	    wcode[0] |= 1;
	    creg = op2->value_o;
	    reg  = op1->value_o;
    }
    creg = ctrl_reg_values[ creg - USPREG ];
    wcode[1] = ((reg&07)<<12) | creg;
    if(areg(reg)){
	wcode[1] |= 0x8000;
    }
}

/* moves_op - for the very weird movs instruction: 		 */
/* 		moves rn,eaddr or moves eaddr,rn	 	 */

moves_op( ip )
    struct ins_bkt *ip;
{
	register struct oper *op1, *op2;
	register reg;
	op1 = operands;
	op2 = &operands[1];
	wcode[0] = ip->opval_i[0];
	code_length = 4;
	if ( op1->type_o == T_REG ){
	    /* from register to other space */
	    reg = op1->value_o;
	    wcode[1] = 1<<11;
	    eaddr( op2, ip->subop_i, 1);
	} else {
	    /* from outer space to register */
	    reg = op2->value_o;
	    wcode[1] = 0;
	    eaddr( op1, ip->subop_i, 0);
	} 
	wcode[1] |= (reg&07)<<12;
	if(areg(reg)){
	    wcode[1] |= 0x8000;
	}
}


int reverse_bits( forward )
	int forward ;

/*
	result = forward with with 16 low order bits in reverse order.
*/

{
int bit, new ;
long i ;

new = 0 ;
bit = 0x8000 ;

for (i=0 ; i<16 ; i++ ) 
	{
	if (forward & 1) new |= bit ;
	forward = forward >> 1 ;
	bit = bit >> 1 ;
	}
return(new) ;
}

/* movem_op -	of the form: movem #xxx,<eaddr> or movem <eaddr>,#xxx */

movem_op( ip )
	struct ins_bkt *ip;
{
	register struct oper *op1, *op2;
	op1 = operands;
	op2 = &operands[1];
	switch (op1->type_o) 
		{
		case T_REGLIST:	/*	movem	<reglist>,<ea> */
			if (op2->type_o == T_PREDEC) /* reverse mask for an@- */
				op1->value_o = reverse_bits(op1->value_o) ;
		case T_IMMED:	/*	movem	#xxx,<ea> */
			{
			wcode[0] = ip->opval_i[0];
			rel_val(op1, SUBOP_W, 0);
			eaddr(op2, SUBOP_W, 1);
			break ;
			}
		default: 
			{
				/*	movem	<ea>,#xxx	*/
				/*	movem	<ea>,<reglist>	*/
			wcode[0] = ip->opval_i[1];
			rel_val(op2, SUBOP_W, 0);
			eaddr(op1, SUBOP_W, 0);
			}
		}
}

/* movep_op - of the form:  movep Dx,Ay@(d) or movep Ay@(d),Dx */

movep_op(ip)
	struct ins_bkt *ip;
{
	register struct oper *op1, *op2;
	int r1, r2;
	/*
	 * opval_i[0] -- memory-to-register
	 * opval_i[1] -- register-to-memory
	 */
	op1 = operands;
	op2 = &operands[1];
	r1 = (int)op1->value_o;
	r2 = (int)op2->value_o;
	if (op1->type_o == T_REG ) {
	    /* register-to-memory */
	    wcode[0] = ip->opval_i[1] | (r1 << 9) | (op2->reg_o & 07);
	    rel_val(op2, SUBOP_W, 0);		
	} else {
	    /* memory-to-register */
	    wcode[0] = ip->opval_i[0] | (r2 << 9) | (op1->reg_o & 07);
	    rel_val(op1, SUBOP_W, 0);
	}
}	


/* moveq -  form: moveq #<data>,Dn */

moveq_op( ip )
	struct ins_bkt *ip;
{
	register struct oper *op1, *op2;
	int r2;
	op1 = operands;
	op2 = &operands[1];
	r2 = (int)op2->value_o;
	if (op1->value_o > 0177 || op1->value_o < -0200)
		PROG_ERROR(E_CONSTANT);
	wcode[0] = ip->opval_i[0] | (r2 << 9) | ((short)op1->value_o & 0377);
}


/* trap - form: trap #xxx */

trap( ip )
	struct ins_bkt *ip;
{
        wcode[0] = ip->opval_i[0] | (((char)operands[0].value_o) & 017);
}

/* stop instruction -- form is stop #xxxx */
stop( ip )
	struct ins_bkt *ip;
{
        wcode[0] = ip->opval_i[0];
	rel_val( &operands[0], ip->subop_i, 0 );
}

/* rts instruction -- may or may not take an operand */
rts_op( ip )
	struct ins_bkt *ip;
{
	register struct oper *opp = operands;
	/*
	 * opval_i[0] -- no  operand
	 * opval_i[1] -- one operand
	 */
	if (numops == 1){
		/* new form -- with operand */
		wcode[0] = ip->opval_i[1];
		wcode[1] = 0xffff & opp->value_o;
		code_length = 4;
	} else {
		wcode[0] = ip->opval_i[0];
	}
}

/*
 *  chk2_op -- instructions of chk2X and cmp2X families: 
 *  take register number (plus one bit) in an extension word
 */
chk2_op( ip )
    struct ins_bkt *ip;
{
    /* 
     * opval_i[0] -- opcode 
     * opval_i[1] -- extension word
     */
    wcode[0] = ip->opval_i[0];
    wcode[1] = ip->opval_i[1] | ( operands[1].value_o<<12 );
    code_length += 2;
    eaddr( &operands[0], ip->subop_i, 0 );
}

/*
 * div_op -- long-form divide and multiply instructions that take
 * a register or register pair as operands
 */
div_op( ip )
    struct ins_bkt *ip;
{
    /* 
     * opval_i[0] -- opcode 
     * opval_i[1] -- extension word
     * opval_i[2] -- alternate extension word to be used if regpair 2nd operand
     */
    wcode[0] = ip->opval_i[0];
    if (operands[1].type_o == T_REGPAIR){
	wcode[1] = ip->opval_i[2] | (operands[1].reg_o<<12) | operands[1].value_o;
    } else {
	wcode[1] = ip->opval_i[1] | (operands[1].value_o<<12) | operands[1].value_o;
    }
    code_length += 2;
    eaddr( &operands[0], SUBOP_L, 0 );
}

/* callm_op -- process the callm instruction */
callm_op( ip )
    struct ins_bkt *ip;
{
    wcode[0] = ip->opval_i[0];
    wcode[1] = operands[0].value_o;
    code_length += 2;
    eaddr( &operands[1], SUBOP_L, 0 );
}

/*
 * cas1_op -- stupid comapare & set.
 */
cas1_op( ip )
    struct ins_bkt *ip;
{
    wcode[0] = ip->opval_i[0];
    wcode[1] = (operands[1].value_o<<6) + (operands[0].value_o);
    code_length += 2;
    eaddr( &operands[2], ip->subop_i, 1);
}

/*
 * cas2_op -- stupid comapare & set.
 */
cas2_op( ip )
    struct ins_bkt *ip;
{
    register struct oper * op = &operands[0];
    wcode[0] = ip->opval_i[0];
    wcode[1] = (op[2].value_o<<12) + (op[1].value_o<<6) + (op[0].value_o);
    wcode[2] = (op[2].reg_o<<12) + (op[1].reg_o<<6) + (op[0].reg_o);
    code_length += 4;
}

/*
 * pack_op -- the ridiculous bcd pack instruction.
 */
pack_op( ip )
    struct ins_bkt *ip;
{
    wcode[0] = ip->opval_i[0] | ((operands[0].type_o==T_REG)?0 : (1<<3));
    wcode[0] |= operands[0].value_o&07 | ((operands[1].value_o&07)<<9);
    rel_val( &operands[2], SUBOP_W, 0);
}
