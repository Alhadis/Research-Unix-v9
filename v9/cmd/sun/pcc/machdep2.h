/*	@(#)machdep2.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifdef VAX
extern int fregs;
extern int maxargs;
#else
# define SAVEREGION 8 /* number of bytes for save area */
#endif

extern SUTYPE fregs;

#ifdef VAX
# define BYTEOFF(x) ((x)&03)
#else
# define BYTEOFF(x) ((x)&01)
#endif
# define wdal(k) (BYTEOFF(k)==0)
# define BITOOR(x) ((x)>>3)  /* bit offset to oreg offset */


#ifdef VAX
# define REGSZ 16
# define TMPREG FP
#else
# define REGSZ (3*8) /* three banks of eight each */
/* # define FREETEMP */ /* define these for temporaries at SP+(offset) */
/* # define TMPREG SP */
# define TMPREG A6
# define TMPMAX (0x7fff*8) /* temps must be addressable by 16-bit offset */
			   /* temp addressing is internally in bits      */
#endif

#define R2REGS   /* permit double indexing */

# define STOARG(p)     /* just evaluate the arguments, and be done with it... */
# define STOFARG(p)
# define STOSTARG(p)
# define genfcall(a,b) gencall(a,b)

# define NESTCALLS

# define MYREADER(p) myreader(p)
int optim2();
#ifdef VAX
# define special(a, b) 0
#endif


#ifndef VAX
	/* shape for constants between -128 and 127 */
/* # define SCCON (SPECIAL+100) */
	/* shape for constants between 0 and 32767 */
# define SICON (SPECIAL+101)
	/* shape for constants between 1 and 8 */
# define S8CON (SPECIAL+102)
	/* shape for appropriate FLD nodes (should be using MULTILEVEL here, but .. */
#define SPEC_FLD (SPECIAL+103)
#define SPEC_FLT (SPECIAL+104)
#define SPEC_DFLT (SPECIAL+105)
#define SBASE (SPECIAL+106)
#define SXREG (SPECIAL+107)
#define SNONPOW2 (SPECIAL+108)
#define SFLOAT_SRCE (SPECIAL+109)
#define SPEC_PVT (SPECIAL+110)
#define SAUTOINC (SPECIAL+111)
#define SZEROLB (SPECIAL+112)
#define SFZERO (SPECIAL+113)


#define fltused _fltused
extern int fltused, usesky;
extern int chk_ovfl;

#define R2UPKFLGS(flags,shortx,ibit,scale)\
{	scale = flags&15;\
	flags >>= 4;\
	ibit = flags&1;\
	flags >>= 1;\
	shortx = flags&1;\
}

#define R2PACKFLGS(flags,shortx,ibit,scale)\
{	flags = shortx;\
	flags <<= 1;\
	flags |= ibit;\
	flags <<= 4;\
	flags |= scale;\
}

/* 
 * this stuff is for dealing with compound su-numbers: they are 4 chars
 * in a struct. Three are currently used.
 * They represent the number of registers of each of the (three) types
 * necessary to carry out a calculation.
 */

#define SUPRINT( v ) printf(", SU = (%d,%d,%d)\n", v.d, v.a, v.f)
#define SUTOOBIG( v ) (v.d>fregs.d || v.a>fregs.a || v.f>fregs.f)
#define SUSUM( v ) (v.d + v.a + v.f)
#define SUTEST( v ) (SUSUM(v) != 0)
#define SUGT( a, b) (SUSUM(a) > SUSUM(b))


/*
 * tags for identifying machine-specific code templates in table.c
 */

#define OP68020    0x80000000	/* match only if(use68020)   */
#define NO68020    0x40000000	/* match only if(!use68020)  */
#define OP68881    0x20000000	/* match only if(use68881)   */
#define NO68881    0x10000000	/* match only if(!use68881)  */
#define OP68SPEC   0xf0000000	/* mask out all of the above */

#endif not VAX

#define szty(t)	((t)==DOUBLE? 2 : 1)
#define needpair(r,t) ((t)==DOUBLE && !iscreg(r))
#define rewfld(p)  1
#define callreg(p) D0
