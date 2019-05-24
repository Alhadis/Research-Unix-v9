/*	@(#)sky.h 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * as include file for interfacing the sky ffp board
 *		21 June, 1983	rt
 */

/*  shape of the ffp register data structure: */
struct sky {
	unsigned short	sky_command;
	unsigned short	sky_status;
	union{
	    short sword[2];
	    long  slong;
	}s;
#define	sky_data	s.slong
#define sky_d1reg	s.sword[0]
	long	sky_ucode;
};

/*
 *
 * some commands:
 *
 */
/* control commands: */
#define	S_INIT		0x1000
#define	S_SAVE		0x1040
#define	S_REST		0x1041
#define	S_NOP		0x1063
/* state-free conversions: b <- f(a) */
#define	S_ITOS		0x1024
#define	S_ITOD		0x1044
#define	S_STOD		0x1042
#define	S_DTOS		0x1043
#define	S_DTOI		0x1045
#define	S_STOI		0x1027
/* state-free single-precision arithmetic: b <- f(a1,a2) */
#define	S_SADD3		0x1001
#define	S_SSUB3		0x1007
#define	S_SMUL3		0x100B
#define	S_SDIV3		0x1013
#define S_SPVT3		0x1017	/* b <- a3 + a1*a2 */
/* state-dependent single-precision arithmetic: b <- f(r0,a1) */
#define	S_SADD2		0x1003	/* S_SADD3+2 */
#define	S_SSUB2		0x1009	/*   &c.     */
#define	S_SMUL2		0x100D
#define	S_SDIV2		0x1015
/* state-free double-precision arithmetic: b <- f(a1,a2) */
#define	S_DADD3		0x1002	/* S_SADD3+1 */
#define	S_DSUB3		0x1008	/*   &c.     */
#define	S_DMUL3		0x100C
#define	S_DDIV3		0x1014
#define S_DPVT3		0x1018	/* b <- a3 + a1*a2 */
/* state-dependent double-precision arithmetic: b <- f(r0,a1) */
#define	S_DADD2		0x1004	/* S_DADD3+2 or S_SADD2+1 or S_SADD3+3 */
#define	S_DSUB2		0x100A	/*		  &c. 		       */
#define	S_DMUL2		0x100E
#define	S_DDIV2		0x1016
/* state-free comparisons: b <- a1 vs. a2 */
#define	S_SCMP3		0x105D
#define	S_DCMP3		0x105E
/* state-dependent comparisons: b <- r0 vs. a1 */
#define	S_SCMP2		0x105F	/* S_SCMP3+2 */
#define	S_DCMP2		0x1060	/* S_DCMP3+2 */
/* random data movement	*/
#define	S_LDS		0x1031
#define	S_LDD		0x1034
