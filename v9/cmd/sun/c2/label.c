#ifndef lint
static	char sccsid[] = "@(#)label.c 1.1 86/02/03 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

/* for local labels: ``[0-9]:'', and references ``[0-9][bh]'' */
extern char *ll_format;
int   ll_val[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };


/* handle definition of label */
slabel(t)
    char * t;
{
    register char *token = t;
    register char *p;
    register NODE *np;
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
#   ifdef EBUG
	if (debflag)
	    printf("Label %s, line %d offset 0x%X\n",sbp->name_s,line_no,dot);
#   endif
	if (sbp->attr_s & (S_LABEL|S_REG)) {
		prog_error(E_MULTSYM);
	}
        sbp->attr_s |= S_LABEL | S_DEC | S_DEF;
        sbp->csect_s = cur_csect_name;
        sbp->value_s = dot;

    if (!(cinfo[t[0]] & D)) 
       last_symbol = sbp;

#if C2
    if (cur_csect_name == C_TEXT){
	/* make label node */
	np = new();
	np->op = OP_LABEL;
	np->name = sbp;
	sbp->where_s = np;
	addnode(np);
    } else {
	/* print directly */
	printf("%s:\n", token);
    }
#endif

    *p = nextc; /* replace next character before proceeding */
} /* end slabel */

void
newreference( l, r )
    NODE *l, *r;
{
    /*
     * l is a label node.
     * r is a jump node that should be made to reference it.
     * r has already been unreferenced from wherever it pointed before.
     */
    r->lnext = l->luse;
    r->luse=l;
    l->luse=r;
    l->nref++;
}

void
unreference( l, r )
    NODE *l, *r;
{
    /*
     * r is a jump that references node l.
     * unhook r from l's use chain, and decrement l's reference count.
     */
    register NODE **up;

    if (l == NULL) {
	if (r->op == OP_EXIT)
	    return;
	sys_error("no label to unreference, op = 0x%x", r->op);
    }
    up = & l->luse;

    while( *up ){
	if ( *up == r ){
	    *up = r->lnext;
	    l->nref--;
	    return;
	}
	up = & (*up)->lnext;
    }
    sys_error( "tangled usage list for label %s\n", l->name->name_s );
}

relabel(){
    /*
     * find all unreferenced labels and delete them.
     * find all instances (in .text) of multiple labels in a row
     * and collapse them into one label. The difficulty here is
     * the case where we cannot find all references to one of the labels
     * for instance, where a reference is in a switch list, so must treat
     * that one very carefully.
     */
    register NODE * n, *lastlabel;
    register NODE *labelref;
    NODE * nextlabel;
    int nchanged, nuses;
    char labelmoved;

    nchanged = 0;
    for (n=first.forw ; n != &first; n = n->forw){
	if (n->op != OP_LABEL) continue;
	labelmoved = 0;
	lastlabel = n;
	/* find the last label of this group of labels */
	while (lastlabel->forw->op == OP_LABEL)
	    lastlabel = lastlabel->forw;
	while (n != lastlabel){
	    /*
	     * for all labels but the last, retarget all references
	     * to that last one.
	     */
	    if (n->nref){
		if ( labelref = n->luse ){
		    nuses = 1;
		    while (labelref->lnext){
			labelref=labelref->lnext;
			nuses++;
		    }
		} else {
		    nuses = 0;
		}
		/*
		 * if we couldn't find all the references, we must
		 * leave this label in place, so it can be referenced.
		 * It would be nice to emit an equate statement between 
		 * this label and the one we plan on keeping, but the assembler
		 * cannot handle such a reference, so we cannot do it.
		 * try to make this label the "last" one, if this hasn't 
		 * already been done.
		 */
		if (nuses != n->nref){
		    nextlabel = n->forw;
		    n->back->forw = n->forw;
		    n->forw->back = n->back;
		    n->back = lastlabel;
		    n->forw = lastlabel->forw;
		    lastlabel->forw->back = n;
		    lastlabel->forw = n;
		    if (!labelmoved){
			lastlabel = n;
			labelmoved++;
		    }
		    n = nextlabel;
		    continue;
		}
		/*
		 * we have a chain of references to this label.
		 * n->luse addresses the first use, labelref addresses
		 * the last use. add this to lastlabel's reference chain.
		 */
		if (labelref){
		    labelref->lnext = lastlabel->luse;
		    lastlabel->luse = n->luse;
		    lastlabel->nref+= nuses;
		    for (labelref=n->luse; labelref != 0; labelref=labelref->lnext){
			labelref->luse = lastlabel;
		    }
		}
	    }
	    /* this label is now useless */
	    n = deletenode( n );
	    meter.nrlab++;
	    nchanged++;
	    n=n->forw;
	}
	/* we are now looking at the last label in a bunch delete it if unreferenced */
	if (lastlabel->nref==0){
	    n = deletenode( lastlabel );
	    meter.nrlab++;
	    nchanged++;
	}
    }
    return nchanged;
}
