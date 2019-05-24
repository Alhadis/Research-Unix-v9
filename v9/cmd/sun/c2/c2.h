/*	@(#)c2.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


/*
 * Header for object code improver
 */



/*
 * a regmask is how we talk about registers being used or set by the
 * current instruction, and also how we keep track of "dead" registers.
 * Each d-register is represented by three bits: for byte, word, and
 * longword access (pipe dreaming here). Each a-register is represented
 * by only one bit, as they are always used and set as longwords. Each
 * f-register is also represented by one bit, as are each of the condition 
 * code registers.
 * It looks like this:
 * +--+--+--+--+--+--+--+--+-----+-----+-----+-----+-----+-----+-----+-----+
 * |a7|a6|a5|a4|a3|a2|a1|a0| d7  | d6  | d5  | d4  | d3  | d2  | d1  | d0  |
 * +--+--+--+--+--+--+--+--+-----+-----+-----+-----+-----+-----+-----+-----+
 *                                           +--+--+--+--+--+--+--+--+--+--+
 *                                           |fc|cc|f7|f6|f5|f4|f3|f2|f1|f0|
 *                                           +--+--+--+--+--+--+--+--+--+--+
 * where each of the d-register fields is subdivided as:
 *			+-----------+
 *			| L | W | B |
 *			+-----------+
 *
 * We can only deal with reading OR writing at one time here, even though the
 * touch field in the instruction description talks about both at once.
 */

typedef struct rm_type{ 
	unsigned da; 
	unsigned short f; 
}  regmask;

regmask exitmask, regmask0, regmask_nontemp, regmask_all;
regmask makemask(), movemmask(), compute_normal(), addmask();
regmask submask(), andmask(), notmask();

void init_csects(), freeoperand(), rectify(), freetree();
void dumpprogram(), xref(), quicken();
void newreference(), unreference(), cannibalize();
struct node * addnode();

typedef
struct node {
	opcode_t    op:8;
	subop_t	    subop:8;	/* to quickly recognize certain nodes */
	struct	node	*forw;
	struct	node	*back;
	union {
	    struct	ins_bkt	*i; /* for Instruct and PseudoOp nodes */
	    struct	sym_bkt	*n; /* if we're a label */
	} p;
# define instr p.i
# define name  p.n
	int	nref; /* number operands or number references here */
	struct	oper	*ref[OPERANDS_MAX]; /* our operands */
	struct  node    *luse; /* operand, use chain for jumps*/
	struct  node	*lnext;
	regmask ruse,
		rset,
		rlive;
	short   lineno;
} NODE;

# define RMASK 3
# define WMASK 014
# define TOUCHWIDTH 5
# define TOUCHMASK  037
# define BR  1
# define WR  2
# define LR  3
# define BW  4
# define WW  8
# define LW  12
# define SPEC(n) (16+n)
# define MAKERMASK( regno, rtouch ) RegMasks[regno][(rtouch)&RMASK]
# define MAKEWMASK( regno, wtouch ) RegMasks[regno][((wtouch)&WMASK)>>2]
extern regmask RegMasks[PCREG+1][LR+1];

struct	node	first;
struct	node	*freenodes;
struct	node	*new();
struct  node    *deletenode();
char	*curlp;
struct meter{
    int	ndrop;
    int	nbrbr;
    int	nsaddr;
    int	redunm;
    int	iaftbr;
    int	nredext;
    int	nredadd;
    int	nredsub;
    int	nredmul;
    int	ndbra;
    int ndbrarev;
    int	nredor;
    int	nredshf;
    int	nmtoc;
    int	nmtos;
    int	nctot;
    int	nmmtmo;
    int	nrmtfr;
    int	namwl;
    int	njp1;
    int	nrlab;
    int	nxjump;
    int	ncmot;
    int	nrevbr;
    int	loopiv;
    int	nredunj;
    int	nskip;
    int	ncomj;
    int	nttomo;
    int	nrtst;
    int	nwmov;
    int	nchange;
    int nwop;
    int nskyreg;
    int ndpsky;
    int nplusminus;
    int nusecr;
}			meter;

int	debug;
extern int	fortranprog; /* set if a5, a4 point at "regular" memory */

extern int verbose; /* flag set by -v option */

