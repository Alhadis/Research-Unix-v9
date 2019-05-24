#ifndef lint
static	char sccsid[] = "@(#)instruction.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#if C2
#include "c2.h"
#endif

extern char *malloc();
struct oper *newoperand();
struct oper  operands[OPERANDS_MAX];
int numops, code_length;

#if C2
NODE first = { OP_FIRST, SUBOP_OTHER, &first, &first };

/* does this branch read the c or v bit of the condition code? */
char read_cc_cv[]={ 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0 };
static struct ins_bkt * moveq = NULL;
#endif

instruction( ip )
    register struct ins_bkt *ip;
{
#if C2
    register NODE *np;
#endif
    register i;

    if ((ip->xflags_i&I20) && !ext_instruction_set)
	PROG_ERROR( E_OPCODE );
    i = 0;
    if (ip->noper_i != numops && ip->noper_i <= OPERANDS_MAX){
	if ( (ip->touchop_i&TOUCH1(TOUCHMASK)) == TOUCH1(SPEC(2)) && numops==1 && ip->subop_i == SUBOP_W )
	    /* shifts are a special case...*/
	    i = 0;
	else if ( (ip->touchop_i&TOUCH1(TOUCHMASK)) == TOUCH1(SPEC(6)) && numops==0 )
	    /* so is rts */
	    i = 0;
	else
	    i = (E_NUMOPS);
    } else switch (ip->noper_i){
    default: i = 0; break;
    case 1:
	    i = operand_ok( ip, &operands[0], (struct oper *)NULL, (struct oper *)NULL); break;
    case 2:
	    i = operand_ok( ip, &operands[0], &operands[1], (struct oper *)NULL);break;
    case 3:
	    i = operand_ok( ip, &operands[0], &operands[1], &operands[2] );break;
    }
    if (i)
	PROG_ERROR(i);
    if (ip->align_i & dot) /* don't want instructions on odd bounds */
	    PROG_ERROR(E_ODDADDR);
#if AS
    if (ISINSTRUC( ip->op_i ) )
	    code_length = 2; /* all instructions at least this long */
#endif
    (ip->routine_i)(ip, numops);
#if AS
    if (code_length) {
	  put_words(code,code_length);  /* output text */
	  bc = code_length;             /* increment LC */
    }
#endif
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
	AM_REGPAIR, -1, -1, -1, -1,
	AM_REGLIST,AM_FREGLIST,AM_FCREGLIST
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

    op1->access_o = opbits1 = op_to_bits( op1 );
    if (op2)
	op2->access_o = opbits2 = op_to_bits( op2 );
    else
	opbits2 = 0;
    if (op3)
	op3->access_o = opbits3 = op_to_bits( op3 );
    else
	opbits3 = 0;
    switch (ip->touchop_i&TOUCH2(TOUCHMASK)){
    case TOUCH2(SPEC(2)):
	/* special hackery for shifts */
	if (opbits1 == AM_IMMED && ((i=op1->value_o)<1 || i>8 || op1->sym_o))
	    return E_CONSTANT;
	break;
    } 
    switch (ip->touchop_i&TOUCH1(TOUCHMASK)){
    case TOUCH1(SPEC(0)):
    case TOUCH1(SPEC(1)):
	/* special hackery for bit ops */
	if (opbits1 == AM_IMMED){
	    i = op1->value_o;
	    if (op1->sym_o) return E_CONSTANT;
	    if (opbits2 == AM_DREG){
		if (i < 0 || i > 31 ) return E_CONSTANT;
	    } else 
		if (i < 0 || i > 7 ) return E_CONSTANT;
	}
	break;
    }
    switch (noperands){
    case 0: return 0;
    case 1:
	for (i=0; i<N_OPTYPES; i+=1)
	    if (opbits1&ip->optype_i[i]) return 0;
	break;
    case 2:
	for (i=0; i<N_OPTYPES; i+=2)
	    if ((opbits1&ip->optype_i[i]) && (opbits2&ip->optype_i[i+1]) ) return 0;
	break;
    case 3:
	for (i=0; i<N_OPTYPES; i+=3)
	    if ((opbits1&ip->optype_i[i]) && (opbits2&ip->optype_i[i+1]) && (opbits3&ip->optype_i[i+2]) ) return 0;
	break;
    }
    return E_OPERAND;
}


/* table used to determine node type. */
/* 
 * we would really like this table to be subscripted by the enumerated type
 * opcode_t, but C cannot hack that. Order here is VERY IMPORTANT,
 * and this table must change if the opcode order or number changes!!
 */
char opcodetypes[] = {
	0, 0, 0,			/* FIRST, COMMENT, LABEL */
	PSEUDOCODE, PSEUDOCODE, 	/* LONG, WORD */
	PSEUDOCODE, PSEUDONOCODE,	/* BYTE,TEXT */
	PSEUDONOCODE, PSEUDONOCODE,	/* DATA, DATA1 */
	PSEUDONOCODE, PSEUDONOCODE,	/* DATA2, BSS */
	PSEUDONOCODE,			/* PROC */
	PSEUDONOCODE, PSEUDONOCODE,	/* GLOBL, COMM */
	PSEUDOCODE, PSEUDOCODE, 	/* EVEN, ALIGN */
	PSEUDOCODE, PSEUDOCODE, 	/* ASCIZ, ASCII */
	PSEUDOCODE, PSEUDOCODE, 	/* FLOAT, DOUBLE */
	PSEUDONOCODE,			/* STABS */
	PSEUDONOCODE, PSEUDONOCODE,	/* STABD, STABN */
	PSEUDOCODE, PSEUDONOCODE,	/* SKIP, LCOMM */
	PSEUDONOCODE,			/* CPID */
	INSTRTYPE, INSTRTYPE,		/* CSWITCH, FSWITCH */
	INSTRTYPE, INSTRTYPE,		/* BRANCH, MOVE */
	INSTRTYPE, INSTRTYPE,		/* MOVEM, EXIT */
	INSTRTYPE, INSTRTYPE,		/* DBRA, CALL */
	INSTRTYPE, INSTRTYPE,		/* JUMP, DJMP */
	INSTRTYPE, INSTRTYPE,		/* LINK, CMP */
	INSTRTYPE, INSTRTYPE,		/* PEA, ADD */
	INSTRTYPE, INSTRTYPE,		/* AND, EXT */
	INSTRTYPE, INSTRTYPE,		/* OR, TST */
	INSTRTYPE, INSTRTYPE,		/* ASL, ASR */
	INSTRTYPE, INSTRTYPE,		/* SUB, UNLK */
	INSTRTYPE, INSTRTYPE,		/* LEA, CLR */
	INSTRTYPE, INSTRTYPE,		/* BOP, EOR */
	INSTRTYPE,           		/* OTHER */
};
