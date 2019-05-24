/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../machine/clock.h"
#include "../machine/interreg.h"

/*
 * Machine-dependent clock routines.
 *
 * Clockstart restarts the real-time clock, which provides
 * hardclock interrupts to clock.c.
 *
 * Clkinit initializes the time of day hardware which provides
 * date functions.  Its primary function is to use some file
 * system information in case the hardware clock lost state.
 *
 * Clkset restores the time of day hardware after a time change.
 */

/*
 * Start the real-time clock.
 */
clkstart()
{

	/*
	 * We will set things up to interrupt every 1/100 of a second.
	 * locore.s currently only calls hardclock every other clock
	 * interrupt, thus assuming 50 hz operation.
	 */
	if (hz != 50)
		panic("clkstart");

	CLKADDR->clk_intrreg = CLK_INT_HSEC;	/* set 1/100 sec clock intr */
	set_clk_mode(IR_ENA_CLK5, 0);		/* turn on level 5 clock intr */
}

/*
 * Set and/or clear the desired clock bits in the interrupt
 * register.  We have to be extremely careful that we do it
 * in such a manner that we don't get ourselves lost.
 */
set_clk_mode(on, off)
	u_char on, off;
{
	register u_char interreg, dummy;

	/*
	 * make sure that we are only playing w/ 
	 * clock interrupt register bits
	 */
	on &= (IR_ENA_CLK7 | IR_ENA_CLK5);
	off &= (IR_ENA_CLK7 | IR_ENA_CLK5);

	/*
	 * Get a copy of current interrupt register,
	 * turning off any undesired bits (aka `off')
	 */
	interreg = *INTERREG & ~(off | IR_ENA_INT);
	*INTERREG &= ~IR_ENA_INT;

	/*
	 * Next we turns off the CLK5 and CLK7 bits to clear
	 * the flip-flops, then we disable clock interrupts.
	 * Now we can read the clock's interrupt register
	 * to clear any pending signals there.
	 */
	*INTERREG &= ~(IR_ENA_CLK7 | IR_ENA_CLK5);
	CLKADDR->clk_cmd = (CLK_CMD_NORMAL & ~CLK_CMD_INTRENA);
	dummy = CLKADDR->clk_intrreg;			/* clear clock */
#ifdef lint
	dummy = dummy;
#endif

	/*
	 * Now we set all the desired bits
	 * in the interrupt register, then
	 * we turn the clock back on and
	 * finally we can enable all interrupts.
	 */
	*INTERREG |= (interreg | on);			/* enable flip-flops */
	CLKADDR->clk_cmd = CLK_CMD_NORMAL;		/* enable clock intr */
	*INTERREG |= IR_ENA_INT;			/* enable interrupts */
}

#define ABS(x)	((x) < 0? -(x) : (x))

/*
 * Initialize the system time, based on the time base which is, e.g.
 * from a filesystem.
 */
clkinit(base)
	time_t base;
{
	register unsigned todr;
	register long deltat;
	int ticks;
	int s;

	if (base < (85 - YRREF) * SECYR) {	/* ~1985 */
		printf("WARNING: preposterous time in file system");
		goto check;
	}
	s = splclock();
	time = todget(&ticks);
	(void) splx(s);
	if (time < SECYR) {
		time = base;
		printf("WARNING: TOD clock not initialized");
		clkset();
		goto check;
	}
	deltat = time - base;
	if (deltat < 0)
		deltat = -deltat;
	if (deltat < 2*SECDAY)
		return;
	printf("WARNING: clock %s %d days",
	    time < base ? "lost" : "gained", deltat / SECDAY);
check:
	printf(" -- CHECK AND RESET THE DATE!\n");
}

/*
 * Dummy routine for version 9 compatability
 */
clkwrap()
{
	return(0);
}

clktrim()
{
	int ticks, sec;
	int s = splclock();

	/*
	 * Sync the software clock with the hardware clock
	 */
	sec = todget(&ticks);
	lbolt += hz * (sec - time) + (ticks - lbolt);
	(void) splx(s);
}

/*
 * Reset the TODR based on the time value; used when the TODR
 * has a preposterous value and also when the time is reset
 * by the settimeofday system call.  We call tod_set at splclock()
 * to avoid synctodr() from running and getting confused.
 */
clkset()
{
	int s;

	s = splclock();
	todset();
	(void) splx(s);
}

/*
 * For Sun-3, we use the Intersil ICM7170 for both the
 * real time clock and the time-of-day device.
 */

static u_int monthsec[12] = {
	31 * SECDAY,	/* Jan */
	28 * SECDAY,	/* Feb */
	31 * SECDAY,	/* Mar */
	30 * SECDAY,	/* Apr */
	31 * SECDAY,	/* May */
	30 * SECDAY,	/* Jun */
	31 * SECDAY,	/* Jul */
	31 * SECDAY,	/* Aug */
	30 * SECDAY,	/* Sep */
	31 * SECDAY,	/* Oct */
	30 * SECDAY,	/* Nov */
	31 * SECDAY	/* Dec */
};

#define	MONTHSEC(mon, yr)	\
	(((((yr) % 4) == 0) && ((mon) == 2))? 29*SECDAY : monthsec[(mon) - 1])

/*
 * Set the TOD based on the argument value; used when the TOD
 * has a preposterous value and also when the time is reset
 * by the settimeofday system call.
 */
todset()
{
	register int t = time;
	u_short hsec, sec, min, hour, day, mon, weekday, year;

	/*
	 * Figure out the (adjusted) year
	 */
	for (year = (YRREF - YRBASE); t > SECYEAR(year); year++)
		t -= SECYEAR(year);

	/*
	 * Figure out what month this is by subtracting off
	 * time per month, adjust for leap year if appropriate.
	 */
	for (mon = 1; t >= 0; mon++)
		t -= MONTHSEC(mon, year);

	t += MONTHSEC(--mon, year);	/* back off one month */

	sec = t % 60;			/* seconds */
	t /= 60;
	min = t % 60;			/* minutes */
	t /= 60;
	hour = t % 24;			/* hours (24 hour format) */
	day = t / 24;			/* day of the month */
	day++;				/* adjust to start at 1 */
	weekday = day % 7;		/* not right, but it doesn't matter */

	hsec = 0;

	CLKADDR->clk_cmd = (CLK_CMD_NORMAL & ~CLK_CMD_RUN);
	CLKADDR->clk_weekday = weekday;
	CLKADDR->clk_year = year;
	CLKADDR->clk_mon = mon;
	CLKADDR->clk_day = day;
	CLKADDR->clk_hour = hour;
	CLKADDR->clk_min = min;
	CLKADDR->clk_sec = sec;
	CLKADDR->clk_hsec = hsec;
	CLKADDR->clk_cmd = CLK_CMD_NORMAL;
}

/*
 * Read the current time from the clock chip and convert to UNIX form.
 * Assumes that the year in the counter chip is valid.
 */
todget(ticks)
int *ticks;
{
	u_char now[CLK_WEEKDAY];
	register int i;
	register u_char *cp = (u_char *)CLKADDR;
	register u_int t = 0;
	u_short year;

	for (i = CLK_HSEC; i < CLK_WEEKDAY; i++)	/* read counters */
		now[i] = *cp++;

	/*
	 * Add the number of seconds for each year onto our time t.
	 * We start at YRBASE, and count up to the year value given
	 * by the chip.  If the year is greater/equal to the difference
	 * between YRREF and YRBASE, then that time is added into
	 * the Unix time value we are calculating.
	 */
	for (year = 0; year < now[CLK_YEAR]; year++)
		if (year >= (YRREF - YRBASE))
			t += SECYEAR(year);

	/*
	 * Now add in the seconds for each month that has gone
	 * by this year, adjusting for leap year if appropriate.
	 */
	for (i = 1; i < now[CLK_MON]; i++)
		t += MONTHSEC(i, year);

	t += (now[CLK_DAY] - 1) * SECDAY;
	t += now[CLK_HOUR] * (60*60);
	t += now[CLK_MIN] * 60;
	t += now[CLK_SEC];

	*ticks = now[CLK_HSEC] * hz / 100;
	return (t);
}
