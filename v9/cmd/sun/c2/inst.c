#ifndef lint
static	char sccsid[] = "@(#)inst.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "as.h"
#include "c2.h"

#if AS
extern int no_op(), stop(), one_op(), ctrl_op(), move_op(), two_op(), jbrnch(),
	cbrnch(), branch(), brnchs(), regmem(), shift_op(), reg_op(), postinc(),
	bit_op(), memreg(), regbrnch(), exg_op(), addr_op(), link_op(),
	movem_op(), movep_op(), moveq(), trap(), rts(), movec_op(), moves_op();
#endif

#define I( a, b, c, d, e, f, g, h ) {a, b, c, d, e, f, g, h  },
#define P( a, b, c, d, e ) {a, b, e, d, c },

#define A_ONE( a ) { a }
#define A_TWO( a, b ) { a, b }
#define A_TH3( a, b, c ) { a, b, c }
#define A_4R( a, b, c, d ) { a, b, c, d }
#define A_5V( a, b, c, d, e ) { a, b, c, d, e }
#define A_6X( a, b, c, d, e, f ) { a, b, c, d, e, f }
#define A_8T( a, b, c, d, e, f, g, h ) { a, b, c, d, e, f, g, h }

#define T_X     0
#define T_ONE(a) (a)
#define T_TWO(a,b) (((b)<<TOUCHWIDTH)|(a))
#define T_THREE(a,b,c) (((((c)<<TOUCHWIDTH)|(b))<<TOUCHWIDTH)|(a))

/* List of 68000 op codes */
struct ins_bkt op_codes[] = {

#   include "characteristics"

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
  { "fp0", FP0REG+0 },
  { "fp1", FP0REG+1 },
  { "fp2", FP0REG+2 },
  { "fp3", FP0REG+3 },
  { "fp4", FP0REG+4 },
  { "fp5", FP0REG+5 },
  { "fp6", FP0REG+6 },
  { "fp7", FP0REG+7 },
  { "cc",  CCREG    },
  /* fpcc */
  { "pc",  PCREG    }, 
  { "sr",  SRREG    },
  { "usp", USPREG+0 },
  { "sfc", USPREG+1 },
  { "dfc", USPREG+2 },
  { "vbr", USPREG+3 },
  { "cacr",USPREG+4 }, 
  { "caar",USPREG+5 },
  { "msp", USPREG+6 },
  { "isp", USPREG+7 },
  { "fpc", FPCREG   },
  { "fps", FPSREG   },
  { "fpi", FPIREG   },
  0, 0
};

unsigned reg_access[] = {
/*	d0       d1       d2       d3       d4       d5       d6       d7   */
	AM_DREG, AM_DREG, AM_DREG, AM_DREG, AM_DREG, AM_DREG, AM_DREG, AM_DREG,
/*	a0       a1       a2       a3       a4       a5       a6       a7   */
	AM_AREG, AM_AREG, AM_AREG, AM_AREG, AM_AREG, AM_AREG, AM_AREG, AM_AREG,
/*	fp0      fp1      fp2      fp3      fp4      fp5      fp6      fp7   */
	AM_FREG, AM_FREG, AM_FREG, AM_FREG, AM_FREG, AM_FREG, AM_FREG, AM_FREG,
/*	fpcc	*/
	0,
/*      cc       pc       sr       usp                                      */
	AM_CCREG,AM_PCREG,AM_CCREG,AM_USPREG|AM_CTRLREG,
/*      src         dfc         vbr         cacr                            */
	AM_CTRLREG, AM_CTRLREG, AM_CTRLREG, AM_CTRLREG, 
/*      caar        msp         isp                                         */
	AM_CTRLREG, AM_CTRLREG, AM_CTRLREG, 
/*      fpc	     fps          fpi                                       */
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
	"Faddd",	LR+LW,  LR+LW,  LR+LW,  LW,	/* fvaddi */
	"Fsubd",	LR+LW,  LR+LW,  LR+LW,  LW,	/* fvsubi */
	"Fmuld",	LR+LW,  LR+LW,  LR+LW,  LW,	/* fvmuli */
	"Fdivd",	LR+LW,  LR+LW,  LR+LW,  LW,	/* fvdivi */
	"Fcmpd",	LR+LW,  LR+LW,  LR+LW,  LW,	/* fvcmpi */
	"Fadds",	LR+LW,  LR+LW,  LW,	LW,	/* fvaddis */
	"Fsubs",	LR+LW,  LR+LW,  LW,	LW,	/* fvsubis */
	"Fmuls",	LR+LW,  LR+LW,  LW,	LW,	/* fvmulis */
	"Fdivs",	LR+LW,  LR+LW,  LW,	LW,	/* fvdivis */
	"Fcmps",	LR+LW,  LR+LW,  LW,	LW,	/* fvcmpis */
	"Ffltd",	LR+LW,  LW,	LW,	LW,	/* fvflti */
	"Fflts",	LR+LW,  LW,	LW,	LW,	/* fvfltis */
	"Fintd",	LR+LW,  LR+LW,  LW,	LW,	/* fvfixi */
	"Fints",	LR+LW,  LW,	LW,	LW,	/* fvfixis */
	"Fstod",	LR+LW,  LW,	LW,	LW,	/* fvdoublei */
	"Fdtos",	LR+LW,  LR+LW,  LW,	LW,	/* fvsinglei */
	/* fortrash */
	"Fcosd",	LR+LW,  LR+LW,  LW,	LW,	/* fvcosi */
	"Fsind",	LR+LW,  LR+LW,  LW,	LW,	/* fvsini */
	"Ftand",	LR+LW,  LR+LW,  LW,	LW,	/* fvtani */
	"Facosd",	LR+LW,  LR+LW,  LW,	LW,	/* fvacosi */
	"Fasind",	LR+LW,  LR+LW,  LW,	LW,	/* fvasini */
	"Fatand",	LR+LW,  LR+LW,  LW,	LW,	/* fvatani */
	"Fcoshd",	LR+LW,  LR+LW,  LW,	LW,	/* fvcoshi */
	"Fsinhd",	LR+LW,  LR+LW,  LW,	LW,	/* fvsinhi */
	"Ftanhd",	LR+LW,  LR+LW,  LW,	LW,	/* fvtanhi */
	"Fexpd",	LR+LW,  LR+LW,  LW,	LW,	/* fvexpi */
	"Fpow10d",	LR+LW,  LR+LW,  LW,	LW,	/* fv10toxi */
	"Fpow2d",	LR+LW,  LR+LW,  LW,	LW,	/* fv2toxi */
	"Flogd",	LR+LW,  LR+LW,  LW,	LW,	/* fvlogi */
	"Flog10d",	LR+LW,  LR+LW,  LW,	LW,	/* fvlog10i */
	"Flog2d",	LR+LW,  LR+LW,  LW,	LW,	/* fvlog2i */
	"Fsqrd",	LR+LW,  LR+LW,  LW,	LW,	/* fvsqri */
	"Fsqrtd",	LR+LW,  LR+LW,  LW,	LW,	/* fvsqrti */
	"Faintd",	LR+LW,  LR+LW,  LW,	LW,	/* fvainti */
	"Fanintd",	LR+LW,  LR+LW,  LW,	LW,	/* fvaninti */
	"Fnintd",	LR+LW,  LR+LW,  LW,	LW,	/* fvninti */
	"Fcoss",	LR+LW,  LW,	LW,	LW,	/* fvcosis */
	"Fsins",	LR+LW,  LW,	LW,	LW,	/* fvsinis */
	"Ftans",	LR+LW,  LW,	LW,	LW,	/* fvtanis */
	"Facoss",	LR+LW,  LW,	LW,	LW,	/* fvacosis */
	"Fasins",	LR+LW,  LW,	LW,	LW,	/* fvasinis */
	"Fatans",	LR+LW,  LW,	LW,	LW,	/* fvatanis */
	"Fcoshs",	LR+LW,  LW,	LW,	LW,	/* fvcoshis */
	"Fsinhs",	LR+LW,  LW,	LW,	LW,	/* fvsinhis */
	"Ftanhs",	LR+LW,  LW,	LW,	LW,	/* fvtanhis */
	"Fexps",	LR+LW,  LW,	LW,	LW,	/* fvexpis */
	"Fpow10s",	LR+LW,  LW,	LW,	LW,	/* fv10toxis */
	"Fpow2s",	LR+LW,  LW,	LW,	LW,	/* fv2toxis */
	"Flogs",	LR+LW,  LW,	LW,	LW,	/* fvlogis */
	"Flog10s",	LR+LW,  LW,	LW,	LW,	/* fvlog10is */
	"Flog2s",	LR+LW,  LW,	LW,	LW,	/* fvlog2is */
	"Fsqrs",	LR+LW,  LW,	LW,	LW,	/* fvsqris */
	"Fsqrts",	LR+LW,  LW,	LW,	LW,	/* fvsqrtis */
	"Faints",	LR+LW,  LW,	LW,	LW,	/* fvaintis */
	"Fanints",	LR+LW,  LW,	LW,	LW,	/* fvanintis */
	"Fnints",	LR+LW,  LW,	LW,	LW,	/* fvnintis */
	0
};

static void
add_builtin(d)
    register struct def_builtin * d;
{
    register i;
    register struct sym_bkt *s;

    s = lookup(d->bname);
    s->csect_s = C_UNDEF;
    s->attr_s = S_DEC | S_CRT;
    for (i=0; i < sizeof d->ruse / sizeof d->ruse[0] ; i++){
	s->builtin_s[i] = d->ruse[i];
    }
}

void
init_builtins()
{
    register struct def_builtin * d;

    for (d = def_builtins; d->bname != NULL; d++){
	if (d->bname[0] == 'F') {
	    /* must make a copy for each flavor of floating point. BLEAGH! */
	    add_builtin(d);
	    d->bname[0] = 'V';	/* vectored */
	    add_builtin(d);
	    d->bname[0] = 'S';	/* sky */
	    add_builtin(d);
	    d->bname[0] = 'M';	/* motorola */
	    add_builtin(d);
	    d->bname[0] = 'W';	/* Weitek */
	    add_builtin(d);
	} else {
	    add_builtin(d);
	}
    }
}
#endif
