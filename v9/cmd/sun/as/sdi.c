#ifndef lint
static	char sccsid[] = "@(#)sdi.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"

/*
 * module to handle span-dependent instructions, e.g. jbr
 * see CACM Vol 21 No 4 April 1978 for a description of the
 * algorithm for resolving sdi's between the 1st and 2nd pass
 */

#define SQUID( x ) ( ((x)==C_TEXT)||(rflag && ((x) != C_BSS)) )
#define INFINITY	0x7fffffff	/* larger than any address */

struct sdi {	/* information for span dependent instructions (sdi's) */
	struct sdi 	*sdi_next;      /* next sdi in list		   */
	long int 	sdi_loc;	/* location of the sdi		   */
	union{
	    struct sym_bkt 	*sdi_Sym;/* symbol part of the sdi address  */
	    int			*sdi_Add;
	}su;
#define	sdi_sym	su.sdi_Sym
#define sdi_address su.sdi_Add
	long int 	sdi_off;	/* offset part of the sdi address  */
	short 		sdi_leng;	/* actual length of the sdi	   */
	long int 	sdi_base;	/* origin of the span of the sdi   */
	short		sdi_flags;	/* bits: is/was it a ddi? 	   */
# if EBUG
	unsigned	sdi_lineno;
# endif EBUG
} *sdi_list;				/* linked list of descriptors */

struct sdi *ddi_list;

/* sdi_flag bit */
#define SDI_TYPE 7 /* SDI4, SDI6, SDI8, SDIP */
#define SDI_DDI       010
#define SDI_EXTERN 020

		  /* SDI4, SDI6, SDI8, SDIP, SDIX, SDIL */
static int glen[] = {  4,    6,    8,    4,    6,    4 };


/* grab a new sdi descriptor off of the free list; allocate space
 * for a new free list if necessary
 * stolen from the symbol-table manager
 * We keep the sdi and ddi lists separate because:
 * 1) we want to keep locality in the sdi list
 * 2) we want to give back the ddi list
 */

#define SEGALLOC 	100
static
struct seglist { int allocsize, nalloced; struct sdi *nextfree, *head; char *segname;} 
	sdilist = { SEGALLOC, 0, NULL, NULL, "sdi" },
        ddilist = { SEGALLOC, 0, NULL, NULL, "ddi" };

static long total_delta = 0;

struct sdi *
sdi_alloc(s)
    register struct seglist *s;
{	
    register struct sdi	*sdp, *sdi_free;
    register int i;

    if ((sdp = s->nextfree) != NULL) 
	s->nextfree = sdp->sdi_next;
    else {
        sdp = (struct sdi *)calloc(s->allocsize,sizeof(struct sdi));
        if (sdp == NULL) sys_error("%s storage exceeded\n", s->segname);
	sdp->sdi_next = s->head;
	s->head = sdp++;
	sdi_free = NULL;
        for (i = s->allocsize-2; i--;) {
	    sdp->sdi_next = sdi_free;
	    sdi_free = sdp++;
        }
	s->nextfree = sdi_free;
	s->allocsize *= 2;
    }
    s->nalloced++;
    return(sdp);
}

void
sdi_free(s)
    register struct seglist *s;
{
    register struct sdi *t, *u;
    u = s->head;
    while (u != NULL){
	t = u->sdi_next;
	free(u);
	u = t;
    }
    s->head = NULL;
    s->nextfree = NULL;
    s->allocsize = SEGALLOC;
    s->nalloced = 0;
}

/*
 * short-lived data structure
 * used just for sorting the destination addresses, and keeping pointers
 * to the corresponding sdi's.
 */
struct sdi_destpair { struct sdi *user; int address; };

int * destlist;
int destsize;

sort_destpair( a, b )
    struct sdi_destpair *a, *b;
{
	return a->address - b->address;
}

/*
 * reformulate the base/span representation as follows:
 * for each sdi, form a pair: a pointer to the sdi structure and
 * the destination address (relative to the text base).
 * Sort on destination address, then re-form as an integer array
 * into which point the sdi structures. The array can be segmented
 * to speed up processing it.
 */
make_destlist()
{
	struct sdi_destpair *darray;
	register struct sdi_destpair *dp;
	register struct sdi *s, *d;
	register int *a;
	register ndest = 0;
	register lastaddr, newaddr;
        int ddidelta;

	if ( sdilist.nalloced==0 ) return; /* empty list */
	if ((darray = (struct sdi_destpair *)calloc( sdilist.nalloced , sizeof( struct sdi_destpair))) == NULL)
		sys_error("cannot allocate intermediate structure");
	for (s=sdi_list, dp = darray; s; s = s->sdi_next){
	    if (s->sdi_loc == -1 ) continue;
	    if (s->sdi_sym == NULL){
		dp->address = s->sdi_off;
		dp++ -> user = s;
		ndest++;
	    }
	}
	if (destsize=ndest){
	    qsort( darray, ndest, sizeof (struct sdi_destpair), sort_destpair );
	    destlist = a = (int *)calloc( ndest, sizeof a[0] );
	    if (a==NULL)
		    sys_error("cannot allocate intermediate structure");
	    dp = darray;
	    *a = lastaddr = dp->address;
	    dp++ ->user->sdi_address = a++;
	    while (--ndest > 0){
		if (lastaddr == (newaddr = dp->address)){
		    /* pick up the occasional duplicate destination address */
		    a--;
		    destsize--;
		}else
		    *a = lastaddr = newaddr;
		dp++ ->user->sdi_address = a++;
	    }
	}
	free( darray );
        /*
         * this is the part I hate.
         * sweep through the sdi list and the dest list in parallel.
         * for each sdi that was a ddi, subtract 2 from all destinations
         * above the base address of the s(d)di. At the same time, 
         * sweep through the ddi list and subtract the shrinkage size
         * from all destinations above the base address of the ddi. 
         * These are corrective factors we must apply because of initially
         * assuming that all ddi's would be long. The base addresses of
         * the sdi's have already been fixed by the routine resolve_ddi.
         */
        if (destsize){
            a = destlist;
            ndest = destsize;
            s = sdi_list;
            d = ddi_list;
            ddidelta = 0;
            while(ndest--){
                newaddr = *a;
                while( s != NULL && s->sdi_loc < newaddr ){
                    if (s->sdi_flags & SDI_DDI)
                        ddidelta += 2;
                    s = s->sdi_next;
                }
                while( d != NULL && d->sdi_loc < newaddr ){
                    ddidelta += d->sdi_flags;
                    d = d->sdi_next;
                }
                *a++ -= ddidelta;
	    }
	}
}

/*
 * routine to create a sdi descriptor and insert it into the list
 */
makesdi(op, base, flavor)
	struct oper *op;/* the operand of the sdi */
	long int base;	/* origin of the the initial span of the sdi, to be */
			/* corrected between pass1 and pass2 by subtracting */
			/* from the value of the symbol of the operand */
			/* i.e. 0 for resolving short absolute address modes */
			/* Dot[+increment] for PC relative addresses */
{
	register struct sdi *s, **p;
	register int loc;
	static long lastloc = INFINITY; /* you wanna see hackery? */
	static struct sdi * lastsdi;	 /* we got hackery         */

	if (!op ){                 /* put the procedure boundary: .proc */
		if (Oflag) return;
		s = sdi_alloc(&sdilist);
		s->sdi_loc = -1;
		for ( p = lastsdi ? &(lastsdi->sdi_next) : &sdi_list;
		     *p;
		      p = &(*p)->sdi_next );
		s->sdi_next = *p;
		*p = s;
		return;
	}
	if (op->flags_o&O_COMPLEX)	/* not a simple address */
		return(0);
	s = sdi_alloc(&sdilist);
	s->sdi_loc = dot; /* future bug here */
	s->sdi_flags = flavor;
	s->sdi_sym = op->sym_o;
        s->sdi_off = op->value_o-op->sym_o->value_s; /* collect offset-from-label */
	s->sdi_base = base;
	s->sdi_leng = 2;	/* shortest length */
# if EBUG
	s->sdi_lineno = line_no;
# endif EBUG
	loc = dot;
	for ( p = (loc < lastloc) ? &sdi_list : &(lastsdi->sdi_next);
	     *p; 
	      p = &(*p)->sdi_next)
		if (loc < (*p)->sdi_loc)
			break;
	s->sdi_next = *p;
	*p = s;
	lastloc = loc;
	lastsdi = s;
	return(2);	/* return the current length */
}

/*
 * resolve sdi's between pass1 and pass2
 * basic algorithm is to repeatedly look for sdi that must use the
 * long form, and update the span of other sdi's.
 * When this terminates, all remaining sdi's can use the short form
 */
sdi_resolve()
{
	register struct sdi *s;
	register struct sym_bkt *p;
	register int t;
	struct sdi *first, *end, *q;
	long loc, big_delta = 0;
	int *dest, *start;
	int changed;
	int csect_offsets[5];

	csect_offsets[C_UNDEF] = 0;
	csect_offsets[C_TEXT ] = 0;
	csect_offsets[C_DATA ] = tsize;
	csect_offsets[C_DATA1] = tsize + dsize;
	csect_offsets[C_DATA2] = tsize + dsize + d1size;

	for (s = sdi_list; s && s->sdi_loc == -1; s = s->sdi_next );
	sdi_list = s;             /* get rid of .proc at the beginning */

	for (s = sdi_list; s; s = s->sdi_next) {
		if (s->sdi_loc == -1) continue;
		p = s->sdi_sym;
		if (SQUID(p->csect_s) && (p->attr_s & S_DEF)){
			s->sdi_off += p->value_s + csect_offsets[p->csect_s];
			s->sdi_sym = NULL;
		} else
			s->sdi_flags |= SDI_EXTERN;
	}
        ddi_resolve();
	make_destlist();

	for (first = sdi_list; first; ) {
		for (s=first; s && s->sdi_loc != -1; s=s->sdi_next )
			end = s;         /* find last sdi in this procedure */
		for (s=end->sdi_next; s && s->sdi_loc == -1; s=s->sdi_next );
		if (s)
			loc = s->sdi_loc + total_delta;
		else
			loc = INFINITY;
		if (destsize) {
		    for (dest = destlist;
		        *dest <= loc && dest < destlist + destsize;
		         dest++ );
		    start = --dest;     /* last label before 'end' */ 
		}
		do {
			changed = 0;
			for (s = first; s && s->sdi_loc!=-1 ; s = s->sdi_next) {
				if ((t = sdi_len(s) - s->sdi_leng) > 0) {
					big_delta += t;
					longsdi(s, t, first, start);
					changed = 1;
				} else if (t < 0) {
					sys_error("Pathological sdi\n");
				}
			}
		} while (changed);

		for (s=end->sdi_next; s && s->sdi_loc == -1; s=s->sdi_next );
		first = s;
		end->sdi_next = first;   /* get rid of boundary .proc */
		for (q = first; q; q = q->sdi_next )
			q->sdi_base += big_delta;
		if (destsize)
			for ( dest = destlist + destsize - 1; dest > start; dest-- )
				*dest += big_delta;
		total_delta += big_delta;
		big_delta = 0;
	}
}

/*
 * update sdi list assuming the specified sdi must be long
 */
longsdi(s, delta, first, start )
register struct sdi *s, *first;
register int delta;
int * start;     
{
	register struct sdi *t;
	register long loc;
	register int *dest;

	s->sdi_leng += delta;	/* update the length of this sdi */
	/* get the real location of the sdi */
	loc = s->sdi_loc + sdi_inc( s->sdi_loc, first);
	for (t = s->sdi_next; t && t->sdi_loc != -1; t = t->sdi_next)
		t->sdi_base += delta;
	if (destsize){
	    for ( dest = start; *dest > loc && dest >= destlist; dest-- )
		*dest += delta;
	}
}

/*
 * compute the length of the specified sdi by looking at the possible choices
 */
sdi_len(s)
register struct sdi *s;
{
	struct bchoice { int len; int lbound, ubound; };
	static struct bchoice bsdi68[] = {
		{ 2, -128, 127 },
		{ 4, -32768, 32767 },
		{ 0,      0,     0 },
	};
	static struct bchoice bsdi4[] = {
		{ 2, -128, 127 },
		{ 0,    0,   0 },
	};
	static struct bchoice bsdip[] = {
		{ 2, -32768, 32767 },
		{ 0,      0,     0 },
	};
					  /* SDI4,   SDI6,   SDI8,  SDIP, */
	static struct bchoice *blists[] = { bsdi4, bsdi68, bsdi68, bsdip, };
	/* SDIX and SDIL are never span-dependent, so are not in this table! */

	register struct bchoice *b;
	register span;

	span = *s->sdi_address - s->sdi_base;
	if (!(s->sdi_flags&SDI_EXTERN)){
		for (b = blists[ s->sdi_flags&SDI_TYPE ]; b->len; b++ )
		    if ( b->lbound <= span && span <= b->ubound)
			return(b->len);
	}
	/* only the most general case will do */
	return(glen[s->sdi_flags&SDI_TYPE]);
}

/*
 * return the total number of extra bytes due to long sdi's before
 * the specified offset
 */
sdi_inc(offset, first)
	struct sdi *first;
	register long offset;
{
	register struct sdi *s;
	register int total;

	if (first) {
		s = first;
		total = total_delta;
	} else {
		s = sdi_list;
		total = 0;
	}
	for (; s; s = s->sdi_next) {
	    if (s->sdi_loc == -1 ) continue;
	    if (offset <= s->sdi_loc)
		    break;
	    total += s->sdi_leng - 2;
            if (s->sdi_flags&SDI_DDI){
                total -= 2;
            }
	}
       for (s = ddi_list ; s; s = s->sdi_next) {
            if (offset <= s->sdi_loc)
                    break;
            total -= s->sdi_flags; /* if its still on the list, it shrank */
        }
	return(total);
}

/* 
 * We are given an array of pointers to symbol-table entries for
 * text labels, sorted by location. Update these by the changes in the
 * sdi's. This used to be done by repeated calls to sdi_inc(), but this
 * was obscenely expensive. Now that we have the lists sorted the same way,
 * we should be able to compute this incrementally. Of course, there was
 * the extra overhead of sorting the symbol list in the first place.
 */
sdi_sym_update( symp, nsyms )
    register struct sym_bkt **symp;
    register int nsyms;
{
    register struct sdi      *sdi;
    register struct sdi      *ddi;
    register curloc, curdelta;
    register symloc, curddi;
    curdelta = 0;

    if (nsyms==0) return;
    if ((sdi = sdi_list) == NULL)
	curloc = INFINITY;
    else
	curloc = sdi->sdi_loc;
    if ((ddi = ddi_list) == NULL)
        curddi = INFINITY;
    else
        curddi = ddi->sdi_loc;
    while (nsyms--){
	symloc = (*symp)->value_s;
	while (symloc > curloc){
	    curdelta += sdi->sdi_leng - 2;
            if (sdi->sdi_flags&SDI_DDI){
                curdelta -= 2;
            }
	    if ((sdi = sdi->sdi_next) == NULL){
		curloc = INFINITY;
	    } else {
		curloc = sdi->sdi_loc;
	    }
	}
        while (symloc > curddi){
            curdelta -= ddi->sdi_flags ; /* if on ddi list, it shrank */
            if ((ddi = ddi->sdi_next) == NULL){
                curddi = INFINITY;
            } else {
                curddi = ddi->sdi_loc;
            }
        }
	(*symp++)->value_s = symloc + curdelta;
    }
}

/*
 * Data-dependent instructions 
 * are those whose length we may not be able to ascertain at first pass,
 * but which we would like to address in the most effecient manner.
 * Determining their length could, in full generality, be an iterative
 * process, but I think we can make some useful simplifications that
 * will do most of what we desire in one pass over our intermediate
 * data structure. The gross structure will mimic the sdi code above.
 * The instructions we are dealing with here have three forms:
 * long absolute, word absolute, and PC-relative. Thus, there are
 * two lengths of addressing extensions: two bytes and four bytes.
 */

/*
 * Statistically, ALL data references are long. Thus it makes little
 * sense to follow the sdi strategy of assuming the shortest form until
 * proven otherwise. Since the ddi list is only looked at once, we
 * don't have to maintain the data structures required by the sdi algorithm.
 * In fact, once we've decided that a reference is long, we can take it 
 * entirely off the list, if we make this assumption.
 */
makeddi(op, base, flavor, wrtflg )
        struct oper *op;/* the operand of the sdi */
        long int base;  /* origin of the the initial span of the sdi, to be */
                        /* corrected between pass1 and pass2 by subtracting */
                        /* from the value of the symbol of the operand */
                        /* i.e. 0 for resolving short absolute address modes */
                        /* Dot[+increment] for PC relative addresses */
        int flavor;     /* form of ddi: usually SDIP, but for index SDIX, or SDIL for link*/
        int wrtflg;     /* 0 => not memory write destination: PC-relative ok */
                        /* 1 => memory write destination: PC-relative NOT ok */
{
        register struct sdi *s, **p;
        register int loc;
        static long lastloc = INFINITY;  /* you wanna see hackery? */
        static struct sdi * lastddi;     /* we got hackery         */
  
        if (op->flags_o&O_COMPLEX)      /* not a simple address */
                return(0);
        s = sdi_alloc( &ddilist );
        s->sdi_loc = dot; /* soon to be a bug */
        s->sdi_sym = op->sym_o;
        s->sdi_off = op->value_o;
        s->sdi_base = base;
        s->sdi_leng = 4;        /* assumed length */
        s->sdi_flags =  flavor;
# if EBUG
        s->sdi_lineno = line_no;
# endif EBUG
        if (wrtflg) s->sdi_flags |= SDI_DDI;
        loc = dot;

        for ( p = (loc < lastloc) ? &ddi_list : &(lastddi->sdi_next);
             *p;
             p = &(*p)->sdi_next)
                if (loc < (*p)->sdi_loc)
                        break;
        s->sdi_next = *p;
        lastloc = loc;
        lastddi = s;
        *p = s;
        return(flavor==SDI6?6:4);       /* return the current length */
}


ddi_resolve()
{
        register struct sdi *s, **spp, *q;
        register struct sym_bkt *p;
        register int t;

        spp = &ddi_list;
        while (s = *spp){
                p = s->sdi_sym;
                switch (s->sdi_flags){
                case SDIP:
                    if (SQUID(p->csect_s ) && (p->attr_s & S_DEF) ){
                            /* this looks like a job for PC@()  */
                            /* fill out the sdi structure, and 
                                insert it in the sdi list */
                            register struct sdi **pp;
                            struct sdi *newsdi;

                            s->sdi_off += p->value_s;
                            s->sdi_sym = NULL;
                            /* take ourselves off of ddi list */
                            *spp = s->sdi_next;
                            /* 
                             * here's the tricky part:
                             * all the addresses in our lists assume this
                             * address has length four. We will now change
                             * them so it appears to have length two.
                             */
                            shortddi( s, 2 );
                            s->sdi_flags  = SDI_DDI|SDIP;
                            /* now put ourselves in the sdi list */
                            for (pp = &sdi_list; *pp; pp = &(*pp)->sdi_next)
                                    if (s->sdi_loc < (*pp)->sdi_loc)
                                            break;
                            newsdi = sdi_alloc(&sdilist);
                            *newsdi = *s;
                            newsdi->sdi_next = *pp;
                            *pp = newsdi;
                            /* fix addresses on sdi list, too */
			    newsdi->sdi_leng -= 2;
			    for ( q= newsdi->sdi_next; q; q = q->sdi_next )
				    q->sdi_base -= 2;
                            /* sdi routines will fix it up later */
                            continue;
                    } 
                    /* else fall through */
                case SDIP|SDI_DDI:
  		case SDI4:
  		case SDI4|SDI_DDI:
  		    if ((p->attr_s & S_DEF) && p->csect_s == C_UNDEF) {
  			    /* try on a short absolute */
  			    t = p->value_s + s->sdi_off;
  			    if (t >= -32768 && t <= 32767 ){
  				    shortddi( s, 2 );
  				    spp = &s->sdi_next;
  				    continue;
  			    }
  
  		    }
  		    /* else it does not shrink -- take off list */
  		    break;
  		case SDI6:
  		case SDI6|SDI_DDI:
  		    /*
  		     * Index mode. 
  		     */
  		    /* Write bit only confuses us: delete it */
  		    s->sdi_flags = SDI6;
  		    if ((p->attr_s & S_DEF) && p->csect_s == C_UNDEF){
  			/* try very short, then just plain short */
  			t = p->value_s + s->sdi_off;
  			if ( t >= -128 && t <= 127 ){
  			     /* shrinks much */
  			    shortddi( s, 4 );
  			    spp = &s->sdi_next;
  			    continue;
  			} else if (t >= -32768 && t <= 32767 ){
  			    /* shrinks some */
  			    shortddi( s, 2 );
  			    spp = &s->sdi_next;
  			    continue;
  			} 
  		    }
  		    /* else it does not shrink -- take off list */
  		    break;
  		case SDIX:
  		case SDIX|SDI_DDI:
  		    /*
  		     * should be displacement mode, but will be
  		     * index mode with supressed indexing if the
  		     * displacement is too big.
  		     */
  		    /* FALL THROUGH */
  		case SDIL:
  		    /* 
  		     * link instruction: looks like SDIP, but is not
  		     * PC-relative.
  		     */
  		    /* Write bit only confuses us: delete it */
  		    s->sdi_flags &= SDI_TYPE;
  		    if ((p->attr_s & S_DEF) && p->csect_s == C_UNDEF){
  			/* try short */
  			t = p->value_s + s->sdi_off;
  			if (t >= -32768 && t <= 32767 ){
  			    /* shrinks */
  			    shortddi( s, glen[s->sdi_flags&SDI_TYPE] - 2 );
  			    spp = &s->sdi_next;
  			    continue;
  			}
  		    }
  		    /* else it does not shrink -- take off list */
  		    break;
  		}
  		/* take ddi off the list */
  		*spp = s->sdi_next;
  	}
  	if (ddi_list == NULL){
  	    sdi_free(&ddilist); /* all gone */
  	} else {
  	    register int cursdi;
  	    /*
  	     * Sweep through ddi list and sdi list in parallel,
  	     * fixing up sdi base-addresses.
  	     */
  	    s = ddi_list;
  	    t = 0;
  	    q = sdi_list;
  	    while (q != NULL){
  		cursdi = q->sdi_base;
  		while ( s != NULL && s->sdi_base < cursdi ){
  		    t += s->sdi_flags;
  		    s = s->sdi_next;
  		}
  		q->sdi_base -= t;
  		q = q->sdi_next;
  	    }
  	}
  }

shortddi( s, d )
    register struct sdi *s;
    register d;
{
    /*
     * A ddi shrank! Adjust all lists accordingly.
     */
    register struct sdi *t;
    register loc = s->sdi_loc+sdi_inc( s->sdi_loc, NULL);

    s->sdi_flags = d; /* how much I shrank */
    for (t = ddi_list; t; t = t->sdi_next)
	    if (t != s && t->sdi_base > loc)
		    t->sdi_base -= d;
}
