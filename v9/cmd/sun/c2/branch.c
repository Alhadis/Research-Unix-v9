#ifndef lint
static	char sccsid[] = "@(#)branch.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

char *unbranch[] = { 
     "jne", "jeq", "jgt", "jlt", "jge", "jle", 
     "jhi", "jls", "jpl", "jmi", "jf", 
     "jcc", "jcs", "jvc", "jvs", "jra",
     "jfneq", "jfeq", "jfnlt", "jflt",
     "jfnle", "jfle", "jfngt", "jfgt",
     "jfnge", "jfge",
     "fjneq", "fjeq", "fjngt", "fjgt", "fjnge", "fjge",
     "fjnlt", "fjlt", "fjnle", "fjle", "fjngl", "fjgl",
     "fjngle", "fjgle", "fjule", "fjogt", "fjult", "fjoge",
     "fjuge", "fjolt", "fjugt", "fjole", "fjueq", "fjogl",
     "fjun", "fjor", "fjst", "fjsf", "fjsneq", "fjseq"
};
extern NODE *deletenode();

NODE *
insert_label( p ) 
    register NODE *p;
{
    static int newlabno = 0;
    register NODE *l;
    register struct sym_bkt *s;
    char labname[20];

    l = new();
    l->op = OP_LABEL;
    l->nref = 0;
    sprintf( labname, "LY%05d", newlabno++ );
    l->name = s = lookup(labname);
    s->attr_s |= S_DEF|S_DEC;
    s->csect_s = C_TEXT;
    s->where_s = l;
    l->forw = p;
    l->back = p->back;
    l->back->forw = l;
    p->back = l;
    l->ruse = l->rset = regmask0;
    l->rlive = l->forw->rlive;
    return l;
}

NODE *
merge_labels( to, from )
    register NODE * to, *from ;
{
    /*
     * there are two kinds of labels:
     * those all of whose references we can find, and
     * the others.
     * In the latter case, we will want to move the label,
     * and let someone smarter than us drop the equate.
     */
    register refs = 0;
    register NODE *luse;
    if (from->luse!=NULL){
	for ( luse=from->luse; ; luse=luse->lnext ){
	    luse->luse = to;
	    refs++;
	    if (luse->lnext==NULL) break;
	}
	luse->lnext=to->luse;
	to->luse =from->luse;
    }
    if (refs == from->nref){
	from = deletenode( from )->forw;
	meter.nrlab++;
    } else {
	/*
	 * at this point there are not references on our list, but
	 * still references to this label somewhere out there
	 * move the damn thing 
	 * and null out the use list
	 */
	NODE *temp;
	from->nref -= refs;
	from->luse = NULL;
	temp = from->forw;
	from->back->forw = from->forw;
	from->forw->back = from->back;
	from->forw = to->forw;
	from->forw->back = from;
	from->back = to;
	to->forw = from;
	from = temp;
    }
    to->nref += refs;
    return from;
}

int
fallthrough( t )
    register NODE *t;
{
    /*
     * return 0 if t is a node that cannot "fall through" to the
     * next instruction, such as an unconditional jump or code-
     * emitting pseudo-instruction
     * return 1 if this is not the case.
     */
    if (ISDIRECTIVE(t->op) ) return 1;
    else if (ISPSEUDO(t->op) ) return 0;
    else switch (t->op){
	case OP_CSWITCH:
	case OP_FIRST:
	case OP_EXIT:
	    return 0;
	case OP_BRANCH:
	case OP_JUMP:
	    return  t->subop!=JALL;
	default:
	    return 1;
    }
    /* NOTREACHED */
}


# define nexti( t ) while( t->op==OP_LABEL ) t=t->forw
tangle()
{
    /*
     * remove:
     *   trivial jumps
     *   jumps to jumps
     *   jumps around jumps
     */
    
    int changed = 1;
    int niter=0;
    register NODE *p, *t, *u;
    NODE *j, *jdest, *savelabel;

    while (changed){
	changed=0;
	for (p=first.forw; p!=&first; p=p->forw){
	    if ((p->op==OP_JUMP&&p->subop!=JIND)||p->op==OP_CSWITCH){
		t = p->luse;
		nexti( t );
		if (t==p){
		    /* selfloop */
		    if (p->luse->nref==1 && !fallthrough(p->back)){
			/* unreachable */
			unreference( p->luse, p);
			p = deletenode( p );
			meter.nbrbr++;
			changed++;
		    }
		    continue;
		}
		if (t->op==p->op && (t->subop==JALL || t->subop==p->subop)){
		    /* jump-to-jump -- remove the middle reference */
		    if (p->luse==t->luse){
			/* jump into selfloop */
			continue;
		    }
		    unreference( p->luse, p);
		    newreference( t->luse, p);
		    t = p->luse;
		    nexti( t );
		    meter.nbrbr++;
		    changed++;
		}
		if (p->op==OP_CSWITCH ) continue;
		if (p->luse==p->forw){
		    /* trivial jump */
		    unreference( p->luse, p);
		    p = deletenode( p );
		    changed++;
		    meter.njp1++;
		    continue;
		}
		if (p->subop == JALL){
		    u = (t=p->luse)->back;
		    /* find the non-pseudo-op before the destination */
		    while (ISDIRECTIVE(u->op))
			u = u->back;
		    if ( t->nref==1 && ( u->op==OP_FIRST ||
		    ((u->op==OP_JUMP&&u->subop==JALL)|| u->op==OP_EXIT))){
			/*
			 * this piece of code is reachable from p but not by
			 * fall through. move it to after p and delete 
			 * the jump. stop moving after a cswitch 
			 * or unconditional goto or return.
			 */
			jdest = u->luse;
			savelabel = 0;
			u = t;
			while ((u=u->forw)->op!= OP_FIRST){
			    if (u->op==OP_LABEL ){
				if ( u->back->op==OP_CSWITCH )
				    break;
				else if (savelabel != jdest 
				&& !(u->forw->op==OP_CSWITCH ) )
				    savelabel = u;
			    }else if ((u->op==OP_JUMP && u->subop==JALL)
			    || u->op==OP_EXIT ){
				u = u->forw;
				break;
			    }
			}
			/* t points to the first node to move, and    */
			/* u points to the first node not to move.    */
			j = u->back;
			if (j==p){
			    /*
			     * oh, my... a loop. It may be without any 
			     * way in, but I'm too 'fraid to try deleteing
			     * it. Lets try pivoting around an appropriate
			     * label in the loop.
			     */
			     if (savelabel){
				u = savelabel;
				j = u->back;
			    } else
				 continue;
			}
			if (j->op != OP_CSWITCH 
			&& !(j->op==OP_JUMP && j->subop==JALL)
			&& j->op != OP_EXIT
			&& u->op==OP_LABEL){
			    /* add unconditional branch here */
			    j->forw = new();
			    j->forw->back = j;
			    j = j->forw;
			    cannibalize( j, "jra" );
			    j->op = OP_JUMP; /* rather than OP_BRANCH */
			    newreference( u, j);
			}
			u->back = t->back;
			t->back->forw = u;
			t->back = p;
			j->forw = p->forw;
			p->forw->back = j;
			p->forw = t;
			/* the details will take care of themselves */
			p = p->back;
			changed++;
			meter.ncmot++;
			continue;
		    }
		    /* delete dead code after unconditional branch */
		    /* should do after exits, too */
		    u = p->forw;
		    while ((t=u)->op!=OP_LABEL && t->op!=OP_FIRST
		      && t->op != OP_EXIT){
			u = t->forw;
			if (ISPSEUDO( t->op ))
			    continue; /* don't delete pseudoops */
			if (ISBRANCH(t->op))
			    unreference( t->luse, t);
			(void)deletenode( t );
			meter.iaftbr++;
			changed++;
		    }
		} else {
		    /* here for conditional branches */
		    u = p->forw;
		    /* may be no intervening branches here */
		    if (u->op==p->op && (u->subop==JALL || u->subop==p->subop)){
			j = u;
			u = u->forw;
			nexti( u );
			if (u==t){
			    /* jump-around-jump -- reverse condition, go directly */
			    cannibalize( j, unbranch[(int)(p->subop)-(int)JEQ] );
			    j->op = OP_JUMP; /* cannibalize makes an OP_BRANCH*/
			    unreference( p->luse, p );
			    p = deletenode( p );
			    meter.nrevbr++;
			    changed++;
			}
		    } else if ( (int)p->subop <= (int)JNONE && u->op==OP_DJMP && u->subop==JNONE){
			/*
			 *     jXX Y
			 *     dbra dZ,W
			 * should be changed to:
			 *     dbXX dZ,W
			 *     jXX  Y
			 * because its faster. If label Y directly follows 
			 * the dbra, the jump can be deleted entirely!
			 */
			static char dbname[5] = {'d', 'b', '\0', '\0', '\0'};
			/* 
			 * since we cannot search for the correct instruction
			 * indexed by op and subop, we need to synthesize the
			 * new name and call cannibalize.
			 * jump instruction is of form [jb]CC[ls].
			 */
			dbname[2] = p->instr->text_i[1];
			dbname[3] = p->instr->text_i[2];
			cannibalize( u, dbname );
			u->op = OP_DJMP;

			/*
			 * unhook p from the list, reinsert it later
			 */
			u = u->forw;
			nexti( u );
			if (u==t){
			    unreference( p->luse, p );
			    deletenode(p);
			    meter.nskip++;
			} else {
			    p->back->forw = p->forw;
			    p->forw->back = p->back;
			    p->forw = p->back = 0;
			    insert( p, u->back );
			    meter.ndbrarev++;
			}
			p = u->back;
			changed++;
		    }
		}
	    }
	}
	if (++niter > 1000) sys_error("Tangled control-flow logic");
    }
    return niter-1;
}

zipper()
{
    /*
     * For each label: look at all the jumps to that label:
     *    for those that are unconditional:
     *        compare the instructions preceeding to the
     *        instructions preceeding the label. The unconditional
     *        jump could jump to an earlier point in the "fall through"
     *        flow.
     */
    
    register NODE *l, *u, *prec, *uprec, *luse;
    NODE *pr, *upr;
    int refs;
    int nchange=0, didchange;

    for (l=first.forw; l!= &first; l=l->forw){
	if (l->op!=OP_LABEL) continue;
	prec = l->back;
	/* skip over stabs */
	while (ISDIRECTIVE(prec->op) )
	    prec=prec->back;
	if (!ISINSTRUC(prec->op))
	    continue;
	if (prec->op==OP_CSWITCH ) continue;
	if ((prec->op==OP_BRANCH || prec->op==OP_JUMP) && prec->subop==JALL)
	    continue;
	pr = prec;
	for (u = l->luse; u!=NULL; u=upr, prec=pr){
	    upr = u->lnext;
	    uprec = u->back;
	    didchange=0;
	    if (u->op!=OP_JUMP || u->subop!=JALL) continue;
	    while (ISDIRECTIVE(uprec->op) )
		uprec=uprec->back;
	    if (uprec->op==OP_FIRST) continue;
	    while(1){
		if (uprec->op != prec->op ){
		    if (prec->op==OP_LABEL){
			/* skip it */
			prec=prec->back;
			continue;
		    }else 
			break;
		}
		if (prec==uprec || prec==u || uprec==upr) break;
		if  (prec->op == OP_LABEL ){
		    uprec = merge_labels( prec, uprec );
		    didchange++;
		} else{
		    if (ISPSEUDO( prec->op )) goto endzip;
		    if (!sameins( prec, uprec) ) goto endzip;
		    if (ISBRANCH(prec->op))
			if (prec->luse != uprec->luse)
			    goto endzip;
			else
			    unreference( uprec->luse, uprec);
		    uprec = deletenode( uprec )->forw;
		    didchange++;
		}
		prec=prec->back;
		uprec=uprec->back;
	    }
endzip:
	    if (didchange){
		/*
		 * we may have done something here.
		 * uprec addresses the first NON-common node we found.
		 * put the jump after it.
		 */
		if ( uprec->forw != u ){
		    uprec->forw->back = u;
		    u->back->forw = u->forw;
		    u->forw->back = u->back;
		    u->forw = uprec->forw;
		    uprec->forw = u;
		    u->back = uprec;
		}
		if (prec->op!=OP_LABEL)
		    prec = insert_label( prec->forw );
		unreference( u->luse, u);
		newreference( prec, u);
		u->rlive = u->luse->rlive;
		nchange++;
	    }
	}
    }
    meter.ncomj += nchange;
    return nchange;
}

int
tmerge()
{
    /*
     * For each label, z,  in the program, try to find commonality
     * amoung the pieces of code that end up at that label.
     * We are very cautious.
     * We are looking for the forms:
     * 	   jra	elsewhere
     *   L: X
     *      X
     *     jra  z
     * and
     *     jYY L
     *     X
     *	   X
     *	   jra	z
     *  L:
     * which we can combine for smaller and no-slower code.
     */
    
    NODE *z;
    register NODE *a, *b;
    NODE *ajmp;
    NODE *bnext, *anext, *t;
    NODE *bforw, *aforw;
    int nchanged = 0;

    for (z=first.forw; z!=&first; z=z->forw){
	if (z->op != OP_LABEL) continue;
	for (ajmp=z->luse; ajmp!=NULL; ajmp=anext){
	    anext=ajmp->lnext;
	    if (ajmp->op != OP_JUMP || ajmp->subop != JALL) continue;
	    for (b=ajmp->lnext; b!=NULL; b=bnext){
		bnext=b->lnext;
		if (b->op != OP_JUMP || b->subop != JALL) continue;
		bforw = b->forw;
		a = ajmp;
		aforw = a->forw;
		/*
		 * compare backwards until:
		 * -- we hit a non-equality, or
		 * -- we hit a label, or
		 * -- we hit a jump
		 */
		for ( a=a->back, b=b->back;
		    a->op!=OP_FIRST && b->op!=OP_FIRST;
		    a=a->back, b=b->back){

		    while (ISDIRECTIVE( a->op ))
			a=a->back;
		    while (ISDIRECTIVE( b->op ))
			b=b->back;
		    switch (a->op){
		    default:   /*            |   */
			if ( ISPSEUDO(a->op) || !sameins( a, b ) ) break;
			continue;
		    case OP_JUMP:
		    case OP_LABEL:
		    case OP_CSWITCH:
			break; /* to outer break */
		    }/* +--------------------+   */
		     /* v                        */
		    break; /* out of for loop */
		}/* +---------------------+   */
		/*  v
		 * we have now compared just a little too far:
		 * either *a != *b , or they are both pseudo-ops,
		 * jumps, labels, or something else unspeakable.
		 */
		a = a->forw;
		b = b->forw;
		if (a == ajmp)
		    /* no equal sequences */
		    continue;
		/* look for preceeding label, preceeded by unconditional jump */
		if (b->back->op == OP_LABEL && (fallthrough(b->back->back) 
		|| (a->back->op == OP_JUMP && a->back->luse == aforw)) ){
		    /* want to merge into "b" -- exchange */
		    t = b; b = a; a = t;
		    t = bforw; bforw = aforw; aforw = t;
		    /* and fall through into "a" test, which will succeed */
		}
		if (a->back->op == OP_LABEL){
		    if ( (t=b->back)->op == OP_LABEL && !fallthrough(t->back)){
			/* merge into "a" */
			(void)merge_labels( a->back, t);
			/* cream junk trailing merged-out code */
			if (anext == bforw->back)
			    anext = NULL;
			while ( b!=bforw ){
			    if (ISBRANCH(b->op))
				unreference( b->luse, b );
			    b = deletenode( b )->forw;
			}
			nchanged++;
			break;
		    } else if (t->op == OP_JUMP && t->luse == bforw ){
			a = a->back; /* "a" now addresses label */
	    cond_jumparound:
			/* 
			 * delete "b" code, complement branch condition,
			 * change branch target to "a" 
			 */
			if (anext == bforw->back)
			    anext = NULL;
			while ( b!=bforw ){
			    if (ISBRANCH(b->op))
				unreference( b->luse, b );
			    b = deletenode( b )->forw;
			}
			unreference( t->luse, t);
			newreference( a, t);
			cannibalize( t, unbranch[(int)(t->subop)-(int)JEQ] );
			t->op = OP_JUMP; /* not BRANCH */
			nchanged++;
			break;
		    } else if (!fallthrough( a->back->back) ){
			/*
			 * we can just move label "a" to before "b"
			 * and delete the rest of the "a" code
			 */
			while (a != aforw ){
			    if (ISBRANCH(a->op))
				unreference( a->luse, a );
			    a = deletenode( a )->forw;
			}
			a->back->back->forw = a;
			a->back->forw = b;
			t->forw = a->back;
			a->back = a->back->back;
			t->forw->back = t;
			b->back = t->forw;
			nchanged++;
			break;
		    }
		    /* else out of luck */
		} else if (a->back->op == OP_JUMP && a->back->luse == aforw
		&& b->back->op == OP_JUMP && b->back->luse == bforw ){
		    /* 
		     * must insert new label, then treat like one of
		     * the labelled cases above.
		     */
		    a = insert_label( a );
		    t = b->back;
		    goto cond_jumparound;
		}
	    }
	}
    }
    meter.ncomj += nchanged;
    return nchanged;
}

/*
 * assign a sequential node number to each node.
 * Sequence is used to detect backwards edges.
 */
order_nodes()
{
    register NODE *n;
    register nodenumber;

    nodenumber = 0;
    for (n = first.forw; n != &first; n = n->forw) {
	n->lineno = nodenumber++;
    }
}

/*
 * change test-at-the-top loops to test-at-the-bottom
 * with a jump to the test at the front.
 * In order that we not get confused and loop forever moving the test
 * back and forth, we will recompute the node ordering every time a
 * loop is inverted.
 */
int
invloop()
{
    int nchanged;
    register NODE *n;
    register nodenumber;
    register NODE *headlabel, *taillabel;
    NODE *jump, *newlabel;

    nchanged = 0;
    order_nodes();
    for (n = first.forw; n != &first; n = n->forw){
	/*
	 * look for this configuaration:
	 * headlabel -> Lx:
	 *		    <some instructions ( the test )>
	 * n         ->	     conditional jump to Ly
	 *		    <some more instructions ( the loop body )>
	 * jump      ->	    jra Lx
	 * taillabel -> Ly:
	 * If there are many conditional jumps (as in the case of 
	 * while( xx && yy)) then we can key off of any of the jumps, 
	 * but the first one makes the most sense, since it will be 
	 * executed the most frequently.
	 */
	if (n->op == OP_JUMP && n->subop != JALL && 
	(taillabel = n->luse)->lineno > n->lineno){
	    jump = taillabel->back;
	    if (jump->op == OP_JUMP && jump->subop == JALL &&
	    (headlabel = jump->luse)->lineno < n->lineno){
		/* 
		 * Got it: now reorganize as:
		 * Lx:
		 * 	jmp Lnew2
		 * Lnew1:
		 *	< some instructions ( the loop body ) >
		 * Lnew2:
		 *	< some more instructions ( the test ) >
		 *	conditional jump to Lnew1 ( reversed conditions )
		 * Ly:
		 * in the simple cases, Lx and Ly will become unused at this
		 * point.
		 */
		/* move test & jmp code to after the jump */
		unreference( headlabel, jump );
		jump->forw = headlabel->forw;
		headlabel->forw = n->forw;
		n->forw->back = headlabel;
		n->forw = taillabel;
		taillabel->back = n;
		jump->forw->back = jump;
		/* delete the jump, replace by a label */
		newlabel = insert_label( deletenode( jump )->forw );
		/* insert new jump to new label at head of loop */
		jump = new();
		jump->forw = headlabel->forw;
		headlabel->forw = jump;
		jump->back = headlabel;
		jump->forw->back = jump;
		cannibalize( jump, "jra" );
		jump->op = OP_JUMP; /* rather than OP_BRANCH */
		newreference( newlabel, jump );
		/* add another new label after the new jump */
		newlabel = insert_label( jump->forw );
		/* flip condition on conditional, point it at the new label */
		cannibalize( n, unbranch[(int)(n->subop)-(int)JEQ] );
		n->op = OP_JUMP; /* rather than OP_BRANCH */
		unreference( taillabel, n );
		newreference( newlabel, n );
		order_nodes();
		n = taillabel->back;
		/* if either of the old labels is superfluous, delete it */
		if (headlabel->nref == 0 ) (void)deletenode(headlabel);
		if (taillabel->nref == 0 ) (void)deletenode(taillabel);
		nchanged++;
		meter.loopiv++;
	    }
	}
    }
    return nchanged;
}

chase(n,tail)
    register NODE *n, *tail;
{
    register NODE *p;
    if (n == tail)
	return;
    for (p = n->forw; p != n; p = p->forw) {
	if (p == tail)
	    return;
    }
    printf("lost track of node %s\n", tail);
}
