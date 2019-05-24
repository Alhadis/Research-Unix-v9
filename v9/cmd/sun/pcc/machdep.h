/*	@(#)machdep.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifdef VAX

/*	VAX-11 Registers */

	/* scratch registers */
# define R0 0
# define R1 1
# define R2 2
# define R3 3
# define R4 4
# define R5 5

	/* register variables */
# define R6 6
# define R7 7
# define R8 8
# define R9 9
# define R10 10
# define R11 11

	/* special purpose */
# define AP 12		/* argument pointer */
# define FP 13		/* frame pointer */
# define SP 14	/* stack pointer */
# define PC 15	/* program counter */

	/* floating registers */

	/* there are no floating point registers on the VAX */

#else

	/* 68000 registers */
	/* D0-D1/A0-A1/FP0-FP1 are scratch */
	/* D2-D7/A2-A5/FP2-FP7 are available for variable allocation */
	/* A6 and A7 are special purpose */

	/* data registers */
# define D0 0
# define D1 1
# define D2 2
# define D3 3
# define D4 4
# define D5 5
# define D6 6
# define D7 7
	/* address registers */
# define A0 8
# define A1 9
# define A2 10
# define A3 11
# define A4 12
# define A5 13
# define A6 14
# define SP 15
	/* floating registers */
# define FP0 16
# define FP1 17
# define FP2 18
# define FP3 19
# define FP4 20
# define FP5 21
# define FP6 22
# define FP7 23

#endif

/* register cookie for stack pointer */

#ifdef VAX 
# define  STKREG FP
# define  ARGREG AP
#else 
# define  STKREG A6
# define  ARGREG A6
#endif 

/*	maximum and minimum register variables */

#ifdef VAX 
# define MAXRVAR R11
# define MINRVAR R6
#else
# define MIN_DVAR D2
# define MAX_DVAR D7
# define MIN_AVAR A2
# define MAX_AVAR A5
# define MIN_FVAR FP2
# define MAX_FVAR FP7
#endif

/*
 * macros for register variable bookkeeping.  This logically involves
 * three counters (one for each register bank) but the "machine
 * independent part" of the compiler explicitly deals with a single
 * counter.  So we have the following silliness:
 */

#ifndef VAX

#define NEXTD(r) ((r)&0xf)
#define NEXTA(r) ((((r)>>4)&0xf)+A0)
#define NEXTF(r) ((((r)>>8)&0xf)+FP0)

#ifdef	FORT
/* register reserved by iropt for copy of __skybase */
#define SKYBASE(r) (((r)>>12)&0xf)
#endif	FORT

#define SETD(r,x) ((r)&=0xfffffff0, (r)|=((x)&0xf))
#define SETA(r,x) ((r)&=0xffffff0f, (r)|=(((x)-A0)&0xf)<<4)
#define SETF(r,x) ((r)&=0xfffff0ff, (r)|=(((x)-FP0)&0xf)<<8)

/*
 * REGVARMASK represents the set of registers available for
 * variable allocation, starting at A7 on the left and counting
 * down to D0.  The register variable set is <d2-d7,a2-a7>.
 */

#define REGVARMASK 0x3cfc

/*
 * FREGVARMASK represents the set of registers available for
 * floating point variable allocation, starting at FP0 on the
 * left and counting up to FP7. The reg variable set is <fp2-fp7>.
 */

#define FREGVARMASK 0x3f

/*
 * mark a register as used
 */
extern int usedregs;
extern int usedfpregs;

#define markused(r) {\
	if ((r) >= FP0)\
		usedfpregs |= (0x80>>((r)-FP0));\
	else\
		usedregs |= (1<<(r));\
}

#endif

#ifdef VAX
#   define makecc(val,i)  lastcon = (lastcon<<8)|((val<<24)>>24);  
#else 
#   define makecc(val,i)	lastcon = i ? (val<<8)|lastcon : val
#   define MULTIFLOAT
#   ifdef FORT
#       define FLOATMATH 2
#   else
#       define FLOATMATH floatmath
	extern int floatmath;
#   endif
#endif

#ifdef VAX 
# define  ARGINIT 32 
# define  AUTOINIT 0 
#else 
# define  ARGINIT 64 
# define  AUTOINIT 0
#endif 

# define  SZCHAR 8
# define  SZINT 32
# define  SZFLOAT 32
# define  SZDOUBLE 64
# define  SZEXTENDED 96
# define  SZLONG 32
# define  SZSHORT 16
# define SZPOINT 32
# define ALCHAR 8
#ifdef VAX 
# define ALINT 32
# define ALFLOAT 32
# define ALDOUBLE 32
# define ALEXTENDED 32
# define ALLONG 32
# define ALSHORT 16
# define ALPOINT 32
# define ALSTRUCT 8
# define  ALSTACK 32 
#else 
# define ALINT 16
# define ALFLOAT 16
# define ALDOUBLE 16
# define ALEXTENDED 32
# define ALLONG 16
# define ALSHORT 16
# define ALPOINT 16
# define ALSTRUCT 16
# define  ALSTACK 16
#endif 

/*	size in which constants are converted */
/*	should be long if feasable */

# define CONSZ long
#ifdef VAX
# define CONFMT "%ld"
#else
# define CONFMT "0x%lx"
#endif

/*	size in which offsets are kept
/*	should be large enough to cover address space in bits
*/

# define OFFSZ long

/* 	character set macro */

# define  CCTRANS(x) x

	/* various standard pieces of code are used */
# define LABFMT "L%d"

/* show stack grows negatively */
#define BACKAUTO
#define BACKTEMP

/* show field hardware support on VAX */
/* pretend hardware support on 68000  */
#define FIELDOPS 

#ifdef VAX
/* bytes are numbered from right to left */
#define RTOLBYTES
#else
/* bit fields are never sign-extended */
#define UFIELDS
#endif

/* we want prtree included */
# define STDPRTREE
# ifndef FORT
# define ONEPASS
# endif

# define ENUMSIZE(high,low) INT

/* su numbers are ints */


# define ADDROREG
# define FIXDEF(p) outstab(p)
# define FIXARG(p) fixarg(p)

# define STACKPROBE	/* in code.c */
# define STABBING
# define LCOMM
# define ASSTRINGS
# define FLEXNAMES
# define FIXSTRUCT outstruct

/* Machines with multiple register banks have a hard time representing
 * a one-component cost function for code generation. What we really
 * need is a cost vector, with one dimension for each resource (register type).
 * This approximates that.
 */
#ifdef VAX
    typedef int SUTYPE;
#else
    typedef struct sunum{
	char d, /* data registers */
	     a, /* address registers */
	     f, /* float registers */
	     x; /* not used yet */
    } SUTYPE;
#endif

#ifndef VAX
extern int use68020;	/* for code generation options */
extern int use68881;	/* code generation and register allocation */
#endif VAX
