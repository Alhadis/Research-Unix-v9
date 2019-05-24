/*      @(#)mmu.h 1.1 86/02/03 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* 
 * Sun-3 memory management.
 */
#define KCONTEXT	 0	/* Kernel context (no valid user pages) */
#define	NCONTEXT	 8	/* Number of contexts */
#define	CONTEXTMASK	(NCONTEXT-1)

/*
 * Hardware segment and page registers and constants.
 */
#define	NSEGMAP	2048		/* # of segments per context */
#define	SEGINV	(NPMEG-1)	/* invalid pmeg - no access */
#define	NPAGSEG 16		/* # of pages per segment */
#define	NPME	4096		/* number of hardware page map entries */
#define	NPMEG	(NPME/NPAGSEG)	/* # of pme groups (segment allocation) */

/*
 * Function code register values.
 */
#define	FC_UD	1		/* user data */
#define	FC_UP	2		/* user program */
#define	FC_MAP	3		/* Sun-3 memory maps */
#define	FC_SD	5		/* supervisor data */
#define	FC_SP	6		/* supervisor program */
#define	FC_CPU	7		/* cpu space */

/*
 * FC_MAP base addresses
 */
#define	IDPROMBASE	0x00000000	/* id prom base */
#define	PAGEBASE	0x10000000	/* page map base */
#define	SEGMENTBASE	0x20000000	/* segment map base */
#define	CONTEXTBASE	0x30000000	/* context map base */

#define IDPROMSIZE	0x20		/* size of id prom in bytes */

/*
 * Masks for relevant bits of virtual address
 * when accessing control space devices
 */
#define	PAGEADDRBITS	0x0FFFE000	/* page map virtual address mask */
#define	SEGMENTADDRBITS	0x0FFE0000	/* segment map virtual address mask */

/*
 * 68020 Cache Control Register
 */
#define CACHE_ENABLE	0x1	/* enable the cache */
#define CACHE_FREEZE	0x2	/* freeze the cache */
#define CACHE_CLRENTRY	0x4	/* clear entry specified by cache addr reg */
#define CACHE_CLEAR	0x8	/* clear entire cache */
