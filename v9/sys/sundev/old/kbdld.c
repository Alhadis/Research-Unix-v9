/*
 * Copyright (C) 1985 by Sun Microsystems, Inc.
 */
#include "kbd.h"
#if NKBD > 0
/*
 * Keyboard input line discipline.
 * Console output line discipline.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/stream.h"
#include "../h/ttyio.h"
#include "../h/ttyld.h"
#include "../h/conf.h"
#include "../machine/sunromvec.h"
#include "../sundev/kbdvar.h"

/*
 * For now these are shared.
 */
extern int nkeytables;
extern struct keyboard	*keytables[];
extern char keystringtab[16][KTAB_STRLEN];

#define	TIMEOUT	04

struct	kbdld {
/* Added for 9th edition */
	struct	queue *key_q;	/* queue for the keyboard */
	struct	queue *cons_q;	/* queue for output to the console */
	short	c_state;	/* state of the console */
/* Defined by sun */
	u_char	k_id;
	u_char	k_idstate;
	u_char	k_state;
	u_char	k_rptkey;
	u_int	k_buckybits;
	u_int	k_shiftmask;
	struct	keyboard *k_curkeyboard;
	u_int	k_togglemask;	/* Toggle shifts state */
} kbd[NKBD], nullkbd;

/*
 * States of keyboard ID recognizer
 */
#define	KID_NONE	0		/* startup */
#define	KID_IDLE	1		/* saw IDLE code */
#define	KID_LKSUN2	2		/* probably Sun-2 */
#define	KID_OK		3		/* locked on ID */

/*
 * Constants setup during the first open of a kbd (so that hz is defined).
 */
int	kbd_repeatrate;
int	kbd_repeatdelay;

int	kbdopen(), kbdclose(), kbdldin(), kbdisrv(), kbdldout();
static struct qinit kbdrinit = { kbdldin, kbdisrv, kbdopen, kbdclose,300, 60};
static struct qinit kbdwinit = { kbdldout, NULL, kbdopen, kbdclose, 200, 100 };
struct streamtab kbdinfo = { &kbdrinit, &kbdwinit};

/*
 * Keyboard and console open
 */
kbdopen(qp, dev)
register struct queue *qp;
{
	register struct kbdld *k;

	/* Set these up only once so that they could be changed from adb */
	if (!kbd_repeatrate) {
		kbd_repeatrate = (hz+29)/30;
		kbd_repeatdelay = hz/2;
	}
	if (qp->ptr)				/* already attached */
		return(1);
	for (k = kbd; k->key_q != 0; k++)
		if (k >= &kbd[NKBD])
			return(0);
	*k = nullkbd;
	k->key_q = qp;
	k->cons_q = WR(qp);
	qp->ptr = (caddr_t)k;
	WR(qp)->ptr = (caddr_t)k;
	kbdreset(k);
	return(1);
}

kbdclose(qp)
register struct queue *qp;
{
	register struct kbdld *k = (struct kbdld *)qp->ptr;

	k->key_q = 0;
	k->cons_q = 0;
}

/*
 * Console write put routine
 */
kbdldout(q, bp)
register struct queue *q;
register struct block *bp;
{
	register union stmsg *sp;
	register struct kbdld *k = (struct kbdld *)q->ptr;

	switch(bp->type) {

	case M_IOCTL:
		sp = (union stmsg *)bp->rptr;
		switch (sp->ioc0.com) {

		case TIOCGDEV:
			sp->ioc3.sb.ispeed =
			  sp->ioc3.sb.ospeed = B9600;
			bp->type = M_IOCACK;
			qreply(q, bp);
			return;
		case TIOCSDEV:
			bp->wptr = bp->rptr;
			bp->type = M_IOCACK;
			qreply(q, bp);
			return;
		default:
			bp->type = M_IOCNAK;
			bp->wptr = bp->rptr;
			qreply(q, bp);
			return;
		}

	case M_STOP:
		k->c_state |= TTSTOP;
		break;

	case M_START:
		k->c_state &= ~TTSTOP;
		kbdstart(k);
		break;

	case M_FLUSH:
		flushq(q, 0);
		break;

	case M_DELAY:
	case M_DATA:
		putq(q, bp);
		kbdstart(k);
		return;
	
	default:
		break;
	}
	freeb(bp);
}

kbdtime(k)
register struct kbdld *k;
{
	k->c_state &= ~TIMEOUT;
	kbdstart(k);
}

kbdstart(k)
register struct kbdld *k;
{
	register s;
	register struct block *bp;
	register u_char *cp;

	if (k->cons_q==NULL)
		return;
	s = spl1();
	while ((k->c_state & (TIMEOUT|TTSTOP))==0 && k->cons_q->count) {
		bp = getq(k->cons_q);
		switch (bp->type) {

		case M_DATA:
			/* Must clear high bit for monitor */
			for(cp = bp->rptr; cp < bp->wptr; cp++)
				*cp &= 0177;
			(*romp->v_fwritestr)(bp->rptr, bp->wptr - bp->rptr,
					romp->v_fbaddr);
			freeb(bp);
			break;

		case M_DELAY:
			timeout(kbdtime, (caddr_t)k, (int)*bp->rptr);
			k->c_state |= TIMEOUT;
			freeb(bp);
			splx(s);
			return;
		default:
			freeb(bp);
			break;
		}
	}
	splx(s);
}

kbdldin(q, bp)
struct queue *q;
register struct block *bp;
{
	register struct kbdld *k = (struct kbdld *)q->ptr;

	/* Pass along anything but data */
	if (bp->type != M_DATA) {
		(*q->next->qinfo->putp)(q->next, bp);
		return;
	}

	while (bp->rptr < bp->wptr)
		kbdinput(*bp->rptr++, k);
	freeb(bp);
	while ((q->next->flag&QFULL)==0 && (bp = getq(q)))
		(*q->next->qinfo->putp)(q->next, bp);
}

/*
 * keyboard server processing.
 */
kbdisrv(q)
register struct queue *q;
{
	register struct block *bp;

	while ((q->next->flag&QFULL)==0 && (bp = getq(q)))
		(*q->next->qinfo->putp)(q->next, bp);
}

/*
 * kbdclick is used to remember the current click value of the
 * Sun-3 keyboard.  This brain damaged keyboard will reset the
 * clicking to the "default" value after a reset command and
 * there is no way to read out the current click value.  We
 * cannot send a click command immediately after the reset
 * command or the keyboard gets screwed up.  So we wait until
 * we get the ID byte before we send back the click command.
 * Unfortunately, this means that there is a small window
 * where the keyboard can click when it really shouldn't be.
 * A value of -1 means that kbdclick has not been initialized yet.
 */
int kbdclick = -1;

/*
 * Send command byte to keyboard
 */
kbdcmd(k, cmd)
register struct kbdld *k;
char cmd;
{
	register struct queue *q = WR(k->key_q)->next;

	putd(q->qinfo->putp, q, cmd);
	if (cmd == KBD_CMD_NOCLICK)
		kbdclick = 0;
	else if (cmd == KBD_CMD_CLICK)
		kbdclick = 1;
}

/*
 * Reset the keyboard
 */
kbdreset(k)
register struct kbdld *k;
{
	k->k_idstate = KID_NONE;
	k->k_state = NORMAL;
	kbdcmd(k, KBD_CMD_RESET);
}

kbdidletimeout(k)
register struct kbdld *k;
{
	untimeout(kbdidletimeout, (caddr_t)k);
	/*
	 * Double check that was waiting for idle timeout.
	 */
	if (k->k_idstate == KID_IDLE)
		kbdinput(IDLEKEY, k);
}

/*
 * Process a keypress
 */
kbdinput(key, k)
register u_char key;
register struct kbdld *k;
{
	switch (k->k_idstate) {

	case KID_NONE:
		if (key == IDLEKEY) {
			k->k_idstate = KID_IDLE;
			timeout(kbdidletimeout, (caddr_t)k, hz/10);
		} else if (key == RESETKEY)
			k->k_idstate = KID_LKSUN2;
		return;

	case KID_IDLE:
		if (key == IDLEKEY)
			kbdid(k, KB_KLUNK);
		else if (key == RESETKEY)
			k->k_idstate = KID_LKSUN2;
		else if (key & 0x80)
			kbdid(k, (int)(KB_VT100 | (key&0x40)));
		else
			kbdreset(k);
		return;

	case KID_LKSUN2:
		if (key == 0x02) {	    /* Sun-2 keyboard */
			kbdid(k, KB_SUN2);
			return;
		}
		if (key == 0x03) {	    /* Sun-3 keyboard */
			kbdid(k, KB_SUN3);
			/*
			 * We just did a reset command to a Sun-3 keyboard
			 * which sets the click back to the default
			 * (which is currently ON!).  We use the kbdclick
			 * variable to see if the keyboard should be
			 * turned on or off.  If it has not been set,
			 * then on a sun3 we use the eeprom to determine
			 * if the default value is on or off.  In the
			 * sun2 case, we default to off.
			 */
			switch (kbdclick) {
			case 0:
				kbdcmd(k, KBD_CMD_NOCLICK);
				break;
			case 1:
				kbdcmd(k, KBD_CMD_CLICK);
				break;
			case -1:
			default:
				{
#ifdef sun3
#include "../sun3/eeprom.h"

				if (EEPROM->ee_diag.eed_keyclick ==
				    EED_KEYCLICK)
					kbdcmd(k, KBD_CMD_CLICK);
				else
#endif sun3
					kbdcmd(k, KBD_CMD_NOCLICK);
				}
				break;
			}
			return;
		}
		kbdreset(k);
		return;

	case KID_OK:
		if (key == 0 || key == 0xFF) {
			kbdreset(k);
			return;
		}
		break;
	}
			
	switch (k->k_state) {

	normalstate:
		k->k_state = NORMAL;
	case NORMAL:
		if (k->k_curkeyboard && key == k->k_curkeyboard->k_abort1) {
			k->k_state = ABORT1;
			break;
		}
		kbdtranslate(k, key);
		if (key == IDLEKEY)
			k->k_state = IDLE1;
		break;

	case IDLE1:
		if (key & 0x80)	{	/* ID byte */
			if (k->k_id == KB_VT100)
				k->k_state = IDLE2;
			else 
				kbdreset(k);
			break;
		}
		if (key != IDLEKEY) 
			goto normalstate;	/* real data */
		break;

	case IDLE2:
		if (key == IDLEKEY) k->k_state = IDLE1;
		else goto normalstate;
		break;

	case ABORT1:
		if (k->k_curkeyboard) {
			if (key == k->k_curkeyboard->k_abort2) {
				DELAY(100000);
				montrap(*romp->v_abortent);
				k->k_state = NORMAL;
				kbdtranslate(k, (u_char)IDLEKEY); /* fake */
				return;
			} else {
				kbdtranslate(k, k->k_curkeyboard->k_abort1);
				goto normalstate;
			}
		}
	}
}

kbdid(k, id)
register struct kbdld *k;
int	id;
{
	k->k_id = id & 0xF;
	k->k_idstate = KID_OK;
	k->k_shiftmask = 0;
	if (id & 0x40)
		/* Not a transition so don't send event */
		k->k_shiftmask |= CAPSMASK;
	k->k_buckybits = 0;
	k->k_curkeyboard = keytables[k->k_id];
	k->k_rptkey = IDLEKEY;	/* Nothing happening now */
}

/*
 * This routine determines which table we should look in to decode
 * the current keycode.
 */
struct keymap *
settable(k, mask)
register struct kbdld *k;
register u_int mask;
{
	register struct keyboard *kp;

	kp = k->k_curkeyboard;
	if (kp == NULL)
		return (NULL);
	if (mask & UPMASK)
		return (kp->k_up);
	if (mask & CTRLMASK)
		return (kp->k_control);
	if (mask & SHIFTMASK)
		return (kp->k_shifted);
	if (mask & CAPSMASK)
		return (kp->k_caps);
	return (kp->k_normal); 
}

kbdrpt(k)
register struct kbdld *k;
{
	kbdtranslate(k, k->k_rptkey);
	if (k->k_rptkey != IDLEKEY)
		timeout(kbdrpt, (caddr_t)k, kbd_repeatrate);
}

kbdcancelrpt(k)
register struct kbdld *k;
{
	if (k->k_rptkey != IDLEKEY) {
		untimeout(kbdrpt, (caddr_t)k);
		k->k_rptkey = IDLEKEY;
	}
}

kbdtranslate(k, keycode)
register struct kbdld *k;
register u_char keycode;
{
	register u_char key, newstate, entry;
	register u_char enF0;
	register char *cp;
	struct keymap *km;
	register struct queue *q = k->key_q;

	newstate = STATEOF(keycode);
	key = KEYOF(keycode);

	km = settable(k, (u_int)(k->k_shiftmask | newstate));
	if (km == NULL) {		/* gross error */
		kbdcancelrpt(k);
		return;
	}
	entry = km->keymap[key];
	enF0 = entry & 0xF0;
	/*
	 * Handle the state of toggle shifts specially.
	 * Toggle shifts should only come on downs.
	 */
	if (((entry >> 4) == (SHIFTKEYS >> 4)) &&
	    ((1 << (entry & 0x0F)) & k->k_curkeyboard->k_toggleshifts)) {
		if ((1 << (entry & 0x0F)) & k->k_togglemask) {
			newstate = RELEASED;
		} else {
			newstate = PRESSED;
		}
	}

	if (newstate == PRESSED && entry != NOSCROLL &&
	    enF0 != SHIFTKEYS && enF0 != BUCKYBITS &&
	    !(entry >= LEFTFUNC && entry <= BOTTOMFUNC+15)) {
		if (k->k_rptkey != keycode) {
			kbdcancelrpt(k);
			timeout(kbdrpt, (caddr_t)k, kbd_repeatdelay);
			k->k_rptkey = keycode;
		}
	} else if (key == KEYOF(k->k_rptkey))	/* key going up */
		kbdcancelrpt(k);

	switch (entry >> 4) {

	case 0: case 1: case 2: case 3:
	case 4: case 5: case 6: case 7:
		putd(putq, q, entry | k->k_buckybits);
		break;

	case SHIFTKEYS >> 4: {
		u_int shiftbit = 1 << (entry & 0x0F);

		/* Modify toggle state (see toggle processing above) */
		if (shiftbit & k->k_curkeyboard->k_toggleshifts) {
			if (newstate == RELEASED) {
				k->k_togglemask &= ~shiftbit;
				k->k_shiftmask &= ~shiftbit;
			} else {
				k->k_togglemask |= shiftbit;
				k->k_shiftmask |= shiftbit;
			}
		} else
			k->k_shiftmask ^= shiftbit;
		break;
		}

	case BUCKYBITS >> 4:
		k->k_buckybits ^= 1 << (7 + (entry & 0x0F));
		break;

	case FUNNY >> 4:
		switch (entry) {
		case NOP:
			break;

		/*
		 * NOSCROLL/CTRLS/CTRLQ exist so that these keys, on keyboards
		 * with NOSCROLL, interact smoothly.  If a user changes
		 * his tty output control keys to be something other than those
		 * in keytables for CTRLS & CTRLQ then he effectively disables
		 * his NOSCROLL key.  One could imagine computing CTRLS & CTRLQ
		 * dynamically by watching TIOCSETC ioctl's go by in kbdioctl.
		 */
		case NOSCROLL:
			if (k->k_shiftmask & CTLSMASK)	goto sendcq;
			else				goto sendcs;

		case CTRLS:
		sendcs:
			k->k_shiftmask |= CTLSMASK;
			putd(putq, q, ('S'-0x40) | k->k_buckybits);
			break;

		case CTRLQ:
		sendcq:
			putd(putq, q, ('Q'-0x40) | k->k_buckybits);
			k->k_shiftmask &= ~CTLSMASK;
			break;

		case IDLE:
			/*
			 * Minor hack to prevent keyboards unplugged
			 * in caps lock from retaining their capslock
			 * state when replugged.  This should be
			 * solved by using the capslock info in the 
			 * KBDID byte.
			 */
			if (keycode == NOTPRESENT)
				k->k_shiftmask = 0;
			/* Fall thru into RESET code */

		case RESET:
		gotreset:
			k->k_shiftmask &= k->k_curkeyboard->k_idleshifts;
			k->k_shiftmask |= k->k_togglemask;
			k->k_buckybits &= k->k_curkeyboard->k_idlebuckys;
			kbdcancelrpt(k);
			break;

		case ERROR:
			printf("kbd: Error detected\r\n");
			goto gotreset;

		/*
		 * Remember when adding new entries that,
		 * if they should NOT auto-repeat,
		 * they should be put into the IF statement
		 * just above this switch block.
		 */
		default:
			goto badentry;
		}
		break;

	case STRING >> 4:
		cp = &keystringtab[entry & 0x0F][0];
		while (*cp != '\0') {
			putd(putq, q, *cp);
			cp++;
		}
		break;

	/*
	 * Remember when adding new entries that,
	 * if they should NOT auto-repeat,
	 * they should be put into the IF statement
	 * just above this switch block.
	 */
	default:
		if (entry >= LEFTFUNC && entry <= BOTTOMFUNC+15) {
			char	buf[10], *strsetwithdecimal();

			if (newstate == RELEASED)
				break;
			cp = strsetwithdecimal(&buf[0], (u_int)entry,
			    sizeof (buf) - 1);
			putd(putq, q, '\033');
			putd(putq, q, '[');
			while (*cp != '\0') {
				putd(putq, q, *cp);
				cp++;
			}
			putd(putq, q, 'z');
		}
	badentry:
		break;
	}
}

char *
strsetwithdecimal(buf, val, maxdigs)
	char	*buf;
	u_int	val, maxdigs;
{
	int	hradix = 5;
	char	*bp;
	int	lowbit;
	char	*tab = "0123456789abcdef";

	bp = buf + maxdigs;
	*(--bp) = '\0';
	while (val) {
		lowbit = val & 1;
		val = (val >> 1);
		*(--bp) = tab[val % hradix * 2 + lowbit];
		val /= hradix;
	}
	return (bp);
}
#endif NKBD > 0
