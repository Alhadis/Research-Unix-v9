#ifndef KERNEL
#include <sys/ioctl.h>
#endif

#ifndef _IO
#define _IO(c,n)	 (('c')<<8|n)
#define _IOR(c,n,t)	 _IO(c,n)
#define _IOW(c,n,t)	 _IO(c,n)
#define _IOWR(c,n,t)	 _IO(c,n)
#endif

/*
 * these values are critical;
 * dsseq depends on da being 0
 * and ad being 1
 */
# define DA		((int)0)
# define AD		((int)1)

/*
 * start address for d/a and a/d converters
 */
#define	ADBASE	((int)0)
#define	DABASE	((int)010)

/*
 * ASC sequence table bit for last entry
 */
#define LAST_SEQ    bit(7)

/*
 * Ioctl commands.
 */
# define DSRATE		_IOW(s,1,int)		/* set rate */
# define DS08KHZ	_IO(s,2)		/* set 08kHz filter */
# define DS04KHZ	_IO(s,3)		/* set 04kHz filter */
# define DSBYPAS	_IO(s,5)		/* set bypass filter */
# define DSERRS		_IOR(s,6,struct ds_err)	/* get errors */
# define DSRESET	_IO(s,7)		/* reset dsc */
# define DSTRANS	_IOR(s,8,struct ds_trans)	/* get transit. counts */
# define DSDONE		_IOR(s,14,int)		/* amnt. done */
# define DSDEBUG	_IO(s,19)		/* debug */
# define DSWAIT		_IO(s,20)		/* wait for io to finish */
# define DSSTEREO	_IO(s,21)		/* switch to stereo mode */
# define DSMONO		_IO(s,22)		/* switch to mono mode */
# define DSSTOP		_IO(s,23)		/* stop conversion */
# define DSFILTER	_IOW(s,24,int)		/* select filter */
# define DSCOMPLETE	_IOW(s,25,int)		/* wait for buffer io done */
# define DSRECORD	_IO(s,26)		/* begin recording */

# define NADSB		3		/* number of buffers chaining with */

/*
 * reg specifies a sequence register (0-15).
 * conv specifies a converter.
 * dirt specifies the direction when
 * setting up the sequence ram (DSSEQ).
 */
struct ds_seq {
	short reg;
	short conv;
	short dirt;			/* shared by DSSEQ and DSRATE */
};
/*
 * Format of returned converter
 * errors.
 */
struct ds_err {
	short dma_csr;
	short asc_csr;
	short errors;
};

/*
 * Format of returned transition counts
 */
struct ds_trans {
	short to_idle;
	short to_active;
};
