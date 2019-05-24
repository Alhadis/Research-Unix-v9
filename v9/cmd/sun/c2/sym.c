#ifndef lint
static	char sccsid[] = "@(#)sym.c 1.1 86/02/03 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */


#include "as.h"

/* Allocation increments for symbol buckets and character blocks */
#define	SYM_INCR	50
#define CBLOCK_INCR	512

extern struct stab_sym_bkt *stabkt_head;

struct sym_bkt *last_symbol;			/* last symbol defined */
struct sym_bkt *sym_hash_tab[HASH_MAX];		/* Symbol hash table */
struct sym_bkt *sym_free = NULL;		/* head of free list */
char *cblock = NULL;				/* storage for symbol names */
int ccnt = 0;					/* number of chars left in c block */

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
{
	register int i;
	for (i=0; i<HASH_MAX; i++) sym_hash_tab[i] = NULL;
} /* end Sym_Init */

char *
sstring(string)
	register char *string;
{	
	register char *p,*q;	/* working char string */
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
{
	register struct sym_bkt	*sbp;	/* general purpose ptr */
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
	sbp->value_s =  sbp->attr_s = 0;
#if AS
	sbp->id_s = 0;
#endif
	sbp->csect_s = C_UNDEF;
	sbp->next_s = sym_hash_tab[Save];	/* and insert on top of list */
	if (s == local || *s == *ll_format) sbp->attr_s |= S_LOCAL;
	return(sym_hash_tab[Save] = sbp);
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
