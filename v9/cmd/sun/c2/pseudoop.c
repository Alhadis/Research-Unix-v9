#ifndef lint
static	char sccsid[] = "@(#)pseudoop.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

/* Csect descriptor */
struct csect {
       char *name_cs;    /* print name 					 */
       long len_cs;	 /* Length in machine addresses,		 */ 
                         /* i.e., highest address referenced		 */
       long dot_cs;	 /* current dot in this cs, in machine addresses */
       short id_cs;	 /* UNIX a.out name for this csect		 */
       short which_cs;	 /* our name for this csect (see below)		 */
};

/* table used to determine node type. */
/* 
 * we would really like this table to be subscripted by the enumerated type
 * opcode_t, but C cannot hack that. Order here is VERY IMPORTANT,
 * and this table must change if the opcode order or number changes!!
 */
char opcodetypes[] = {
	0, 			/* FIRST	*/
	PSEUDONOCODE, 		/* COMMENT	*/
	0,			/* LABEL	*/
	PSEUDOCODE, 		/* LONG		*/
	PSEUDOCODE, 		/* WORD		*/
	PSEUDOCODE, 		/* BYTE		*/
	PSEUDONOCODE,		/* TEXT		*/
	PSEUDONOCODE, 		/* DATA		*/
	PSEUDONOCODE,		/* DATA1	*/
	PSEUDONOCODE, 		/* DATA2	*/
	PSEUDONOCODE,		/* BSS		*/
	PSEUDONOCODE, 		/* GLOBL	*/
	PSEUDONOCODE,		/* COMM		*/
	PSEUDOCODE, 		/* EVEN		*/
	PSEUDOCODE, 		/* ALIGN 	*/
	PSEUDOCODE, 		/* ASCIZ	*/
	PSEUDOCODE,		/* ASCII	*/
	PSEUDOCODE, 		/* SINGLE	*/
	PSEUDOCODE,		/* DOUBLE	*/
	PSEUDONOCODE,		/* STABS	*/
	PSEUDONOCODE, 		/* STABD	*/
	PSEUDONOCODE,		/* STABN	*/
	PSEUDOCODE, 		/* SKIP		*/
	PSEUDONOCODE,		/* LCOMM	*/
	PSEUDONOCODE,		/* CPID		*/
	INSTRTYPE|BRANCHTYPE, 	/* CSWITCH	*/
	INSTRTYPE|BRANCHTYPE,	/* FSWITCH	*/
	INSTRTYPE|BRANCHTYPE, 	/* BRANCH	*/
	INSTRTYPE,		/* MOVE		*/
	INSTRTYPE, 		/* MOVEM	*/
	INSTRTYPE|BRANCHTYPE,	/* EXIT		*/
	INSTRTYPE|BRANCHTYPE, 	/* DBRA		*/
	INSTRTYPE,		/* CALL		*/
	INSTRTYPE|BRANCHTYPE|JUMPTYPE, 	/* JUMP		*/
	INSTRTYPE|BRANCHTYPE|JUMPTYPE,	/* DJMP		*/
	INSTRTYPE, 		/* LINK		*/
	INSTRTYPE,		/* CMP		*/
	INSTRTYPE, 		/* PEA		*/
	INSTRTYPE,		/* ADD		*/
	INSTRTYPE, 		/* AND		*/
	INSTRTYPE,		/* EXT		*/
	INSTRTYPE, 		/* OR		*/
	INSTRTYPE,		/* TST		*/
	INSTRTYPE, 		/* ASL		*/
	INSTRTYPE,		/* ASR		*/
	INSTRTYPE, 		/* SUB		*/
	INSTRTYPE,		/* UNLK		*/
	INSTRTYPE, 		/* LEA		*/
	INSTRTYPE,		/* CLR		*/
	INSTRTYPE, 		/* BOP		*/
	INSTRTYPE,		/* EOR		*/
	INSTRTYPE, 		/* FTST		*/
	INSTRTYPE,           	/* OTHER	*/
};
extern struct sym_bkt *dot_bkt;

struct csect csects[] = {
  ".text", 0,0,0,C_TEXT,	  	/* text csect */
  ".data", 0,0,0,C_DATA, 		/* data csect */
  ".data1",0,0,0,C_DATA1,		/* data csect */
  ".data2",0,0,0,C_DATA2,		/* data csect */
  ".bss",  0,0,0, C_BSS,  		/* uninitialized csect */
  0
} ;

struct csect *cur_csect   = &(csects[0]);/* ptr to current csect */
struct csect *all_csects[] = { 0 ,
	&(csects[0]), &(csects[1]), &(csects[2]), &(csects[3]), &(csects[4])
};

short cur_csect_name      = C_TEXT;

void
init_csects(){
    register struct sym_bkt *sbp;
    struct csect *csp;
    for (csp = csects; csp->name_cs != NULL; csp++){
	sbp = lookup(csp->name_cs);
	sbp->attr_s |= S_DEC | S_DEF | S_LOCAL;
	sbp->csect_s = csp->which_cs;
	sbp->value_s = 0;
    }
}

void
new_csect()
{ 
    extern struct csect    *cur_csect;	/* ptr to current csect */
    extern struct sym_bkt  *dot_bkt;	/* sym_bkt for location counter */
	
    cur_csect = all_csects[cur_csect_name];
    dot = cur_csect->dot_cs;
    dot_bkt->csect_s = cur_csect_name;/* update dot's csect. dot_bkt->value_s */
                                      /* will be updated in the main loop     */
}

int
ascii(csectname)
{	register char *p;
	int count;
	extern char *malloc();

	if (numops!=1 || operands[0].type_o!=T_STRING)
	  { PROG_ERROR(E_OPERAND); return 0; }
	if (csectname == C_TEXT){
	    count = strlen(operands[0].stringval_o);
	    p = malloc( count+1);
	    strcpy( p, operands[0].stringval_o);
	    operands[0].stringval_o = p;
	} else
	    p = operands[0].stringval_o;
	/* scan out the string, so we can figure how much space it will take */
	count = 0;
	while (*p) {
		count++;
		if ( *p++ == '\\' )
		    switch ( *p ){
		    case '\\':
		    case '"':
			    p++;
			    break;
		    default:
		    {
			register i;
			/* \octal number.  Max of 3 octal digits */

			for(i=0;i<3;i++,p++) {
				if (!((*p >= '0') && (*p <= '7'))) 
				    break;
			}
		    }
		}
	}
	return count;
}

int
pseudo_size( ip )
    struct ins_bkt *ip;
{
    int val, delta;
    switch( ip->op_i ){
    case OP_LONG:
	return numops*sizeof(long);
    case OP_WORD:
	return numops*sizeof(short);
    case OP_BYTE:
	return numops*sizeof(char);
    case OP_SKIP:
	return operands[0].value_o;
    case OP_EVEN:
	return dot&1;
    case OP_ALIGN:
	val = operands[0].value_o;
	delta = val - (dot%val);
	if (delta == val) delta = 0;
	return delta;
    }
    return 0;
}

int
pseudo_code( op ) opcode_t op; 
{
    /*
     *  1 => can generate code
     *  0 => cannot generate code
     * -1 => not a pseudo-op
     */
    if (opcodetypes[(int)op]&PSEUDONOCODE) return 0;
    if (opcodetypes[(int)op]&PSEUDOCODE  ) return 1;
    return -1;
}

void
save_stabs(csectname)
{
    char *p;
    /* IF IN TEXT, save stabs string */
    if (csectname != C_TEXT) return;
    /* like ascii above */
    p = malloc( 1 + strlen(operands[0].stringval_o));
    strcpy( p, operands[0].stringval_o);
    operands[0].stringval_o = p;
}
