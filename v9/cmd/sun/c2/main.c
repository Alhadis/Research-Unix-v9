#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "as.h"
#include "c2.h"
#include "cctypes.h"

char title[STR_MAX];
struct sym_bkt *dot_bkt ;	/* Ptr to location counter's symbol bucket */
struct ins_ptr *ins_hash_tab[HASH_MAX];
struct ins_ptr *inst;
char *source_name = NULL;
char *out_name = NULL;
char *asm_name;	/* set from argv[0] below: used in error.c */
int  xrefflag;
int  Xperimental;
int  ext_instruction_set;
int  debflag, statflag, verboseflag, globflag;
char	sebuf[BUFSIZ];
struct ins_bkt *moveq, *subql, *addql;
struct ins_bkt *tstw, *tstl;

regmask exitmask = 077; /* d0/d1 long */

init(argc,argv)
	char *argv[];
{

	asm_name = *(argv++);
	while (--argc) {
	  if (argv[0][0] == '-') switch (argv[0][1]) {
	    case 'D':	debflag++;
			break;
	    case 'x':   xrefflag++;
			break;
	    case 'n':   statflag++;
			break;
	    case 'v':   verboseflag++;
			break;
	    case 'g':	globflag++;
			break;
	    case 'X':	Xperimental++;
			break;
	    case '1':	if (argv[0][2]== '0' && argv[0][3] == '\0') {
			    ext_instruction_set = 0;
			    break;
			}
			goto unknown;
	    case '2':	if (argv[0][2]== '0' && argv[0][3] == '\0') {
			    ext_instruction_set = 1;
			    break;
			}
			goto unknown;

	    unknown:
	    default:	fprintf(stderr,"%s: Unknown option '%c' ignored.\n",
				asm_name, argv[0][1]);
	  } else if (source_name != NULL) {
		if (out_name != NULL)
		    fprintf(stderr,"%s: Too many file names given\n", asm_name);
		else
		out_name = argv[0];
		if (freopen(out_name,"w",stdout) == NULL) {
		    fprintf(stderr,"%s: Can't open source file: %s\n",
			asm_name, out_name);
		    exit(1);
		 }
	  } else {
		source_name = argv[0];
		if (freopen(source_name,"r",stdin) == NULL) {
		    fprintf(stderr,"%s: Can't open source file: %s\n",
			asm_name, source_name);
		    exit(1);
		 }
	  }
	  argv++;
	}


	/* Initialize symbols */
	sym_init();
	dot_bkt = lookup(".");		/* make bucket for location counter */
	dot_bkt->csect_s = cur_csect_name;
	dot_bkt->attr_s = S_DEC | S_DEF | S_LABEL; 	/* "S_LABEL" so it cant be redefined as a label */
	init_regs();			/* define register names */
	init_builtins();		/* define magic subroutines */
	init_csects();
	d_ins();			/* set up opcode hash table */
	perm();
	moveq = sopcode("moveq");
	subql = sopcode("subql");
	addql = sopcode("addql");
	tstw = sopcode("tstw");
	tstl = sopcode("tstl");
}

void
freeprogram(){
    register NODE *p;
    register struct oper **op;
    register i,n;

    p = first.forw;
    while( p != &first){
	switch (p->op){
	default:
	    if (n=p->nref){
		op = &p->ref[0];
		for (i=0; i<n; i++)
		    freeoperand( *(op++) );
	    }
	    break;
	case OP_LABEL:
	    p->name->attr_s &= ~S_DEF;
	    break;
	}
	p=p->forw;
	(p->back )->forw = freenodes;
	freenodes = (p->back );
    }
    first.forw = first.back = &first;
}

prstats(){
		fflush(stdout);
		fflush(stderr);
		setbuf(stderr, sebuf);
		fprintf(stderr,"%d jumps to jumps\n", meter.nbrbr);
		fprintf(stderr,"%d inst. after jumps\n", meter.iaftbr);
		fprintf(stderr,"%d inst. after rts\n", meter.ndrop);
		fprintf(stderr,"%d jumps to .+2\n", meter.njp1);
		fprintf(stderr,"%d redundant labels\n", meter.nrlab);
		fprintf(stderr,"%d jump-started loops\n", meter.nxjump);
		fprintf(stderr,"%d inverted loops\n", meter.loopiv);
		fprintf(stderr,"%d code motions\n", meter.ncmot);
		fprintf(stderr,"%d branches reversed\n", meter.nrevbr);
		fprintf(stderr,"%d redundant moves\n", meter.redunm);
		fprintf(stderr,"%d moves weakened\n", meter.nwmov);
		fprintf(stderr,"%d ops weakened\n", meter.nwop);
		fprintf(stderr,"%d simplified addresses\n", meter.nsaddr);
		fprintf(stderr,"%d redundent sign extensions\n", meter.nredext);
		fprintf(stderr,"%d dbra's inserted\n", meter.ndbra);
		fprintf(stderr,"%d jump/dbra's flipped\n", meter.ndbrarev);
		fprintf(stderr,"%d redundant shifts\n", meter.nredshf);
		fprintf(stderr,"%d redundant jumps\n", meter.nredunj);
		fprintf(stderr,"%d common seqs before jmp's\n", meter.ncomj);
		fprintf(stderr,"%d skips over jumps\n", meter.nskip);
		fprintf(stderr,"%d redundant tst's\n", meter.nrtst);
		fprintf(stderr,"%d addl's merged with link's\n", meter.namwl);
		fprintf(stderr,"%d movem's removed\n", meter.nrmtfr);
		fprintf(stderr,"%d movem's changed to move's\n", meter.nmmtmo);
		fprintf(stderr,"%d mov's changed to clr's\n", meter.nmtoc);
		fprintf(stderr,"%d mov's changed to sub's\n", meter.nmtos);
		fprintf(stderr,"%d cmp's changed to tst's\n", meter.nctot);
		fprintf(stderr,"%d cmp's changed to mov's\n", meter.nttomo);
		fprintf(stderr,"%d indirections combined with incr/decr\n", 
			meter.nplusminus);
		fprintf(stderr,"%d single precision SKY operations changed to use registers\n", meter.nskyreg);
		fprintf(stderr,"%d double precision SKY moves simplified\n", 
			meter.ndpsky);
		fprintf(stderr,"%d uses of fmovecr\n", meter.nusecr);
		fflush(stderr);
}

main(argc,argv)
	char *argv[];
{
	int scanflag;
	int iterate;
	init(argc,argv);                       /* Initialization */
	do{
	    scanflag = scan();
	    rectify();
	    onceonly();
	    do {
		if (debflag){
		    printf("________________________________________\n");
		    dumpprogram(verboseflag);
		}
		do {
		    iterate  = relabel();
		    iterate += tangle();
		    iterate += zipper();
		    iterate += tmerge();
		    iterate += invloop();
		    if (debflag>1){
			printf("========================================\n");
			dumpprogram(verboseflag);
			printf("========================================\n");
		    }
		} while (iterate);
		livereg();
		iterate += stackops();
		iterate += coalesce();
		iterate += shorten();
		iterate += content();
	    } while (iterate);
	    quicken();
	    dumpprogram(verboseflag);
	    if (xrefflag) xref();
	    freeprogram();
	} while( scanflag );
	if (statflag) {
	    prstats();
	}
	exit(errors? -1: 0);
}

char *
c2pseudocomment( p )
    char *p;
{
    extern char *term();
    static char procfooey[6] = {'P','R','O','C','#',' ' };
    if (strncmp(p, procfooey, sizeof procfooey) == 0){
	p += sizeof procfooey;
	(void)term( p, &operands[0] );
	if (operands[0].type_o!=T_NORMAL || operands[0].sym_o != NULL)
	    prog_error(E_OPERAND);
	switch (operands[0].value_o){
	case UNDEF: /* void function returns no value */
	    exitmask = regmask0; 
	    break;
	case DOUBLE: /* returns value in d0/d1 */
	    exitmask = addmask(MAKERMASK( D0REG, LR ) , MAKERMASK( D0REG+1, LR));
	    break;
	default:     /* returns value in d0 */
	    exitmask = MAKERMASK( D0REG, LR );
	    break;
	}
	return globflag?p:NULL;
    }
    return p;
}
