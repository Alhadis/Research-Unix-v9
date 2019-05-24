#ifndef lint
static	char sccsid[] = "@(#)ps.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include <a.out.h>

/* a sop here to the cross-assemblers */
#define put_words( x, y ) put_text( x, y )
#define CSECT_MAX       5       /* number of control sections. */
/* Csect descriptor */
struct csect {
    char *name_cs;	/* print name					*/
    long  len_cs;	/* Length in machine addresses,			*/
			/* i.e., highest address referenced		*/
    long  dot_cs;	/* current dot in this cs, in machine addresses */
    short id_cs;	/* UNIX a.out name for this csect		*/
    short which_cs;	/* C_name for csect				*/
};

struct csect csect[CSECT_MAX] = {
  ".text", 0,0,N_TEXT,C_TEXT,	  	/* text csect */
  ".data", 0,0,N_DATA,C_DATA, 		/* data csect */
  ".data1",0,0,N_DATA,C_DATA1,		/* data csect */
  ".data2",0,0,N_DATA,C_DATA2,		/* data csect */
  ".bss",  0,0,N_BSS, C_BSS   		/* uninitialized csect */
} ;

short cur_csect_name      = C_TEXT;

int cansdi = (C_TEXT == C_TEXT) ; /* i.e. yes */


prog_end()
{ 
	register int i;
	register struct csect *p;

	/* pass 2 */
	if (pass > 1) 	       /* On the second pass, */
	   { fix_rel();	       /* patch up object file */
	     return; 
	   }

	/* pass 1 */
	current_file= 0;
	new_csect( C_TEXT );	/* end in text segment */
	/* export these values to sdi_resolve */
	if (even_align_flag == 0)
		{
	dsize  = (csect[C_DATA-1].len_cs  + 3) & ~3;
	d1size = (csect[C_DATA1-1].len_cs + 3) & ~3;
	d2size = (csect[C_DATA2-1].len_cs + 3) & ~3;
	bsize  = (csect[C_BSS-1].len_cs   + 3) & ~3;
	tsize  = (csect[C_TEXT-1].len_cs  + 3) & ~3;	/* make long aligned */
		}
	else
		{
	dsize  = (csect[C_DATA-1].len_cs  + 1) & ~1;
	d1size = (csect[C_DATA1-1].len_cs + 1) & ~1;
	d2size = (csect[C_DATA2-1].len_cs + 1) & ~1;
	bsize  = (csect[C_BSS-1].len_cs   + 1) & ~1;
	tsize  = (csect[C_TEXT-1].len_cs  + 1) & ~1;	/* make word aligned */
		}
	
	sdi_resolve();	       /* resolve span dependent instructions */

	csect[C_TEXT-1].len_cs += sdi_inc( csect[C_TEXT-1].len_cs, NULL);
	/* re-compute values for sym_fix and rel_header */
	if (even_align_flag == 0)
		{
	dsize  = (csect[C_DATA-1].len_cs  + 3) & ~3;
	d1size = (csect[C_DATA1-1].len_cs + 3) & ~3;
	d2size = (csect[C_DATA2-1].len_cs + 3) & ~3;
	bsize  = (csect[C_BSS-1].len_cs   + 3) & ~3;
	tsize  = (csect[C_TEXT-1].len_cs  + 3) & ~3;	/* make long aligned */
		}
	else
		{
	dsize  = (csect[C_DATA-1].len_cs  + 1) & ~1;
	d1size = (csect[C_DATA1-1].len_cs + 1) & ~1;
	d2size = (csect[C_DATA2-1].len_cs + 1) & ~1;
	bsize  = (csect[C_BSS-1].len_cs   + 1) & ~1;
	tsize  = (csect[C_TEXT-1].len_cs  + 1) & ~1;	/* make word aligned */
		}
	sym_fix();	               /* relocate and globalize */
	rel_header();	               /* Initialize output stuff */
	start_pass();	               /* Init per-pass variables */
	return;
} /* end prog_end */

/* Initialize per-pass variables */
start_pass()
{ 
	register int i;

	last_symbol = dot_bkt;	/* last defined symbol at start of pass */
	line_no = 0;
	errors = 0;
	implicit_cpid = INITIAL_CPID ;
        pass++;
	if (pass == 1) {
		for (i=0; i<CSECT_MAX; i++) 
	        csect[i].dot_cs = 0;
	} else { 
	    csect[C_TEXT-1].dot_cs = 0;
	    csect[C_DATA-1].dot_cs = tsize;
	    csect[C_DATA1-1].dot_cs = tsize + dsize;
	    csect[C_DATA2-1].dot_cs = tsize + dsize + d1size;
	    csect[C_BSS-1].dot_cs = tsize + dsize + d1size + d2size;
	}
	dot = 0;
	new_csect( C_TEXT ); /* start in text segment */
} /* end Start_pass */


/* .even handler */
even_op( ip, nops )
    struct ins_bkt *ip;
{ 
    int align_val, dot_delta;
    if (nops == 0 ){
	/* just plain .even */
	align_val = 2;
    } else {
	/* .align something */
	if (operands[0].sym_o)
	    PROG_ERROR(E_OPERAND);
	align_val = operands[0].value_o;
    }
    switch( align_val ){
    case 1: /* no-op */
	break;
    case 4:
	if (cansdi){
	    PROG_ERROR(E_ALIGN);
	    break;
	}
	/* FALL THROUGH */
    case 2:
	dot_delta = align_val - (dot%align_val);
	if (dot_delta != 0 && dot_delta != align_val){
	    dot += dot_delta;
	    wcode[0] = 0;
	    wcode[1] = 0;
	    put_text(wcode, dot_delta);
	}
	break;
    default:
	PROG_ERROR(E_OPERAND);
    }
} /* end Even */

byteword( ip )
    struct ins_bkt *ip;
{ 
    register int i;
    register struct oper *p;
  
    for (i=0, p=operands; i < numops; i++, p++) { 
	if (p->type_o != T_NORMAL) { 
	     p->sym_o = 0;
  	     p->type_o = T_NORMAL;
  	     p->value_o = 0;
  	     PROG_ERROR(E_OPERAND);
	 } else if (p->sym_o) 
	      put_rel(p, ip->subop_i, dot+bc,(p->flags_o&O_COMPLEX)!=0);
  
  	switch (ip->subop_i){
	case SUBOP_L:	*(unsigned *)code = p->value_o;
			/* if cross-assembling, we die here */
  			put_words(wcode,4);
  			bc += 4;
  			break;
  
	case SUBOP_W:	wcode[0] = p->value_o;
  	  		put_words(wcode,2);
  			bc += 2;
  			break;
  
	case SUBOP_B:	code[0] = p->value_o;
  			put_text(code,1);
  			bc++;
  			break;
  	    } /* end switch */
       } /* end for */
} /* end ByteWord */

/* handle .ascii and .asciz pseudo ops -- zero<>0 indicates that
 * user wants zero byte at end of string.
 */
ascii_op( ip )
    struct ins_bkt *ip;
{
    register char *p;
    char c;
    /*
     * opval_i[0] == 1 => ASCIZ
     *               0 => ASCII
     */

    if (numops!=1 || operands[0].type_o!=T_STRING){
	PROG_ERROR(E_OPERAND);
	return;
    }
    p = operands[0].stringval_o;
    while (*p) {
	    bc++; 
	    if ((c = *p++) != '\\') {
		    put_text(&c,1);
	    } else switch ( c = *p){
		case '\\':
		case '"':
			put_text(&c, 1);
			p++;
			break;
		default:
		{
		    register i;
		    /* \octal number.  Max of 3 octal digits */

		    for(i=0,c=0;i<3;i++) {
			    if ((*p >= '0') && (*p <= '7')) {
				    c = c * 8 + (*p++) - '0';
			    } else
				    break;
		    }
		    put_text(&c,1);
		}
	    }
    }
    if (ip->opval_i[0]) { put_text(p,1); bc++; }
} /* end Ascii */	

/*
 * handle .float and .double pseudo-ops. We distinguish by the opcode in
 * the instruction bucket handed us. We expect >=0 operands of type
 * T_FLOAT or T_DOUBLE.
 */
float_op( ip )
    struct ins_bkt *ip;
{ 
    register int i;
    register struct oper *p;
    double d;
    float f;
  
    switch (ip->op_i){
    case OP_FLOAT:
	for (i=0, p=operands; i < numops; i++, p++) {
	    switch (p->type_o){
	    default:
		PROG_ERROR( E_OPERAND );
		f = 0.0;
		break;
	    case T_NORMAL:
		if (p->sym_o )
		    PROG_ERROR( E_CONSTANT );
		f = (double) p->value_o;
		break;
	    case T_FLOAT:
		f =  p->fval_o;        break;
	    case T_DOUBLE:
		f = (float) p->dval_o; break;
	    }
	    put_text((char *) &f, sizeof f );
	    bc += sizeof f;
       }
	break;
    case OP_DOUBLE:
	for (i=0, p=operands; i < numops; i++, p++) {
	    switch (p->type_o){
	    default:
		PROG_ERROR( E_OPERAND );
		d = 0.0;
		break;
	    case T_NORMAL:
		if (p->sym_o )
		    PROG_ERROR( E_CONSTANT );
		d = (double) p->value_o;
		break;
	    case T_FLOAT:
		d = (double) p->fval_o; break;
	    case T_DOUBLE:
		d = p->dval_o;          break;
	    }
	    put_text((char *) &d, sizeof d );
	    bc += sizeof d;
	}
	break;
    default:
	sys_error("broken .float\n");
    }
}

csect_op( ip )
    struct ins_bkt *ip;
{ 
    new_csect( (int)(ip->op_i) - (int)OP_TEXT + C_TEXT );
}

new_csect( cname )
    int cname;
{
    register struct sym_bkt *sbp;        /* for defining new symbol */
    register struct csect   *csp;	/* ptr to current csect */
    extern struct sym_bkt  *last_symbol;	/* used for local symbols */
    extern struct sym_bkt  *dot_bkt;	/* sym_bkt for location counter */
	
    /* update current csect */
    csp = &csect[ cur_csect_name -1 ];
    csp->dot_cs = dot;
    if (dot > csp->len_cs) csp->len_cs = dot;
    /* now install new one */
    csp = &csect[cname-1];
    cur_csect_name = csp->which_cs;
    dot = csp->dot_cs;
    dot_bkt->csect_s = cur_csect_name;/* update dot's csect. dot_bkt->value_s */
    dot_bkt->value_s = dot;
                                      /* will be updated in the main loop     */
    sbp = lookup(csp->name_cs);
    sbp->attr_s |= S_DEC | S_DEF | S_LOCAL | S_PERM;
    sbp->csect_s = cur_csect_name;
    cansdi = !Jflag && (cur_csect_name==C_TEXT);
    sbp->value_s = 0;
} /* end New_csect */


proc_op( ip )
    struct ins_bkt *ip;
{
    if (pass == 1) makesdi( NULL, 0, 0 ); 
}

globl_op( ip )
    struct ins_bkt *ip;
{ 
    register int i;
    register struct sym_bkt *sbp;
  
    if (pass == 1) 
	for (i=0; i<numops; i++) {
	     sbp = operands[i].sym_o;
	     if (sbp == NULL) {
		PROG_ERROR(E_SYMBOL);
	     } else { 
		  sbp->attr_s |= S_DEC | S_EXT;	/* declared and external */
		  if (!(sbp->attr_s & S_DEF)){
		      sbp->csect_s = C_UNDEF;   /* don't know which */
		  }
	     }
	}
    return;
} /* end Globl */


comm_op( ip )
    struct ins_bkt *ip;
{ 
	register struct sym_bkt *sbp;

	sbp = operands[0].sym_o;
	if (sbp == NULL) {
	    PROG_ERROR(E_OPERAND);
	    return;
	}
	if (ip->op_i == OP_COMM ){
	    /* .comm */
	    if (pass == 1){
		sbp->csect_s = C_UNDEF;	/* make it undefined */
		sbp->attr_s |= S_DEC | S_EXT | S_COMM;
		sbp->value_s = operands[1].value_o;
	    }
	} else {
	    /* .lcomm */
	    /*
	     * switch to bss segment;
	     * bump dot and plant label;
	     * switch back
	     */
	    auto save_csect = cur_csect_name;
	    new_csect( C_BSS );
	    if (pass == 1) {
		    sbp->attr_s |=S_LABEL|S_DEC|S_DEF;
		    sbp->csect_s = C_BSS;
		    sbp->value_s = dot;
	    } else {
		if (sbp->csect_s != C_BSS || sbp->value_s != dot)
		    prog_error(E_MULTSYM);
	    }
	    dot += operands[1].value_o;
	    new_csect( save_csect );
    }
}

skip_op( ip )
    struct ins_bkt *ip;
{
	register i;
	static int zed[1024] = {0};
	bc += i = operands[0].value_o;
	if (cur_csect_name != C_BSS){
		while (i>sizeof zed){
			put_text(zed, sizeof zed);
			i -= sizeof zed;
		}
		if (i) {
			put_text(zed,i);
		}
	}
}
 
cpid_op( ip )
     struct ins_bkt *ip ;
{
implicit_cpid = operands[0].value_o ;
}
