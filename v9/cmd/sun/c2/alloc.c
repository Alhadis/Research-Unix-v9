
#ifndef lint
static	char sccsid[] = "@(#)alloc.c 1.1 86/02/03 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

char *malloc();
void freeoperand();
struct oper * freeoper;
struct optree * freeoptree;


#define NCHUNK 100	/* NODES at a time, for starters */
#define MAXCHUNK 1600	/* max value of 'allocsize' parameter */
NODE *
new(){
    NODE *p;
    static int allocsize = NCHUNK;
    static NODE z = { OP_COMMENT, SUBOP_Z /* rest zero */ };

    if (!freenodes){
	/* allocate new ones */
	register NODE * t;
	register int nnode;
	freenodes = (NODE *)malloc( (unsigned)(allocsize * sizeof *p));
	if (freenodes==NULL) 
		sys_error("cannot allocate %d new nodes", allocsize);
	/* put all on free list */
	for (t=freenodes, nnode=allocsize-1; --nnode; t++ )
	    t->forw = t+1;
	t->forw = NULL;
	if (allocsize < MAXCHUNK)
	    allocsize += allocsize; /* double allocation for next time */
    }
    p = freenodes;
    freenodes = p->forw;
    *p = z;
    p->lineno = line_no;
    return p;
}

NODE *
addnode( p ) 
    register NODE *p;
{
    p->back = first.back;
    p->back->forw = p;
    first.back = p;
    p->forw = &first;
    return p;
}

NODE *
deletenode( p )
    register NODE *p;
{
    NODE *pp = p->back;
    register int i;
    if (p->op == OP_FIRST)
	sys_error("deletenode: Freeing First\n");
    if (ISINSTRUC( p->op ) || ISPSEUDO( p->op ) )
	for (i=0; i<p->nref; i++)
	    freeoperand( p->ref[i] );
    pp->forw = p->forw;
    p->forw->back = pp;
    p->forw = freenodes;
    freenodes = p;
    return pp;
}


struct oper * 
newoperand( o )
    struct oper *o;
{
    struct oper *p;
    static int allocsize = NCHUNK;

    if (!freeoper){
	/* allocate new ones */
	register struct oper * t;
	register int nnode;
	freeoper = (struct oper *)malloc( (unsigned)(allocsize * sizeof *p));
	if (freeoper==NULL) 
		sys_error("cannot allocate %d new operands", allocsize);
	/* put all on free list */
	for (t=freeoper, nnode=allocsize-1; --nnode; t++ )
	    t->nsym_o = t+1;
	t->nsym_o = NULL;
	if (allocsize < MAXCHUNK)
	    allocsize += allocsize; /* double allocation for next time */
    }
    p = freeoper;
    freeoper = p->nsym_o;
    *p = *o;
    return p;
}

void
freeoperand( o )
    struct oper *o;
{
    if (o==NULL) return;
    o->nsym_o = freeoper;
    freeoper = o;
}

void
freetree( o )
    struct optree *o;
{
    if (o==NULL) return;
    o->right_t = (struct oper *)freeoptree;
    freeoptree = o;
}

struct optree *
newtree()
{
    struct optree *p;
    static int allocsize = NCHUNK;

    if (!freeoptree){
	/* allocate new ones */
	register struct optree * t;
	register int nnode;
	freeoptree = (struct optree *)malloc( (unsigned)(allocsize * sizeof *p));
	if (freeoptree==NULL) 
		sys_error("cannot allocate %d new operand subtrees", allocsize);
	/* put all on free list */
	for (t=freeoptree, nnode=allocsize-1; --nnode; t++ )
	    t->right_t = (struct oper *)(t+1);
	t->right_t = NULL;
	if (allocsize < MAXCHUNK)
	    allocsize += allocsize; /* double allocation for next time */
    }
    p = freeoptree;
    freeoptree = (struct optree *)(p->right_t);
    return p;
}
