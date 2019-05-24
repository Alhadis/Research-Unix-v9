/*      @(#)eccreg.h 1.1 86/02/03 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#define	OBIO_ECCREG 0x1C0000	/* addr of ECC memory regsiters in obio space */

#define MAX_ECC		4	/* maximium number of ECC memory cards */

#define ECCREG_ADDR 0x0FFE8000	/* virtual address we map eccreg to be at */

#ifdef LOCORE
#define	ECCREG ECCREG_ADDR
#else
struct eccreg {
	struct eccena {
		u_int ena_type : 3;	/* r/o - board type identifier */
		u_int ena_mode : 2;	/* r/w - operation mode */
#define ENA_MODE_NORMAL		0
#define ENA_MODE_DIAG_GENERATE	1
#define ENA_MODE_DIAG_DETECT	2
#define ENA_MODE_DIAG_INIT	3
		u_int ena_size : 2;	/* r/o - board size */
#define ENA_SIZE_4MB		0
#define ENA_SIZE_8MB		1
#define ENA_SIZE_16MB		2
#define ENA_SIZE_32MB		3
		u_int ena_scrub : 1;	/* r/w - enable refresh scrub cycle */
		u_int ena_busena : 1;	/* r/w - enable mem bus ecc reporting */
		u_int ena_boardena : 1;	/* r/w - overall board enable */
		u_int ena_addr : 6;	/* r/w - base address <A27..A22> */
		u_int : 16;
	} eccena;
	struct syndrome {
		u_int sy_synd : 8;	/* r/o - syndrome for first error */
		u_int : 1;
		u_int sy_addr : 22;	/* r/o - real addr bits <A24..A3> */
		u_int sy_ce : 1;	/* r/o - correctable error */
	} syndrome;
	struct eccdiag_reg {
		u_int : 8;
		u_int dr_cb32 : 1;	/* w/o - check bit 32 (data D<71>) */
		u_int : 16;
		u_int dr_cb16 : 1;	/* w/o - check bit 16 (data D<70>) */
		u_int : 32;
		u_int dr_cb8 : 1;	/* w/o - check bit 8  (data D<69>) */
		u_int dr_cb4 : 1;	/* w/o - check bit 4  (data D<68>) */
		u_int dr_cb2 : 1;	/* w/o - check bit 2  (data D<67>) */
		u_int dr_cb1 : 1;	/* w/o - check bit 1  (data D<66>) */
		u_int dr_cb0 : 1;	/* w/o - check bit 1  (data D<65>) */
		u_int dr_cbX : 1;	/* w/o - check bit X  (data D<64>) */
	} eccdiag_reg;
	u_char eccreg_pad[64 - sizeof (struct eccdiag_reg) -
	    sizeof (struct syndrome) - sizeof (struct eccena)];
};

#define	ECCREG ((struct eccreg *)ECCREG_ADDR)
#endif

#define SYNDERR_BITS	"\20\10S32\7S16\6S8\5S4\4S2\3S1\2S0\1SX"
