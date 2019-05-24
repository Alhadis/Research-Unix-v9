/*	@(#)scsi.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Standard SCSI control blocks.
 * These go in or out over the SCSI bus.
 */
struct	scsi_cdb {		/* scsi command description block */
	u_char	cmd;		/* command code */
	u_char	lun	  : 3;	/* logical unit number */
	u_char	high_addr : 5;	/* high part of address */
	u_char	mid_addr;	/* middle part of address */
	u_char	low_addr;	/* low part of address */
	u_char	count;		/* block count */
	u_char	vu_57	  : 1;	/* vendor unique (byte 5 bit 7) */
	u_char	vu_56	  : 1;	/* vendor unique (byte 5 bit 6) */
	u_char		  : 4;	/* reserved */
	u_char	fr	  : 1;	/* flag request (interrupt at completion) */
	u_char	link	  : 1;	/* link (another command follows) */
};

/*
 * defines for SCSI tape cdb.
 */
#define	t_code		high_addr
#define	high_count	mid_addr
#define	mid_count	low_addr
#define low_count	count

struct	scsi_scb {		/* scsi status completion block */
	/* byte 0 */
	u_char	ext_st1	: 1;	/* extended status (next byte valid) */
	u_char	vu_06	: 1;	/* vendor unique */
	u_char	vu_05	: 1;	/* vendor unique */
	u_char	is	: 1;	/* intermediate status sent */
	u_char	busy	: 1;	/* device busy or reserved */
	u_char	cm	: 1;	/* condition met */
	u_char	chk	: 1;	/* check condition: sense data available */
	u_char	vu_00	: 1;	/* vendor unique */
	/* byte 1 */
	u_char	ext_st2	: 1;	/* extended status (next byte valid) */
	u_char	reserved: 6;	/* reserved */
	u_char	ha_er	: 1;	/* host adapter detected error */
	/* byte 2 */
	u_char	byte2;		/* third byte */
};

struct	scsi_sense {		/* scsi sense for error classes 0-6 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char	class	: 3;	/* error class (0-6) */
	u_char	code	: 4;	/* error code */
	u_char	high_addr;	/* high byte of block addr */
	u_char	mid_addr;	/* middle byte of block addr */
	u_char	low_addr;	/* low byte of block addr */
	u_char	extra[12];	/* pad this struct so it can hold max num */
				/* of sense bytes we may receive */
};

struct	scsi_ext_sense {	/* scsi extended sense for error class 7 */
	/* byte 0 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char		: 7;	/* fixed at binary 1110000 */
	u_char	seg_num;	/* segment number, applies to copy cmd only */
	/* byte 2 */
	u_char	fil_mk	: 1;	/* file mark on device */
	u_char	eom	: 1;	/* end of media */
	u_char	ili	: 1;	/* incorrect length indicator */
	u_char		: 1;	/* reserved */
	u_char	key	: 4;	/* sense key, see below */
	u_char	info_1;		/* information byte 1 */
	u_char	info_2;		/* information byte 2 */
	u_char	info_3;		/* information byte 3 */
	u_char	info_4;		/* information byte 4 */
	u_char	add_len;	/* number of additional bytes */
	/* additional bytes follow, if any */
};
	
/* 
 * Sense key values for extended sense.
 */
#define	SC_NO_SENSE		0x0
#define	SC_RECOVERABLE_ERROR	0x1
#define	SC_NOT_READY		0x2
#define	SC_MEDIUM_ERROR		0x3
#define	SC_HARDWARE_ERROR	0x4
#define	SC_ILLEGAL_REQUEST	0x5
#define	SC_UNIT_ATTENTION	0x6
#define	SC_DATA_PROTECT		0x7
#define	SC_BLANK_CHECK		0x8
#define	SC_VENDOR_UNIQUE	0x9
#define	SC_COPY_ABORTED		0xa
#define	SC_ABORT_COMMAND	0xb
#define	SC_EQUAL		0xc
#define	SC_VOLUME_OVERFLOW	0xd
#define SC_MISCOMPARE		0xe
#define SC_RESERVED		0xf

/*
 * SCSI Operation codes. 
 */
#define	SC_TEST_UNIT_READY	0x00
#define	SC_REZERO_UNIT		0x01
#define	SC_REQUEST_SENSE	0x03
#define	SC_READ			0x08
#define	SC_WRITE		0x0a
#define	SC_SEEK			0x0b

/*
 * Messages that SCSI can send.
 */
#define	SC_COMMAND_COMPLETE	0x00
#define SC_SAVE_DATA_PTR	0x02
#define SC_RESTORE_PTRS		0x03
#define SC_DISCONNECT		0x04
#define SC_IDENTIFY		0x80
#define SC_DR_IDENTIFY		0xc0

#define	MORE_STATUS 0x80	/* More status flag */
#define	STATUS_LEN  3		/* Max status len for SCSI */

#define	cdbaddr(cdb, addr) 	(cdb)->high_addr = (addr) >> 16;\
				(cdb)->mid_addr = ((addr) >> 8) & 0xFF;\
				(cdb)->low_addr = (addr) & 0xFF


#define NLPART	NDKMAP		/* number of logical partitions (8) */

/*
 * SCSI unit block.
 */
struct scsi_unit {
	char	un_target;		/* scsi bus address of controller */
	char	un_lun;			/* logical unit number of device */
	char	un_present;		/* unit is present */
	u_char	un_scmd;		/* special command */
	u_short un_unit;		/* real unit number of device */
	struct	scsi_unit_subr *un_ss;	/* scsi device subroutines */
	struct	scsi_ctlr *un_c;	/* scsi ctlr */
	struct	mb_device *un_md;	/* mb device */
	struct	mb_ctlr *un_mc;		/* mb controller */
	struct	buf un_utab;		/* queue of requests */
	struct	buf un_sbuf;		/* fake buffer for special commands */
	struct	buf un_rbuf;		/* buffer for raw i/o */
	/* current transfer: */
	u_short	un_flags;		/* misc flags relating to cur xfer */
	int	un_baddr;		/* virtual buffer address */
	daddr_t	un_blkno;		/* current block */
	short	un_sec_left;		/* sector count for single sectors */
	short	un_cmd;			/* command (for cdb) */
	short	un_count;		/* num sectors to xfer (for cdb) */
	int	un_dma_addr;		/* dma address */
	u_short	un_dma_count;		/* byte count expected */
	short	un_retries;		/* retry count */
	short	un_restores;		/* restore count */
	char	un_wantint;		/* expecting interrupt */
	/* the following save current dma information in case of disconnect */
	int	un_dma_curaddr;		/* current addr to start dma to/from */
	u_short	un_dma_curcnt;		/* current dma count */
	u_short	un_dma_curdir;		/* direction of dma transfer */
};

/* 
 * bits in the scsi unit flags field
 */
#define SC_UNF_DVMA	0x0001		/* set if cur xfer requires dvma */

struct scsi_ctlr {
	struct	scsi_ha_reg *c_har;	/* sc bus registers in i/o space */
	struct 	scsi_si_reg *c_sir;	/* si scsi ctlr logic regs */
	struct	scsi_cdb c_cdb;		/* command description block */
	struct	scsi_scb c_scb;		/* status completion block */
	struct	scsi_sense *c_sense;	/* sense info on errors */
	int	c_flags;		/* misc state flags */
	struct 	scsi_unit *c_un;	/* scsi unit using the bus */
	struct	scsi_ctlr_subr *c_ss;	/* scsi device subroutines */
	struct	udc_table c_udct;	/* scsi dma info for Sun3/50 */
	struct	buf *c_disqh;		/* ptr to head of disconnect q */
	struct	buf *c_disqt;		/* ptr to tail of disconnect q */
	struct	buf *c_flush;		/* ptr to last element to flush */
};

/* misc controller flags */
#define SCSI_PRESENT	0x0001		/* scsi bus is alive */
#define SCSI_ONBOARD	0x0002		/* scsi logic is onboard */
#define SCSI_EN_RECON	0x0004		/* reconnect attempts are enabled */
#define SCSI_EN_DISCON	0x0008		/* disconnect attempts are enabled */
#define SCSI_FLUSH_DISQ	0x0010		/* flush disconnected tasks */
#define SCSI_FLUSHING	0x0020		/* flushing in progress */

/* 
 * Unit specific subroutines called from controllers.
 */
struct	scsi_unit_subr {
	int	(*ss_attach)();
	int	(*ss_start)();
	int	(*ss_mkcdb)();
	int	(*ss_intr)();
	int	(*ss_unit_ptr)();
	char	*ss_devname;
};

/* 
 * Controller specific subroutines called from units.
 */
struct	scsi_ctlr_subr {
	int	(*scs_ustart)();
	int	(*scs_start)();
	int	(*scs_done)();
	int	(*scs_cmd)();
	int	(*scs_getstat)();
	int	(*scs_cmd_wait)();
	int	(*scs_off)();
	int	(*scs_reset)();
	int	(*scs_dmacount)();
	int	(*scs_go)();
};

/*
 * Defines for getting configuration parameters out of mb_device.
 */
#define	TYPE(flags)	(flags)
#define TARGET(slave)	((slave >> 3) & 07)
#define LUN(slave)	(slave & 07)

#define SCSI_DISK	0
#define SCSI_TAPE	1
#define SCSI_FLOPPY	2

#define NUNIT		8	/* max nubmer of units per controller */

/*
 * SCSI Error codes passed to device routines.
 * The codes refer to SCSI general errors, not to device
 * specific errors.  Device specific errors are discovered
 * by checking the sense data.
 * The distinction between retryable and fatal is somewhat ad hoc.
 */
#define	SE_NO_ERROR	0
#define	SE_RETRYABLE	1
#define	SE_FATAL	2

struct scsi_disk {
	struct	dk_map un_map[NLPART];	/* logical partitions */
	struct	dk_geom un_g;		/* disk geometry */
};

struct scsi_tape {
	int	un_mspl;		/* ptr to mode select param list */
	int	un_bufcnt;		/* number of buffers in use */
	int	un_next_block;		/* next record on tape */
	int	un_last_block;		/* last record of file, if known */
	short	un_status;		/* status from last sense */
	short	un_retry_ct;		/* retry count */
	short	un_underruns;		/* number of underruns */
	char	un_openf;		/* tape is open */
	char	un_eof;			/* eof seen but not sent to user */
	char	un_read_only;		/* tape is read only */
	char	un_lastiow;		/* last i/o was write */
	char	un_lastior;		/* last i/o was read */
	char	un_reten_rewind;	/* retension on next rewind */
	char	un_qic;			/* qic format */
	char	un_ctype;		/* controller type */
	char	un_reset_occurred;	/* reset occured since last command */
	char	un_readmode;		/* sysgen left in read mode */
	char	un_err_pending;		/* illegal cmd error pending */
	u_char	un_last_cmd;		/* last scsi command issued */
#ifdef sun2
	char	un_cipher_test;		/* test for cipher drive in progres */
#endif
};

struct scsi_floppy {
	u_char	sf_flags;
	u_char	sf_mdb;			/* media descriptor byte */
	u_char	sf_spt;			/* sectors per track */
	u_char	sf_nhead;		/* number of heads */
	int	sf_nblk;		/* number of blocks on the disk */
};
