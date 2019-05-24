#ifndef lint
static	char sccsid[] = "@(#)stab.c 1.1 86/02/03 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "scan.h" 
#include "as.h"

extern struct sym_bkt *sym_hash_tab[];
extern long dot;                       /* current offset in csect (i.e. addr)*/


struct stab_sym_bkt *stab_free = NULL;
#define STAB_INCR 50

#ifdef EBUG
#	define DEBUG( X ) printf(X)
#else
#	define DEBUG( X )
#endif

struct stab_sym_bkt *
gstab()
{
    /* get a stab symbol bucket -- analogous with gsbkt()/sym.c */
    register struct stab_sym_bkt *sbp;
    register int i;

    if ((sbp = stab_free) != NULL) {
	stab_free = sbp->next_stab;
    } else {
	sbp = (struct stab_sym_bkt *) calloc( STAB_INCR, sizeof(struct stab_sym_bkt));
	if (sbp == NULL) sys_error("Stab storage exceeded\n", 0);
	for (i=STAB_INCR-1;i--;) {
	    sbp->next_stab = stab_free;
	    stab_free = sbp++;
	}
    }
    return sbp;
}

stab_op( ip )
    struct ins_bkt *ip;
{
    /* 
      allocate a new stab bucket, and process the stabs/d/n
      arguments which have already been parsed for us.
    */
    struct stab_sym_bkt *s;
    register int opno;
    opcode_t opcode = ip->op_i;

    if (pass != 2) return;
    s = gstab();
    if (opcode==OP_STABS){
	/* have a string operand -- parse it a la Ascii(), and save it off */
	register char *cp;
	register nchars;
	char *sp;
	if (numops != 5 ){
	    PROG_ERROR( E_NUMOPS); return;
	}
	if (operands[0].type_o != T_STRING ) {
	    DEBUG("first stabs operand not a string\n");
	    PROG_ERROR( E_OPERAND); return;
	}
	nchars = 0;
	cp = operands[0].stringval_o;
	while (*cp++)++nchars;
	cp = operands[0].stringval_o;
	s->ch = sp = (char *) calloc( nchars +1, sizeof(char));
	if (sp==NULL) sys_error( "Out of string space\n", 0);
	/* nchars is only an approximate character count -- recount for real */
	nchars = 0;
	while (*cp){
	    nchars++;
	    if ((*sp = *cp++)!= '\\'){
		sp++;
	    } 
	    else switch (*cp) {
		int i;
		int c;
		/* bet none of this is ever executed */
		case '\\':
		case '"':
		    *sp++ = *cp++;
		    break;
		default:
		    /* \ octal */
		    for(i=0,c=0; i<3; i++) {
			if((*cp >= '0') && (*cp <= '7'))
			    c = (c<<3) + *cp-- - '0';
			else 
			    break;
		    }
		    *sp++ = c;
	    }
	}
	s->id = nchars;
	opno = 1;
    } else if ( opcode == OP_STABD ) {
	if (numops != 3 ) { 
	    DEBUG("stabd has the wrong number of operands\n");
	    PROG_ERROR(E_NUMOPS);
	    return;
	}
	opno = 0;
	s->id = 0;
    } else { /* opcode == OP_STABN */
	if (numops != 4 ) {
	    DEBUG("stabn has the wrong number of operands\n");
	    PROG_ERROR(E_NUMOPS);
	    return;
	}
	opno = 0;
	s->id = 0;
    }
    /* stuff the next three numbers without too much thought */
    if (operands[opno].sym_o != NULL || operands[opno].type_o != T_NORMAL){
	DEBUG("stab first numeric operand bad\n");
	PROG_ERROR( E_OPERAND); return;
    }
    s->type = operands[opno++].value_o;
    if (operands[opno].sym_o != NULL || operands[opno].type_o != T_NORMAL){
	DEBUG("stab second numeric operand bad\n");
	PROG_ERROR( E_OPERAND); return;
    }
    s->other = operands[opno++].value_o;
    if (operands[opno].sym_o != NULL || operands[opno].type_o != T_NORMAL){
	DEBUG("stab third numeric operand bad\n");
	PROG_ERROR( E_OPERAND); return;
    }
    s->desc = operands[opno++].value_o;

    if (opcode == OP_STABD) {
	/* value is dot */
	s->value = dot;
    } else {
	/* value is either a label or its a number */
	if (operands[opno].type_o != T_NORMAL){
	    DEBUG("stab last operand isn't `normal'\n");
	    PROG_ERROR(E_OPERAND); return;
	}
	switch (s->type) {
	case N_FUN:
	case N_EFUN:
	case N_STSYM:
	case N_LCSYM:
	case N_SLINE:
	case N_SO:
	case N_ESO:
	case N_SOL:
	case N_ENTRY:
	case N_LBRAC:
	case N_RBRAC:
	case N_ECOML:
		/* it a label */
		if (!operands[opno].sym_o) {
		    DEBUG("stab last operand isn't a label\n");
		    PROG_ERROR(E_OPERAND); return;
		}
		break;
	default:
		/* its a number */
		if ( operands[opno].sym_o) {
		    DEBUG("stab last operand isn't just a number\n");
		    PROG_ERROR(E_OPERAND); return;
		}
		break;
	}
	s->value = operands[opno].value_o;
    }
    link_stab_tab( s );
}
/* ************************************************************************* */

  /* Link the incoming stab bucket to the stab symbol table.       */
  /* The addition of the incoming stab is linked to the end of the */
  /* current linked list.                                          */


link_stab_tab(s)
struct stab_sym_bkt *s;


{ if (stabkt_head == NULL)                 /* if no entry on linked list */
     { stabkt_head = s;                         /* let both head and tail     */
       stabkt_tail = s;                         /* pointing to this entry s   */
       stabkt_head->next_stab = NULL;      /* and make the next one null */
     }
     else { stabkt_tail->next_stab = s;          /* otherwise make this entry */
            stabkt_tail = stabkt_tail->next_stab; /* the next entry and move  */
            stabkt_tail->next_stab = NULL;  /* tail pointing to this one */
          }
} /* end Link_Stab_Tab */
