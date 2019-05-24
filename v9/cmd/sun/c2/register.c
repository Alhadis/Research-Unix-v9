#ifndef lint
static	char sccsid[] = "@(#)register.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

extern struct ins_bkt *sopcode();
extern NODE *insert_label();
extern struct oper *newoperand();
extern unsigned short make_touchop();
int fortranprog = 0;
static struct sym_bkt *skyname;
#if TRACKSP
extern int spoffset; /* track a6-a7 distance */
#endif TRACKSP

extern struct ins_bkt *moveq, *subql, *addql;

#define NREG (FP7REG-D0REG+1)
#define CONSRC 0
#define CONDST 1
struct regcon {
    struct oper con;
    subop_t size;
} regcon[ NREG ][2];

#define NMEM	100	/* totally arbitrary number */
static struct memcon {
    struct oper addr;
    subop_t     size;
    struct oper value;
} memcon[ NMEM ];
int nmem = 0; /* max memcon slot taken */

int bytesize[] = { 
	1,	/* SUBOP_B */
	2,	/* SUBOP_W */
	4,	/* SUBOP_L */
	4,	/* SUBOP_S */
	8,	/* SUBOP_D */
	12,     /* SUBOP_X */
	12	/* SUBOP_P */
    };
    
#define BYTESIZE( s ) (bytesize[ (int)(s)])

int
dead_areg( p )
NODE *p;
{
    register i;
    regmask m; m = p->rlive;
    for (i=A6REG-1; i>=A0REG; i--)
	if (!inmask(i, m )) 
	    return i;
    return -1;
}

int
dead_dreg( p )
NODE *p;
{
    register i;
    regmask m; m = p->rlive;
    for (i=A0REG-1; i>=D0REG; i--)
	if (!inmask(i, m )) 
	    return i;
    return -1;
}


int
cancache( o ) register struct oper *o;
{
    /*
     * currently, the only memory references we can cache are stack 
     * bound variables (and FORTRAN locals)
     */
    extern int Xperimental;

    if (Xperimental)
	switch (o->type_o){
	case T_REG:
	case T_IMMED:
	case T_PREDEC:
	case T_POSTINC: return 0;
	default:        return 1; /* slut */
	}
    if (o->type_o == T_DEFER || o->type_o == T_DISPL)
	if (o->reg_o == FPREG || fortranprog && (o->reg_o==A0REG+4 || o->reg_o==A0REG+5))
	    return 1;
    return 0;
}

static int
istmp( o ) register struct oper *o;
{
    if ((o->type_o==T_DEFER && o->reg_o==FPREG)
    ||  (o->type_o==T_DISPL && o->reg_o==FPREG && o->value_o>=0)) 
	    return 1;
    return 0;
}

static int
isindir( o ) struct oper *o;
{
    switch (o->type_o){
    case T_DEFER :
	if (o->value_o == A6REG) return 0;
	else return 1;
    case T_DISPL:
	if (o->reg_o == A6REG) return 0;
	else return 1;
    case T_INDEX:
	return 1;
    default:
	return 0;
    }
}

struct oper *
get_regcon( regno )
    int regno;
{
    return &regcon[regno][CONSRC].con;
}

void
forgetall()
{
    register regno;
    for (regno=0; regno< NREG ; regno++){
	regcon[regno][CONSRC].con.type_o = T_NULL;
	regcon[regno][CONDST].con.type_o = T_NULL;
    }
    nmem = 0;
}

static void
forgetlocals(forgetsky)
{
    register regno;
    register struct sym_bkt *sk;
    register struct oper *sp;
    sk =  (forgetsky) ? (struct sym_bkt *)-1 : skyname ;
    for (regno=0; regno< NREG ; regno++){
	if ( (sp= &regcon[regno][CONSRC].con)->type_o!=T_NULL
	&& sp->type_o!=T_IMMED && sp->sym_o != sk)
	    sp->type_o = T_NULL;
	regcon[regno][CONDST].con.type_o = T_NULL; /* always */
    }
    nmem = 0;
}

static struct memcon *
memfind( o, l, inexact, startaddr, startn )
    register struct oper *o;
    subop_t l;
    int *inexact;
    struct memcon *startaddr;
    int *startn;
{
    /*
     * lookup this operand at this length in the memory/constant
     * table. Return pointer to the slot where we find it. If
     * the address-length correspondence is inexact, set *inexact.
     * Assume cancache(o).
     * Assume there is no aliasing problem in the memory table.
     */
    register i;
    register struct memcon *mo;
    register operand_t t=o->type_o;
    int membegin = 0, memend, conbegin, conend;
    int r;
    switch (t){
    case T_DISPL: 
	r = o->reg_o;
	/* FALL THROUGH */
    case T_ABSS:
    case T_ABSL:
    case T_NORMAL:
	membegin = o->value_o; 
	memend = membegin + BYTESIZE(l)-1;
	break;
    case T_DEFER:
	r = o->value_o;
	memend = membegin + BYTESIZE(l)-1;
    }
    for (i= *startn, mo = startaddr; --i >= 0; mo++ )
	if (mo->addr.type_o != T_NULL && t == mo->addr.type_o ){
	    switch ( t ){
	    case T_ABSS:
	    case T_ABSL:
	    case T_NORMAL:
		if (o->sym_o != mo->addr.sym_o) continue;
		conbegin = mo->addr.value_o;
		goto compare;
	    case T_DISPL:
		if (mo->addr.reg_o != r) continue;
		conbegin = mo->addr.value_o;
		goto compare;
	    case T_DEFER:
		if (mo->addr.value_o != r) continue;
		conbegin = 0;
	    compare:
		conend = conbegin + BYTESIZE(mo->size)-1;
		if ( membegin< conend && conbegin < memend ){
		    *inexact = ! (membegin==conbegin && l==mo->size);
		    *startn = i;
		    return mo;
		}
	    }
	}
    *startn = 0;
    return NULL;
}

struct oper *
memlookup( o, l )
{
    /* abstract away the "inexact" problem from memfind() */
    int inexact;
    int ntable = nmem;
    struct memcon *m = &memcon[-1];

    while ( (m=memfind( o, l, &inexact, m+1, &ntable)) != NULL )
	if ( !inexact ) return (&m->value);
    return NULL;
}

int
regfind( o, l, inexact, firstreg, firstpart, part )
    register struct oper *o;
    subop_t l;
    int *inexact;
    register firstpart;
    int *part;
{
    /*
     * lookup this operand at this length in the register
     * table. Return the register number that matched. If
     * the address-length correspondence is inexact, set *inexact.
     * Assume cancache(o) or o->type_o==T_IMMED.
     */
    register i,j;
    register struct regcon *mo;
    register operand_t t=o->type_o;
    int membegin , memend, conbegin, conend;
    int r;
    switch (t){
    case T_DISPL: 
	r = o->reg_o;
	/* FALL THROUGH */
    case T_ABSS:
    case T_ABSL:
    case T_NORMAL:
	membegin = o->value_o; 
	memend = membegin + BYTESIZE(l);
	break;
    case T_DEFER:
	r = o->value_o;
	membegin = 0;
	memend = BYTESIZE(l);
    }
    for (i=firstreg ; i<NREG; i++ ){
	for (j=firstpart; j<=CONDST; j++){
	    mo = & regcon[i][j];
	    if (mo->con.type_o != T_NULL && t == mo->con.type_o ){
		switch ( t ){
		case T_IMMED:
		    if ((mo->con.flags_o & O_FLOAT) != (o->flags_o & O_FLOAT)){
			break;
		    } else if (mo->con.flags_o & O_FLOAT ){
			if (mo->con.fval_o == o->fval_o){
			    *inexact = 0;
			     return    i;
			}
		    } else if (mo->con.value_o == o->value_o
			&&  mo->con.sym_o == o->sym_o ){
			    *inexact = 0;
			     return    i;
		    }
		    break;
		case T_ABSS:
		case T_ABSL:
		case T_NORMAL:
		    if (o->sym_o != mo->con.sym_o) continue;
		    conbegin = mo->con.value_o;
		    goto compare;
		case T_DISPL:
		    conbegin = mo->con.value_o;
		    if (mo->con.reg_o != r) continue;
		    goto compare;
		case T_DEFER:
		    conbegin = 0;
		    if (mo->con.value_o != r) continue;
		compare:
		    conend = conbegin + BYTESIZE(mo->size);
		    if ( membegin< conend && conbegin < memend ){
			*inexact = ! (membegin==conbegin && l==mo->size);
			*part = j;
			return i;
		    }
		}
	    }
	}
	firstpart = CONSRC;
    }
    return -1;
}

int
reglookup( o, l )
    struct oper *o;
    subop_t l;
{
    /* abstract away the "inexact" problem from regfind() */
    int inexact, part;
    int r;
    r = regfind( o, l, &inexact, D0REG, CONSRC, &part );
    if ( inexact ) return -1;
    else return r;
}

static void
meminsert( o , v, l )
    struct oper *o;
    struct oper *v;
    subop_t l;
{
    /*
     * look for a free slot in the memcon table to insert an entry.
     * try to add at the end, but if the table is complete, then look
     * for holes. If we still cannot find anything, just forget it.
     * Assume that our caller already tried for an address match
     * and would have inserted by easier means if possible.
     */
    struct memcon *m;
    if (nmem < NMEM){
	/* regular case */
	m = &memcon[nmem++];
    } else {
	/* have to search */
	for (m= &memcon[0]; m > &memcon[NMEM-1]; m++){
	    if (m->addr.type_o == T_NULL) goto gotone;
	}
	return ; /* oh, just forget it */
    }
gotone:
    m->addr = *o;
    m->size = l;
    m->value = *v;
}

static void
put_in_mem_table( o, v, l)
    struct oper *o;
    struct oper *v;
    subop_t l;
{
    /*
     * put this address/value in the memcon table
     * deal with aliasing. This is tricky.
     */
    int nalias = 0, nexact = 0;
    int inexact, ntabl = nmem;
    struct memcon * m;
    int  src, dest, nbytes ;
    union stuff{ /* KNOWS ABOUT 68000 MEMORY LAYOUT !!! */
    char bytes[4];
    short words[2];
    long  longword;
    } mine, his;
    int sympart = v->sym_o!=NULL;


    /* initialize my equivalencing structure */
    mine.longword = 0L;
    if (!sympart)
	switch (l){
	case SUBOP_B: mine.bytes[0] = v->value_o; break;
	case SUBOP_W: mine.words[0] = v->value_o; break;
	default:      mine.longword = v->value_o; break;
	}

    m = &memcon[-1];
    while( (m=memfind( o, l, &inexact, m+1, &ntabl  )) != NULL ){
	if (inexact) {
	    /*
	     * either the address or the length don't agree.
	     * fix up the current entry as appropriate, then continue.
	     */
	    if (sympart || m->value.sym_o != NULL){
		/*
		 * had or will have symbolic part
		 * cannot deal with inexact matches
		 */
		m->addr.type_o = T_NULL; /* forget it */
		continue;
	    }
	    /* fill in his equivalencing structure */
	    his.longword = 0L;
	    switch (m->size){
	    case SUBOP_B: his.bytes[0] = m->value.value_o; break;
	    case SUBOP_W: his.words[0] = m->value.value_o; break;
	    default:      his.longword = m->value.value_o; break;
	    }
	    if (o->value_o >= m->addr.value_o){
		/* new starts after old */
		src  = 0;
		dest = o->value_o - m->addr.value_o;
	    } else {
		src  = m->addr.value_o - o->value_o ;
		dest = 0;
	    }
	    nbytes = BYTESIZE( l );
	    if (nbytes > BYTESIZE( m->size )-dest )
		nbytes = BYTESIZE(m->size)-dest;
	    while (nbytes--){
		his.bytes[dest++] = mine.bytes[src++];
	    }
	    /* shovel back */
	    switch (m->size){
	    case SUBOP_B: m->value.value_o = his.bytes[0]; break;
	    case SUBOP_W: m->value.value_o = his.words[0]; break;
	    default:      m->value.value_o = his.longword; break;
	    }
	    nalias++;
	} else {
	    m->value = *v;
	    nexact++;
	}
    }
    if ((nalias && nexact) || (nalias>4))
	sys_error("Multiple aliasing in memory table\n");
    if (nalias==0 && nexact==0){
	/* new entry -- stick it in. */
	meminsert( o , v, l );
    }
}


static void
forgetop( o, l )
    struct oper *o;
    subop_t l;
{
    int r, inexact, part;
    int ntable = nmem;
    struct memcon *m = &memcon[-1];

    r = D0REG;
    part = CONSRC;
    while( (r = regfind( o, l, &inexact, r, part, &part )) >= D0REG){
	regcon[ r ][ part ].con.type_o = T_NULL;
	if ( ++part > CONDST ){
	    r++;
	    part = CONSRC;
	}
    }

    while ( (m=memfind( o, l, &inexact, m+1, &ntable)) != NULL )
	m->addr.type_o = T_NULL;
}

static void
substitute( p, op, new )
    NODE *p;
    struct oper *op, *new;
{
    /*
     * p is an instruction.
     * op points to an operand of that instruction.
     * new points to a (register) operand structure that we want to
     * substitute for the one op addresses.
     * Do the substitution and calculate the new properties of p
     */
    *op = *new;
    installinstruct( p, p->instr );
    p->rlive = compute_normal( p, p->forw->rlive);
}

static int
track_cc( n )
    NODE *n;
{
    /*
     * we just moved a constant into a register.
     * look for subsequent compares/ tests of
     * the register, followed by jumps, even after cc is kilt
     */
    register struct oper *op2, *comperand;
    struct oper *o;
    register NODE *p;
    NODE *target;
    subop_t so;
    long v, compval;
    static struct oper conz = { T_IMMED, 0, 0, NULL, 0, 0, 0  };
    int didchange = 0;

    if (n->op==OP_MOVE){
	op2 = n->ref[1];
	comperand = n->ref[0];
	if (comperand->type_o != T_IMMED) return 0;
    } else { 
	/* CLR */
	op2 = n->ref[0];
	comperand = &conz;
    }
    if (!deladdr(op2)) return 0;
    so = n->subop;
    v = comperand->value_o;
    for (p = n->forw; p->op!=OP_FIRST ; p = p->forw){
	switch (p->op){
	case OP_LABEL:
		continue;
	case OP_JUMP:
	    if (p->subop==JALL){
		if (p->luse == p->back){
                    /* selfloop */
                    break;
                }
		/* follow! */
		p = p->luse->back;
		continue;
	    }
	    break;
	case OP_TST:
	    if ( sameops( p->ref[0], op2 ) && p->subop==so){
		if (comperand->sym_o!=NULL) break;
		compval = 0;
		goto regcmp;
	    }
	    break;
	case OP_CMP:
	    if ( !sameops(p->ref[1], op2 ) ) break;
	    if ( (o=p->ref[0])->type_o != T_IMMED ) break;
	    if ( o->sym_o != comperand->sym_o ) break;
	    compval = o->value_o;
	regcmp:
	    p=p->forw;
	    if (p->op==OP_JUMP){
		/* this is it */
		if (inmask( CCREG, p->luse->rlive ) || 
		    (p->subop!=JALL && inmask( CCREG, p->forw->rlive)))
		    break; /* too hard -- must do the cmp */
		switch(p->subop){
		case JEQ: compval = v==compval; break;
		case JNE: compval = v!=compval; break;
		case JLE: compval = v<=compval; break;
		case JGE: compval = v>=compval; break;
		case JLT: compval = v< compval; break;
		case JGT: compval = v> compval; break;
		case JCC: compval = ((unsigned)v) >= ((unsigned)compval); break;
		case JLS: compval = ((unsigned)v) <= ((unsigned)compval); break;
		case JHI: compval = ((unsigned)v) >  ((unsigned)compval); break;
		case JCS: compval = ((unsigned)v) <  ((unsigned)compval); break;
		case JALL: compval = 1;         break;
		case JNONE: compval = 0;        break;
		default: goto nojmp;
		}
		if (compval) target = p->luse;
		else{
		    target = p->forw;
		    if (target->op!=OP_LABEL)
			target = insert_label(target);
		}
		cannibalize( p=new(), "jra" );
		p->op = OP_JUMP;
		newreference( target, p);
		p->ruse = p->rset = regmask0;
		p->rlive = target->rlive;
		insert( p, n );
		didchange++;
		meter.nxjump++;
		/*
		 * we just created an unconditional jump. 
		 * Follow it, just like jumps we find in code.
		 */
		p = p->luse->back;
		continue;
	    }
	}
    nojmp:
	break;
    }
    return didchange;
}

static int
addrlookup( o )
    register struct oper *o;
{
    struct oper q;
    int regname;
    /*
     * assume that o points to an index mode, indirect address node.
     * find the memory address through which we are indirecting
     * and see if we already have it in an address register. If so, we
     * can simplify the addressing mode in disindex().
     */
    if (o->flags_o&O_PREINDEX) return -1;
    if (o->flags_o&O_BSUPRESS){
	q.type_o = T_NORMAL;
	q.value_o = o->disp_o;
	q.sym_o = o->sym_o;
	q.flags_o = o->flags_o&O_COMPLEX;
    } else {
	q.type_o = T_DISPL;
	q.reg_o = o->reg_o;
	q.value_o = o->disp_o;
	q.sym_o = o->sym_o;
	q.flags_o = o->flags_o&O_COMPLEX;
	if (q.value_o == 0 && q.sym_o == NULL)
	    q.type_o = T_DEFER;
    }
    if (!cancache( &q)) return -1;
    regname = reglookup( &q, SUBOP_L );
    if (areg(regname)) return regname;
    return -1;
}

static void
disindex( o, regname )
    register struct oper *o;
    int regname;
{
    register flags;
    /*
     * o points to an operand of type T_INDEX with the O_INDIRECT flag set.
     * o is not preindexed. The memory cell through which o indirects is
     * also contained in the A register named by regname. o can be simplified
     * because of this: secondary displacement becomes primary displacement,
     * post indexing becomes pre indexing, and the operand is no longer indirect.
     * it may be simplified to T_DEFER, T_DISPL, or T_INDEX.
     */
    flags = o->flags_o & ~O_INDIRECT;
    o->reg_o = regname;
    o->disp_o = o->disp2_o;
    o->sym_o  = o->sym2_o;
    if (flags&O_COMPLEX2)
	flags = (flags & ~O_COMPLEX2) | O_COMPLEX;
    if (flags&O_WDISP2)
	flags = (flags & ~O_WDISP2)|O_WDISP;
    else if (flags&O_LDISP2)
	flags = (flags & ~O_LDISP2)|O_LDISP;
    if ((flags & O_POSTINDEX) == 0){
	if (o->disp_o == 0 && o->sym_o == NULL){
	    o->type_o = T_DEFER;
	} else {
	    o->type_o = T_DISPL;
	}
    } else {
	o->type_o = T_INDEX;
	flags = (flags& ~O_POSTINDEX)|O_PREINDEX;
    }
    o->flags_o = flags;
}

static void
kill_uses( regno )
    register int regno;
{
    register struct regcon *r;
    register struct memcon *m, *l;
    for (r = &regcon[0][CONSRC]; r <= &regcon[NREG-1][CONDST]; r++)
	if (r->con.type_o != T_NULL && operand_uses( &r->con, regno )){
	    r->con.type_o = T_NULL;
	}
    for (m = &memcon[0], l = &memcon[nmem]; m < l; m++)
	if (m->addr.type_o != T_NULL && operand_uses( &m->addr, regno)){
	    m->addr.type_o = T_NULL;
	}
}

void
con_to_reg( regno, o, size )
    struct oper *o;
    subop_t size;
{
    struct regcon *r = &regcon[regno][CONSRC];

    regcon[regno][CONDST].con.type_o = T_NULL;
    r->con =  *o;
    if (dreg(regno))
	r->size = size;
    else if (areg(regno))
	r->size = SUBOP_L;
    else 
	r->size = SUBOP_X;
    if (Xperimental)
	kill_uses( regno );
}

static int
use_moveq( n )
    register NODE *n;
{
    NODE *p;
    static struct oper reg  = { T_REG,   0, 0, NULL, 0, 0, 0  };
    /*
     * doing an immediate operation with a small constant.
     * we can moveq the constant into a dead D register,
     * then do operations from that register. As a bonus,
     * we'll have the value around.
     * Avoid the case of "movl #3,d3". This will get fixed
     * in quicken() later on.
     */

    if (!long_immediate(n->op)
    || n->subop != SUBOP_L
    || (reg.value_o=dead_dreg(n))<0
    || !operand_ok(n->instr, &reg, n->ref[1], 0)
    || (n->op==OP_MOVE && dreg_addr( n->ref[1] ) )){
	return 0;
    }
    p = new();
    p->nref = 2;
    p->ref[0] = newoperand( n->ref[0] );
    p->ref[1] = newoperand( &reg );
    insert( p, n->back );
    con_to_reg( reg.value_o, n->ref[0], SUBOP_L );
    substitute( n, n->ref[0], &reg );
    installinstruct( p, moveq );
    p->rlive = compute_normal( p, n->rlive );
    return 1;
}

static void
kill_cached_mem( n, touchop )
    register NODE *n;
    register touchop;
{
    register i;
    /* if a local is modified, invalidate our cached value */
    for (i=0;i<n->nref;i++){
	if (touchop&(BW|LW|WW))
	    if (isindir(n->ref[i]))
		forgetlocals(0);
	    else if ( cancache(n->ref[i]))
		forgetop( n->ref[i], n->subop );
	touchop >>= TOUCHWIDTH;
    }
}

dumpmem(){
    int i,j;
    extern char *rnames[];
    static char sizes[] = "bwlsdxp";
    for (i=0; i<NREG; i++){
	printf("%s	: ", rnames[i]);
	for (j=0; j <= 1; j++)
	    if (regcon[i][j].con.type_o == T_NULL)
		printf("XXXX		");
	    else{
		printoperand( &regcon[i][j].con );
		printf("{%c}	", sizes[(int)regcon[i][j].size]);
	    }
	printf("\n");
    }
    printf("\n");
    for (i=0; i<nmem; i++){
	if (memcon[i].addr.type_o != T_NULL){
	    printoperand( &memcon[i].addr );
	    printf("{%c}: ", sizes[(int)memcon[i].size]);
	    printoperand( &memcon[i].value );
	    printf("\n");
	}
    }
}

static int
substitute_move( n, o, reg, saveop )
    register NODE *n;
    struct oper *o, *reg, *saveop;
{
    if (n->instr == moveq)
	return 0;
    substitute( n, o, reg );
    if (saveop->type_o==T_IMMED && cancache(n->ref[1]))
	put_in_mem_table( n->ref[1], saveop, n->subop );
    meter.nsaddr++;
    return 1;
}

/* 
 * we really should be doing flow analysis here.
 * lacking that, recognize special case SKY code:
 *	1: tstw	a1@(-4)
 *	   bge	1b
 */
# define CHECK_LABEL(n) \
    if (n->op==OP_LABEL){ \
	if (!(n->nref==1 && n->luse==n->forw->forw && emptymask(n->forw->rset) && n->forw->forw->op==OP_JUMP)) forgetall(); \
	continue; }

# define IMMED_OR_SKY( o ) \
    (o->type_o==T_IMMED || (o->type_o==T_NORMAL && o->value_o==0 && o->sym_o==skyname))

/* move to self: kill unless cc live */
# define DELETE_SELFMOVES( n ) \
    if ( n->ref[1]->value_o==regno && (!inmask( CCREG, n->forw->rlive) || areg(regno))){ \
	    n = deletenode( n ); meter.redunm++; didchange++; continue; }

# define SMALL_IMMED( o ) \
    ((o)->type_o==T_IMMED && (o)->sym_o==NULL && !((o)->flags_o&O_FLOAT) && (v=(o)->value_o)>=-128 && v<=127)

int
content()
{
    /*
     * Follow the flow of control (roughly). Look at loads into
     * registers. When a constant is loaded we know:
     * 1) how the CC is set.
     * 2) a cheap source of that constant.
     * Special case if the same register is reloaded with that constant.
     * SPECIAL HACK: Assume that the contents of the global var named
     * "__skybase" are constant!!
     */
    register NODE *n;
    register v, regno;
    register struct oper *o;
    struct oper *op2, *op3;
    int didchange=0;
    int local_read, dest_reg;
    int i;
    struct oper saveop;


    static struct oper reg  = { T_REG,   0, 0, NULL, 0, 0, 0  };

    /* initialize constant table to empty. Find __skybase */
    forgetall();
    if (skyname==NULL) skyname = lookup("__skybase");
#if TRACKSP
    spoffset=0;
#endif TRACKSP

    for (n=first.forw; n!=&first; n=n->forw){
	CHECK_LABEL(n);
	if (!ISINSTRUC(n->op)) continue;
	dest_reg = -1;
	if (n->nref){
	    v = make_touchop( n, n->instr->touchop_i);
	    kill_cached_mem( n, v );
	    o = n->ref[0];
	    local_read = cancache(o) && (v&(BW|LW|WW))==0;
	    if (IMMED_OR_SKY( o ) || local_read ){
		/* do lookup -- do we already have it */
		regno = reglookup( o, n->subop );
		if (regno>=0 ){
		    /* try to substitute */
		    reg.value_o = regno;
		    saveop = *o;
		    if (operand_ok(n->instr, &reg, n->ref[1], 0)){
			if (n->op==OP_MOVE){
			    didchange += substitute_move( n, o, &reg, &saveop );
			    if ( datareg_addr( n->ref[1] ) ){
				DELETE_SELFMOVES( n );
				con_to_reg( n->ref[1]->value_o, &saveop, n->subop );
				didchange += track_cc(n);
				dest_reg = n->ref[1]->value_o;
			    }
			} else {
			    /* not a move -- don't think, just do it */
			    substitute( n, o, &reg );
			    meter.nsaddr++;
			    didchange++;
			}
		    }
		} 
		if (n->nref==2 && SMALL_IMMED( o )){
		    didchange+= use_moveq( n );
		}
		if (local_read && (op2=memlookup(o, n->subop))!= NULL){
		    /*
		     * this is a local that contains an immediate at this
		     * time. substitute the immediate value for the mem-op,
		     * if we may.
		     */
		    if (operand_ok(n->instr, op2, n->ref[1], 0)){
			substitute( n, o, op2 );
			meter.nsaddr++;
			didchange++;
		    }
		}
		if (n->op==OP_MOVE){
		    op2 = n->ref[1];
		    if (datareg_addr(op2)){
			/* move to a register */
			con_to_reg( op2->value_o, o, n->subop );
		    } else if (cancache( op2)){
			if(o->type_o==T_IMMED )
			    put_in_mem_table( op2, o, n->subop );
			else if (o->type_o==T_REG && datareg( regno=o->value_o )
			&& (op3= &regcon[regno][CONSRC].con)->type_o == T_IMMED)
			    put_in_mem_table( op2, op3, n->subop );
		    }
		    /*
		     * we may have moved a constant into a register.
		     * look for subsequent compares/ tests of
		     * the register, followed by jumps, even after cc is kilt
		     */
		    didchange += track_cc(n);
		    if (op2->type_o == T_REG)
			dest_reg = op2->value_o;
		}
	    } else if (n->op==OP_MOVE && n->ref[0]->type_o==T_REG 
	    && cancache( n->ref[1] )){
		/* movl d0,a6@(4) ; movl a6@(4),d1 */
		regcon[v=n->ref[0]->value_o][CONDST].con = *(n->ref[1]);
		regcon[v][CONDST].size = n->subop;
		dest_reg = v;
	    } else if (n->op==OP_CALL){
		forgetlocals(1);
	    } else if (n->op==OP_CLR){
		didchange += track_cc(n);
		if (n->ref[0]->type_o == T_REG)
		    dest_reg = n->ref[0]->value_o;
            }
	    /*
	     * try to avoid indirecting through things 
	     * of which we already know the value 
	     */
	    for (i=n->nref, v=0; v<i; v++){
		if ((o = n->ref[v])->type_o == T_INDEX && (o->flags_o&O_INDIRECT))
		    if ((regno = addrlookup( o )) >= 0 ){
			disindex( o, regno );
			substitute( n, o, o );
			meter.nsaddr++;
		    }
	    }
	}
	if (!emptymask(n->rset) ){
	    /* kill a constant value */
	    register sets = n->rset.da; 
	    while (sets){
		v = ffs(sets)-1;
		sets ^= 1<<v;
		if (v<24)
		    v /=3;
		else 
		    v -=(24-A0REG);
		if ( v == dest_reg )
		    continue;
		regcon[v][CONSRC].con.type_o = T_NULL;
		regcon[v][CONDST].con.type_o = T_NULL;
		if (Xperimental)
		    kill_uses( v );
#ifdef TRACKSP
		if (v==SPREG){
		    /* try to keep track of distance from a6 to a7 */
		    track_sp( n );
		}
#endif TRACKSP
	    }
	    sets = n->rset.f & 0377; /* registers only, no cc */
	    while (sets){
		v = ffs(sets)-1;
		sets ^= 1<<v;
		v += FP0REG;
		if ( v == dest_reg )
		    continue;
		regcon[v][CONSRC].con.type_o = T_NULL;
		regcon[v][CONDST].con.type_o = T_NULL;
	    }
	}
    }
    return didchange;
}
