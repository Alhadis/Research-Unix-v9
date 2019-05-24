/*	@(#)saio.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * header file for standalone I/O package
 */

#include <sys/types.h>
#include <machine/sunromvec.h>

/*
 * io block: the structure passed to or from the device drivers.
 * 
 * Includes pointers to the device
 * in use, a pointer to device-specific data (iopb's or device
 * state information, typically), cells for the use of seek, etc.
 * NOTE: expand at end to preserve compatibility with PROMs
 */
struct saioreq {
	char	si_flgs;
	struct boottab *si_boottab;	/* Points to boottab entry if any */
	char	*si_devdata;		/* Device-specific data pointer */
	int	si_ctlr;		/* Controller number or address */
	int	si_unit;		/* Unit number within controller */
	daddr_t	si_boff;		/* Partition number within unit */
	daddr_t	si_cyloff;
	off_t	si_offset;
	daddr_t	si_bn;			/* Block number to R/W */
	char	*si_ma;			/* Memory address to R/W */
	int	si_cc;			/* Character count to R/W */
	struct	saif *si_sif;		/* interface pointer */
	char 	*si_devaddr;		/* Points to mapped in device */
	char	*si_dmaaddr;		/* Points to allocated DMA space */
};


#define F_READ	01
#define F_WRITE	02
#define F_ALLOC	04
#define F_FILE	010
#define	F_EOF	020	/* EOF on device */

/*
 * io block: includes an
 * inode, cells for the use of seek, etc,
 * and a buffer.
 */
#undef i_buf	/* curse you clever folk from inode.h */
struct iob {
	struct saioreq	i_si;		/* I/O request block for this file */
	struct inode	i_ino;		/* Inode for this file */
	char		i_buf[BUFSIZE];/* Buffer for reading inodes & dirs */
};
#define i_flgs		i_si.si_flgs
#define i_boottab	i_si.si_boottab
#define i_devdata	i_si.si_devdata
#define i_ctlr		i_si.si_ctlr
#define i_unit		i_si.si_unit
#define i_boff		i_si.si_boff
#define i_cyloff	i_si.si_cyloff
#define i_offset	i_si.si_offset
#define i_bn		i_si.si_bn
#define i_ma		i_si.si_ma
#define i_cc		i_si.si_cc

/*
 * request codes. Must be the same as F_XXX above
 */
#define	READ	F_READ
#define	WRITE	F_WRITE

#define	NBUFS	4

char	b[NBUFS][BUFSIZE];
daddr_t	blknos[NBUFS];

#define NFILES	4
struct	iob iob[NFILES];

/*
 * Ethernet interface descriptor
 */
struct saif {
	int	(*sif_xmit)();		/* transmit packet */
	int	(*sif_poll)();		/* check for and receive packet */
	int	(*sif_reset)();		/* reset interface */
};

/*
 * Types of resources that can be allocated by resalloc().
 */
enum RESOURCES { 
	RES_MAINMEM,		/* Main memory, accessible to CPU */
	RES_RAWVIRT,		/* Raw addr space that can be mapped */
	RES_DMAMEM,		/* Memory acc. by CPU and by all DMA I/O */
	RES_DMAVIRT,		/* Raw addr space accessible by DMA I/O */
};
char *resalloc(), *devalloc();


/*
 * Delay units are in microseconds.
 */
#define	DELAY(n)	\
{ \
	extern int cpudelay; \
	register int N = (((n)<<4) >> cpudelay); \
 \
	while (--N > 0) ; \
}

#define	CDELAY(c, n)	\
{ \
	extern int cpudelay; \
	register int N = (((n)<<3) >> cpudelay); \
 \
	while (--N > 0) \
		if (c) \
			break; \
}

/*
 * Translate our virtual address (in DMA-able memory) into a physical DMA
 * address, as it would appear to a Multibus device.  (In VMEbus systems,
 * this assumes a MB/VME adapter.)
 */
#define	MB_DMA_ADDR(x)	(((int)(x))&0x000FFFFF)
