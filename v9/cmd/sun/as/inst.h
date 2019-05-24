/*	@(#)inst.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */



/* 
 * When this table is changed, we must also change:
 *     the table in instruction.c that is indexed by it.
 */
typedef enum{
	OP_FIRST = 0, OP_COMMENT, OP_LABEL,
	OP_LONG, OP_WORD, OP_BYTE, OP_TEXT, 
	OP_DATA, OP_DATA1, OP_DATA2, OP_BSS, 
	OP_PROC, OP_GLOBL, OP_COMM, OP_EVEN, OP_ALIGN, 
	OP_ASCIZ, OP_ASCII, OP_FLOAT, OP_DOUBLE,
	OP_STABS, OP_STABD, OP_STABN, OP_SKIP, OP_LCOMM, OP_CPID, 
	OP_CSWITCH, OP_FSWITCH, OP_BRANCH, OP_MOVE, OP_MOVEM, 
	OP_EXIT, OP_DBRA, OP_CALL, OP_JUMP, OP_DJMP, 
	OP_LINK, OP_CMP, OP_PEA, OP_ADD, OP_AND, 
	OP_EXT, OP_OR, OP_TST, OP_ASL, OP_ASR, 
	OP_SUB, OP_UNLK, OP_LEA, OP_CLR , OP_BOP,
	OP_EOR,
	OP_OTHER
} opcode_t;

typedef enum{	/* First eight are 68881 data types. */
    SUBOP_L = 0, SUBOP_S = 1, SUBOP_X = 2,
    SUBOP_P = 3, SUBOP_W = 4, SUBOP_D = 5,
    SUBOP_B = 6, SUBOP_C = 7,
    JEQ = 8, JNE, JLE, JGE, JLT, JGT, JLS, JHI, JMI, JPL,
    JALL, JCS, JCC, JVS, JVC, JNONE, JIND,
    SUBOP_Z	/* Last type for typeless instructions. */
} subop_t;

#if AS

#define N_OPTYPES 8 	/* Maximum number of operand type descriptors. */

struct ins_bkt {
	char		*text_i;
	int	       (*routine_i)();
	opcode_t	 op_i:8;
	subop_t          subop_i:8;
	short		 noper_i:8;
	short		 align_i:8;
	unsigned short	 touchop_i;
	unsigned short
			 opval_i[5]; /* info, usually opcodes */
	unsigned 	 optype_i[8]; /* up to 4 pairs of 2 operands */
	short		 xflags_i;
};

/* instruction flags */
#define I20	1	/* instruction only on 68020 */
#define I81	2	/* instruction only on 68881 */

struct ins_ptr {	/* arranged so op-code table can be read-only */
	char 	       *name_p;
	struct ins_ptr *next_p;
	struct ins_bkt *this_p;
};

#endif
extern struct ins_ptr *inst;

extern char   opcodetypes[];
#define INSTRTYPE	1
#define PSEUDONOCODE	2
#define PSEUDOCODE	4
#define ISINSTRUC( op ) (opcodetypes[(int)(op)]&INSTRTYPE)
#define ISPSEUDO( op ) (opcodetypes[(int)(op)]&(PSEUDOCODE|PSEUDONOCODE))
#define ISDIRECTIVE( op ) (opcodetypes[(int)(op)]&PSEUDONOCODE)

/* operand access bits */
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
# define TOUCH1(a) ((a)<<2*TOUCHWIDTH)
# define TOUCH2(a) ((a)<<TOUCHWIDTH)
# define TOUCH3(a) (a)
/* operand access bits for floating point */
# define SR	LR
# define XR	LR
# define PR	LR
# define DR	LR
# define CR	LR
# define FR	LR
# define SW	LW
# define XW	LW
# define PW	LW
# define DW	LW
# define CW	LW
# define FW	LW

/* size code fields for floating point */

# define L_SIZE	0*02000
# define S_SIZE	1*02000
# define X_SIZE	2*02000
# define P_SIZE	3*02000
# define W_SIZE	4*02000
# define D_SIZE	5*02000
# define B_SIZE	6*02000
# define C_SIZE	7*02000

/* floating point condition code fields */

# define FCC_F		00
# define FCC_EQ		01
# define FCC_OGT	02
# define FCC_OGE	03
# define FCC_OLT	04
# define FCC_OLE	05
# define FCC_OGL	06
# define FCC_OR		07
# define FCC_UN		010
# define FCC_UEQ	011
# define FCC_UGT	012
# define FCC_UGE	013
# define FCC_ULT	014
# define FCC_ULE	015
# define FCC_NEQ	016
# define FCC_T		017
# define FCC_SF		020
# define FCC_SEQ	021
# define FCC_GT		022
# define FCC_GE		023
# define FCC_LT		024
# define FCC_LE		025
# define FCC_GL		026
# define FCC_GLE	027
# define FCC_NGLE	030
# define FCC_NGL	031
# define FCC_NLE	032
# define FCC_NLT	033
# define FCC_NGE	034
# define FCC_NGT	035
# define FCC_SNEQ	036
# define FCC_ST		037

/* operand addressing mode bits */

# define AM_DREG	01
# define AM_AREG	02
# define AM_DEFER	04
# define AM_POSTINC	010
# define AM_PREDEC	020
# define AM_DISPL	040
# define AM_INDEX	0100
# define AM_ABSS	0200
# define AM_ABSL	0400
# define AM_IMMED	01000
# define AM_PCDISPL	02000
# define AM_PCINDEX	04000
# define AM_NORMAL	010000
# define AM_REG		(AM_DREG|AM_AREG)
# define AM_ALL		017777
# define AM_FREG	020000
# define AM_CCREG	040000
# define AM_CTRLREG	0100000
# define AM_USPREG	0200000
# define AM_PCREG	0400000
# define AM_REGPAIR	01000000
# define AM_FCTRLREG	02000000
# define AM_REGLIST	04000000
# define AM_FREGLIST	010000000
# define AM_FCREGLIST	020000000

# define AM_AMEM	(AM_DEFER|AM_POSTINC|AM_PREDEC|AM_DISPL|AM_INDEX|AM_ABSS|AM_ABSL|AM_NORMAL)
# define AM_ADAT	(AM_AMEM|AM_DREG)
# define AM_CTRL	(AM_DEFER|AM_DISPL|AM_INDEX|AM_ABSS|AM_ABSL|AM_NORMAL|AM_PCDISPL|AM_PCINDEX)
# define AM_ACTRL	(AM_DEFER|AM_DISPL|AM_INDEX|AM_ABSS|AM_ABSL|AM_NORMAL)
# define AM_MEM		(AM_ALL&~AM_REG)
# define AM_ADDR	(AM_ALL&~AM_REG&~AM_IMMED)
# define AM_DATA	(AM_ALL&~AM_AREG)
# define AM_AA		(AM_ALL&~(AM_PCDISPL|AM_PCINDEX|AM_IMMED))

#if C2
/* cc set */
# define CC0		0
# define CC1		1
# define CC2		2
# define CCX		3
# define CCR		0100
#endif
