#ifndef lint
static	char sccsid[] = "@(#)scan.c 1.1 86/02/03 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "scan.h" 
#include <sys/signal.h>

/* process lines from source file, returns when EOF detected */

#define LSIZE	4*BUFSIZ+2	/* max size of input line + fudge */


char *sassign(),*exp(),*term();
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
extern int   ll_val[10];
extern char * ll_format;

sexit(){
	exit(3);
}

main(argc,argv)
	char *argv[];
{
	stabkt_head = stabkt_tail = NULL;      /* initialize stabs/d  symbol table */
	signal( SIGINT, sexit );
	init(argc,argv);                       /* Initialization */
	scan();                                /* 1st pass through .s file */
	prog_end();                            /* end of 1st pass          */

	/*
	* Resolve the unresolved addresses in .stabs and .stabd directives.
	* .stabs type 0x24 (N_Fun for procedure name), 0x26 (N_STSYM for static
	* symbol), and 0x64 (N_SO for source file name) have a labeled-address
	* tagged in the directive in the last field "value".  These addresses
	* have to be resolved during or before the 2nd Pass (in this version
	* resolved before the second pass).
	*/

	scan();                        /* 2nd pass through .s file */
	prog_end();                    /* end of 2nd pass          */
	exit(errors? -1: 0);
} /* end main */

scan()
{
	register int i;
	register char *p;	 /* pointer into input line */
	char *token;		 /* pointer to beginning of last token */
	struct ins_bkt * opindex;/* index of opcode on current line */
	struct oper exp_arg ; 	 /* argument for exp */

	/* setup stupid local-lable array */
	for( i = 0; i < 10; ll_val[i++] = -1 ) ;

one_more_file:
	while (iline[LSIZE-2]='\0', fgets(iline,LSIZE,source_file[current_file]) == iline) {
	      line_no++;
	      if (line_no == break_line && pass == break_pass) break_here();
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
		  continue;
	      }
	      if (!(i & (S|D))) { PROG_ERROR(E_BADCHAR); continue; }
	      bc = 0;
	      code_length = 0;

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
		    goto update;
	      }
 
              /* check for cpid following op code */
 
                if (i == IND)
                        {    
                        *p = ' ' ;      /* Blank out the @ sign. */
                        p = p + 1 ;     /* Increment pointer past @. */
                        p = exp(p, &exp_arg) ; /* Evaluate current cpid.*/
                        current_cpid = exp_arg.value_o ;
                        skipb(p) ;      /* Skip blanks to next field. */
                        }
                else
                        current_cpid = implicit_cpid ;


	      /* otherwise this must be opcode, find its index */
	      if ((opindex = sopcode(token)) == 0) {
		PROG_ERROR(E_OPCODE);
		continue;
	      }

	      if (i == EOL) { numops = 0; goto doins; }

	      /* keep reading operands until we run out of room or hit EOL */
	      for (numops = 1; numops <= OPERANDS_MAX; numops++) {
		    p = soperand(p,&operands[numops-1]);
		    if (opshow) printop(&operands[numops-1]);
		    skipb(p);
		    i = cinfo[*p];
		    if (i == COM) { p++; skipb(p); continue; }
		    if (i == EOL) goto doins;
		    PROG_ERROR(E_OPERAND);
		    goto next;
	      }
	      PROG_ERROR(E_NUMOPS);
  next:	      continue;

  doins:      instruction(opindex);
  update:     dot += bc;
	      dot_bkt->value_s = dot;
	  
	      /* hack to make ; in line act as seperator. */
	      if (*p == ';'){ p++; goto restart;}
	}
	if (pass == 1)
		rewind(source_file[current_file]);
	if (current_file < file_count-1) {
		current_file++;
		goto one_more_file;};
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

/* handle definition of label */
slabel(t)
    char * t;
{
    register char *token = t;
    register char *p;
    struct sym_bkt *sbp;
    char nextc;
    static char ltoken[20];

    /* make asciz version of label */
    p = token;
    while (cinfo[*p] & T) p++;
    nextc = *p; /* may be a , a + a \n ... */
    *p = '\0';

    /* look for numeric-only labels, as opposed to numeric-$ labels */
    if ( (p == token+1) && (cinfo[token[0]]&D)){
	sprintf( ltoken, ll_format, token[0], ++ll_val[token[0]-'0'] );
	token=ltoken;
    }

    /* find/enter symbol in the symbol table */
    sbp = lookup(token);

    /* on pass 1 look for multiply defined symbols.  if ok, label
       value is dot in current csect
    */
#ifdef EBUG
    if (debflag)
	printf("Label %s, line %d offset 0x%X\n", sbp->name_s, line_no, dot );
#endif
    if (pass==1) { 
	if (sbp->attr_s & (S_LABEL|S_REG)) {
		prog_error(E_MULTSYM);
	}
        sbp->attr_s |= S_LABEL | S_DEC | S_DEF;
        sbp->csect_s = cur_csect_name;
	if (cur_csect_name == C_TEXT)
	    ntlabels += 1;	/* count text labels */
        sbp->value_s = dot;
    } 
    else if (sbp->csect_s!=cur_csect_name || sbp->value_s!=dot){
	prog_error(E_PHASE);
#ifdef EBUG
	if (debflag)
	    printf("	was 0x%x, but is 0x%x\n", sbp->value_s, dot);
#endif
    }

    if (!(cinfo[t[0]] & D)) 
       last_symbol = sbp;

/* fprintf(stderr,"slabel(%s) = %ld\n",token,Dot); */

    *p = nextc; /* replace next character before proceeding */
} /* end slabel */

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

  /* make asciz version of label */
  p = token;
  while (cinfo[*p] & T) p++;
  nextc = *p;
  *p = '\0';

  /* find/enter symbol in the symbol table, and get its new value */
  sbp = lookup(token);
  lptr = exp(lptr,&value);

  /* if assignment is to dot, we'll treat it specially */
  if (sbp == dot_bkt) {
	int deltadot;
	static char zed[1024];
	if (value.sym_o && value.sym_o->csect_s!=cur_csect_name)
	    PROG_ERROR(E_OPERAND);
	deltadot = value.value_o - dot;
	if (deltadot<0)
	    PROG_ERROR(E_OPERAND);

	operands[0].value_o = deltadot;
	skip_op( NULL );
  } else {
	sbp->value_s = value.value_o;
	sbp->csect_s = (value.sym_o!=NULL) ? value.sym_o->csect_s : C_UNDEF;
	if (sbp->csect_s  == C_TEXT)
	    ntlabels += 1;	/* count text labels */
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
