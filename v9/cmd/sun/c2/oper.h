/*	@(#)oper.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* operand types */
/* T_PCPLUS used inside ins.c only! */
typedef enum { T_NULL=0, T_REG, T_DEFER, T_POSTINC, T_PREDEC, T_DISPL, T_INDEX,
	       T_ABSS, T_ABSL, T_IMMED, T_NORMAL, 
	       T_REGPAIR, T_STRING, T_FLOAT, T_PCPLUS,
	       T_REGLIST, T_FREGLIST, T_FCREGLIST
} operand_t;


/* operand structure */
struct oper {
       operand_t	type_o :8;	/* operand type info 		*/
       unsigned 	flags_o:16,	/* operand flags 		*/
	        	reg_o:8,	/* Register subfield value 	*/
			scale_o:8;	/* index scale factor           */
       struct sym_bkt  *sym_o;		/* symbol used for relocation	*/
       struct sym_bkt  *sym2_o;		/* symbol used for relocation	*/
#if C2
       struct oper     *nsym_o;		/* chain of users of this symbol */
#endif
       long		value_o;	/* register # for T_REG, offset for T_DISPL */
       double		fval_o;		/* floating-point value for T_FLOAT */
       char	       *stringval_o;	/* string pointer for T_STRING */
       long		disp_o;		/* displacement value for index mode */
       long		disp2_o;	/* displacement value for full index mode */
       unsigned		access_o;	/* access bits */
       int		bfoffset_o;	/* bit-field offset modifier    */
       int		bfwidth_o;	/* bit-field width modifier     */

};

/* operand flags */
#define O_WINDEX  1
#define O_LINDEX  2
#define O_COMPLEX 4
#define O_BFLD    010	/* bit field modifier */
#define O_COMPLEX2 040000 /* second displacement of index is complex */
#define O_FLOAT	  0100000 /* immediate value is floating-point */
/* modifiers for full indexing mode */
#define O_BSUPRESS 020  /* base  supress */
#define O_INDIRECT 040  /* memory indirect */
#define O_POSTINDEX 0100 /* index after indirection */
#define O_PREINDEX 0200 /* index before indirection */
#define O_WDISP    0400 /* word-length displacement */
#define O_LDISP    01000 /* long-length displacement */
#define O_LDISP2   02000 /* long-length 2nd displacement */
#define O_WDISP2   04000 /* word-length 2nd displacement */
/* modifiers for bit fields */
#define O_BFOREG   010000 /* offset field is a register */
#define O_BFWREG   020000 /* width field is a register */

struct optree {
    struct oper  *right_t;
    struct oper  *left_t;
    char op_t;
};
