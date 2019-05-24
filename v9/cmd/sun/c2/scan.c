#ifndef lint
static	char sccsid[] = "@(#)scan.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"

/* process lines from source file, returns when EOF detected */

#define LSIZE	4*BUFSIZ+2	/* max size of input line + fudge */

char *sassign(),*sdefer(),*exp(),*term();
struct ins_bkt * sopcode();
int slabel();

char iline[LSIZE];	/* current input line resides */
int  line_no;		/* current input line number */
char code[CODE_MAX];	/* where generated code is stored */
long dot;		/* offset in current csect */
int  bc;		/* size of code for current line */
int  opshow = 0;
long break_line = -1, break_pass = 1;

/* for local labels: ``[0-9]:'', and references ``[0-9][bh]'' */
extern char *ll_format;
extern int   ll_val[10];

scan()
{
	register int i;
	register char *p;	 /* pointer into input line */
	char *token;		 /* pointer to beginning of last token */
	struct ins_bkt * opindex;/* index of opcode on current line */
# ifdef C2
	char *docomment();
# endif

#ifdef AS
	/* setup stupid local-lable array */
	for( i = 0; i < 10; ll_val[i++] = -1 ) ;
#endif

	while (iline[LSIZE-2]='\0', fgets(iline,LSIZE,stdin) == iline) {
	      line_no++;
#ifdef AS
	      if (line_no == break_line && pass == break_pass) break_here();
#endif
#ifdef C2
	      if (line_no == break_line ) break_here();
#endif
	      /* a cheap way to find if the line was too long */
	      if (iline[LSIZE-2] != '\0'){ PROG_ERROR(E_LINELONG); continue; }
	      p = iline;

	      /* see what's the first thing on the line. if newline or comment
	       * char just ignore line all together.  if start of symbol see
	       * what follows.  otherwise error.
	       */
 restart:	skipb(p);
	      i = cinfo[*p];	/* see what we know about next char */
	      /* hack to make # in col 1 a comment -- for cpp leavings */
	      if (i == EOL || i == IMM) {
		    /* further hack to make ; in line act as seperator. */
		    if (*p == ';'){ p++; goto restart;}
# ifdef C2
		    p = docomment(p);
		    if (p==NULL) 
			return 1; /* special return */
# endif
		    continue;
	      }
	      if (!(i & (S|D))) { PROG_ERROR(E_BADCHAR); continue; }
	      bc = 0;
# ifdef AS
	      code_length = 0;
# endif

	      /* what follows is either label or opcode, gobble it up */
	      token = p;
	      skips(p);
	      skipb(p);
	      i = cinfo[*p];

	      /* if next char is ":", this is label definition */
	      if (i == COL) { p++; slabel(token); goto restart; }

	    /* if next char is "=", this is label assignment */
	    if (i == EQL) {
		p = sassign(++p,token);
		if (cinfo[*p] != EOL) PROG_ERROR(E_BADCHAR);
		/* hack to make ; in line act as seperator. */
		if (*p == ';'){ p++; goto restart;}
		continue;
	    }

	    /* otherwise this must be opcode, find its index */
	    if ((opindex = sopcode(token)) == 0) {
		PROG_ERROR(E_OPCODE);
		continue;
	    }

	    if (i == EOL) { numops = 0; goto doins; }

	    /* keep reading operands until we run out of room or hit EOL */
	    for (numops = 1; numops <= OPERANDS_MAX; numops++) {
		  p = soperand(p,&operands[numops-1]);
		  skipb(p);
		  i = cinfo[*p];
		  if (i == COM) { p++; skipb(p); continue; }
		  if (i == EOL) goto doins;
		  PROG_ERROR(E_OPERAND);
		  goto next;
	    }
	    PROG_ERROR(E_NUMOPS);
  next:	      continue;

  doins:      
	    instruction(opindex);
	    dot += bc;
	    dot_bkt->value_s = dot;
	  
	    /* hack to make ; in line act as seperator. */
	    if (*p == ';'){ p++; goto restart;}
	}
	return 0; /* normal exit */
} /* end Scan */

/* lookup token in opcode hash table, return 0 if not found */
struct ins_bkt * 
sopcode(token)
  register char *token;
{
  register struct ins_ptr *ipp;
  char *mnem = token;
  char savechar;

  /* make asciz version of mnemonic */
  while (cinfo[*token] & T) token++;
  savechar = *token;
  *token = '\0';

  /* look through appropriate hash bucket */
  ipp = ins_hash_tab[hash(mnem)];
  while (ipp) { 
	if (strcmp(ipp->name_p,mnem) == 0) break;
	  ipp = ipp->next_p;
	}
  *token = savechar;

  if( ipp == 0) return 0;
  return(ipp->this_p);
} /* end sopcode */


/* handle assignment to a label, return updated line pointer */
char *
sassign(lptr,token)
  register char *token;
  register char *lptr;
{
  register char *p;
  register struct sym_bkt *sbp;
  struct oper value;
  char nextc;
  static struct oper zero_oper  = { T_NULL, 0, 0, 0, 0, NULL};

  /* make asciz version of label */
  p = token;
  while (cinfo[*p] & T) p++;
  nextc = *p;
  *p = '\0';
  value = zero_oper;

  /* find/enter symbol in the symbol table, and get its new value */
  sbp = lookup(token);
  lptr = exp(lptr,&value);

  /* if assignment is to dot, we'll treat it specially */
  if (sbp == dot_bkt) {
#ifdef AS
	int deltadot;
#endif AS
	if (value.sym_o && value.sym_o->csect_s!=cur_csect_name)
	    PROG_ERROR(E_OPERAND);
#ifdef AS
	deltadot = value.value_o - dot;
	if (deltadot<0)
	    PROG_ERROR(E_OPERAND);
	operands[0].value_o = deltadot;
	skip_op( NULL );
#endif
  } else {
	if (value.sym_o != NULL) {
		sbp->value_s = value.sym_o->value_s;
	} else {
		sbp->value_s = value.value_o;
	}
	sbp->csect_s = (value.sym_o!=NULL) ? value.sym_o->csect_s : C_UNDEF;
	if (sbp->attr_s & S_LABEL) {
            PROG_ERROR(E_EQUALS);
	} else if (value.type_o == T_REG)
	    sbp->attr_s = (sbp->attr_s&S_EXT) | S_REG | S_DEC | S_DEF;
	else
	    sbp->attr_s = (sbp->attr_s&S_EXT)  |
		((value.sym_o!=NULL) ?
	         (value.sym_o->attr_s & ~(S_LABEL|S_PERM)): (S_DEC | S_DEF));
   }

   if (opshow) fprintf(stderr,"sassign(%s) = %ld, %ld\n",
	token, value.value_o, sbp->attr_s);
#if C2
    /* echo assignment immediately. RHS involving (.) will break. Fix later */
    printf("	%s	=	", sbp->name_s);
    printoperand( &value );
    putchar('\n');
#endif

    *p = nextc;

    /* return update line pointer */
    skipb(lptr);
    return(lptr);
} /* end sassign */

/* hashing routine for Symbol and Instruction hash tables */
hash(s)
	register char *s;
{
	register int i = 0;

	while (*s) i = i*10 + *s++ - ' ';
	i = i % HASH_MAX;
	return(i<0 ? i+HASH_MAX : i);
} /* end hash */


break_here(){ }
