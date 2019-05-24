#ifndef lint
static  char sccsid[] = "@(#)eaddr.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* eaddr - put in stuff for an effective address */

#include "as.h"

#define forward_ref(o) ((o)->sym_o && !((o)->sym_o->attr_s&S_DEF))

operand_t displacement();

eaddr(opptr, size ,wrtflag)
	register struct oper *opptr;
	subop_t size;
{
	int reg = (int)opptr->value_o;
	int was_displ;
	if (opptr->flags_o & O_BFLD)
	    PROG_ERROR(E_OPERAND); /* unexpected bit field */
	switch(opptr->type_o) {
	case T_REG:
		if (areg(reg) || dreg(reg))
			wcode[0] |= (int)opptr->value_o;
		else 
		PROG_ERROR(E_REG) ;
		break;
	case T_DEFER:
		wcode[0] |= (((int)opptr->value_o) & 07) | 020;
		break;
	case T_POSTINC:
		wcode[0] |= (((int)opptr->value_o) & 07) | 030;
		break;
	case T_PREDEC:
		wcode[0] |= (reg & 07) | 040;
		break;
	case T_DISPL:
		if (reg<-32768 || reg>32767 || opptr->sym_o && 
		!(opptr->sym_o->attr_s&S_DEF) ){
		    if (ext_instruction_set)
			{
			/* looks like a potential index mode */
			opptr->disp_o = reg;
			opptr->value_o = 0;
			was_displ = 1;
			goto  indexmode;
		    }else{
		        if ( !pcreg(opptr->reg_o) || (opptr->flags_o) ) 
			PROG_ERROR(E_OPERAND);
		    }
		}
		if (areg(opptr->reg_o)) {
			wcode[0] |= (opptr->reg_o & 07) | 050;
			rel_val(opptr, SUBOP_W, 0);	/* install displacement */
		} else {
			wcode[0] |= 072;
			if (opptr->sym_o)
				{ /* relocatable or external */
				if (opptr->sym_o && opptr->sym_o->attr_s & S_DEF)
					{ /* symbol is defined in this assembly */
					opptr->sym_o = 0;
					}
				opptr->value_o -= dot + code_length ; /* bias offset */
				rel_val(opptr, SUBOP_W, 1); /* install displacement */
				}
				else
				{ /* absolute */
				rel_val(opptr, SUBOP_W, 0); /* install displacement */
				}
			}
		break;
	case T_PCPLUS:
		/* especially called from ctrl_op to do external PC@() addressing */
		/* looks a lot like the above */
		wcode[0] |= 072;
		opptr->value_o -= dot + code_length ; /* bias offset */
		rel_val(opptr, SUBOP_W, 1); /* <== PC-relative relocation */
		break;
	case T_INDEX:
		was_displ = 0;
	indexmode:
		if (pcreg(opptr->reg_o))
			{
			wcode[0] |= 073;
			if (opptr->sym_o) opptr->disp_o -= dot+code_length ;
			index(opptr, was_displ);	/* compute index word */
			if (opptr->sym_o) opptr->disp_o += dot+code_length ;
			} 
		else 
			{
			wcode[0] |= (opptr->reg_o & 07) | 060;
			index(opptr, was_displ);	/* compute index word */
			}
		break;
	case T_ABSS:
short_absolute:
		wcode[0] |= 070;
		rel_val(opptr, SUBOP_W, 0);	/* install short address */
		break;
	case T_NORMAL:
		/*
		 * Heretofore, this has been handled as long absolute, making
		 * the most expensive addressing mode the default one, and the
		 * only one used by c programs. Because of the rules of C, this
		 * is probably a quite reasonable assumption. But assembly-
		 * language programmers might be more tricky in their placement
		 * of code and data, and might be able to use short absolute.
		 * They might even be able to use PC-relative in an intellegent
		 * manner.
		 */
		/* we have three choices of addressing modes to
		 * use: absolute long, absolute short, and PC
		 * relative.  The latter two are the same size,
		 * but for entirely different applications. One
		 * is for reaching low memory, and the other is
		 * for reaching nearby data in the text segment.
		 */
		if (cansdi){
		    switch (displacement( opptr, !wrtflag, 0, 0, 0 )){
		    case T_ABSS:
			wcode[0] |= 070;
			break;
		    case T_PCPLUS:
			wcode[0] |= 072;
			break;
		    case T_ABSL:
			wcode[0] |= 071;
			break;
		    default:
			sys_error("Unfamiliar sdi address mode in ea:\n %s\n", iline);
		    }
		    break;
		}
		/* else FALL THROUGH */
	case T_ABSL:
		wcode[0] |= 071;
		rel_val(opptr, SUBOP_L, 0);
		break;
	case T_IMMED:
		wcode[0] |= 074;
		rel_val(opptr, (size == SUBOP_B)? SUBOP_W:size, 0);	/* change bytes to words */
		break;
	default:
		sys_error("Unrecognized address mode in:\n %s\n", iline);
	}
}

static
vshort_absolute(opptr)
    register struct oper *opptr;
{
    if (opptr->sym_o==0 && -128 <= opptr->value_o && opptr->value_o <= 127){
	return 1;
    } 
    return 0;
}

static
short_absolute(opptr)
    register struct oper *opptr;
{
    if (d2flag
	|| (opptr->sym_o==0 && -32768 <= opptr->value_o && opptr->value_o <= 32767 )){
	rel_val(opptr, SUBOP_W, 0);
	return 1;
    } 
    return 0;
}

static
long_absolute(opptr, pc_ok, null_ok, indexing, was_displ)
    struct oper *opptr;
{
    /* the form of last resort. Must always succeed */
    int sdiflavor;
    if (pass==1 && cansdi && opptr->sym_o && !(opptr->sym_o->attr_s&S_DEF)  ){
	if (indexing || was_displ) sdiflavor = SDIX;
	else if (null_ok) sdiflavor = SDI6;
	else sdiflavor = SDIP;
	(void)makeddi( opptr, dot+code_length, sdiflavor, !pc_ok);
    }
    rel_val( opptr, SUBOP_L, 0);
    return 1;
}

pc_relative(opptr)
    struct oper *opptr;
{
	register struct sym_bkt *sym = opptr->sym_o;
	if (sym && (sym->attr_s & S_DEF) && 
	    (sym->csect_s == cur_csect_name
	    ||(rflag  && sym->csect_s != C_BSS))){
	    /* looks like pc-relative is a real possibility */
	    if (pass==1){
		(void)makesdi( opptr, dot+code_length, SDIP);
		rel_val( opptr, SUBOP_W, 0);
 		return 1;
	    } else {
		register int offset = opptr->value_o - (dot + code_length);

		if ( offset >= -32768L  && offset <= 32767L){
		    opptr->value_o -= dot + code_length;
		    opptr->sym_o = 0; /* no relocation */
		    rel_val(opptr, SUBOP_W, 0);	/* install displacement */
		    return 1;
		}
	    }
	}
	return 0;
}

static operand_t
displacement( opptr, pc_ok, null_ok, indexing, was_displ )
    register struct oper *opptr;
    int pc_ok, null_ok, indexing, was_displ;
{
    /* we have three choices of addressing modes to
     * use: absolute long, absolute short, and PC
     * relative.  The latter two are the same size,
     * but for entirely different applications. One
     * is for reaching low memory, and the other is
     * for reaching nearby data in the text segment.
     */
    if (null_ok && opptr->sym_o == 0 && opptr->value_o == 0 ){
	return T_NULL;
    } else if (indexing && vshort_absolute(opptr)){
	return T_INDEX;
    } else if (short_absolute(opptr)){
	return T_ABSS;
    } else if ( pc_ok &&  cansdi && pc_relative(opptr) ){
	return T_PCPLUS;
    } else {
	long_absolute( opptr, pc_ok, null_ok, indexing, was_displ );
	return T_ABSL;
    }
}

/*
 * index -	Use data in operand structure to compute an index word.
 */

static
index(opptr, was_displ)
	register struct oper *opptr;
	int was_displ;
{
	register int indexval = 0;
	int displ1=0, displ2=0, shortform;
	int c, f, reg;

	reg = opptr->value_o;
	if (areg(reg) || dreg(reg))
	    indexval = reg << 12;
	else PROG_ERROR(E_REG);
	if (opptr->flags_o&O_LINDEX) indexval |= 0x0800;
	switch (opptr->scale_o){
	case 1 : break;
	case 2 : indexval |= 1<<9; break;
	case 4 : indexval |= 2<<9; break;
	case 8 : indexval |= 3<<9; break;
	}
	opptr->value_o = opptr->disp_o;
	f = opptr->flags_o;
	c = code_length >>1;
	code_length += 2;
	if (f&O_WDISP){
	    /* force word displacement */
	    if ( opptr->disp_o < -32768 || opptr->disp_o > 32767
	      || opptr->sym_o && opptr->sym_o->csect_s == 0 )
		PROG_ERROR(E_OFFSET) ;
	    shortform = 0;
	    displ1 = 2<<4; 
	    rel_val(opptr,SUBOP_W , 0);
	}else if (f&O_LDISP || (ext_instruction_set && !cansdi)){
	    /* force long displacement */
	    shortform = 0;
	    displ1 = 3<<4; 
	    rel_val(opptr,SUBOP_L , 0);
	}else{
	    shortform =  (!(f & (O_BSUPRESS|O_POSTINDEX|O_INDIRECT))
		&& (f&O_PREINDEX) );
	    if (ext_instruction_set && cansdi && !(f&O_BDISP)){
		if (pass == 1 && forward_ref(opptr)) {
		    prog_error(E_FORWARD);
		}
		switch (displacement(opptr, 0 , 1, shortform, was_displ)){
		case T_NULL: /* displacement is 0 */
		    displ1 = 1<<4; 
		    break;
		case T_INDEX: /* displacement is small */
		    break;
		case T_ABSS: /* displacement is moderate */
		    displ1 = 2<<4; 
		    shortform = 0;
		    break;
		case T_ABSL: /* displacement is huge*/
		    displ1 = 3<<4; 
		    shortform = 0;
		    break;
		}
	    }
	    /* else !ext_instruction_set: use short form */
	    if (shortform || (f&O_BDISP)){
		if (opptr->disp_o < -128 || opptr->disp_o > 127
		  || opptr->sym_o && opptr->sym_o->csect_s == 0)
		    PROG_ERROR(E_OFFSET) ;
		indexval |= opptr->disp_o & 0377;
		wcode[c] = indexval; /* install index word */
		return;
	    }
	}
	/* long form indexing */
	/* first displacement */
	if (f&O_BSUPRESS)
	    indexval |= 1<<7; /* base  supress */
	if (!(f&(O_PREINDEX|O_POSTINDEX)))
	    indexval |= 1<<6; /* index supress */
	indexval |= displ1 | (1<<8);
	if (!(f&O_INDIRECT))
	    displ2 = 0; /* no indirection */
	else {
	    opptr->value_o = opptr->disp2_o;
	    opptr->sym_o = opptr->sym2_o;
	    if (f&O_LDISP2 || !cansdi){
		/* long displacement */
		displ2 = 3;
		rel_val(opptr,SUBOP_L , 0);
	    }else if (f&O_WDISP2){
		/* word displacement */
		if (opptr->disp2_o < -32768 || opptr->disp2_o > 32767
		  || opptr->sym2_o && opptr->sym2_o->csect_s == 0)
		    PROG_ERROR(E_OFFSET) ;
		displ2 = 2;
		rel_val(opptr,SUBOP_W , 0);
	    } else {
		if (pass == 1 && forward_ref(opptr)) {
		    prog_error(E_FORWARD);
		}
		switch (displacement( opptr, 0, 1, 0, 0 )){
		case T_NULL:
		    displ2 = 1; break;
		case T_ABSS:
		    displ2 = 2; break;
		case T_ABSL:
		    displ2 = 3; break;
		}
	    }
	    indexval |= displ2 | ((f&O_POSTINDEX)?(1<<2):0);
	    
	}
	wcode[c] = indexval; /* install index word */
}
