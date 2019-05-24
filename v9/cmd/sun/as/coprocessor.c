#ifndef lint
static  char sccsid[] = "@(#)coprocessor.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"

cp_general( ip )
	struct ins_bkt *ip ;

{
struct oper *op1, *op2 ;
unsigned r1, r2 ;

/*
	opval_i[0] -- fop ea,fpn
	opval_i[1] -- fop fpm,fpn
*/

op1 = &operands[0] ;
op2 = &operands[1] ;
r2 = (int) op2 -> value_o - FP0REG ;
code_length = 4 ;
wcode[0] = 0xf000 + current_cpid * 01000 ;

if (freg_addr(op1))
	{
	r1 = (int) op1 -> value_o - FP0REG ;
	wcode[1] = ip->opval_i[1] + r2 * 0200 + r1 * 02000 ;
	}
else
	{
	wcode[1] = ip->opval_i[0] + r2 * 0200 ;
	eaddr( op1, ip->subop_i, 0) ;
	}
}

cp_oneop( ip )
	struct ins_bkt *ip ;

{
struct oper *op1;
unsigned r1;

/*
	opval_i[0] -- fop ea
	opval_i[1] -- fop fpm
*/

op1 = &operands[0] ;
code_length = 4 ;
wcode[0] = 0xf000 + current_cpid * 01000 ;

if (freg_addr(op1))
	{
	r1 = (int) op1 -> value_o - FP0REG ;
	wcode[1] = ip->opval_i[1] + r1 * 02000 ;
	}
else
	{
	wcode[1] = ip->opval_i[0] ;
	eaddr( op1, ip->subop_i, 0) ;
	}
}

cp_move( ip )
	struct ins_bkt *ip ;

{
struct oper *op1, *op2 ;
unsigned r1, r2 ;

/*
	opval_i[0] -- fmov? ea,fpn
	opval_i[1] -- fmov? fpm,ea
	opval_i[2] -- fmovx fpm,fpn
		   -- fmovl ea,fpctrl
	opval_i[3] -- fmovl fpctrl,ea

*/

op1 = &operands[0] ;
op2 = &operands[1] ;
r1 = (int) op1 -> value_o - FP0REG ;
r2 = (int) op2 -> value_o - FP0REG ;
code_length = 4 ;
wcode[0] = 0xf000 + current_cpid * 01000 ;

if (freg_addr(op1)) 
{
if (freg_addr(op2))
	{ /* fmovx fpm,fpn */ 
	wcode[1] = ip->opval_i[2] + r1 * 02000 + r2 * 0200 ;
	}
else
	{ /* fmov? fpm,ea */
	wcode[1] = ip->opval_i[1] + r1 * 0200 ;
	eaddr( op2, ip->subop_i, 1) ;
	}
}
else
	if (freg_addr(op2))
		{ /* fmov? ea,fpn */
		wcode[1] = ip->opval_i[0] + r2 * 0200 ;
		eaddr( op1, ip->subop_i, 0) ;
		}
	else 
	{
	if (fctrlreg_addr(op2))
		{ /* fmovl ea,fpctrl */
		switch((int) op2->value_o)
			{
			case FPCREG: { r2 = 4 ; break ; }
			case FPSREG: { r2 = 2 ; break ; }
			case FPIREG: { r2 = 1 ; break ; }
			}
		wcode[1] = ip->opval_i[2] + r2 * 02000 ;
		eaddr( op1, ip->subop_i, 0) ;
		}
	else
		{ /* fmovl fpctrl,ea */
		switch((int) op1->value_o)
			{
			case FPCREG: { r2 = 4 ; break ; }
			case FPSREG: { r2 = 2 ; break ; }
			case FPIREG: { r2 = 1 ; break ; }
			}
		wcode[1] = ip->opval_i[3] + r2 * 02000 ;
		eaddr( op2, ip->subop_i, 0) ;
		}
	}
}

cp_regpair( ip )
	struct ins_bkt *ip ;

/* Works like cp_general except the result is put in a PAIR
 * of fp registers. 
 */

{
struct oper *op1, *op2 ;
unsigned r1, r2, r3 ;

/*
	opval_i[0] -- fop ea,fpn:fpq
	opval_i[1] -- fop fpm,fpn:fpq
*/

op1 = &operands[0] ;
op2 = &operands[1] ;
r2 = (int) op2 -> reg_o - FP0REG ;
r3 = (int) op2 -> value_o - FP0REG ;
code_length = 4 ;
wcode[0] = 0xf000 + current_cpid * 01000 ;

if (freg_addr(op1))
	{
	r1 = (int) op1 -> value_o - FP0REG ;
	wcode[1] = ip->opval_i[1] + r2 * 0200 + r1 * 02000 + r3 ;
	}
else
	{
	wcode[1] = ip->opval_i[0] + r2 * 0200 + r3 ;
	eaddr( op1, ip->subop_i, 0) ;
	}
}

cp_movecr( ip )
	struct ins_bkt *ip ;

/*

	For general format coprocessor instructions that put an immediate
	operand in the op code extension field.

*/

{
struct oper *op1, *op2 ;
unsigned r1, r2 ;

/*
	opval_i[0] -- fop #ccc,fpn
*/

op1 = &operands[0] ;
op2 = &operands[1] ;
r2 = (int) op2 -> value_o - FP0REG ;
code_length = 4 ;
wcode[0] = 0xf000 + current_cpid * 01000 ;
wcode[1] = ip->opval_i[0] + r2 * 0200 + op1->value_o ;
}


int reverse_8bits( forward )
        int forward ;

/*
        result = forward with with 8 low order bits in reverse order.
*/
 
{
int bit, new ;
long i ;

new = 0 ;
bit = 0x80 ;

for (i=0 ; i<8 ; i++ )
        {
        if (forward & 1) new |= bit ;
        forward = forward >> 1 ;
        bit = bit >> 1 ;
        }
return(new) ;
}
 
cp_movem( ip )
	struct ins_bkt *ip ;

/*

	Floating move multiples come in too many varieties:

	fmovem	ea,#imm
	fmovem	ea,freglist
	fmovem	ea,dn
	fmovem	ea,fcreglist
	fmovem	#imm,ea
	fmovem	#imm,sp@-
	fmovem  freglist,ea
	fmovem	freglist,sp@-
	fmovem	dn,ea
	fmovem	dn,sp@-
	fmovem	fcreglist,ea

*/

{
struct oper *op1, *op2 ;

/*
	opval_i[0] -- fmovem	ea,#imm or freglist or dn
	opval_i[1] -- fmovem	...,ea
	opval-i[2] -- fmovem	ea,fcreglist
	opval-i[3] -- fop	fcreglist,ea
*/

op1 = &operands[0] ;
op2 = &operands[1] ;
code_length = 4 ;
wcode[0] = 0xf000 + current_cpid * 01000 ;
switch (op2->type_o)
{
	case T_FCREGLIST:/*	fmovem	ea,fcreglist	*/
		{
		wcode[1] = ip->opval_i[2] + op2->value_o * 0x400 ;
		eaddr( op1, ip->subop_i, 0) ;
		break ;
		}
	case T_IMMED:	/*	fmovem	ea,#imm	*/
		{
		wcode[1] = ip->opval_i[0] + op2->value_o ;
		eaddr( op1, ip->subop_i, 0) ;
		break ;
		}
	case T_FREGLIST:/*	fmovem	ea,freglist	*/
		{
		wcode[1] = ip->opval_i[0] + reverse_8bits(op2->value_o) ;
		eaddr( op1, ip->subop_i, 0) ;
		break ;
		}
	case T_REG:	/*	fmovem	ea,dn	*/
		{
		wcode[1] = ip->opval_i[0] + 0x800 + (op2->value_o-D0REG)*0x10;
		eaddr( op1, ip->subop_i, 0) ;
		break ;
		}
default: 
	{
	switch(op1->type_o)
	{
	case T_FCREGLIST:/*	fmovem	fcreglist,ea	*/
		{
		wcode[1] = ip->opval_i[3] + op1->value_o * 0x400 ;
		break ;
		}
	case T_IMMED:	/*	fmovem	#imm,ea	*/
		{
		wcode[1] = ip->opval_i[1] + op1->value_o ;
		if (op2->type_o != T_PREDEC) wcode[1] |= 0x1000 ; 
		break ;
		}
	case T_FREGLIST:/*	fmovem	freglist,ea	*/
		{
		wcode[1] = ip->opval_i[1] ;
		if (op2->type_o == T_PREDEC) 
			wcode[1] |= op1->value_o ;
		else
			wcode[1] |= 0x1000 | reverse_8bits(op1->value_o); 
		break ;
		}
	case T_REG:	/*	fmovem	dn,ea	*/
		{
		wcode[1] = ip->opval_i[1] + 0x800 + (op1->value_o-D0REG)*0x10;
		if (op2->type_o != T_PREDEC) wcode[1] |= 0x1000 ; 
		break ;
		}
	}
	eaddr( op2, ip->subop_i, 1) ;
	}
}
}

cp_oneword( ip )
	struct ins_bkt *ip ;

/* For one word coprocessor instructions like FSAVE and FRESTORE. */

{
struct oper *op1 ;

/*
	opval_i[0] -- fop ea
*/

op1 = &operands[0] ;
wcode[0] = ip->opval_i[0] + current_cpid * 01000 ;
eaddr( op1, ip->subop_i, 0) ;
	
}

cp_branch( ip )
	struct ins_bkt *ip ;

/* 	For coprocessor 16, 32, and indeterminate branches.	*/

{
        long offs = 0;
        register struct oper *opp = operands;
	int offlong, instlong ;
	int useshort;
 
        wcode[0] = ip->opval_i[0] + current_cpid * 01000 ;
        if (ip->noper_i == 0) 
		{ /* fnop */
		code_length = 4 ;
		wcode[1] = 0 ;
		return ;	
		}
        offs = opp->value_o - (dot + 2);
	if (hflag )
	   useshort = 1;
	else
	   useshort = d2flag||jsrflag;
        if (opp->flags_o & O_COMPLEX) {
           /* not a simple address  */
           opp->value_o = (int) offs;
           rel_val(opp, SUBOP_W, opp->sym_o != 0);
           return;
        }
        if (pass == 1 ) {
          if (cansdi) 
               code_length += useshort? 2 : makesdi(opp, dot+2, SDIP);
	  else
	       code_length += (useshort)?2:4;
          put_rel(opp, SUBOP_L, dot+2, 0);
        }
        else {
           /* pass 2 */
	  if ( opp->sym_o && (opp->sym_o->attr_s & S_DEF) == 0){
                /* external branch -- good luck */
                ;
          } else if (opp->sym_o == 0 || opp->sym_o->csect_s != cur_csect_name){
                PROG_ERROR(E_RELOCATE);
          } else {
               opp->sym_o = 0; /* mark as non relocateable expression */
          }
          offlong = (offs > 32767L || offs < -32768L) ;
          if (ip->opval_i[0] & 0100)      instlong = 1 ; /* fbccl*/ 
	  else if (ip->opval_i[0] & 0200) instlong = 0 ;  /* fbcc  */
	       else				
	          { /* fjcc */
	             instlong = offlong ;
                     if (instlong && !useshort ) 	
			wcode[0] |= 0300 ; /* Op code for fbccl.  */ 
	             else  
                        wcode[0] |= 0200 ; /* Op code for fbcc . */
	          }
	  if (offlong &~ instlong) PROG_ERROR(E_OFFSET); 
          opp->value_o = (int)offs; 
          if (instlong && !useshort) 
	     rel_val(opp, SUBOP_L, opp->sym_o != 0);
	  else 
             rel_val(opp, SUBOP_W, opp->sym_o != 0);
        }
}

cp_conditional ( ip )
	struct ins_bkt *ip ;
/*

	Coprocessor conditional instructions:
		fscc	ea
		fdbcc	dn,label
		ftcc
		ftccw	#imm
		ftccl	#imm

*/

{
struct oper *op1, *op2 ;
long offs = 0;

/*
	opval_i[0] -- fop ea,fpn
	opval_i[1] -- fop fpm,fpn
*/

code_length = 4 ;
wcode[0] = ip->opval_i[0] + current_cpid * 01000 ;
wcode[1] = ip->opval_i[1] ;		/* condition code */
switch(ip->noper_i)
	{
	/* case 0: ftcc - nothing more to do */
	case 1: {
		op1 = &operands[0] ;
		if (op1->type_o == T_IMMED)
			{ /* ftccw or ftccl */
			rel_val(op1, ip->subop_i, 0) ;
			}
		else
			{ /* fscc */
			eaddr( op1, ip->subop_i, 0) ;
			}
		break ;
		}
	case 2: { /* fdbcc */
		op1 = &operands[0] ;
		op2 = &operands[1] ;
       	     	wcode[0] |= op1->value_o;
	        if ( op2->sym_o && (op2->sym_o->attr_s & S_DEF) == 0){
                	/* external branch -- good luck */
                	;
        	} else if (op2->sym_o == 0 || op2->sym_o->csect_s != cur_csect_name){
                	PROG_ERROR(E_RELOCATE);
        	} else {
                	op2->sym_o = 0; /* mark as non relocateable expression */
        	}
        	offs = op2->value_o - (dot + 4);
        	if (offs > 32767L || offs < -32768L)
                	PROG_ERROR(E_OFFSET);
        	op2->value_o = (int)offs;
        	rel_val(op2, SUBOP_W, op2->sym_o != 0);
		break ;
		}
	} 
}

