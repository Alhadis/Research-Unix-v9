# define TNULL PTR    /* pointer to UNDEF */
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
# define VOID FTN	/* function returning UNDEF (for void) */
# define UNDEF 17
# define BITS 18
# define UBITS 19

# define PTR  0100
# define FTN  0200
# define ARY  0300

# define BTMASK 077
# define BTSHIFT 6
# define TSHIFT 2
# define TMASK 0300

# define BTYPE(x)  (x&BTMASK)   /* basic type of x */
# define ISPTR(x) ((x&TMASK)==PTR)
# define ISFTN(x)  ((x&TMASK)==FTN)  /* is x a function type */
# define ISARY(x)   ((x&TMASK)==ARY)   /* is x an array type */
# define INCREF(x) (((x&~BTMASK)<<TSHIFT)|PTR|(x&BTMASK))
# define DECREF(x) (((x>>TSHIFT)&~BTMASK)|(x&BTMASK))
