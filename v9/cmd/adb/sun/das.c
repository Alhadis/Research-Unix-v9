/*
 * disassemble 68020 opcodes
 */

#include "defs.h"
#include "optab.h"
#include "space.h"

#define	SZ_MASK		00300
#define	SZ_SHIFT	6		/* bits  (7-6) */

static char *areg();
static char *dreg();
static char *adreg();

static struct opmask {
	long mask;
	int shift;
} opmask[] = {
	0,		0,	/* DIG	0   ignore this address */
	0x0000003f,	0,	/* DEA	1   E.A. to low order 6 bits */
	0x00000007,	0,	/* DRG	2   register to low order 3 bits */
	0x00000e00,	9,	/* DRGL	3   register to bits 11-9 */
	0x000000ff,	0,	/* DBR	4   branch offset (short) */
	0x000000ff,	0,	/* DMQ	5   move-quick 8-bit value */
	0x00000e00,	9,	/* DAQ	6   add-quick 3-bit value in 11-9 */
	0,		0,	/* DIM	7   Immediate value, according to size */
	0x00000fc0,	6,	/* DEAM	8   E.A. to bits 11-6 as in move */
	0,		0,	/* DBCC	9   branch address as in "dbcc" */
	0x0000000f,	0,	/* DTRAP 10 immediate in low 4 bits */
	0x00070000,	16,	/* D2L	11  register to bits 0-2 of next word */
	0x70000000,	16+12,	/* D2H	12  register to bits 12-14 of next word */
	0x001f0000,	16,	/* DBL	13  qty in bits 0-5 of next word */
	0x07c00000,	16+6,	/* DBH	14  qty in bits 6-11 of next word */
	0x0fff0000,	16,	/* DCR	15  control reg a bit combination in 0-11 */
};

static int dsp;

printins(isp)
int isp;
{
	register struct optab *op;
	register int i;
	register WORD w, w1;
	int w1f = 0;
	extern struct optab optab[];

	dsp = isp;
	w = stow(sget(dot, isp));
	chkerr();
	for (op = optab; op->opname; op++) {
		if ((w & op->mask) != op->opcode)
			continue;
		if ((op->flags & I2W) == 0)
			break;		/* 1-word match */
		if (w1f == 0) {
			w1 = stow(sget(dot+2, isp));
			w1f++;
		}
		if ((w1 & op->mk2) == op->op2)
			break;		/* 2-word match */
	}
	if (op->opname == NULL) {
		printf("\tnumber\t%R", w);
		dotinc = 2;
		return;
	}
	w &= 0xffff;
	if ((op->flags & I2W) == 0)
		dotinc = 2;
	else {
		w |= w1 << 16;
		dotinc = 4;
	}
	printf("\t%s", op->opname);
	for (i = 0; i < op->nrand; i++) {
		if (i == 0)
			printf("\t");
		else
			printf(",");
		dorand(w, op->rand[i], op->flags & SZ);
	}
}

#define	ENSIGN(x)	((WORD)(short)(x))
#define	ENSIGNC(x)	((WORD)(char)(x))

static
dorand(w, rand, size)
register WORD w;
register short rand;
int size;
{
	struct opmask *om;
	WORD val;

	om = &opmask[rand & DMASK];
	if (om->mask)
		val = (w & om->mask) >> om->shift;
	switch(rand & DMASK) {
	case DEA:	/* effective address spec */
		ea(val >> 3, val & 07, size);
		return;

	case DRG:	/* abs register */
	case DRGL:
	case D2H:
	case D2L:
		if (rand & ADEC)
			printf("%s@-", areg(val));
		else if (rand & AINC)
			printf("%s@+", areg(val));
		else if (rand & AAREG)
			printf("%s", areg(val));
		else if (rand & ADREG)
			printf("d%d", val);
		else
			printf("DRGgok");
		return;

	case DBR:	/* branch displacement */
		if (val == 0) {
			val = stow(sget(dot+dotinc, dsp));
			if (val & 0x8000)
				val |= ~0xffff;
			dotinc += 2;
		}
		else if (val == 0xff) {
			val = ltow(lget(dot+dotinc, dsp));
			dotinc += 4;
		}
		else if (val & 0x80)
			val |= ~0xff;
		val += dot + 2;
		psymoff(val, dsp, "");
		return;

	case DTRAP:	/* 4-bit quick */
	case DMQ:	/* 8-bit quick */
		printf("#%d", val);
		return;

	case DBH:	/* 6-bit strange quick */
	case DBL:	/* other 6-bit strange quick */
		printf("#");
		psymoff(val, dsp, "");
		return;

	case DAQ:	/* silly 3-bit immediate */
		if (val == 0)
			val = 8;
		printf("#%d", val);
/*		psymoff(val, dsp, ""); */
		return;

	case DIM:	/* immediate */
		if (rand & AONE) {
			printf("#1");
			return;
		}
		if (rand & AWORD)
			size = W;
		switch (size) {
		case B:
			val = ENSIGN(ctow(cget(dot+dotinc, dsp)));
			dotinc += 2;	/* sic */
			break;

		case W:
			val = ENSIGN(stow(sget(dot+dotinc, dsp)));
			dotinc += 2;
			break;

		case L:
			val = ltow(lget(dot+dotinc, dsp));
			dotinc += 4;
			break;
		}
		printf("#%R", val);
/*		printf("#");
		psymoff(val, dsp, "");
*/
		return;

	case DEAM:	/* assinine backwards ea */
		ea(val & 07, val >> 3, size);
		return;

	case DBCC:	/* branch displacement a la dbcc */
		val = stow(sget(dot+dotinc, dsp));
		dotinc += 2;
		val += dot + 2;
		psymoff(val, dsp, "");
		return;

	case DCR:
		dcr(val);
		return;

	case DSREG:
		if (rand & C)
			printf("ccr");
		else if (rand & SR)
			printf("sr");
		else if (rand & U)
			printf("usp");
		else
			printf("GOKdsreg");
		return;
	}
	printf("GOK");
}

static
ea(mode, reg, size)
int mode, reg;
{
	WORD disp;

	switch(mode){
	case 0:
		printf("d%d", reg);
		return;

	case 1:
		printf("%s", areg(reg));
		return;

	case 2:
		printf("%s@", areg(reg));
		return;

	case 3:
		printf("%s@+", areg(reg));
		return;

	case 4:
		printf("%s@-", areg(reg));
		return;

	case 5:
		disp = ENSIGN(stow(sget(dot+dotinc, dsp)));
		dotinc += 2;
		printf("%s@(%d)", areg(reg), disp);
/*		psymoff(disp, dsp, "");
		printf(")");
*/
		return;

	case 6:
		doindex(reg);	/* ugh */
		return;

	case 7:
		switch (reg) {
		case 0:
			disp = ENSIGN(stow(sget(dot+dotinc, dsp)));
			dotinc += 2;
			psymoff(disp, ANYSP, "");
			return;

		case 1:
			disp = ltow(lget(dot+dotinc, dsp));
			dotinc += 4;
			psymoff(disp, ANYSP, "");
			return;

		case 4:
			switch(size) {
			case B:
				disp = ENSIGN(ctow(cget(dot+dotinc, dsp)));
				dotinc += 2;	/* sic */
				psymoff(disp, ANYSP, "");
				return;

			case W:
				disp = ENSIGN(stow(sget(dot+dotinc, dsp)));
				dotinc += 2;
				psymoff(disp, ANYSP, "");
				return;

			case L:
				disp = ltow(lget(dot+dotinc, dsp));
				dotinc += 4;
				psymoff(disp, ANYSP, "");
				return;
			}
		}
	}
	printf("gok%d:%d", mode, reg);
}

static
doindex(addreg)
{
	register WORD w;
	register WORD base, outer;
	WORD indexreg, indexsize;
	char indexscale;

	base = outer = 0;
	w = stow(sget(dot+dotinc, dsp));
	dotinc += 2;
	indexreg = (w & 0xf000) >> 12;
	indexscale = 1 << ((w & 0x0600) >> 9);
	indexsize = (w & 0x0800) ? 'l' : 'w';
	if ((w & 0x0100) == 0) {		/* brief format */
		base = ENSIGNC(w & 0xff);
		printf("%s@(%d,%s:%c:%d)", areg(addreg), base,
			adreg(indexreg), indexsize, indexscale);
		return;
	}
	else {				/* full format */
		switch (w & 0x30) {
		case 0:			/* ugh */
		case 0x10:		/* null displacement */
			break;

		case 0x20:
			base = stow(sget(dot+dotinc, dsp));
			outer = stow(sget(dot+dotinc+2, dsp));
			dotinc += 4;
			break;

		case 0x30:
			base = ltow(lget(dot+dotinc, dsp));
			outer = ltow(lget(dot+dotinc+4, dsp));
			dotinc += 8;
			break;
		}
	}
	/* stuff */
	printf("index");
}

static
dcr(reg)
int reg;
{

	switch (reg) {
	case 0x000:
		printf("sfc");
		return;

	case 0x001:
		printf("dfc");
		return;

	case 0x002:
		printf("cacr");
		return;

	case 0x800:
		printf("usp");
		return;

	case 0x801:
		printf("vbr");
		return;

	case 0x802:
		printf("caar");
		return;

	case 0x803:
		printf("msp");
		return;

	case 0x804:
		printf("isp");
		return;

	default:
		printf("cr%x", reg);
		return;
	}
}

/*
 * return a string to print out an address register
 */
static char *areg(reg)
{
	static char string[4];

	if (reg == 7)
		return("sp");
	sprintf(string, "a%d", reg);
	return(string);
}

/*
 * return a string to print out an data register
 */
static char *dreg(reg)
{
	static char string[4];

	sprintf(string, "d%d", reg);
	return(string);
}

/*
 * return a string to print out an address register or data register
 * If bit 0x8 is set it is an address register.
 */
static char *adreg(reg)
{
	if (reg & 0x8)
		return(areg(reg & 0x7));
	return(dreg(reg));
}
