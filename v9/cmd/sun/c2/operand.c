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
#if AS
char *ll_format = "*%c%06d";
#else C2
char *ll_format = "LX%c%06d";
#endif

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
#define SX_DISPL	8	/* long-size displacement specified */
#define SX_GOTINDEX	020	/* index register specified */
#define SX_GOTDISP	040	/* displacement specified */
#if C2
#define SX_CXDISP 	0100	/* displacement is complex */
#endif

static struct oper zero_oper  = { T_NULL, 0, 0, NULL, 0, 0, 0};

char *suffix(), *exp(), *term(), *scan_reg_or_immed();
char * scan_float();
double atof();

/* 
 * Fetches operand value and register subfields and loads them into
 * the operand structure. This routine will fetch only one set of value
 * and register subfields. It will move line pointer to first untouched char.
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
		if (opnd->type_o == T_FLOAT)
		    /* immediate value is floating-point */
		    opnd->flags_o |= O_FLOAT; 
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
#if AS
			if (!ext_instruction_set)
			    PROG_ERROR(E_OPERAND);
#endif
			opnd->type_o = T_REGPAIR;
			opnd->reg_o = spare_oper.value_o;
			break;
		case IND:
			if (!(areg(opnd->value_o)||pcreg(opnd->value_o)))
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
			    simplify2( opnd, &zero_suffix, &fix[1] );
			    break;
			}
#if AS
			if (opnd->type_o == T_INDEX && !ext_instruction_set)
			    if ( (opnd->flags_o&(O_BSUPRESS|O_INDIRECT|O_WDISP|O_LDISP)) ||
			    opnd->scale_o != 1 || !(opnd->flags_o&O_PREINDEX))
				PROG_ERROR( E_OPERAND );
#endif
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
	} else if ((spare_oper.sym_o && !(spare_oper.sym_o->attr_s&S_DEF)) || v < 0 || v > 31){
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
	} else if ((spare_oper.sym_o && !(spare_oper.sym_o->attr_s&S_DEF)) || v < 0 || v > 31){
	    PROG_ERROR(E_CONSTANT);
	}
	opnd->bfwidth_o = v;
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
opt_length( lptr, fp, wflag, lflag )
    register char * lptr;
    struct suffix * fp;
{
    char * save = lptr;
    if (cinfo[ *lptr ] == COL){
	lptr++;
	skipb( lptr );
	switch ( *lptr ){
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
     * specifier, then index register, optional length specifier, optional
     * scale specifier.
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
#if C2
	if (o.flags_o&O_COMPLEX)
	    fixp->flags_sx |= SX_CXDISP;
#endif
	lptr = opt_length( lptr, fixp, SX_DISPW, SX_DISPL );
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
    lptr = opt_length( lptr, fixp, SX_INDXW, SX_INDXL );
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
	    else if (fp->flags_sx & SX_DISPW)
		oper->flags_o|= O_WDISP;
	    /* else short form indexing */
	}
	break;
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
	break;
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
#if C2
    if (fp->flags_sx&SX_CXDISP)
	oper->flags_o |= O_COMPLEX;
#endif
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

#if C2
    if (fp->flags_sx&SX_CXDISP)
	oper->flags_o |= O_COMPLEX2;
#endif
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
#if AS
#   define MKCOMPLEX( op )
#endif
#if C2
#   define MKCOMPLEX( op ) if (arg1->sym_o&&arg2.sym_o || \
    arg1->flags_o&O_COMPLEX || arg2.flags_o&O_COMPLEX){ \
	ocomplex( op, arg1, &arg2 ); continue; \
    }
#endif

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
#if AS
			if (arg1->sym_o && arg2.sym_o) 
                           if (pass==2) break;
#endif
			MKCOMPLEX( ADD );
			if (arg2.sym_o) 
                           arg1->sym_o = arg2.sym_o;
			arg1->value_o += arg2.value_o;
			arg1->flags_o |= arg2.flags_o&O_COMPLEX;
			continue;

	    case MUL:	lptr = term(++lptr,&arg2);
			if (arg1->type_o==T_REG || arg2.type_o==T_REG) 
                           break;
#if AS
			if (arg1->sym_o || arg2.sym_o) 
                           if (pass==2) break;
#endif
			MKCOMPLEX( MUL );
			arg1->value_o *= arg2.value_o;
			arg1->flags_o |= arg2.flags_o&O_COMPLEX;
			continue;

	    case DIV:	lptr = term( ++lptr, &arg2 );
			if (arg1->type_o==T_REG || arg2.type_o==T_REG) 
                           break;
#if AS
			if (arg1->sym_o || arg2.sym_o) 
                           if (pass==2) break;
			   else if (arg2.value_o == 0) arg2.value_o = 1;
#endif
			MKCOMPLEX( DIV );
			arg1->value_o /= arg2.value_o;
			arg1->flags_o |= arg2.flags_o&O_COMPLEX;
			continue;

	    case SUB:	lptr = term( ++lptr, &arg2 );
			if (arg1->type_o==T_REG || arg2.type_o==T_REG) 
                           break;
			if (arg2.sym_o){	/* if B is relocatable, */
			   if (arg1->sym_o){  	/* and A is relocatable, */
#if AS
			       if (arg2.sym_o->csect_s != arg1->sym_o->csect_s
			       && pass == 2){
				   break;  /* break into error */
			       } else {   
				    /* result is absolute (no offset) */
				    arg1->sym_o = NULL;	
				    /* but not a simple address for sdi's */
				    arg1->flags_o |= O_COMPLEX;	
			       }
#endif
#if C2
			        /* operand cannot be understood at this time */
				ocomplex( SUB, arg1, &arg2 );
				continue;
#endif
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
#undef MKCOMPLEX
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

	skipb(lptr);
	i = cinfo[*lptr];

	/* here for number */
	if (i & D){
		p = lptr;
		val = 0;
		if (*lptr == '0'){ 
			lptr++;
			switch( *lptr){
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
			case 'f':
				/*
				 * be careful not to confuse the floating-point
				 * constant 0f0.0 with the local label reference                                 * 0f .
				 * for legitimate floating constants, the next
				 * character can be a digit, a decimal point,
				 * a negative sign, or one of the letters
				 * 'n', 'N', 'i', or 'I'.
				 */
				switch (*(lptr+1)){
				case '0':case '1':case '2':case '3':case '4':
				case '5': case '6':case '7':case '8':case '9':
				case '.': case '-': case '+':
				case 'n':case 'N': case 'i':case 'I':
				    break;
				default:
				    val = 0;
				    goto got_a_number;
				}
				/* FALL THROUGH */
			case 'F':
			case 'R':
			case 'r':
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
#if C2
                sbp->nuse_s++;
#endif
		if (sbp->attr_s & S_DEF
#if C2
		&& sbp->csect_s == C_UNDEF
#endif
		){
			/* if it's defined, use its value */
			vp->value_o = sbp->value_s;
	        } else 
			vp->value_o = 0;
		if (sbp->attr_s & S_REG){
			vp->type_o = T_REG;
			vp->sym_o  = NULL;
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
		if (vp->sym_o){
#if AS
		    PROG_ERROR(E_RELOCATE);
#endif
#if C2
		    if (vp->sym_o){
			ocomplex( SUB, vp, 0);
			return(lptr);
		    }
#endif
		}
		vp->value_o = -(vp->value_o);
		return(lptr);
	}

	/* check for complement */
	if (i == NOT) {
		lptr = term(++lptr,vp);
		if (vp->sym_o){
#if AS
		    PROG_ERROR(E_RELOCATE);
#endif
#if C2
		    if (vp->sym_o){
			ocomplex( NOT, vp, 0);
			return(lptr);
		    }
#endif
		}
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

char *
scan_float( lptr, vp )
    register char * lptr;
    struct oper *vp;
{
    /*
     * lptr address a string which is a 'd' or an 'f', followed
     * by a floating-point number. The latter looks like:
     *     [+-]nan( [0-9a-f]+ )
     * or  [+-]inf
     * or  [+-][0-9]+[.[0-9]+][e[+-][0-9]+]
     * in the latter case, we call atof to do the conversion work.
     * We do not take great care in syntax checking the number.
     */
    int s=0;
    union{ double d; float f; int x[2]; } fp;
    unsigned nanno, nanno2;
    register char *nbegin;
    char c;

    lptr++; /* skip the f */
    nbegin = lptr;
again:
    switch ( *lptr++){
    case '+':  if (s) goto bad_num; s = 1; goto again;
    case '-':  if (s) goto bad_num; s = -1; goto again;
    case 'N':
    case 'n': /* nan */
	if (*lptr++ != 'a') goto bad_num;
	if (*lptr++ != 'n') goto bad_num;
	if (*lptr++ != '(') goto bad_num;
	nanno = nanno2 = 0;
	while( isxdigit(c = *lptr++)){
	    if (isdigit(c)) c -= '0';
	    else if (isupper(c)) c -= 'a'-10;
	    else c -= 'A'-10;
	    nanno2 = (nanno2 << 8) | (nanno >> 24);
	    nanno = (nanno<<8) | c;
	}
	if (c != ')') goto bad_num;
	goto make_indef;
	    
    case 'I':
    case 'i':	/* inf */
	if (*lptr++ != 'n') goto bad_num;
	if (*lptr++ != 'f') goto bad_num;
	if (strncmp(lptr,"inity",5) == 0)
		lptr += 5;
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
	switch(c){
	case '+':
	case '-':
	    s = 1;
	    /* FALL THROUGH */
	case 'e':
	case 'E':
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
    vp->fval_o = fp.d;
    vp->type_o = T_FLOAT;
    return lptr;
}


static void
printoffset( value, sym, cmplx )
    int value;
    struct sym_bkt *sym;
    int cmplx;
{
    register struct optree *ot;
    register char c = '?';
    struct oper *op;
    if (cmplx){
        ot = (struct optree*)sym;
        if (ot->right_t == NULL){
            /* unary operation */
            switch(ot->op_t){
            case SUB: c = '-'; break;
            case NOT: c = '~'; break;
            }
            printf("%c(",c);
            op=ot->left_t;
            printoffset(op->value_o, op->sym_o, op->flags_o&O_COMPLEX);
            putchar(')');
        } else {
            /* binary */
            putchar('(');
            op=ot->left_t;
            printoffset(op->value_o, op->sym_o, op->flags_o&O_COMPLEX);
            switch(ot->op_t){
            case ADD: c = '+'; break;
            case SUB: c = '-'; break;
            case MUL: c = '*'; break;
            case DIV: c = '/'; break;
            }
            printf(")%c(", c);
            op=ot->right_t;;
            printoffset(op->value_o, op->sym_o, op->flags_o&O_COMPLEX);
            putchar(')');
        }
        return;
    }
    if (sym != NULL){
        fputs(sym->name_s, stdout);
        if (value)
            printf("+%d", value);
    } else {
        printf("%d", value);
    }
}


char *rnames[] = {
    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "sp", 
    "fp0","fp1","fp2","fp3","fp4","fp5","fp6","fp7",
    "cc", "fcc", "pc", "sr", "usp", "sfc", "dfc", "vbr", 
    "cacr", "caar", "msp", "isp", "fpc", "fps", "fpi"
};

static void
printindex( o )
    register struct oper *o;
{
    printf("%s:%c", rnames[o->value_o], o->flags_o&O_WINDEX ? 'w' : 'l');
    if (o->scale_o != 1) printf(":%d", o->scale_o);
}

printoperand( o )
    register struct oper *o;
{
    switch (o->type_o){
    case T_REG:		printf("%s",   rnames[o->value_o]); break;
    case T_DEFER:	printf("%s@",  rnames[o->value_o]); break;
    case T_POSTINC:	printf("%s@+", rnames[o->value_o]); break;
    case T_PREDEC:	printf("%s@-", rnames[o->value_o]); break;
    case T_DISPL:	printf("%s@(", rnames[o->reg_o]);
			printoffset(o->value_o, o->sym_o, o->flags_o&O_COMPLEX);
			putchar(')');
			break;
    case T_INDEX:	
			if (!(o->flags_o&O_BSUPRESS)){
			    printf("%s@", rnames[o->reg_o]);
			}
			putchar('(');
			printoffset(o->disp_o, o->sym_o, o->flags_o&O_COMPLEX);
			if (o->flags_o & O_WDISP) fputs(":w", stdout);
			else if (o->flags_o & O_LDISP) fputs(":l", stdout);
			if (o->flags_o&O_PREINDEX){
			    putchar(',');
			    printindex( o );
			}
			putchar(')');
			if (o->flags_o&O_INDIRECT){
			    putchar('@');
			    if (o->disp2_o || o->sym2_o || o->flags_o&O_POSTINDEX){
				putchar('(');
				if (o->disp2_o || o->sym2_o ){
				    printoffset(o->disp2_o, o->sym2_o, o->flags_o&O_COMPLEX2);
				    if (o->flags_o&O_POSTINDEX)
					putchar(',');
				}
				if (o->flags_o&O_POSTINDEX){
				    printindex( o );
				}
				putchar(')');
			    }
			}
			break;
    case T_ABSS:
    case T_ABSL:
			printoffset(o->value_o, o->sym_o, o->flags_o&O_COMPLEX);
			fputs( (o->type_o == T_ABSS) ? ":w" : ":l", stdout);
			break;
    case T_IMMED:
			putchar('#');
			if (o->flags_o & O_FLOAT) goto float_val;
			/* FALL THROUGH */
    case T_NORMAL:
			printoffset(o->value_o, o->sym_o, o->flags_o&O_COMPLEX);
			break;
    case T_STRING:
			printf("\"%s\"", o->stringval_o);
			break;
    case T_REGPAIR:	printf("%s:%s", rnames[o->value_o],rnames[o->reg_o]);
			break;
float_val:
    case T_FLOAT:	printf("0r%.17e", o->fval_o); 
			break;
    default:
			printf("UNKNOWN(%d)", o->type_o);
			break;
    }
    if (o->flags_o&O_BFLD){
	/* bit field */
	/* first the offset part */
	if (o->flags_o&O_BFOREG)
	    printf("{%s:",rnames[o->bfoffset_o]);
	else
	    printf("{#%d:",o->bfoffset_o);
	/* then the width part */
	if (o->flags_o&O_BFWREG)
	    printf("%s}",rnames[o->bfwidth_o]);
	else
	    printf("#%d}",o->bfwidth_o);
    }
}
