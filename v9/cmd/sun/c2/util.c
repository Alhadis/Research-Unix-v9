#ifndef lint
static	char sccsid[] = "@(#)util.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

int
cntbits( vl )
{
    register unsigned long v = vl;
    register  n;
    n = 0;
    do{
	n += v&1;
    }while( v>>=1 );
    return n;
}


/* can this address operand be incremented by n? */
int
incraddr( o, n )
    register struct oper *o;
{
    /*
     * Because we don't want to mess up potential device drivers,
     * we must be very prudent here. Too bad. 
     * We can only do this to a memory location if its an offset from
     * the stack or frame pointers. Those had better not point to
     * device registers, damn them.
     * We must also (I fear) be careful around immediates that are still
     * relocatable.
     */
    extern int Xperimental;

    switch (o->type_o){
    case T_IMMED:
	    if (n!=0 && o->sym_o!=NULL) return 1;
	    return 0;
    case T_REG:
	    if (o->value_o >= A0REG && n != 0 ) return 1;
	    else return 0;
    case T_DISPL:
	    if (!Xperimental)
		if ( (o->reg_o < (fortranprog?(A0REG+4):FPREG))
		|| (n+o->value_o > 0x7fff) ) return 1;
	    o->value_o += n;
	    return 0;
    case T_DEFER:
	    if (!Xperimental)
		if (o->value_o < (fortranprog?(A0REG+4):FPREG)) return 1;
	    if (n != 0){
		o->reg_o = o->value_o;
		o->value_o = n;
		o->sym_o = 0;
		o->type_o = T_DISPL;
	    }
	    return 0;
    case T_INDEX:
	    if (!Xperimental)
		if (o->reg_o < (fortranprog?(A0REG+4):FPREG)) return 1;
	    if (!Xperimental)
		if  (o->disp_o+n > 0x7f) return 1;
	    if (o->flags_o & O_INDIRECT)
		o->disp2_o += n;
	    else
		o->disp_o  += n;
	    return 0;
    }
    /* default falls through here */
    return 1; /* no way */
}

/* can this operand reference be deleted? */
int 
deladdr( o )
    struct oper *o;
{
    return !incraddr( o, 0) ;
}

/* compare operands, instructions */

int
sameops( op1, op2)
    register struct oper *op1, *op2;
{
#   define SAME( a ) if (op1->a != op2->a) return 0
    /* are these two operands the same? */
    SAME( type_o );
    SAME( flags_o );
    switch (op1->type_o){
    case T_IMMED:
		if ( op1->flags_o & O_FLOAT ){
		    SAME( fval_o );
		    break;
		}
		/* fall through */
    case T_NORMAL:
    case T_ABSS:
    case T_ABSL:
		SAME( sym_o );
		/* fall through */
    case T_REG: 
    case T_DEFER:
    case T_POSTINC:
    case T_PREDEC:
		SAME( value_o );
		break;
    case T_INDEX:
		SAME( disp_o );
		if (op1->flags_o & O_INDIRECT){
		    SAME( disp2_o );
		    SAME( sym2_o );
		}
		if (op1->flags_o & (O_PREINDEX|O_POSTINDEX)){
		    SAME( scale_o );
		}
		/* fall through */
    case T_DISPL:
		SAME( sym_o );
		/* fall through */
    case T_REGPAIR:
		SAME( value_o );
		SAME( reg_o );
		break;
    default:    return 0;
    }
    if (op1->flags_o & O_BFLD){
	SAME( bfoffset_o );
	SAME( bfwidth_o  );
    }
    return 1;
#   undef SAME
}

int
sameins( i1, i2 )
    register NODE *i1, *i2;
{
    register int refs, nrefs;

    if (!ISINSTRUC( i1->op) ) 
	sys_error("sameins: got non-instruction as comperand\n");
    if (i1->instr != i2->instr|| (nrefs=i1->nref) != i2->nref)
	return 0;
    for (refs=0; refs<nrefs; refs++)
	if (!sameops( i1->ref[refs], i2->ref[refs]) )
	    return 0;
    return 1;
}


extern void printmask();

/* flush an instruction directly to stdout */
putinstr( ip )
    struct ins_bkt *ip;
{
    register i,n;
    printf("	%s", ip->text_i);
    if (n = numops ){
	putchar('\t');
	printoperand( &operands[0] );
	for (i=1; i<n; i++){
	    putchar(',');
	    printoperand( &operands[i] );
	}
    }
    putchar('\n');
}


void
dumpnode( np, verbose )
    register NODE *np;
{
    register i,n;
    struct sym_bkt *switchlabel;

    switch (np->op){
    case OP_LABEL:
	if (verbose)
	    printf("%s:	| referenced %d times",
		np->name->name_s, np->nref );
	else
	    printf("%s:", np->name->name_s );
	break;
    case OP_COMMENT:
	printf("|%s", (char *)(np->ref[0]));
	break;
    default:
	printf("	%s", np->instr->text_i);
	if (n = np->nref ){
	    putchar('\t');
	    printoperand( np->ref[0] );
	    for (i=1; i<n; i++){
		putchar(',');
		printoperand( np->ref[i] );
	    }
	}
	if ( np->op==OP_JUMP || np->op==OP_DJMP ){
	    if (np->subop==JIND){
		fputs("\tpc@(2,d0:w)", stdout);
		switchlabel = np->luse->name;
	    } else {
		putchar((n)?',':'\t');
		fputs(np->luse->name->name_s, stdout);
	    }
	} else if (np->op==OP_CSWITCH){
	    putchar('\t');
	    fputs(np->luse->name->name_s, stdout);
	    putchar('-');
	    fputs(switchlabel->name_s, stdout);
	}
	break;
    }
    if (verbose){
	fputs("	| ", stdout);
	if (!emptymask(np->ruse)){
	    fputs(" reads:", stdout); printmask( np->ruse);
	}
	if (!emptymask(np->rset)){
	    fputs(" writes:", stdout);printmask( np->rset);
	}
	if (!emptymask(np->rlive)){
	    fputs(" live:", stdout); printmask( np->rlive);
	}
    }
    putchar('\n');
}

void
dumpprogram(verbose)
{
    NODE *np;

    printf("	.text\n");
    for (np=first.forw; np != &first; np = np->forw){
	dumpnode( np, verbose );
    }
}

void
sortints( ip, n )
    register int *ip;
    register n;
{
    /* sort the array of n ints. Not quick, but compact */
    register int *np, *p;
    register changed, temp;
    np = ip + n - 1;
    changed = 1;
    while(changed){
	changed = 0;
	for (p = ip; p < np; p++)
	    if (*p > *(p+1) ){
		temp = *p;
		*p = *(p+1);
		*(p+1) = temp;
		changed = 1;
	    }
    }
}

void 
xref(){
    register NODE *p, *u;
    int allocsize = 100, usecnt;
    int *ip, *tp;
    ip = (int *)malloc( allocsize * sizeof * p );
    for (p=first.forw; p != &first; p=p->forw){
	if (p->op==OP_LABEL){
	    fputs( p->name->name_s, stdout );
	    if (p->nref+1 > allocsize ){
		allocsize += allocsize;
		if (p->nref+1 > allocsize )
		    allocsize = p->nref + 5;
		ip = (int *)realloc( ip, allocsize * sizeof * p );
	    }
	    tp = ip;
	    *tp++ = p->lineno;
	    for (usecnt = p->nref, u=p->luse; usecnt--; ){
		if (u==NULL)
		    *tp++ = -1;
		else {
		    *tp++ = u->lineno;
		    u = u->lnext;
		}
	    }
	    sortints( ip, p->nref+1 );
	    putchar( ':' );
	    for ( usecnt = p->nref+1, tp=ip ; usecnt--; tp++ ){
		if (*tp == -1)
		    fputs(" ???", stdout);
		else
		    printf(" %3d", *tp);
		if (*tp == p->lineno)
		    putchar('*' );
	    }
	    putchar('\n');
	}
    }
    free( ip );
}

/*
 * Insert a node after another one
 */
insert(p, after)
NODE	*p;
NODE	*after;
{
	NODE	*lastp;

	for (lastp = p; lastp->forw != NULL; lastp = lastp->forw)
		;
	after->forw->back = lastp;
	lastp->forw = after->forw;
	after->forw = p;
	p->back = after;
}

#ifdef C2
char *
docomment(p)
    register char *p;
{
    extern char *c2pseudocomment();
    NODE *n;

    if (*p != '#' && *p != '|') return p;
    n = new();
    n->op = OP_COMMENT;
    n->ref[0] = (struct oper *)malloc( strlen(p) );
    strcpy( (char *)n->ref[0], p+1 );
    addnode( n );
    if (strncmp(p,C2MAGIC,C2MAGICSIZE) == 0){
	p = c2pseudocomment( p+=C2MAGICSIZE );
    }
    return p;
}
#endif
