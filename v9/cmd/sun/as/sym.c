#ifndef lint
static	char sccsid[] = "@(#)sym.c 1.1 86/02/03 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <a.out.h>
#include "as.h"
#include "scan.h"

/* Allocation increments for symbol buckets and character blocks */
#define	SYM_INCR	50
#define CBLOCK_INCR	512

extern struct stab_sym_bkt *stabkt_head;

struct sym_bkt *last_symbol;			/* last symbol defined */
struct sym_bkt *sym_hash_tab[HASH_MAX];		/* Symbol hash table */
struct sym_bkt *sym_free = NULL;		/* head of free list */
char *cblock = NULL;				/* storage for symbol names */
int ccnt = 0;					/* number of chars left in c block */
int ntlabels = 0;

/* grab a new symbol bucket off of the free list; allocate space
 * for a new free list if necessary
 */
struct sym_bkt *
gsbkt()
  {	register struct sym_bkt	*sbp;
	register int i;

	if ((sbp = sym_free) != NULL) sym_free = sbp->next_s;
	else {
	  sbp = (struct sym_bkt *)calloc(SYM_INCR,sizeof(struct sym_bkt));
	  if (sbp == NULL) sys_error("Symbol storage exceeded\n",0);
	  for (i = SYM_INCR-1; i--;) {
	    sbp->next_s = sym_free;
	    sym_free = sbp++;
	  }
	}

	return(sbp);
}

/* initialize hash table */
sym_init()
  {	register int i;

	for (i=0; i<HASH_MAX; i++) sym_hash_tab[i] = NULL;
} /* end Sym_Init */

char *sstring(string)
  register char *string;
  {	register char *p,*q;	/* working char string */
	register int i;

	i = strlen(string);	/* get length of string */

	if (++i > ccnt) {	/* if not enough room get more */
	  if ((cblock = (char *)calloc(CBLOCK_INCR,1)) == NULL)
	    sys_error("Symbol storage exceeded\n",0);
	  ccnt = CBLOCK_INCR;
	}

	p = q = cblock;		/* copy string into permanent storage */
	while (*p++ = *string++);
	cblock = p;
	ccnt -= i;
	return(q);
} /* end sstring */

/* lookup symbol in symbol table */
struct sym_bkt *
lookup(s)
    register char *s;
  {	register struct sym_bkt	*sbp;	/* general purpose ptr */
	register int Save;		/* save subscript in sym_hash_tab */
	register char *p;
	static char local[250];		/* used for constructing local sym */
	extern char *ll_format;

	if (*s>='0' && *s<='9') {	/* local symbol hackery */
	  /* we hope no-one uses really long local symbols */
	  p = local;
	  while ((*p++ = *s++) && (p < &local[sizeof local-1]));/* copy local symbol */
	  p--;
	  s = last_symbol->name_s;	/* add last symbol defined as suffix */
	  while ((*p++ = *s++) && (p < &local[sizeof local-1]));
	  s = local;			/* this becomes name to deal with */
	}

	/* if the symbol is already in here, return a ptr to it */
	for (sbp = sym_hash_tab[Save=hash(s)]; sbp != NULL ; sbp = sbp->next_s)
	  if (strcmp(sbp->name_s,s) == 0) return(sbp);

	/* Since it's not, make a bucket for it, and put the bucket in the symbol table */
	sbp = gsbkt();				/* get the bucket */
	sbp->name_s = sstring(s);		/* Store it's name */
	sbp->value_s = sbp->id_s = sbp->attr_s = 0;
	sbp->csect_s = C_UNDEF;
	sbp->next_s = sym_hash_tab[Save];	/* and insert on top of list */
	if (s == local || *s == *ll_format) sbp->attr_s |= S_LOCAL;
	return(sym_hash_tab[Save] = sbp);
}

/* Sym_Fix - Assigns index numbers to the symbols.  Also performs 
             relocation of the symbols assuming data segment follows 
             text and bss follows the data.  If global flag, make all 
             undefined symbols defined to be externals.
*/

/* symbol value compare -- for sorting */
static int
symvc( a, b )
    struct sym_bkt **a, **b;
{
    return ((*a)->value_s - (*b)->value_s);
}

sym_fix()
{
  register struct sym_bkt **sbp1, *sbp2;
  int i = 0;
  register int t;
  struct sym_bkt **symlistlist;
  register struct sym_bkt **syml;

  symlistlist = syml = (struct sym_bkt **)calloc( ntlabels, sizeof *symlistlist );
  for (sbp1 = sym_hash_tab; sbp1 < &sym_hash_tab[HASH_MAX]; sbp1++)
      for (sbp2 = *sbp1; sbp2; sbp2 = sbp2->next_s) {
           if ((sbp2->attr_s & (S_DEC|S_DEF)) == 0) {
	         sbp2->attr_s |= S_EXT | S_DEC;
	         sbp2->csect_s = C_UNDEF;
	   }
	   switch (sbp2->csect_s) {
	   case C_TEXT:
		   if (sbp2 != dot_bkt && !(sbp2->attr_s & S_PERM))
		       *syml++ = sbp2;
		   break;
	   case C_DATA:
		   sbp2->value_s += tsize; break;
	   case C_DATA1:
                   sbp2->value_s += tsize + dsize; break;
	   case C_DATA2:
		   sbp2->value_s += tsize + dsize + d1size; break;
	   case C_BSS:
		   sbp2->value_s += tsize + dsize + d1size + d2size; break;
	   }
	   if (sbp2==dot_bkt || sbp2->attr_s & (S_REG|S_LOCAL|S_PERM))
	            sbp2->id_s = -1;
	   else 
		    sbp2->id_s = i++;
      }
  /* consistancy check */
  if (syml - symlistlist != ntlabels)
      sys_error("wrong number of text labels: %d found\n", syml-symlistlist);
  /* sort C_TEXT symbols on value */
  qsort( symlistlist, ntlabels, sizeof *symlistlist, symvc );
  /* and go update those labels using the sdi information */
  sdi_sym_update( symlistlist, ntlabels );
  free( symlistlist );
}


/* sym_write -	Write out the symbols to the specified
		file in b.out format, while computing size
		of the symbol segment in output file.
 */

redosyms()
{
	/* Go through the symbol table and get rid of "L" syms if 
		we are supposed to. */
	long size = 0;
	register struct sym_bkt  **sbp1, *sbp2;

	for (sbp1 = sym_hash_tab; sbp1 < &sym_hash_tab[HASH_MAX]; sbp1++)
	    if (sbp2 = *sbp1)
		for (; sbp2; sbp2 = sbp2->next_s)
		{
		    if (sbp2->id_s != -1 && chkname(sbp2)) {
			    sbp2->final = size++;
		    }
		}
}

long sym_write(file)
  FILE *file;
  { register struct sym_bkt  **sbp1, *sbp2;
    long size = 0;
    struct nlist s;
    int strcount = 4;
    struct stab_sym_bkt *t;
	
    for (sbp1 = sym_hash_tab; sbp1 < &sym_hash_tab[HASH_MAX]; sbp1++)
        if (sbp2 = *sbp1) for (; sbp2; sbp2 = sbp2->next_s)
	   if (sbp2->id_s != -1 && chkname(sbp2)) {
		 /* Write out the symbol table using the VAX a.out format */
		 if ((sbp2->attr_s&S_DEF)== 0) {
		    s.n_type = N_UNDF;
		    if (*(sbp2->name_s) == 'L' ) {
			prog_warning( E_UNDEFINED_L ); /* Undefined L-symbol */
		    }
		 } else {
		    switch (sbp2->csect_s){
		    case C_TEXT:
			s.n_type = N_TEXT; break;
		    case C_UNDEF:
			s.n_type = N_ABS; break;
		    case C_BSS:
			s.n_type = N_BSS; break;
		    default:
			s.n_type = (rflag)?N_TEXT:N_DATA; break;
		    }
		} 
		if (sbp2->attr_s & S_EXT) s.n_type |= N_EXT;
		s.n_value = sbp2->value_s;
		/* For right now, just stuff these with 0 */
		s.n_other = s.n_desc = 0;
		s.n_un.n_strx = strcount;
		strcount += strlen(sbp2->name_s)+1;
		size += sizeof(s);
		fwrite(&s,sizeof(s),1,file);
	    } else if (!(sbp2->attr_s & S_DEF)){
		    PROG_ERROR( E_UNDEFINED_L );
	    }

        /* This is to write out the .stabs and .stabd symbols onto the        */
        /* a.out file.  This is only being written after the regular          */
        /* symbols have been put out (the prior section of the function).     */
        t = stabkt_head;           /* obtain head of stabs/stabd symbol table */
        while (t != NULL)
              { s.n_type   = t->type;
                s.n_other  = t->other;
                s.n_desc   = t->desc;       
                s.n_value  = t->value;                /* zero for testing now */
                if (t->id)
                   { s.n_un.n_strx = strcount;      /* assign string offset   */
                     strcount += t->id + 1;	    /* increment str location */
                   }                                /* in string table.       */
                else s.n_un.n_strx = 0;             /* else if no string is   */
                                                    /* present, assign 0.     */
                size += sizeof(s);
                fwrite(&s, sizeof(s), 1, file);
                t = t->next_stab;
              } /* end while */
	return(size);
}

long 
str_write(file)
	FILE *file;
{
	long size = 0;
	long strcount = 4;
	register struct sym_bkt  **sbp1, *sbp2;
        struct stab_sym_bkt *t;


	fwrite(&strcount,sizeof(long),1,file);
	for (sbp1 = sym_hash_tab; sbp1 < &sym_hash_tab[HASH_MAX]; sbp1++)
		if (sbp2 = *sbp1) for (; sbp2; sbp2 = sbp2->next_s)
		if (chkname(sbp2))
		{
			if (sbp2->id_s != -1) {
				register i;
				strcount +=  i = strlen(sbp2->name_s)+1;
				fwrite(sbp2->name_s,i,1,file);
			}
		}

        /* This is to write out all the symbols (strings) generated by        */
        /* .stabs and .stabd directives.  They are being written onto the     */
        /* string table only after all the regular symbol tables have been    */
        /* written out (by the prior section of this function.                */
        t = stabkt_head;                     /* head of stabs/stabd link list */
        while (t != NULL)
              { register i;
                if (t->id)
                   { strcount += i = t->id + 1;
                     fwrite(t->ch,i,1,file); 
                   }
                t = t->next_stab;
              } /* end while */
	return(strcount);
}

chkname(name)
struct sym_bkt *name;
{
    extern char o_lflag;

    if(o_lflag) return (1);
    if ( *(name->name_s)!='L') return(1);
    if ( (name->attr_s&S_DEF) == 0) return(1); /* don't zap undef L's */
    return(0);
}

/*
 * Perm	Flags all currently defined symbols as permanent (and therefore
 *	ineligible for redefinition.  Also prevents them from being output
 *	in the object file).
 */
perm()
  {	register struct sym_bkt **sbp1, *sbp2;

	for (sbp1 = sym_hash_tab; sbp1 < &sym_hash_tab[HASH_MAX]; sbp1++)
		for (sbp2 = *sbp1; sbp2; sbp2 = sbp2->next_s)
			sbp2->attr_s |= S_PERM;
}
