/*	@(#)error.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/* Error Codes  -- many are antique and currently unused. See error.c */
#define ERR_MAX		50	/* Max number of different error codes */

#define E_END		1
#define E_BADCHAR	2
#define E_MULTSYM	3
#define	E_NOSPACE	4
#define	E_OFFSET	5
#define	E_SYMLEN	6
#define	E_SYMDEF	7
#define	E_CONSTANT	8
#define	E_TERM		9
#define	E_OPERATOR	10
#define	E_RELOCATE	11
#define	E_TYPE		12
#define	E_OPERAND	13
#define	E_SYMBOL	14
#define	E_EQUALS	15
#define	E_NLABELS	16
#define	E_OPCODE	17
#define	E_ENTRY		18
#define	E_STRING	19
#define	E_NRELOCATE	20
#define	E_ATTRIBUTE	21
#define	E_.ERROR	22
#define	E_LEVELS	23
#define	E_CONDITION	24
#define	E_NUMOPS	25
#define	E_LINELONG	26
#define E_REG		27
#define	E_IADDR		28
#define E_UNIMPL	29
#define E_FILE		30
#define E_PAREN		31
#define E_MLENGTH	32
#define E_MACARG	33
#define	E_MACFORMAL	34
#define E_ENDC		35
#define E_RELADDR	36
#define E_ARGUMENT	37
#define E_VECINDEX	38
#define E_VECMNEM	39
#define E_MACRO		40
#define E_TMACRO	41
#define E_CSECT		42
#define E_ODDADDR	43
