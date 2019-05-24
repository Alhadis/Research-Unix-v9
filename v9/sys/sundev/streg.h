/*	@(#)streg.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Defines for SCSI tape.
 */
#define	DEV_BSIZE	512
#define	SENSE_LENGTH	16

/*
 * Open flag codes
 */
#define	CLOSED		0
#define	OPENING		1
#define	OPEN_FAILED	2
#define	OPEN		3
#define	CLOSING		4

/*
 * Operation codes.
 */
#define	SC_REWIND		SC_REZERO_UNIT
#define	SC_WRITE_FILE_MARK	0x10
#define SC_SPACE		0x11
#define SC_MODE_SELECT		0x15
#define SC_ERASE_CARTRIDGE	0x19
#define SC_LOAD			0x1b
#define SC_SPACE_FILE		0x81	/* phony - for internal use only */
#define SC_SPACE_REC		0x82	/* phony - for internal use only */

#define ST_TYPE_INVALID		0x00

/*
 * Parameter list for the MODE_SELECT command.
 * The parameter list contains a header, followed by zero or more
 * block descriptors, followed by vendor unique parameters, if any.
 */
struct st_ms_hdr {
	u_char	reserved1;	/* reserved */
	u_char	reserved2;	/* reserved */
	u_char		  :1;	/* reserved */
	u_char	bufm	  :3;	/* buffered mode */
	u_char	speed	  :4;	/* speed */
	u_char	bd_len;		/* length in bytes of all block descs */
};

struct st_ms_bd {
	u_char	density;	/* density code */
	u_char	high_nb;	/* num of logical blocks on the medium that */
	u_char	mid_nb;		/* are to be formatted with the density code */
	u_char	low_nb;		/* and block length in block desc */
	u_char	reserved;	/* reserved */
	u_char	high_bl;	/* block length */
	u_char	mid_bl;		/* block length */
	u_char	low_bl;		/* block length */
};

/*
 * Mode Select Parameter List expected by emulex controllers.
 */
struct st_emulex_mspl {
	struct st_ms_hdr hdr;	/* mode select header */
	struct st_ms_bd  bd;	/* block descriptor */
	u_char		  :5;	/* unused */
	u_char	dea	  :1;	/* disable erase ahead */
	u_char	aui	  :1;	/* auto-load inhibit */
	u_char	sec	  :1;	/* soft error count */
};
#define EM_MS_PL_LEN	13	/* length of mode select param list */
#define EM_MS_BD_LEN	8	/* length of block descriptors */

/*
 * Sense info returned by Archive controllers.
 */
struct st_archive_sense {
	struct scsi_ext_sense ext_sense; /* generic extended sense format */
/*	u_char	rserved[4];
	u_char	retries_msb;		/* retry count, most signif byte */
/*	u_char	retries_lsb;		/* retry count, most signif byte */
};

/* number of emulex sense bytes in addition to generic extended sense */
#define AR_ES_ADD_LEN			0

/*
 * Macros for getting information from the sense data returned
 * by the tape controller.
 */
#define ST_FILE_MARK(dsi, sense) \
	(((struct scsi_ext_sense *)sense)->fil_mk)

#define ST_WRITE_PROT(dsi, sense) \
	(((struct scsi_ext_sense *)sense)->key == SC_DATA_PROTECT)

#define ST_EOT(dsi, sense) \
	(((struct scsi_ext_sense *)sense)->eom)

#define ST_ILLEGAL(dsi, sense) \
	(((struct scsi_ext_sense *)sense)->key == SC_ILLEGAL_REQUEST)

#define ST_NO_CART(dsi, sense) \
	(((struct scsi_ext_sense *)sense)->key == SC_NOT_READY)

#define ST_RESET(dsi, sense) \
	(((struct scsi_ext_sense *)sense)->key == SC_UNIT_ATTENTION)

#define ST_CORRECTABLE(dsi, sense) \
	(((struct scsi_ext_sense *)sense)->key == SC_RECOVERABLE_ERROR)

#define ST_EOD(dsi, sense) \
	(((struct scsi_ext_sense *)sense)->key == SC_BLANK_CHECK)
