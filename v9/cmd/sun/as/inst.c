#ifndef lint
static	char sccsid[] = "@(#)inst.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "as.h"

#if AS
extern int no_op(), stop(), one_op(), ctrl_op(), two_op(), 
	jbrnch_op(), cbrnch_op(), branch_op(), brnchs_op(),
	regmem_op(), shift_op(), reg_op(), postinc(), quick_op(),
	bit_op(), memreg_op(), exg_op(), addr_op(), link_op(),
	move_op(), movem_op(), movep_op(), moveq_op(), movec_op(), moves_op(),
	trap(), rts_op(), lea_op(); 
extern int chk2_op(), div_op(), callm_op(), bitf_op(), bitfr_op(),
	cas1_op(), cas2_op(), brnchl_op(),pack_op();
extern int ascii_op(), byteword(), comm_op(), ctrl_op(), csect_op(), 
           proc_op(), globl_op();
extern int even_op(), skip_op(), stab_op(), float_op(), cpid_op() ;
extern int cp_general(), cp_move(), cp_regpair(), cp_movecr(), cp_movem() ;
extern int cp_oneop(), cp_conditional(), cp_branch(), cp_oneword() ;

#define I( a, b, c, d, e, f, g, h, i ) { a, b, c, d, e, 1, f, g, h, i },
#define P( a, b, c, d, e, f, g, h ) { a, b, c, d, e, f, 0, {g}, h },

#define A_ONE( a ) { a }
#define A_TWO( a, b ) { a, b }
#define A_TH3( a, b, c ) { a, b, c }
#define A_4R( a, b, c, d ) { a, b, c, d }
#define A_5V( a, b, c, d, e ) { a, b, c, d, e }
#define A_6X( a, b, c, d, e, f ) { a, b, c, d, e, f }
#define A_SEVEN( a, b, c, d, e, f, g ) { a, b, c, d, e, f, g }
#define A_EIGHT( a, b, c, d, e, f, g, h ) { a, b, c, d, e, f, g, h }

#define T_X	0
#define T_ONE(a) (((a)<<TOUCHWIDTH)<<TOUCHWIDTH)
#define T_TWO(a,b) ((((a)<<TOUCHWIDTH)|(b))<<TOUCHWIDTH)
#define T_THREE(a,b,c) (((((a)<<TOUCHWIDTH)|(b))<<TOUCHWIDTH)|(c))

#define ASCII	ascii_op
#define BIT	bit_op
#define BITF	bitf_op
#define BITFR	bitfr_op
#define BRANCH	branch_op
#define BRNCHS	brnchs_op
#define BRNCHL	brnchl_op
#define BYTE	byteword
#define CALLM	callm_op
#define CAS1	cas1_op
#define CAS2	cas2_op
#define CBRNCH	cbrnch_op
#define CHK2	chk2_op
#define COMM	comm_op
#define CPID	cpid_op
#define CTRL	ctrl_op
#define DATA	csect_op
#define DIV_OP	div_op
#define DBRA	branch_op
#define EVEN	even_op
#define EXG	exg_op
#define EXIT	rts_op
#define FLOAT	float_op
#define PROC	proc_op
#define GLOBL	globl_op
#define JBRNCH	jbrnch_op
#define LEA	lea_op
#define LINK	link_op
#define MEMREG	memreg_op
#define MOVE	move_op
#define MOVEC	movec_op
#define MOVEM	movem_op
#define MOVEP	movep_op
#define MOVEQ	moveq_op
#define MOVS	moves_op
#define NO	no_op
#define ONE_OP	one_op
#define PACK	pack_op
#define POSTINC	postinc
#define QUICK	quick_op
#define REG_OP	reg_op
#define REGMEM	regmem_op
#define RTS	rts_op
#define SHIFT_OP	shift_op
#define SKIP	skip_op
#define STAB	stab_op
#define STOP	stop
#define TRAP	trap
#define TWO_OP	two_op
#endif

/* List of 68000 op codes */
struct ins_bkt op_codes[] = {

#if C2
#   include "characteristics"
#else 
#if AS
#   include "as_charac"
#else
#include "NO FLAVOR"
#endif
#endif

    0 };

d_ins()
{
	register struct ins_bkt *insp;
	register struct ins_ptr *ipp;
	register int save;
	char *calloc();

	inst = (struct ins_ptr *) calloc( sizeof op_codes / sizeof op_codes[0], sizeof *inst);
	if (inst == NULL) sys_error("could not allocate instruction symbol space ");
	for ( ipp = &inst[0], insp = &op_codes[0]; insp->text_i; ipp++, insp++){
		ipp->this_p = insp;
		ipp->name_p = insp->text_i;
		ipp->next_p = ins_hash_tab[save = hash(ipp->name_p)];
		ins_hash_tab[save] = ipp;
	}
}

struct regdef{ char * name_r ; short regno_r; }  defregs[] = {
  { "d0", D0REG+0 },
  { "d1", D0REG+1 },
  { "d2", D0REG+2 },
  { "d3", D0REG+3 }, 
  { "d4", D0REG+4 },
  { "d5", D0REG+5 },
  { "d6", D0REG+6 },
  { "d7", D0REG+7 },
  { "a0", A0REG+0 },
  { "a1", A0REG+1 },
  { "a2", A0REG+2 },
  { "a3", A0REG+3 }, 
  { "a4", A0REG+4 },
  { "a5", A0REG+5 },
  { "a6", A6REG   },
  { "a7", A7REG   },
  { "sp", A7REG   },
  { "pc", PCREG   }, 
  { "cc", CCREG   },
  { "sr", SRREG   },
  { "usp", USPREG+0 },
  { "sfc", USPREG+1 },
  { "dfc", USPREG+2 },
  { "vbr", USPREG+3 },
  { "cacr",USPREG+4 }, 
  { "caar",USPREG+5 },
  { "msp", USPREG+6 },
  { "isp", USPREG+7 },
  { "fp0", FP0REG+0 },
  { "fp1", FP0REG+1 },
  { "fp2", FP0REG+2 },
  { "fp3", FP0REG+3 },
  { "fp4", FP0REG+4 },
  { "fp5", FP0REG+5 },
  { "fp6", FP0REG+6 },
  { "fp7", FP0REG+7 },
  { "fpc", FPCREG },
  { "fps", FPSREG },
  { "fpi", FPIREG },
  0, 0
};

unsigned reg_access[] = {
/*	d0       d1       d2       d3       d4       d5       d6       d7   */
	AM_DREG, AM_DREG, AM_DREG, AM_DREG, AM_DREG, AM_DREG, AM_DREG, AM_DREG,
/*	a0       a1       a2       a3       a4       a5       a6       a7   */
	AM_AREG, AM_AREG, AM_AREG, AM_AREG, AM_AREG, AM_AREG, AM_AREG, AM_AREG,
/*      pc       cc       sr       usp                                      */
	AM_PCREG,AM_CCREG,AM_CCREG,AM_USPREG|AM_CTRLREG,
/*      src         dfc         vbr         cacr                            */
	AM_CTRLREG, AM_CTRLREG, AM_CTRLREG, AM_CTRLREG, 
/*      caar        msp         isp                                         */
	AM_CTRLREG, AM_CTRLREG, AM_CTRLREG, 
/*	fp0      fp1      fp2      fp3      fp4      fp5      fp6      fp7   */
	AM_FREG, AM_FREG, AM_FREG, AM_FREG, AM_FREG, AM_FREG, AM_FREG, AM_FREG,
/*      fpc         fps         fpi                                         */
	AM_FCTRLREG, AM_FCTRLREG, AM_FCTRLREG, 
};

init_regs()
  {	register struct sym_bkt *sbp;
	register struct regdef *p = defregs;
	register i;

	i = 0;
	while (p->name_r) {
	  sbp = lookup(p->name_r);	/* Make a sym_bkt for it */
	  sbp->value_s = p->regno_r;	/* Load the sym_bkt */
	  sbp->csect_s = C_UNDEF;
	  sbp->attr_s = S_DEC | S_DEF | S_REG;
	  p++;
	  i++;
	}
}

#if C2
struct def_builtin { char *bname; short ruse[4]; } def_builtins[] = {
	"lmult",	LR+LW,	LR+LW,	0, 	0,
	"ulmult",	LR+LW,	LR+LW,	0,	0,
	"ldivt",	LR+LW,	LR+LW,	0,	0,
	"lmodt",	LR+LW,	LR+LW,	0,	0,
	"uldivt",	LR+LW,	LR+LW,	0,	0,
	"ulmodt",	LR+LW,	LR+LW,	0,	0,
	"mcount",	0,	0,	LR,	0,
	/* floating-point, too */
	"fvaddi",	LR+LW,	LR+LW,	LR+LW,	LW,
	"fvsubi",	LR+LW,	LR+LW,	LR+LW,	LW,
	"fvmuli",	LR+LW,	LR+LW,	LR+LW,	LW,
	"fvdivi",	LR+LW,	LR+LW,	LR+LW,	LW,
	"fvcmpi",	LR+LW,	LR+LW,	LR+LW,	LW,
	"fvaddis",	LR+LW,	LR+LW,	LW,	LW,
	"fvsubis",	LR+LW,	LR+LW,	LW,	LW,
	"fvmulis",	LR+LW,	LR+LW,	LW,	LW,
	"fvdivis",	LR+LW,	LR+LW,	LW,	LW,
	"fvcmpis",	LR+LW,	LR+LW,	LW,	LW,
	"fvflti",	LR+LW,	LW,	LW,	LW,
	"fvfltis",	LR+LW,	LW,	LW,	LW,
	"fvfixi",	LR+LW,	LR+LW,	LW,	LW,
	"fvfixis",	LR+LW,	LW,	LW,	LW,
	"fvdoublei",	LR+LW,	LW,	LW,	LW,
	"fvsinglei",	LR+LW,	LR+LW,	LW,	LW,
	0
};

init_builtins(){
    register struct def_builtin * d; register i; register struct sym_bkt *s;
    d = def_builtins;
    while (d->bname){
	s = lookup(d->bname);
	s->csect_s = C_UNDEF;
	s->attr_s = S_DEC | S_CRT;
	for (i=0; i < sizeof d->ruse / sizeof d->ruse[0] ; i++){
	    s->builtin_s[i] = d->ruse[i];
	}
	d++;
    }
}
#endif
