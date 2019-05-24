#ifndef lint
static	char sccsid[] = "@(#)sky.c 1.1 86/02/03 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"

#define SP_REG		15		/* Stack pointer: a7 */
#define	STATUS_WORD	-2		/* Offset to sky status word */
#define	IDLE		6		/* Idle bit in hibyte of status word */

#define SP_ADD		0x1001		/* Single add: B1 = A2 + A1 */
#define SP_ADDREG	0x104c		/* Single add: R0 = R0 + A1 */
#define SP_ADDB1	0x1003		/* Single add: B1 = R0 + A1 */

#define SP_SUB		0x1007		/* Single sub: B1 = A1 - A2 */
#define SP_SUBREG1	0x104f		/* Single sub: R0 = R0 - A1 */
#define SP_SUBREG2	0x1050		/* Single sub: R0 = A1 - R0 */

#define SP_MUL		0x100b		/* Single mul: B1 = A2 * A1 */
#define SP_MULREG	0x1052		/* Single mul: R0 = R0 * A1 */
#define SP_MULB1	0x100d		/* Single mul: B1 = R0 * A1 */

#define SP_DIV		0x1013		/* Single div: B1 = A2 / A1 */
#define SP_DIVREG1	0x1055		/* Single div: R0 = R0 / A1 */
#define SP_DIVREG2	0x1056		/* Single div: R0 = A1 / R0 */

#define	SP_LOAD		0x1031		/* Single load:  R0 = A1 */
#define	SP_STORE	0x1037		/* Single store: B1 = R0 */

#define isskyop(op)	(findop(op) != NULL)

/*
 * Struct for each sky operator that can be changed.
 * The code generator produces operations that push two operands
 * to the board and fetch one result.  The goal here is to use
 * the on board register as an accumulator.  Therefore, one
 * operand will come from register R0 and the result will be
 * left in R0.
 * For some operations (add and mul), one operand can come
 * from register R0 and the result can be fetched from the
 * board.  Otherwise (sub and div), a SKY operation to fetch
 * the result must be inserted.
 */
struct	skyop	{
	int	op;			/* Sky board opcode */
	int	arg_op[2];		/* Opcode if arg "n" in R0 */
	int	fetch;			/* Opcode to fetch result from board */
};

/*
 * Info about single precision SKY board operators.
 */
struct	skyop	regops[] = {
	{ SP_ADD, SP_ADDREG,  SP_ADDREG,  SP_ADDB1 },
	{ SP_SUB, SP_SUBREG1, SP_SUBREG2, 0 },
	{ SP_MUL, SP_MULREG,  SP_MULREG,  SP_MULB1 },
	{ SP_DIV, SP_DIVREG1, SP_DIVREG2, 0 },
	{ 0 },
};

/*
 * Struct for a sky board operation
 */
struct	skylist	{
	NODE	*skyload;		/* Instruction to load opcode */
	NODE	*arg[2];		/* Args passed to the board */
	NODE	*result;		/* Result fetched from the board */
	NODE	*oldresult;		/* Result fetched from the board */
	int	oldop;			/* Original sky operator */
	int	tstloop;		/* Is result preceeded by tst loop */
	struct	skylist	*next;		/* Linked list */
};

struct	oper	*newoperand();
struct	skylist	*get_sky();
struct	skylist *bld_skylist();
struct	skyop	*findop();
NODE	*newinst();
NODE	*remove();
NODE	*delete_tst_loop();
static	struct	sym_bkt	*skyname;
static  struct	oper skybase;

sky()
{
	NODE *n;
	register v, regno;
	struct oper *op1;
	struct oper *op2;
	int didchange=0;
	int i, inexact;
	struct oper saveop;
	struct skylist *skylist;
	static struct oper reg  = { T_REG,   0, 0, NULL, 0, 0, 0  };
	static struct oper conz = { T_IMMED, 0, 0, NULL, 0, 0, 0  };

	/* initialize constant table to empty. Find __skybase */
	forgetall();
	if (skyname==NULL) skyname = lookup("__skybase");
	skybase.type_o = T_NORMAL;
	skybase.sym_o = skyname;
	skybase.value_o = 0;

	for (n = first.forw; n != &first; n = n->forw){
		if (n->op == OP_LABEL) {
			/* 
			 * we really should be doing flow analysis here.
			 * lacking that, recognize special case SKY code:
			 *	1: tstw	a1@(-4)
			 *	   bge	1b
			 */
			if (n->nref == 1 && n->luse == n->forw->forw && 
			    emptymask( n->forw->rset) && n->forw->forw->op == OP_JUMP) {
				n = n->forw;
				continue;
			}
			/* forget everything */
			forgetall();
			continue;
		}
		if (n->nref == 0) {
			continue;
		}
		if (n->op != OP_MOVE) {
			continue;
		}
		op1 = n->ref[0];
		op2 = n->ref[1];
		skylist = bld_skylist(&n);
		if (skylist != NULL) {
			rewrite_list(skylist);
			free_list(skylist);
			n = n->back;
		} else if (op1->type_o == T_NORMAL && 
			   op1->value_o == 0 && 
			   op1->sym_o == skyname &&
			   op2->type_o==T_REG) {
			/* move to a register */
			con_to_reg( op2->value_o, op1, n->subop );
		}
	}
}

/*
 * Build a list of sky board operations that
 * can use an accumulator between them.  Two operations
 * can use an accumulator if the result of the first operation
 * is the first argument to the second operation.
 */
struct	skylist	*
bld_skylist(np)
NODE	**np;
{
	struct	skylist	*hdr;
	struct	skylist	*sky;
	struct	skylist	**bpatch;
	NODE	*n;

	hdr = NULL;
	n = *np;
	if (is_sky_operation(n)) {
		hdr = get_sky(&n);
		if (hdr == NULL) {
			return(NULL);
		}
		sky = hdr;
		if (result_is_reg(sky)) {
			bpatch = &sky->next;
			while (n->op != OP_LABEL && 
			    n->op != OP_JUMP &&
			    n->op != OP_CALL) {
				if (result_killed(n, sky)) {
					break;
				}
				if (n == &first) {
					n = first.back;
					break;
				}
				if (is_sky_operation(n)) {
					sky = get_sky(&n);
					if (sky == NULL) {
						return(NULL);
					}
					*bpatch = sky;
					bpatch = &sky->next;
					if (!result_is_reg(sky)) {
						break;
					}
				} else {
					n = n->forw;
				}
			}
		}
	}
	*np = n;
	return(hdr);
}

/*
 * Disect a sky board operation.
 * Find the opcode, the two arguments and the result that is fetched
 * off of the board.
 * If it is not a sky board operation return NULL.
 */
struct	skylist	*
get_sky(np)
NODE	**np;
{
	NODE	*n;
	struct	skylist	*sky;
	int	i;

	/*
	 * Get the instruction to load an opcode into the sky board
	 */
	n = *np;
	sky = (struct skylist *) calloc(sizeof(*sky), 1);
	sky->skyload = n;
	n = n->forw;

	/*
	 * Get pointers to the instructions that push arguments
	 */
	for (i = 0; i < 2; i++) {
		if (!move_to_sky(n)) {
			return(NULL);
		}
		sky->arg[i] = n;
		n = n->forw;
	}

	/*
	 * Get the instruction to fetch the result from the board.
	 * It may be preceeded by a test loop.
	 */
	if (n->op == OP_LABEL) {
		for (i = 0; i < 3; i++) {
			n = n->forw;
		}
		sky->tstloop = 1;
	}
	if (!move_from_sky(n)) {
		return(NULL);
	}
	sky->result = n;

	n = n->forw;
	sky->next = NULL;
	*np = n;
	return(sky);
}

/*
 * Is this instruction a movl to the sky board?
 */
move_to_sky(n)
NODE	*n;
{
	struct	oper	*op2;
	int	regno;
	int	inexact;

	if (n->op == OP_MOVE && n->subop == SUBOP_L) {
		regno = reglookup(&skybase, SUBOP_L, &inexact);
		op2 = n->ref[1];
		if (op2->type_o == T_DEFER && regno == op2->value_o) {
			return(1);
		}
	}
	return(0);
}

/*
 * Is this instruction a movl from the sky board?
 */
move_from_sky(n)
NODE    *n;  
{
	struct  oper    *op1;
	int	regno;
	int	inexact;

	if (n->op == OP_MOVE && n->subop == SUBOP_L) {
		regno = reglookup(&skybase, SUBOP_L, &inexact);
		op1 = n->ref[0];
		if (op1->type_o == T_DEFER && regno == op1->value_o) {
			return(1);
		}
	}
	return(0);
}

/*
 * See if this instruction stores the result of the sky operation
 * into memory or otherwise kills the result.
 */
result_killed(n, sky)
NODE	*n;
struct	skylist	*sky;
{
	struct	oper	*result;
	struct	oper	*op1;
	struct	oper	*op2;
	int	i;

	/*
	 * Is this a movl from the register holding the sky result?
	 */
	result = sky->result->ref[1];
	if (n->op == OP_MOVE && n->subop == SUBOP_L) {
		op1 = n->ref[0];
		if (op1->type_o == T_REG && op1->value_o == result->value_o) {
			return(1);
		}
	}

	/*
	 * Is the destination of this instruction the register holding
	 * the sky result?
	 */
	op2 = n->ref[1];
	if (result->type_o == T_REG && 
	    op2->type_o == T_REG && 
	    op2->value_o == result->value_o) {
		return(1);
	}
	return(0);
}

/*
 * Is the result of this sky operation assigned to a register.
 */
result_is_reg(sky)
struct	skylist	*sky;
{
	struct	oper	*result;
	int	i;

	result = sky->result->ref[1];
	if (result->type_o == T_REG) {
		return(1);
	}
	return(0);
}
	
/*
 * Is this instuction a movw of an opcode to the sky board?
 */
is_sky_operation(n)
NODE	*n;
{
	struct	oper	*op1;
	struct	oper	*op2;
	int	regno;
	int	inexact;

	op1 = n->ref[0];
	op2 = n->ref[1];
	if (n->op != OP_MOVE ||
	    op1->type_o != T_IMMED ||
	    !isskyop(op1->value_o) ||
	    op2->type_o != T_DISPL) {
		return(0);
	}
	regno = reglookup(&skybase, SUBOP_L, &inexact);
	if (regno != op2->reg_o ||
	    op2->value_o != -4) {
		return(0);
	}
	return(1);
}

/*
 * Rewrite a list of sky operations doing optimizations along the way.
 * Look the for the results of one operation being used as an operand
 * of the next operations.  Accumulator operations can be used in
 * this case.
 */
rewrite_list(sky) 
struct	skylist	*sky;
{
	struct	skylist	*skyp;
	struct	skylist	*last_changed;
	NODE	*result;
	NODE	*arg;
	int	first;
	int	i;

	first = 1;
	last_changed = NULL;
	for (skyp = sky; skyp->next != NULL; skyp = skyp->next) {
		result = skyp->result;
		if (result == NULL) {
			continue;
		}
		for (i = 0; i < 2; i++) {
			arg = skyp->next->arg[i];
			if (sameops(result->ref[0], arg->ref[1]) &&
			    sameops(result->ref[1], arg->ref[0])) {
				meter.nskyreg++;
				if (first) {
					insert_reg_load(skyp);
					change_to_reg(skyp, 0);
				}
				change_to_reg(skyp->next, i);
				last_changed = skyp->next;
				first = 0;
				break;
			}
		}
		if (i == 2 && last_changed != NULL) {
			fetch_result(last_changed);
			first = 1;
			last_changed = NULL;
		}
	}
	if (!first && last_changed != NULL) {
		fetch_result(last_changed);
	}
}

/*
 * Change an operation to use the accumulator and also
 * to leave its result in the accumulator.
 * Remove the instructions that fetch the result from the board.
 * Add a tst loop after the operation to wait for the result
 * to solidify.
 */
change_to_reg(sky, argn)
struct	skylist	*sky;
int	argn;
{
	NODE	*arg;
	NODE	*n;
	struct	skyop	*skyop;
	struct	oper	*opcode;
	int 	op;
	int	i;

	opcode = sky->skyload->ref[0];
	op = opcode->value_o;
	sky->oldop = op;
	skyop = findop(op);
	if (skyop == NULL) {
		return;
	}
	opcode->value_o = skyop->arg_op[argn];
	arg = sky->arg[argn];
	arg = deletenode(arg);
	if (sky->tstloop) {
		n = sky->result;
		for (i = 0; i < 3; i++) {
			n = n->back;
		}
		delete_tst_loop(n);
		sky->tstloop = 0;
	}
	sky->oldresult = remove(sky->result);
	insert_tst_loop(sky->skyload->forw, sky->skyload->ref[1]);
}

/*
 * Insert a test loop waiting for the SKY idle bit to turn on.
 * Insert the code:
 *	1:	btst	#6,skybase(-2)
 *		beqs	1b
 */
insert_tst_loop(node, skybase)
NODE	*node;
struct	oper	*skybase;
{
	NODE	*label;
	NODE	*btst;
	NODE	*beqs;
	struct	sym_bkt	*sbp;
	char	ltoken[20];
	extern	char	*ll_format;
	extern	int	ll_val[];
	static struct oper btstbit  = { T_IMMED,   0, 0, 0, NULL, NULL, 0, IDLE, 0  };
	static struct oper labelop  = { T_NORMAL,  0, 0, 0, NULL, NULL, 0, 0, 0  };

	sprintf(ltoken, ll_format, '1', ++ll_val[1]);
	sbp = lookup(ltoken);
	sbp->attr_s |= S_LABEL | S_DEC | S_DEF;
	sbp->csect_s = C_TEXT;

	label = new();
	label->op = OP_LABEL;
	label->subop = SUBOP_Z;
	label->nref = 1;
	label->name = sbp;

	btst = new();
	btst->op = OP_OTHER;
	btst->subop = SUBOP_B;
	btst->instr = sopcode("btst");
	btst->nref = 2;
	btst->ref[0] = newoperand(&btstbit);
	btst->ref[1] = newoperand(skybase);
	btst->ref[1]->value_o = STATUS_WORD;

	labelop.sym_o = sbp;
	beqs = new();
	beqs->op = OP_JUMP;
	beqs->subop = JGE;
	beqs->instr = sopcode("beqs");
	beqs->nref = 0;
	beqs->ref[0] = newoperand(&labelop);
	beqs->luse = label;

	label->forw = btst;
	btst->forw = beqs;
	beqs->back = btst;
	btst->back = label;
	insert(label, node);
}

/*
 * Remove a node from the list 
 */
NODE	*
remove(p)
NODE	*p;
{
	p->back->forw = p->forw;
	p->forw->back = p->back;
	p->back = NULL;
	p->forw = NULL;
	return(p);
}

/*
 * Find an opcode in the opcode table
 */
struct	skyop	*
findop(op)
int	op;
{
	struct	skyop	*sp;

	for (sp = regops; sp->op != 0; sp++) {
		if (sp->op == op) {
			return(sp);
		}
	}
	return(NULL);
}

/*
 * Insert code to load the sky register with a value.
 * Load it with the first operand of the sky operation
 * passed in here.
 */
insert_reg_load(sky)
struct	skylist	*sky;
{
	NODE	*p;
	NODE	*load;
	NODE	*arg;

	load = sky->skyload;
	p = newinst(load, load->ref[0], load->ref[1]);
	p->ref[0]->value_o = SP_LOAD;
	insert(p, load->back);

	arg = sky->arg[0];
	p = newinst(arg, arg->ref[0], arg->ref[1]);
	insert(p, load->back);
}

/*
 * Create a new instruction.
 * Copy the old one passed in and then give it the new arguments.
 */
NODE	*
newinst(oldinst, arg1, arg2)
NODE	*oldinst;
struct	oper	*arg1;
struct	oper	*arg2;
{
	NODE	*p;

	p = new();
	*p = *oldinst;
	p->forw = NULL;
	p->back = NULL;
	p->ref[0] = newoperand(arg1);
	p->ref[1] = newoperand(arg2);
	return(p);
}

/*
 * Change the code to fetch the result from the sky board.
 * If possible change last operation to use R0 as an operand
 * and generate a fetch'able result.
 * Otherwise, insert additional code to fetch the result.
 */
fetch_result(sky)
struct	skylist	*sky;
{
	NODE	*n;
	NODE	*p;
	NODE	*load;
	NODE	*result;
	struct	skyop	*skyop;
	int	i;

	skyop = findop(sky->oldop);
	if (skyop->fetch != 0) {
		n = sky->skyload->forw;
		n = n->forw;
		if (n->op == OP_LABEL) {
			n = delete_tst_loop(n);
			sky->tstloop = 0;
		}
		sky->skyload->ref[0]->value_o = skyop->fetch;
		insert(sky->oldresult, n);
	} else {
		load = sky->skyload;
		n = load->forw;
		for (i = 0; i < 3; i++) {
			n = n->forw;
		}		
		p = newinst(load, load->ref[0], load->ref[1]);
		p->ref[0]->value_o = SP_STORE;
		insert(p, n);
		n = n->forw;

		result = sky->oldresult;
		p = newinst(result, result->ref[0], result->ref[1]);
		insert(p, n);
	}
}

/*
 * Delete a test loop.
 * The actual test and branch may differ here.
 */
NODE	*
delete_tst_loop(n)
NODE	*n;
{
	if (n->op != OP_LABEL) {
		fprintf(stderr, "c2: internal error deleting test loop\n");
		exit(1);
	}
	n = deletenode(n);	/* Label */
	n = n->forw;
	n = deletenode(n);	/* test */
	n = n->forw;
	n = deletenode(n);	/* branch */
	return(n);
}

/*
 * Free a list of sky structures
 */
free_list(sky)
struct	skylist	*sky;
{
	struct	skylist	*next;
	struct	skylist	*skyp;

	for (skyp = sky; skyp != NULL; skyp = next) {
		next = skyp->next;
		free(skyp);
	}
}

/*
 * Look for the sequence:
 *
 *	movl	skybaseR@,X
 *	movl	skybaseR@,Y
 *	movl	X,mem1
 *	movl	Y,mem2
 *
 * Change it to
 *
 *	movl	skybaseR@,mem1
 *	movl	skybaseR@,mem2
 *
 * This sequence occurs when a double precision value is fetched from
 * the SKY board and is stored into memory.
 */
skymove(n)
NODE	*n;
{
	NODE	*n1;
	NODE	*n2;
	NODE	*n3;
	int	reg;
	int	reg1;

	n1 = n->forw;
	n2 = n1->forw;
	n3 = n2->forw;
	if (skytoreg(n) && skytoreg(n1)) {
		reg = n->ref[1]->value_o;
		reg1 = n1->ref[1]->value_o;
		if (regtomem(n2, reg) && regtomem(n3, reg1)) {
			if (!inmask(reg, n3->forw->rlive ) &&
			    !inmask(reg1, n3->forw->rlive ) ) {
				n->ref[1] = newoperand(n2->ref[1]);
				n1->ref[1] = newoperand(n3->ref[1]);
				deletenode(n2);
				deletenode(n3);
				meter.ndpsky++;
				return(1);
			}
		}
	}
	return(0);
}

/*
 * Look for the instruction
 *
 *	movl	skybaseR@,X
 *
 * which fetches a value from the sky board and puts it in a register.
 * skybaseR is a register containing __skybase and X is a register.
 */
static
skytoreg(n)
NODE	*n;
{
	struct	oper	*op1;
	struct	oper	*op2;
	struct	oper	*con;
	struct	sym_bkt	*sym;
	int	reg;
	extern  struct oper  *get_regcon();

	if (n->op != OP_MOVE || n->subop != SUBOP_L) {
		return(0);
	}
	op1 = n->ref[0];
	op2 = n->ref[1];
	if (op1->type_o != T_DEFER) {
		return(0);
	}
	reg = op1->value_o;
	sym = get_regcon( reg )->sym_o;
	if (skyname == NULL || sym != skyname) {
		return(0);
	}
	if (op2->type_o != T_REG) {
		return(0);
	}
	return(1);
}

/*
 * Look for the instruction
 *
 * 	movl	X,mem
 *
 * where X is a register.
 */
static
regtomem(n, reg)
NODE	*n;
int	reg;
{
	struct	oper	*op1;
	struct	oper	*op2;

	if (n->op != OP_MOVE || n->subop != SUBOP_L) {
		return(0);
	}
	op1 = n->ref[0];
	op2 = n->ref[1];
	if (op1->value_o == reg && op2->type_o != T_REG) {
		return(1);
	}
	return(0);
}
