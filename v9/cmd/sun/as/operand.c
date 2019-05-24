#ifndef lint
static	char sccsid[] = "@(#)operand.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include <ctype.h>

short cinfo[128] = {
	ERR,	ERR,	ERR,	ERR,	ERR,	ERR,	ERR,	ERR,
	ERR,	SPC,	EOL,	SPC,	SPC,	SPC,	ERR,	ERR,
	ERR,	ERR,	ERR,	ERR,	ERR,	ERR,	ERR,	ERR,
	ERR,	ERR,	ERR,	ERR,	ERR,	ERR,	ERR,	ERR,
	SPC,	ERR,	QUO,	IMM,	S+T,	ERR,	ERR,	ERR,
	LP,	RP,	MUL,	ADD,	COM,	SUB,	S+T,	DIV,
	D+T,	D+T,	D+T,	D+T,	D+T,	D+T,	D+T,	D+T,
	D+T,	D+T,	COL,	EOL,	ERR,	EQL,	ERR,	ERR,
	IND,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,
	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,
	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,
	S+T,	S+T,	S+T,	ERR,	ERR,	ERR,	ERR,	S+T,
	ERR,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,
	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,
	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,	S+T,
	S+T,	S+T,	S+T,	LB,	EOL,	RB,	NOT,	S+T
};  

/* for local labels: ``[0-9]:'', and references ``[0-9][bh]'' */
char *ll_format = "*%c%06d";
int   ll_val[10];

static struct suffix {
    int		indx_sx;	/* index register */
    int		scale_sx;	/* index scaling  */
    long	disp_sx;	/* displacement   */
    struct sym_bkt *sym_sx;	/* symbolic displacement */
    unsigned	flags_sx;	/* see below 	  */
} fix [2];

static struct suffix zero_suffix = {  0  };

/* flags_sx values */
#define SX_INDXW	1	/* word-size index specified */
#define SX_INDXL	2	/* long-size index specified */
#define SX_DISPW	4	/* word-size displacement specified */
#define SX_DISPL	010	/* long-size displacement specified */
#define SX_GOTINDEX	020	/* index register specified */
#define SX_GOTDISP	040	/* displacement specified */
#define SX_DISPB	0100	/* word-size displacement specified */

static struct oper zero_oper  = { T_NULL };

char *suffix(), *exp(), *term(), *scan_reg_or_immed();
char * scan_float();
double atof();
char * reglist () ;

/* 
 * Fetches operand value and register subfields and loads them into
 * the operand structure. This routine will fetch only one set of value
 * and register subfields. 
 * It will move line pointer to first untouched char.
 */
char *
soperand(lptr,opnd)
	register char *lptr;
	struct oper *opnd;
{
	struct oper   spare_oper;
	int v;

	*opnd = zero_oper;
	spare_oper = zero_oper;

	if ((v=cinfo[*lptr]) == IMM) {
		lptr = exp(++lptr,opnd);
		if (opnd->type_o == T_REG) 
			PROG_ERROR(E_REG);
		opnd->immed_o = opnd->type_o ; /* Save type. */
		opnd->type_o = T_IMMED;
	} else if (v == IND) {
	    /* index mode, omitted base */
	    /* expecting LP next        */
	    lptr += 1;
	    skipb(lptr);
	    if (cinfo[*lptr] == LP)
		goto index_mode;
	    else
		PROG_ERROR(E_OPERAND);
	} else { 
	    lptr = exp(lptr,opnd);
	    skipb(lptr);
	    switch (opnd->type_o){
	    case T_REG:
		switch (cinfo[*lptr]){
		case COL:	
			/* should be a register pair */
			lptr = exp( ++lptr, &spare_oper);
			if (spare_oper.type_o != T_REG)
			    PROG_ERROR(E_OPERAND);
			if (!ext_instruction_set)
			    PROG_ERROR( E_OPERAND );
			opnd->type_o = T_REGPAIR;
			opnd->reg_o = spare_oper.value_o;
			break;
		case IND:
			if (!(areg(opnd->value_o)
				|| pcreg(opnd->value_o)))
			    PROG_ERROR(E_OPERAND);
			lptr += 1;
			skipb(lptr);
			switch (cinfo[*lptr]){
			default:
			    opnd->type_o = T_DEFER;
			    break;
			case ADD:
			    opnd->type_o = T_POSTINC;
			    lptr++;
			    break;
			case SUB:
			    opnd->type_o = T_PREDEC;
			    lptr++;
			    break;
			case LP:
			    /* defer or indexing coming up here */
		index_mode:
			    lptr = suffix( lptr, &fix[0] );
			    if (cinfo[*lptr] != IND){
				simplify1( opnd, &fix[0] );
			    } else {
				lptr = suffix( ++lptr, &fix[1] );
				simplify2( opnd, &fix[0], &fix[1] );
			    }
			    break;
			case IND:
			    lptr = suffix( ++lptr, &fix[1] );
			    simplify2( opnd, &zero_suffix, 
				&fix[1] );
			    break;
			}
			if (opnd->type_o == T_INDEX 
				&& !ext_instruction_set)
			if ( (opnd->flags_o &
			(O_BSUPRESS|O_INDIRECT|O_WDISP|O_LDISP))
 			|| opnd->scale_o != 1 
			|| !(opnd->flags_o&O_PREINDEX))
				PROG_ERROR( E_OPERAND );
			/* flow through */
		default:
			break;
		}
		break; 
	    case T_NORMAL:
		if  (cinfo[*lptr] == COL ){
		    switch (*++lptr) { 
		        case 'W':
		        case 'w': opnd->type_o = T_ABSS;
				  lptr++;
				  break;
			case 'L':
			case 'l': opnd->type_o = T_ABSL;
				  lptr++;
				  break;
		    }
		}
		if (cinfo[*lptr] == IND)
		    goto index_mode;
		break; 
	    }
    }
    skipb( lptr );
    if ( cinfo[ *lptr ] == LB ) {
	/* oh, boy, bitfields */
	opnd->flags_o |= O_BFLD;
	spare_oper = zero_oper;
	lptr = scan_reg_or_immed( lptr+1, &spare_oper);
	v = spare_oper.value_o;
	if (spare_oper.type_o == T_REG){
	    opnd->flags_o |= O_BFOREG;
	} else if ((spare_oper.sym_o && !(spare_oper.sym_o->attr_s&S_DEF)) 
		|| v < 0 || v > 31){
	    PROG_ERROR(E_CONSTANT);
	}
	opnd->bfoffset_o = v;
	skipb( lptr );
	if ( cinfo[ *lptr ] == COL)
	    lptr ++;
	else
	    PROG_ERROR(E_OPERAND);
	spare_oper = zero_oper;
	lptr = scan_reg_or_immed( lptr, &spare_oper);
	v = spare_oper.value_o;
	if (spare_oper.type_o == T_REG){
	    opnd->flags_o |= O_BFWREG;
	} else if ((spare_oper.sym_o && !(spare_oper.sym_o->attr_s&S_DEF)) 
		|| v < 0 || v > 32){
	    PROG_ERROR(E_CONSTANT);
	}
	opnd->bfwidth_o = (v == 32 ? 0 : v);
	skipb( lptr );
	if ( cinfo[ *lptr ] == RB )
	    lptr++;
	else
	    PROG_ERROR(E_OPERAND);
    }
    return(lptr);
} /* end soperand */

static char * 
scan_reg_or_immed( lptr, o )
    register char *lptr;
    register struct oper *o;
{
    skipb( lptr );
    if (cinfo[*lptr] == IMM) {
	    lptr = exp(++lptr,o);
	    if (o->type_o == T_REG) 
		    PROG_ERROR(E_REG);
	    if (o->sym_o && !(o->sym_o->attr_s&S_DEF))
		    PROG_ERROR(E_REG);
	    o->type_o = T_IMMED;
    } else {
	lptr = exp( lptr, o );
	if ( o->type_o != T_REG || o->value_o > D7REG)
	    PROG_ERROR(E_OPERAND);
    }
    return lptr;
}

static char *
opt_length( lptr, fp, wflag, lflag, bflag )
    register char * lptr;
    struct suffix * fp;
{
    char * save = lptr;
    if (cinfo[ *lptr ] == COL){
	lptr++;
	skipb( lptr );
	switch ( *lptr ){
	case 'b':
	case 'B':
		if (bflag == 0) return save ;
		fp->flags_sx |= bflag;
		lptr++;
		break;
	case 'l':
	case 'L':
		fp->flags_sx |= lflag;
		lptr++;
		break;
	case 'w':
	case 'W':
		fp->flags_sx |= wflag;
		lptr++;
		break;
	default: return save; /* : yes, length, no: may be scale */
	}
    }
    return lptr;
}
static char *
opt_scale( lptr, fp )
    char * lptr;
    struct suffix * fp;
{
    fp->scale_sx = 1;
    if (cinfo[ *lptr ] == COL){
	lptr++;
	skipb( lptr );
	switch ( *lptr ){
	case '1':
	case '2':
	case '4':
	case '8':
		fp->scale_sx = *lptr - '0';
		lptr++;
		break;
	default: PROG_ERROR(E_OPERAND);
	}
    }
    return lptr;
}

static char *
suffix( lptr, fixp )
    register char * lptr;
    register struct suffix * fixp;
{
    /*
     * scan for open paran. look for displacement, optional length
     * specifier, then index register, optional length specifier, 
     * optional scale specifier.
     * fill in suffix structure with what we find.
     */
    struct oper o;
    o = zero_oper;
    *fixp = zero_suffix;
    skipb( lptr );
    if (cinfo[ *lptr ] == LP )
	lptr += 1;
    else
	return lptr; /* guess that null suffix might be ok */
    lptr = exp( lptr , &o );
    if ( o.type_o == T_NORMAL ){
	fixp->disp_sx = o.value_o;
	fixp->sym_sx = o.sym_o;
	fixp->flags_sx |= SX_GOTDISP;
	lptr = opt_length( lptr, fixp, SX_DISPW, SX_DISPL, SX_DISPB );
	skipb( lptr );
	if (cinfo[ *lptr ] ==  COM)
	    lptr = exp( ++lptr, &o );
	else
	    goto scan_rp;
    }
    /* now we're talking index */
    if ( o.type_o != T_REG ){
	PROG_ERROR( E_OPERAND);
    }
    fixp->flags_sx |= SX_GOTINDEX;
    fixp->indx_sx = o.value_o;
    lptr = opt_length( lptr, fixp, SX_INDXW, SX_INDXL, 0 );
    lptr = opt_scale( lptr, fixp );
scan_rp:
    skipb( lptr );
    if (cinfo[ *lptr ] == RP)
	lptr ++;
    else
	PROG_ERROR( E_OPERAND);
    return lptr;
}

static
simplify1( oper, fp )
    register struct oper * oper;
    register struct suffix *fp;
{
    register bval = oper->value_o;
    /* we have some form of deferred or indexed addressing here */
    /* try to make the best of it.                              */
    /*
     * examples are:
     *    a0@(disp)
     *    a0@(disp,d0:w:2)
     *    a0@(d0:w:2)
     *    pc@(disp)
     *    pc@(disp,d0:w:2)
     *    pc@(d0:w:2)
     *      @(d0)
     *      @(disp,d0)
     */
    switch (oper->type_o){
    case T_REG:
	if( !(fp->flags_sx&(SX_GOTINDEX|SX_DISPL)) ){
	    /* displacement mode */
	    oper->type_o = T_DISPL;
	    oper->reg_o = bval;
	    oper->value_o = fp->disp_sx;
	    oper->sym_o = fp->sym_sx;
	    if (fp->flags_sx & SX_DISPW)
		oper->flags_o|= O_WDISP;
	    else if (fp->flags_sx & SX_DISPB) {
		PROG_ERROR(E_OPERAND);
		oper->flags_o|= O_WDISP;
	    }
	} else {
	    /* index mode */
imode:
	    oper->type_o = T_INDEX;
	    oper->reg_o = bval;
	    oper->disp_o = fp->disp_sx;
	    oper->sym_o = fp->sym_sx;
	    if (fp->flags_sx&SX_GOTINDEX){
		oper->value_o = fp->indx_sx;
		oper->scale_o = fp->scale_sx;
		if (fp->flags_sx&SX_INDXW)
		    oper->flags_o|= O_WINDEX|O_PREINDEX;
		else 
		    oper->flags_o|= O_LINDEX|O_PREINDEX;
	    } else {
		oper->value_o = 0;
		oper->scale_o = 0;
	    }
	    if (fp->flags_sx & SX_DISPL)
		oper->flags_o|= O_LDISP;
	    /* else short form indexing */
	    else if (fp->flags_sx & SX_DISPW)
		oper->flags_o|= O_WDISP;
	    else if (fp->flags_sx & SX_DISPB)
		oper->flags_o|= O_BDISP;
	}
	return;
    case T_NULL:
	/* @(disp,d0)		      */
	/* @(d0)                      */
	/* index mode, supressed base */
	oper->type_o= T_INDEX;
	oper->flags_o|= O_BSUPRESS;
	oper->reg_o = A0REG; /* Supress THIS register */
	oper->disp_o= fp->disp_sx;
	oper->sym_o= fp->sym_sx;
	if(fp->flags_sx & SX_DISPL)
	   oper->flags_o |= O_LDISP;
	else if (fp->flags_sx & SX_DISPW)
	   oper->flags_o |= O_WDISP;
	else if (fp->flags_sx & SX_DISPB) {
	   PROG_ERROR(E_OPERAND);
	   oper->flags_o |= O_WDISP;
	}
    do_index:
	if (fp->flags_sx & SX_GOTINDEX){
	    oper->value_o = fp->indx_sx;
	    oper->scale_o = fp->scale_sx;
	    if (fp->flags_sx & (SX_INDXW) )
		oper->flags_o |= O_PREINDEX|O_WINDEX;
	    else
		oper->flags_o |= O_PREINDEX|O_LINDEX;
	} else {
	    oper->value_o = 0;
	    oper->scale_o = 0;
	}
	return;
    case T_ABSS:
	oper->flags_o = O_WDISP;
	goto normal;
    case T_ABSL:
	oper->flags_o = O_LDISP;
	goto normal;
    case T_NORMAL:
	/* disp@			*/
	/* disp@(d0)			*/
	/* index mode, supressed base 	*/
	oper->flags_o = 0;
    normal:
	oper->type_o = T_INDEX;
	oper->flags_o|= O_BSUPRESS;
	oper->reg_o = A0REG; /* Supress THIS register */
	oper->disp_o = oper->value_o;
	if (fp->flags_sx&SX_GOTDISP)
	    PROG_ERROR(E_OPERAND);
	goto do_index;
    default:
	    PROG_ERROR(E_OPERAND);
    }
}

static
simplify2( oper, fp, dfp )
    register struct oper * oper;
             struct suffix *fp;
    register struct suffix *dfp;
{
    /*
     * we have a memory indirect operand.
     * examples:
     *	a0@(disp:l,d0:l:4)@(disp:w)
     *  a0@(disp)@(disp,d0:l:4)
     *  a0@@
     *    @(disp,d0)@(disp)
     *    @(d0)@
     *    @(disp)@
     *    @(disp,d0)@
     *	  @(disp)@(d0)
     */
    simplify1( oper, fp );
    if (oper->type_o == T_DISPL){
	oper->type_o = T_INDEX;
	oper->disp_o = oper->value_o;
	oper->value_o = 0;
    }
    /* now build on that with second suffix */
    oper->flags_o |= O_INDIRECT;
    oper->disp2_o = dfp->disp_sx;
    oper->sym2_o  = dfp->sym_sx;
    if (dfp->flags_sx & SX_DISPL)
	oper->flags_o |= O_LDISP2;
    else if (dfp->flags_sx & SX_DISPW)
	oper->flags_o |= O_WDISP2;
    else if (dfp->flags_sx & SX_DISPB) {
	PROG_ERROR(E_OPERAND);
	oper->flags_o |= O_WDISP2;
    }
    if (dfp->flags_sx & SX_GOTINDEX){
	if (oper->flags_o&O_PREINDEX)
	    PROG_ERROR(E_OPERAND);
	oper->value_o = dfp->indx_sx;
	oper->flags_o |= O_POSTINDEX;
	oper->scale_o = dfp->scale_sx;
	if (dfp->flags_sx&SX_INDXW)
	    oper->flags_o |= O_WINDEX;
	else
	    oper->flags_o |= O_LINDEX;
    }

}

/* read expression */
char *
exp(lptr, arg1)
    register char *lptr;
    register struct oper *arg1;
{
    struct oper arg2;	       	/* holds value of right hand term */
    register int i;
    static struct oper zero_opr;
    register char *lptr0 ;

    lptr0 = lptr ;		/* Save pointer to beginning for 
					use by reglist. */
	skipb(lptr);		/* Find the operator */
    i = cinfo[*lptr];
    if (i==EOL || i==COM){        /* nil operand is zero */
	arg1->sym_o = NULL;
        arg1->value_o = 0;
        return(lptr);
    }
    lptr = term( lptr, arg1 );

    while (1) {
	  arg2 = zero_opr;
	  skipb(lptr);
	  switch (cinfo[*lptr]) {
	    case ADD:	lptr = term( ++lptr, &arg2 );
			if (arg1->type_o==T_REG || arg2.type_o==T_REG) 
                           break;
			if (arg1->sym_o && arg2.sym_o) 
                           if (pass==2) break;
			if (arg2.sym_o) 
                           arg1->sym_o = arg2.sym_o;
			arg1->value_o += arg2.value_o;
			arg1->flags_o |= arg2.flags_o&O_COMPLEX;
			continue;

	    case MUL:	lptr = term(++lptr,&arg2);
			if (arg1->type_o==T_REG || arg2.type_o==T_REG) 
                           break;
			if (arg1->sym_o || arg2.sym_o) 
                           if (pass==2) break;
			arg1->value_o *= arg2.value_o;
			arg1->flags_o |= arg2.flags_o&O_COMPLEX;
			continue;

	    case DIV:	lptr = term( ++lptr, &arg2 );
			if (arg1->type_o==T_REG || arg2.type_o==T_REG) 
			   if (arg1->type_o==T_REG && arg2.type_o==T_REG) 
                           	{ /* try move multiple */
				lptr = lptr0 ;
				return(reglist( lptr, arg1)) ;
				}
			   else break;
			if (arg1->sym_o || arg2.sym_o) 
                           if (pass==2) break;
			   else if (arg2.value_o == 0) arg2.value_o = 1;
			arg1->value_o /= arg2.value_o;
			arg1->flags_o |= arg2.flags_o&O_COMPLEX;
			continue;

	    case SUB:	lptr = term( ++lptr, &arg2 );
			if (arg1->type_o==T_REG || arg2.type_o==T_REG) 
			   if (arg1->type_o==T_REG && arg2.type_o==T_REG) 
                           	{ /* try move multiple */
				lptr = lptr0 ;
				return(reglist( lptr, arg1)) ;
				}
			   else break;
			if (arg2.sym_o){	/* if B is relocatable, */
			   if (arg1->sym_o){  	/* and A is relocatable, */
			       if ((arg1->sym_o->csect_s == 0) &&
				   ( arg2.sym_o->csect_s != 0))
					{ /* special case: external - relocatable */
				    	arg1->flags_o |= O_COMPLEX;	
			       		}
				else
				if (arg2.sym_o->csect_s 
					!= arg1->sym_o->csect_s
			       && pass == 2){
				   break;  /* break into error */
			       } else {   /* result is absolute (no offset) */
				    arg1->sym_o = NULL;	
				    /* but not a simple address for sdi's */
				    arg1->flags_o |= O_COMPLEX;	
			       }
			   } 
                           else break;    /* if B rel., and A is not, */
                                          /* then break into relocation error */
			}
			arg1->value_o -= arg2.value_o;
			continue;

	    default:	return(lptr);
	  }
	PROG_ERROR(E_RELOCATE);
	return(lptr);
    }
} /* end exp <read expression> */

/* read term: either symbol, constant, or unary minus */
char *
term( lptr, vp )
	register char *lptr;
	register struct oper *vp;
{
	register struct sym_bkt *sbp;
	register int base = 10;
	register long val;
	register char *p;
	register int i;
	char savechar, *t;
	char token[50];

	*vp = zero_oper;
	skipb(lptr);
	i = cinfo[*lptr];

	/* here for number */
	if (i & D){
		p = lptr;
		val = 0;
		if (*lptr == '0'){ 
			lptr++;
			switch (*lptr ){
			case 'x':
			case 'X':
				lptr++; 
				/* hexidecimal number */
				while (1){
					if (cinfo[*lptr] & D)
						val = val*16 + *lptr++ - '0';
					else if (*lptr>='A' && *lptr<='F')
						val = val*16 + *lptr++ - 'A' + 10;
					else if (*lptr>='a' && *lptr<='f')
						val = val*16 + *lptr++ - 'a' + 10;
					else break;
				}
				break;
			case 'r':
			case 'R':
				lptr = scan_float( lptr, vp );
				return (lptr);
			default:
				/* octal number */
				while (cinfo[*lptr] & D){
					if (*lptr == '8' || *lptr == '9')
						PROG_ERROR(E_CONSTANT);
					val = val*8 + *lptr++ - '0';
				}
			}
		} else {
			/* decimal number */
			while (cinfo[*lptr] & D)
				val = val*10 + *lptr++ - '0';
		}
	    got_a_number:
		if (*lptr == '$')
			{ lptr = p; goto sym; }
		if (*lptr == 'b' || *lptr == 'f'){
			/* local label reference, of form [0-9][bh] */
			/* single digits only			    */
			if (lptr > p+1 ){
			    PROG_ERROR(E_SYMLEN);
			}
			val = ll_val[*p-'0'] + (*lptr++=='f');
			if (val < 0 ) PROG_ERROR(E_SYMDEF);
			sprintf(token, ll_format, *p, val);
			sbp = lookup(token);
			goto sym2;

		}
		vp->value_o = val;
		vp->sym_o = NULL;
		vp->type_o = T_NORMAL;
		return(lptr);
	}

	/* here for symbol name */
	if (i & S) {
sym:		t = lptr;
		while (cinfo[*lptr] & T)
			lptr++;
		savechar = *lptr;
		*lptr = '\0';	  
		sbp = lookup(t);		/* find its symbol bucket */
		*lptr = savechar;
sym2:
		if (sbp->attr_s & S_DEF)	/* if it's defined, use its value */
			vp->value_o = sbp->value_s;
	        else 
			vp->value_o = 0;
                if (sbp->attr_s & S_REG) {
                        vp->sym_o = NULL;
                        vp->type_o = T_REG;
               } else {
			vp->sym_o = ((sbp->attr_s & S_DEF) &&
			    (sbp->csect_s == C_UNDEF )) ? 0 : sbp;
			vp->type_o = T_NORMAL;
		}
		return(lptr);
	}

	/* check for unary minus */
	if (i == SUB) {
		lptr = term(++lptr,vp);
		if (vp->sym_o) PROG_ERROR(E_RELOCATE);
		vp->value_o = -(vp->value_o);
		vp->fval_o = -(vp->fval_o);
		vp->dval_o = -(vp->dval_o);
		return(lptr);
	}

	/* check for complement */
	if (i == NOT) {
		lptr = term(++lptr,vp);
		if (vp->sym_o) PROG_ERROR(E_RELOCATE);
		vp->value_o = ~(vp->value_o);
		return(lptr);
	}

	/* here for string */
	if (i == QUO) {
		vp->stringval_o = lptr+1;
		/* scan over the quoted string, being careful of \-stuff */
		do{
			i = *++lptr;
	      		if (i=='\\')
			switch (i = *++lptr){
			case '"': i = '\\'; /* fake-out loop test by covering up \"*/
	      		}
	  	}while (i!='\n' && i!='"');
		if (i == '\n'){
			PROG_ERROR(E_STRING);
		}
		*lptr++ = 0;
	  	vp->sym_o = NULL;
		vp->type_o = T_STRING;
	  	return(lptr);
	}

	/* new, improved kluge -- parens !! */
	if (i==LP) {
		lptr = exp(++lptr, vp);
		skipb(lptr);
		if ((i=cinfo[*lptr]) == RP) {
			lptr++;
		} else {
			PROG_ERROR( E_PAREN );
		}
		return( lptr );
	}

	PROG_ERROR(E_TERM);
	return(lptr);
} /* end term */

char lowcase( c ) 
	char c ;
{
if (('A' <= c) && (c <= 'Z')) return(c+32) ; else return(c) ;
}

char *
scan_float( lptr, vp )
    register char * lptr;
    struct oper *vp;
{
    /*
     * lptr address a string which is a 'd' or an 'f', followed
     * by a floating-point number. The latter looks like:
     *     [+-]nan
     * or  [+-]nan( [0-9a-f]+ )
     * or  [+-]inf
     * or  [+-][0-9]+[.[0-9]+][e[+-][0-9]+]
     * Upper/lower case may be used interchangeably.
     * For numbers, call atof to do the conversion work.
     * We do not take great care in syntax checking the number.
     */
    int s=0;
    union{ double d; float f; int x[2]; } fp;
    unsigned nanno, nanno2;
    register char *nbegin;
    char c;

    	lptr++ ;	/* Skip F or f. */
	nbegin = lptr ;
again:
    switch (lowcase( *lptr++)){
    case '+':  if (s) goto bad_num; s = 1; goto again;
    case '-':  if (s) goto bad_num; s = -1; goto again;
    case 'n': /* nan */
	if (lowcase(*lptr++) != 'a') goto bad_num;
	if (lowcase(*lptr++) != 'n') goto bad_num;
	nanno = nanno2 = 0;
	if (*lptr == '(')
		{ /* nan significand */ 
		lptr++ ; 	/* pass paren */
		while( isxdigit(c = *lptr++)){
	    	if (isdigit(c)) c -= '0';
	    	else if (isupper(c)) c -= 'a'-10;
	    	else c -= 'A'-10;
	    	nanno2 = (nanno2 << 8) | (nanno >> 24);
	    	nanno = (nanno<<8) | c;
		}
		if (c != ')') goto bad_num;
		} /* nan significand */
	if ((nanno | nanno2) == 0) nanno = -1 ; /* avoid zero nan */
	goto make_indef;
	    
    case 'i':	/* inf */
	if (lowcase(*lptr++) != 'n') goto bad_num;
	if (lowcase(*lptr++) != 'f') goto bad_num;
        if (strncmp(lptr,"inity",5) == 0) lptr += 5;
 	nanno = nanno2 = 0;
    make_indef:
	    fp.x[0] = ((s<0)?0xfff00000:0x7ff00000) | nanno2;
	    fp.x[1] = nanno;
	break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': 
	/* scan for more digits */
	while( isdigit( c = *lptr++ ) )
	    continue;
        if (c != '.' )
            goto scan_exp;
        /* FALL THROUGH */
      case '.':
        /* scan for more digits */
        while( isdigit( c = *lptr++ ) )
            continue;
      scan_exp:
 	s = 0;
	switch(lowcase(c)){
	case '+':
	case '-':
	    s = 1;
	    /* FALL THROUGH */
	case 'e':
	    switch ( c= *lptr++ ){
	    case '+':
	    case '-':
		if (s) goto bad_num;
		c = *lptr++;
		/* FALL THROUGH */
	    default:
		s = 0; /* count chars: 0 is not ok */
		while( isdigit(c) ){
		    c = *lptr++;
		    s++;
		}
		if (s == 0) goto bad_num;
		break;
	    }
	    break;
	}
	    fp.d = atof( nbegin );
	lptr--;
	break;
    default:
    bad_num:
	PROG_ERROR( E_BADCHAR );
	return lptr;
    }
	vp->dval_o = fp.d;
	vp->value_o = (int) fp.d;
	vp->type_o = T_DOUBLE;
    return lptr;
}

static void
printoffset( v, s )
    struct sym_bkt *s;
{
    if (s != NULL){
	fputs(s->name_s, stdout);
	if (v)
	    printf("+%d", v);
    } else {
	printf("%d", v);
    }
}

static char *rnames[] = {
    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", 
    "sp", "pc", "cc", "sr", "usp", "sfc", "dfc", "vbr", 
};

static void
printindex( o )
    register struct oper *o;
{
    printf(",%s:%c", rnames[o->value_o], 
	o->flags_o&O_WINDEX ? 'w' : 'l');
    if (o->scale_o != 1) printf(":%d", o->scale_o);
}

printop( o )
    register struct oper *o;
{
    switch (o->type_o){
    case T_REG:		printf("%s",   rnames[o->value_o]); break;
    case T_DEFER:	printf("%s@",  rnames[o->value_o]); break;
    case T_POSTINC:	printf("%s@+", rnames[o->value_o]); break;
    case T_PREDEC:	printf("%s@-", rnames[o->value_o]); break;
    case T_DISPL:	printf("%s@(", rnames[o->reg_o]);
			printoffset(o->value_o, o->sym_o);
			putchar(')');
			break;
    case T_INDEX:	printf("%s@(", (o->flags_o&O_BSUPRESS) 
				? "XX" : rnames[o->reg_o]);
			printoffset(o->disp_o, o->sym_o);
			if (o->flags_o & O_WDISP) fputs(":w", stdout);
			else if (o->flags_o & O_LDISP) fputs(":l", stdout);
			if (o->flags_o&O_PREINDEX)
			    printindex( o );
			putchar(')');
			if (o->flags_o&O_INDIRECT){
			    fputs("@(", stdout);
			    printoffset(o->disp2_o, o->sym2_o);
			    if (o->flags_o&O_POSTINDEX)
				printindex( o );
			    putchar(')');
			}
			break;
    case T_ABSS:
    case T_ABSL:
			printoffset(o->value_o, o->sym_o);
			fputs( (o->type_o == T_ABSS) ? ":w" : ":l", stdout);
			break;
    case T_IMMED:
			putchar('#');
			/* FALL THROUGH */
    case T_NORMAL:
			printoffset(o->value_o, o->sym_o);
			break;
    case T_STRING:
			printf("\"%s\"", o->stringval_o);
			break;
    case T_REGPAIR:	printf("%s:%s", rnames[o->value_o],rnames[o->reg_o]);
			break;
    case T_FLOAT:	
    case T_DOUBLE:	printf("0d%g", o->dval_o ); break;
    default:
			printf("UNKNOWN(%d)", o->type_o);
			break;
    }
    if (o->flags_o&O_BFLD){
	/* bit field */
	/* first the offset part */
	if (o->flags_o&O_BFOREG)
	    printf("[%s:",rnames[o->bfoffset_o]);
	else
	    printf("[#%d:",o->bfoffset_o);
	/* then the width part */
	if (o->flags_o&O_BFWREG)
	    printf("%s]",rnames[o->bfwidth_o]);
	else
	    printf("#%d]",o->bfwidth_o);
    }
    putchar('\n');
}

int aregs ;		/* Contains bits for a7..a0. */
int dregs ;		/* Contains bits for d7..d0. */
int fregs ;		/* Contains bits for fp7..fp0. */
int cregs ;		/* Contains bits for fpc..fpi. */
int otheregs ;		/* Contains 1 for any other regs encountered. */

storeg( n )
int n ;

{
if ((A0REG <= n) && (n <= A7REG)) aregs |= 1 << (n-A0REG) ;
else
if ((D0REG <= n) && (n <= D7REG)) dregs |= 1 << (n-D0REG) ;
else
if ((FP0REG <= n) && (n <= FP7REG)) fregs |= 1 << (n-FP0REG) ;
else
if ((FPCREG <= n) && (n <= FPIREG)) cregs |= 1 << (FPIREG-n) ;
else
otheregs = 1 ;
}

storegs( m, n )
int m, n ;

{
int t ;

if (m > n)
	{
	t = m ;
	m = n ;
	n = t ;
	}
if ((A0REG <= m) && (n <= A7REG)) for (t=m ; t <= n ; ) aregs |= 1 << (t++-A0REG) ;
else
if ((D0REG <= m) && (n <= D7REG)) for (t=m ; t<=n ; ) dregs |= 1 << (t++-D0REG) ;
else
if ((FP0REG <= m) && (n <= FP7REG)) for (t=m ; t<=n ; ) fregs |= 1 << (t++-FP0REG) ;
else
if ((FPCREG <= m) && (n <= FPIREG)) for (t=m ; t<=n ; ) cregs |= 1 << (FPIREG-t++) ;
else
otheregs = 1 ;
}

char * reglist ( lptr, arg1 )

	char *lptr ;
	struct oper *arg1 ;

/*	
	reglist is called from exp when a register list operand is
	suspected.  If it fails, E_RELOCATE occurs.
*/

{
struct oper arg2 ;

aregs = dregs = fregs = cregs = otheregs = 0 ;
skipb(lptr) ;
lptr = term(lptr, arg1) ;
while (arg1->type_o == T_REG)
	{
	skipb(lptr) ;
	switch (cinfo[*lptr])
		{
		case EOL:
		case COM:	/* end of list */
			{
			storeg(arg1->value_o) ;
			if (otheregs) break ;
			if ((aregs|dregs) != 0)
				{ /* list of a's and d's */
				if ((fregs|cregs) != 0) break ;
				arg1->type_o = T_REGLIST ;
				arg1->value_o = dregs | (aregs << 8) ; 
				return(lptr) ;
				}
			else
			if (fregs != 0)
				{ /* list of fp's */
				if (cregs != 0) break ;
				arg1->type_o = T_FREGLIST ;
				arg1->value_o = fregs ;
				return(lptr) ;
				}
			else 
			if (cregs != 0)
				{ /* list of fc's */
				arg1->type_o = T_FCREGLIST ;
				arg1->value_o = cregs ;
				return(lptr) ;
				}
			break ;
			}
		case DIV:	/* rn/... */
			{
			storeg(arg1->value_o) ;
			lptr++ ;
			skipb(lptr) ;
			lptr = term(lptr, arg1) ;
			continue ;
			}
		case SUB:	/* rn-... */
			{
			lptr = term (++lptr, &arg2) ;
			if (arg2.type_o != T_REG) break ;
			storegs(arg1->value_o,arg2.value_o) ;
			continue ;
			}
		}
	break ;
	}
arg1->type_o = T_NULL ; /* Avoid problems later. */
PROG_ERROR(E_REGLIST) ;
return(lptr) ;
}

