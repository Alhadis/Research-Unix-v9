/*	@(#)as.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>

/* Assembler parameters */
#define FILES_MAX	10	/* Max number of source files read */
#define	STR_MAX		32	/* number of chars in any single token */
#define OPERANDS_MAX	6	/* number of operands allowed per instruction */
#define	HASH_MAX	256	/* size of symbol, command, and macro hash tables */
#define CODE_MAX	12	/* number of bytes generated for 1 machine instruction */

/* Error Codes  -- See error.c */

typedef enum {	E_NOERROR = 0, E_BADCHAR, E_MULTSYM, E_NOSPACE, E_OFFSET, 
		E_SYMLEN, E_SYMDEF, E_CONSTANT, E_TERM, E_OPERATOR,
		E_RELOCATE, E_TYPE, E_OPERAND, E_SYMBOL, E_EQUALS,
		E_NLABELS, E_OPCODE, E_STRING, E_PHASE,
		E_NUMOPS, E_LINELONG, E_REG, E_IADDR, E_PAREN,
		E_ODDADDR, E_UNDEFINED_L, E_REGLIST
} err_code;

/* Symbol attributes */
#define	S_DEC	01
#define S_DEF	02
#define S_EXT	04
#define S_LABEL	010
#define S_CRT	020
#define S_REG	040
#define S_LOCAL 0100
#define S_COMM	0200
#define S_PERM	0400

#include "inst.h"

#include "oper.h"

/* local names for csects */
#define C_UNDEF	0
#define C_TEXT	1
#define C_DATA	2
#define C_DATA1	3
#define C_DATA2	4
#define C_BSS	5

/* Symbol bucket definition */
struct sym_bkt {
       char   *name_s;		/* symbol identifier		*/
       struct sym_bkt *next_s;	/* next bkt on linked list	*/
       long   value_s;		/* it's value			*/
#if AS
       short  id_s;		/* id number for .b file	*/
       int    final;
#endif
#if C2
       char  builtin_s[4];	/* read-write attributes for D0,D1,A0,A1 */
#endif
       short  attr_s;		/* attributes			*/
       short  csect_s;		/* name of its csect		*/
       short  nuse_s;		/* number of references		*/
       struct node *where_s;	/* first use, or defining node	*/
};

extern char *soperand(),*exp();
extern char iline[],code[];
extern short cinfo[];
extern int numops, errors, line_no;
extern struct oper operands[];
extern struct ins_ptr *ins_hash_tab[];
extern struct sym_bkt *lookup();
extern struct sym_bkt *last_symbol;
extern struct sym_bkt *dot_bkt;
extern struct ins_bkt *sopcode();
extern long dot;
extern short cur_csect_name;
extern char rel_name[], *source_name;
extern int pass, bc;

#if AS
extern int d2flag, jsrflag, pcflag, rflag;
#ifdef EBUG
extern int debflag;
#endif
extern long tsize, dsize, d1size, d2size, bsize;
extern int code_length;	/* number of bytes in WCode */
/* complain during second pass only */
#define PROG_ERROR( n )	{if(pass == 2)prog_error( n );}
#endif
#if C2
#define PROG_ERROR( n )	prog_error( n )
#endif

/* skip to next non-spacing character */
#define skipb(p) while (cinfo[*p] == SPC) p++

/* skip to end of symbol */
#define skips(p) while (cinfo[*p] & T) p++

#if C2
#   define C2MAGIC "|#"
#   define C2MAGICSIZE 2
    extern char * p2pseudocomment();
#endif

/* some register names (indexes into the register table) */
/* also used as bit indexes in register mask structures! */
# define D0REG	0
# define D7REG  7
# define A0REG	8
# define A6REG	14
# define A7REG	15
# define FPREG	A6REG
# define SPREG	A7REG
# define FP0REG	16
# define FP7REG	23
# define CCREG	24
# define FPCCREG	25
# define PCREG  26
# define SRREG  27
# define USPREG 28
# define FPCREG 36
# define FPSREG 37
# define FPIREG 38

/* what the registers are good for */
extern /* const */ unsigned reg_access[];
#define ctrl_reg(reg) (reg_access[(reg)]&AM_CTRLREG)
#define dreg(reg) (reg_access[(reg)]&AM_DREG)
#define areg(reg) (reg_access[(reg)]&AM_AREG)
#define freg(reg) (reg_access[(reg)]&AM_FREG)
#define dareg(reg) (reg_access[(reg)]&(AM_AREG|AM_DREG))
#define datareg(reg) (reg_access[(reg)]&(AM_AREG|AM_DREG|AM_FREG))
#define pcreg(reg) (reg_access[(reg)]&AM_PCREG)
#define srreg(reg) (reg_access[(reg)]&AM_CCREG)
#define sr_addr(op) ((op)->type_o==T_REG && srreg((op)->value_o))
#define usp_addr(op) ((op)->type_o==T_REG && (reg_access[(op)->value_o]&AM_USPREG))
#define dreg_addr(op) ((op)->type_o==T_REG && dreg((op)->value_o))
#define areg_addr(op) ((op)->type_o==T_REG && areg((op)->value_o))
#define freg_addr(op) ((op)->type_o==T_REG && freg((op)->value_o))
#define dareg_addr(op) ((op)->type_o==T_REG && dareg((op)->value_o))
#define datareg_addr(op) ((op)->type_o==T_REG && datareg((op)->value_o))
#define fctrlreg_addr(op) ((op)->type_o==T_REG && (reg_access[(op)->value_o]&AM_FCTRLREG))

/* bits found in character info array cinfo[] */
#define D 0x0100	/* digit */
#define S 0x0200	/* can start symbol */
#define T 0x0400	/* can be part of symbol */

#define COL 0x00	/* label definition */
#define EQL 0x01	/* label assignment */
#define EOL 0x02	/* end of line -- newline or comment char */
#define ADD 0x03	/* addition operator */
#define SUB 0x04	/* subtraction operator */
#define SPC 0x05	/* spacing character */
#define ERR 0x06	/* illegal character */
#define IMM 0x07	/* immediate operand indicator */
#define LP  0x08	/* left paren */
#define RP  0x09	/* right paren */
#define COM 0x0A	/* operand separator */
#define IND 0x0B	/* indirection operator */
#define MUL 0x0C	/* multiplication operator */
#define NOT 0x0D	/* complement operator */
#define QUO 0x0E	/* beginning/end of string */
#define DIV 0x0F	/* division operator */
#define LB  0x10	/* left square bracket */
#define RB  0x11	/* right square bracket */

#if AS
/* flavors of sdi's */
#define SDI4  0
#define SDI6  1
#define SDI8  2
#define SDIP  3
#define SDIX  4
#define SDIL  5
#endif AS
