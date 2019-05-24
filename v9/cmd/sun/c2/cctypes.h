
/*	@(#)cctypes.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * type stuff that cc uses to compose type words. We read those words
 * to determine what type (how many registers) a function returns.
 */

/*	type names, used in symbol table building */
# define TNULL PTR    /* pointer to UNDEF */
# define TVOID FTN	/* function returning UNDEF (for void) */
# define UNDEF 0
# define FARG 1
# define CHAR 2
# define SHORT 3
# define INT 4
# define LONG 5
# define FLOAT 6
# define DOUBLE 7
# define STRTY 8
# define UNIONTY 9
# define ENUMTY 10
# define MOETY 11
# define UCHAR 12
# define USHORT 13
# define UNSIGNED 14
# define ULONG 15

/* type modifiers */

# define PTR  020
# define FTN  040
# define ARY  060

/* type packing constants */

# define TMASK 060
# define TMASK1 0300
# define TMASK2  0360
# define BTMASK 017
# define BTSHIFT 4
# define TSHIFT 2

/*	macros	*/

# define MODTYPE(x,y) x = ( (x)&(~BTMASK))|(y)  /* set basic type of x to y */
# define BTYPE(x)  ( (x)&BTMASK)   /* basic type of x */
# define ISUNSIGNED(x) ((x)<=ULONG&&(x)>=UCHAR)
# define UNSIGNABLE(x) ((x)<=LONG&&(x)>=CHAR)
# define ENUNSIGN(x) ((x)+(UNSIGNED-INT))
# define DEUNSIGN(x) ((x)+(INT-UNSIGNED))
# define ISPTR(x) (( (x)&TMASK)==PTR)
# define ISFTN(x)  (( (x)&TMASK)==FTN)  /* is x a function type */
# define ISARY(x)   (( (x)&TMASK)==ARY)   /* is x an array type */
# define INCREF(x) ((( (x)&~BTMASK)<<TSHIFT)|PTR|( (x)&BTMASK))
# define DECREF(x) ((( (x)>>TSHIFT)&~BTMASK)|( (x)&BTMASK))
