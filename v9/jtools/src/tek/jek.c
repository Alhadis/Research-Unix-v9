#include "jerq.h"
#include <signal.h>

#define	CURSOR	'\001'

#define NUL '\000'
#define SOH '\001'
#define STX '\002'
#define ETX '\003'
#define EOT '\004'
#define ENQ '\005'
#define ACK '\006'
#define BEL '\007'
#define BS  '\010'
#define HT  '\011'
#define NL  '\012'
#define VT  '\013'
#define FF  '\014'
#define CR  '\015'
#define SO  '\016'
#define SI  '\017'
#define DLE '\020'
#define DC1 '\021'
#define DC2 '\022'
#define DC3 '\023'
#define DC4 '\024'
#define NAK '\025'
#define SYN '\026'
#define ETB '\027'
#define CAN '\030'
#define EM  '\031'
#define SUB '\032'
#define ESC '\033'
#define FS  '\034'
#define GS  '\035'
#define RS  '\036'
#define US  '\037'
#define DEL '\177'

#define NORMAL		0
#define DEFOCUSED	1
#define WRITETHRU	2

#define OUTMODED	-1
#define DISALLOWED	0
#define ALLOWED		1
#define FLASHED		2
#define PENDING		3
#define DIMMED		4
#define REMOTE		6
#define LOCAL		7
#define ALPHA		8
#define GIN		9
#define GRAPH		10
#define POINT		11
#define SPECIALPOINT	12
#define INCREMENTAL	13
#define RESET		14
#define EXIT		15

#define SAVEC		64
#define SAVEL		32
#define SAVEP		128

#define TEKXMAX		4096
#define TEKYMAX		3120

#define MAGICSIZE	16

/* There is no checking on buffer overrun in magic mode, since there is no
   way to recover if it happens. */

char magicbase[MAGICSIZE],*magicbot=magicbase,*magictop=magicbase;

short menu2_bits[] = {
	 0x1FF8, 0x1008, 0x1008, 0x1008,
	 0x1FF8, 0x1FF8, 0x1FF8, 0x1C38,
	 0x1188, 0x13C8, 0x1188, 0x1DB8,
	 0x1188, 0x1008, 0x1008, 0x1FF8,
};
Cursor menu2;

short sunset_bits[] = {
	 0x5006, 0xA819, 0x00A0, 0x04A0,
	 0x049F, 0x12A4, 0x0808, 0x03E0,
	 0x2412, 0x0808, 0x0808, 0x3FFF,
	 0x3C1F, 0x7E7E, 0x783E, 0xFCFC,
};
Cursor sunset;

short none_bits[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
Cursor none;

short grey_bits[] = {
	0x1111, 0x4444, 0x1111, 0x4444, 0x1111, 0x4444, 0x1111, 0x4444,
	0x1111, 0x4444, 0x1111, 0x4444, 0x1111, 0x4444, 0x1111, 0x4444,
};
Texture grey;

short hdots_bits[] = {
	 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA,
	 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA,
	 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA,
	 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA,
};
Texture hdots;

short vdots_bits[] = {
	 0xFFFF, 0x0000, 0xFFFF, 0x0000,
	 0xFFFF, 0x0000, 0xFFFF, 0x0000,
	 0xFFFF, 0x0000, 0xFFFF, 0x0000,
	 0xFFFF, 0x0000, 0xFFFF, 0x0000,
};
Texture vdots;

char style_dotted[] = { 2, 2, 0};
char style_dot_dash[] = {7, 2, 2, 1, 2, 2, 0};
char style_short_dash[] = {4, 4, 0};
char style_long_dash[] = {8, 8, 0};
char *styles[] = {
			0,			/*	 = 0 ==> solid		*/
			style_dotted,		/*	 = 1 ==> dotted		*/
			style_dot_dash,		/*	 = 2 ==> dot-dash	*/
			style_short_dash,	/* 	 = 3 ==> short-dash	*/
			style_long_dash,	/* 	 = 4 ==> long-dash	*/
};

extern Font defont;

int width[]={56,51,34,31,56,51,34,31},height[]={88,82,53,48,88,82,53,48};
Font *font[8];

/* Map for size 16 x 16 */

unsigned char dither[16][16] = {
127, 223,  95, 247, 119, 215,  87, 253, 125, 221,  93, 245, 117, 213,  85, 255, 
128,  32, 160,   8, 136,  40, 168,   2, 130,  34, 162,  10, 138,  42, 170,   0, 
 64, 224,  96, 200,  72, 232, 104, 194,  66, 226,  98, 202,  74, 234, 106, 192, 
176,  16, 144,  56, 184,  24, 152,  50, 178,  18, 146,  58, 186,  26, 154,  48, 
112, 208,  80, 248, 120, 216,  88, 242, 114, 210,  82, 250, 122, 218,  90, 240, 
140,  44, 172,   4, 132,  36, 164,  14, 142,  46, 174,   6, 134,  38, 166,  12, 
 76, 236, 108, 196,  68, 228, 100, 206,  78, 238, 110, 198,  70, 230, 102, 204, 
188,  28, 156,  52, 180,  20, 148,  62, 190,  30, 158,  54, 182,  22, 150,  60, 
124, 220,  92, 244, 116, 212,  84, 254, 126, 222,  94, 246, 118, 214,  86, 252, 
131,  35, 163,  11, 139,  43, 171,   1, 129,  33, 161,   9, 137,  41, 169,   3, 
 67, 227,  99, 203,  75, 235, 107, 193,  65, 225,  97, 201,  73, 233, 105, 195, 
179,  19, 147,  59, 187,  27, 155,  49, 177,  17, 145,  57, 185,  25, 153,  51, 
115, 211,  83, 251, 123, 219,  91, 241, 113, 209,  81, 249, 121, 217,  89, 243, 
143,  47, 175,   7, 135,  39, 167,  13, 141,  45, 173,   5, 133,  37, 165,  15, 
 79, 239, 111, 199,  71, 231, 103, 205,  77, 237, 109, 197,  69, 229, 101, 207, 
191,  31, 159,  55, 183,  23, 151,  61, 189,  29, 157,  53, 181,  21, 149,  63, 
};

int dispmode, kbdmode, tekmargin, currfont, penstate, flash_state, flashmode,
    dim_state, zaxis, drawallowed, bypass, intensity, aplmode, dithermode,
    lastmouse, smear, F_DRAW, lockfont;
char *linestyle;
Point tekcursor,tekhairs,jerqcursor,jerqhairs;
Point jerqorigin,jerqsize;
Rectangle jerqrect;

int DELimpliesLOY = 0, LFeffect = 0, CReffect = 0,
    GINcount = 5, DIMallowed = ALLOWED;

char *switchtext[]=
	{"local","page","reset","font","flash","random",
	 "focus","shift","exit",NULL};
Menu switchmenu={switchtext};

char *fonttext[]=
	{"lock  ","apl   "," jumbo"," big  "," small"," tiny ",NULL};
Menu fontmenu={fonttext};

int waitforchar() 
{

	int perm,xoff;
	int c,delay;

	perm = RCV|KBD|MOUSE;
	xoff = 0;
/*	flash_state = ALLOWED;*/
	flash_state = DISALLOWED;
	if (dispmode == ALPHA)
		dim_state = DIMallowed;
	else
		dim_state = DISALLOWED;
	for(;;) {
/* 
 *		if (got&ALARM) {
 *			if (flash_state == ALLOWED) {
 *				flash_state = flash();
 *				alarm(6);
 *			} else if (flash_state == FLASHED) {
 *				unflash();
 *				flash_state = DISALLOWED;
 *				alarm(60);
 *			} else if (dim_state == ALLOWED) {
 *				dim_state = PENDING;
 *				alarm(60);
 *				delay = 90;
 *			} else if ((dim_state == PENDING) && (--delay == 0)) {
 *				dim_state = dim();
 *				alarm(60);
 *		}
 */
		wait(perm);
		if (P->state&RESHAPED) {
			c = reshape();
			if (c == OUTMODED)
				break;
		}
		if (button123()) {
			if button3()
				continue;
			else if ((c = dobuttons()) != ALLOWED)
				break;
		}
		if (P->state&KBD) {
			c = kbdchar();
/*
 *			if (c == 0xB2) {
 *				addmagic(ESC);
 *				addmagic(FF);
 *			} else
 */
			if (dispmode == GIN) {
				if (kbdmode == REMOTE) {
					/*sendchar(c);*/
					reportcursor(c, tekhairs);
					dispmode = ALPHA;
					c = OUTMODED;
					break;
				} /* else
					throw character away; */
			} else {
				if (kbdmode == LOCAL)
					break;
				if (c == DC3) {
					perm &= ~RCV;
					xoff = 1;
				} else {
					if (xoff) {
						xoff = 0;
						perm |= RCV;
						if (c != DC1)
							sendchar(c);
					} else
						sendchar(c);
				}
			}
		}
		if ((c = magicchar()) != -1)
			break;
		if ((perm&RCV) && P->state&RCV) {
			c = rcvchar();
			if (kbdmode != LOCAL)
				break;
		}
	}
/*	if (flash_state == FLASHED)
 *		unflash();
 */
	if (dim_state == DIMMED)
		undim();
	return c;
}

int dim() {
	texture(&display,jerqrect,&grey,F_XOR);
	return DIMMED;
}

undim() {
	texture(&display,jerqrect,&grey,F_XOR);
}

int magicchar() {
	if (magicbot < magictop)
		return *magicbot++;
	magicbot = magictop = magicbase;
	return -1;
}

addmagic(c)
char c;
{
	*magictop++ = c;
}

clearmagic()
{
	magicbot = magictop = magicbase;
}

Point jerqtotek(p)
Point p;
{
	Point q;

	q.x = muldiv(p.x - jerqorigin.x,TEKXMAX,jerqsize.x);
	q.y = TEKYMAX - muldiv(p.y - jerqorigin.y,TEKYMAX,jerqsize.y);
	if (q.x < 0)
		q.x = 0;
	if (q.x >= TEKXMAX)
		q.x = TEKXMAX-1;
	if (q.y < 0)
		q.y = 0;
	if (q.y >= TEKYMAX)
		q.y = TEKYMAX-1;
	return q;
}

static int *xtab, *ytab;

maketable()
{
	int x, y, *p;
	for (x=0, p=xtab; x<TEKXMAX; x++)
		*p++ = jerqorigin.x + muldiv(x,jerqsize.x,TEKXMAX);
	for (y=0, p=ytab; y<TEKYMAX; y++)
		*p++ = jerqorigin.y + muldiv(TEKYMAX-y,jerqsize.y,TEKYMAX);
}

Point tektojerq(p)
Point p;
{
	Point q;

	q.x = xtab[p.x];
	q.y = ytab[p.y];
	return q;
}

reportcursor(c, p)
Point p;
{
	static char buf[]={' ',' ',' ',' ',' ',CR,EOT};

	buf[0] = c;
	buf[1] = 040 | (p.x>>7);
	buf[2] = 040 | ((p.x>>2)&037);
	buf[3] = 040 | (p.y>>7);
	buf[4] = 040 | ((p.y>>2)&037);
	if (c)
		sendnchars(GINcount+1,buf);
	else
		sendnchars(GINcount,buf+1);
}

int reshape() {
	int omode;
	P->state &= ~RESHAPED;
	jerqorigin = add(display.rect.origin,Pt(5,7));
	jerqsize = sub(sub(display.rect.corner,Pt(5,7)), jerqorigin);
	jerqrect = inset(display.rect,3);
	maketable();
	jerqcursor = tektojerq(tekcursor);
	jerqhairs = tektojerq(tekhairs);
	omode = dispmode;
	clearscreen();
	if (omode == GIN)
		hairs(jerqhairs);
	addmagic(ESC);
	addmagic(FF);
	return OUTMODED;
}

hairs(p)
Point p;
{
	texture(&display,Rect(0,p.y,Drect.corner.x,p.y+1),&hdots,F_XOR);
	texture(&display,Rect(p.x,0,p.x+1,Drect.corner.y),&vdots,F_XOR);
}

dofontmenu() {
	Cursor *old;
	int which, oldfont;
	static char *whichmagic = "89:;";
	if (lockfont) {
		switchtext[3] = "font";
		lockfont = 0;
		return;
	}
	old = cursswitch(&menu2);
	while (!button123())
		wait(MOUSE);
	if (button13()) {
		cursswitch(old);
		return;
	}
	if (aplmode)
		fonttext[1] = "ascii ";
	else
		fonttext[1] = "apl   ";
	oldfont = currfont - aplmode + 2;
	fonttext[oldfont][0] = '>';
	cursswitch((Texture *)0);
	switch(which = menuhit(&fontmenu,2)) {
		case 0:
			lockfont = 1;
			switchtext[3] = "unlock font";
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			addmagic(ESC);
			addmagic(whichmagic[which-2]);
			break;
		case 1:
			addmagic(ESC);
			if (aplmode)
				addmagic(SI);
			else
				addmagic(SO);
			break;
	}
	fonttext[oldfont][0] = ' ';
	cursswitch(old);
}

int dobuttons() {
/* returns ALLOWED if waitforchar should continue; else char to return */
	Cursor *old;
	Point p;
	if (button1() && (dispmode == GIN) &&(flash_state == DISALLOWED)) {
		old = cursswitch(&none);
		while (button1()) {
			p = mouse.xy;
			if ((!eqpt(p,jerqhairs)) && ptinrect(p,jerqrect)) {
				hairs(jerqhairs);
				jerqhairs = p;
				tekhairs = jerqtotek(jerqhairs);
				hairs(jerqhairs);
			}
			wait(MOUSE);
		}
		cursswitch(old);
		return ALLOWED;
	}
	if (button2()) {
		if (dim_state == DIMMED) {
			switchtext[7] = "shift";
			switchtext[8] = "exit";
			switchtext[9] = NULL;
		} else {
			switchtext[7] = "exit";
			switchtext[8] = NULL;
		}
		switch(menuhit(&switchmenu,2)) {
			case 0:
				if (kbdmode == REMOTE) {
					switchtext[0] = "line";
					kbdmode = LOCAL;
				} else {
					switchtext[0] = "local";
					kbdmode = REMOTE;
				}
				return ALLOWED;
			case 1:
				addmagic(ESC);
				addmagic(FF);
				return ALLOWED;
			case 2:
				dispmode = RESET;
				return OUTMODED;
			case 3:
				dofontmenu();
				return ALLOWED;
			case 4:
				if (flashmode) {
					switchtext[4] = "flash";
					flashmode = 0;
				} else {
					switchtext[4] = "no flash";
					flashmode = 1;
				}
				return ALLOWED;
			case 5: 
				if (dithermode) {
					switchtext[5] = "dither";
					dithermode = 0;
				} else {
					switchtext[5] = "random";
					dithermode = 1;
				}
				return ALLOWED;
			case 6: if (smear) {
					switchtext[6] = "smear";
					smear = 0;
				} else {
					switchtext[6] = "focus";
					smear = 1;
				}
				return ALLOWED;
			case 7:
				if (dim_state == DIMMED)
					return DISALLOWED;
				/* else fall through */
			case 8:
				old = cursswitch(&sunset);
				while (!button123())
					wait(MOUSE);
				while (button3())
					wait(MOUSE);
				cursswitch(old);
				if (button12()) {
					while (button123())
						wait(MOUSE);
					return ALLOWED;
				}
				dispmode = EXIT;
				return OUTMODED;
		}
	}
	return ALLOWED;
}

statemachine()
{
	for(dispmode = RESET; dispmode != EXIT; ) {
		switch(dispmode) {
			case RESET:
				resetmode();
				break;
			case ALPHA:
				alphamode();
				break;
			case GIN:
				ginmode();
				break;
			case GRAPH:
			case POINT:
				graphmode();
				break;
			case SPECIALPOINT:
				specialpointmode();
				break;
			case INCREMENTAL:
				incrementalmode();
				break;
		}
	}
}

resetmode() {
	intensity = 100;
	if (!lockfont) {
		currfont = 0;
		aplmode = 0;
		defont = *font[currfont];
#ifdef X11
		XSetFont(dpy, gc, defont.fid);
#endif X11
	}
	homecursor();
	dispmode = ALPHA;
	zaxis = NORMAL;
	linestyle = styles[0];
/*	flash_state = ALLOWED; */
	/*flashmode = 0;*/
}

int controlchar(c)
int c;
{
	if (c == OUTMODED)
		return;
	switch(c) {
		case BEL:
			ringbell();
			bypass = 0;
			return 0;
		case BS:
		case HT:
		case VT:
			bypass = 0;
			if (dispmode == ALPHA)
				return c;
			return 0;
		case CR:
			if (dispmode == ALPHA) {
				if (CReffect)
					addmagic(NL);
				return c;
			}
			dispmode = ALPHA;
			bypass = 0;
			addmagic(c);
			if (CReffect)
				addmagic(NL);
			return OUTMODED;
		case FS:
			if ((dispmode == ALPHA) || (dispmode == GRAPH)) {
				dispmode = POINT;
				return OUTMODED;
			}
			return 0;
		case GS:
			if ((dispmode == ALPHA) || (dispmode == GRAPH)) {
				dispmode = GRAPH;
				drawallowed = 0;
				return OUTMODED;
			}
			return 0;
		case NL:
			if (LFeffect)
				addmagic(CR);
			if (dispmode == ALPHA)
				return c;
			return 0;
		case RS:
			if (dispmode != GIN) {
				dispmode = INCREMENTAL;
				penstate = 0;
				return OUTMODED;
			}
			return 0;
		case US:
			if (dispmode == ALPHA)
				return 0;
			dispmode = ALPHA;
			bypass = 0;
			return OUTMODED;
		case ESC:
			return escmode();
		default:
			if (c < 040)
				return 0;
			return c;
	}
}

int escmode() {
	int c,ignore;

	do {
		c = waitforchar();
		ignore = 0;
		switch(c) {
		case CAN:
			bypass = 1;
			return 0;
		case OUTMODED:
		case CR:
			ignore = 1;
			break;
		case ENQ:
			bypass = 1;
			if (dispmode != GIN) {
				/*sendchar(status());*/
				reportcursor(status(), tekcursor);
				return 0;
			} 
			reportcursor(0, tekhairs);
			dispmode = ALPHA;
			return OUTMODED;
		case ETB:
			/* Hard copy */
			bypass = 0;
			return 0;
		case FF:
			clearscreen();
			homecursor();
			bypass = 0;
			dispmode = ALPHA;
			return OUTMODED;
		case FS:
			if ((dispmode == INCREMENTAL) || ( dispmode == GIN))
				return 0;
			dispmode = SPECIALPOINT;
			return OUTMODED;
		case SI:
			
			if (aplmode && !lockfont) {
				currfont -= aplmode;
				aplmode = 0;
				goto outfont;
			}
			return 0;
		case SO:
			if (!aplmode && !lockfont) {
				aplmode = 4;
				currfont += aplmode;
				goto outfont;
			}
			return 0;
		case SUB:
			dispmode = GIN;
			bypass = 1;
			return OUTMODED;
		case '8':
			if (!lockfont) {
				currfont = 0 + aplmode;
				goto outfont;
			}
			return 0;
		case '9':
			if (!lockfont) {
				currfont = 1 + aplmode;
				goto outfont;
			}
			return 0;
		case ':':
			if (!lockfont) {
				currfont = 2 + aplmode;
				goto outfont;
			}
			return 0;
		case ';':
			if (!lockfont) {
				currfont = 3 + aplmode;
outfont:
				defont = *font[currfont];
#ifdef X11
				XSetFont(dpy, gc, defont.fid);
#endif X11
			}
			return 0;
		default:
			if ((c=='?') && (DELimpliesLOY))
				return DEL;
			if ((c<'`')||(c>'w'))
				break;
			c -= '`';
			if (((c + 1) & 7) >= 6)
				break;
			zaxis = c / 8;
				/* zaxis = 0 ==> Normal
				 *	 = 1 ==> Defocused
				 *	 = 2 ==> Write-through
				 */
			if ((c + 1) & 7)
				linestyle = styles[c & 7];
				/*	 = 0 ==> solid
				 *	 = 1 ==> dotted
				 *	 = 2 ==> dot-dash
				 * 	 = 3 ==> short-dash
				 * 	 = 4 ==> long-dash
				 */
			return 0;
		}
	} while (ignore);
	return 0;
}

ginmode() {
	int c;
	hairs(jerqhairs);
	c = waitforchar();
	hairs(jerqhairs);
	c = controlchar(c);
}

specialpointmode() {
	int c;
	static char intents[] = {
	 14, 16, 17, 19, 20, 22, 23, 25, 28, 31, 34, 38, 41, 44, 47, 50,
/*	SP , ! , " , # , $ , % , & , ' , ( , ) , * , + , , , - , . , / ,	*/
	 56, 62, 69, 75, 81, 88, 94,100, 56, 62, 69, 75, 81, 88, 94,100,
/*	 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 , : , ; , < , = , > , ? ,	*/
	  0,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  3,  3,  3,  3,
/*	 @ , A , B , C , D , E , F , G , H , I , J , K , L , M , N , O ,	*/
	  4,  4,  4,  5,  5,  5,  6,  6,  7,  8,  9, 10, 11, 12, 12, 13,
/*	 P , Q , R , S , T , U , V , W , X , Y , Z , [ , \ , ] , ^ , _ ,	*/
	 14, 16, 17, 19, 20, 22, 23, 25, 28, 31, 34, 38, 41, 44, 47, 50,
/*	 ` , a , b , c , d , e , f , g , h , i , j , k , l , m , n , o ,	*/
	 56, 62, 69, 75, 81, 88, 94,100, 56, 62, 69, 75, 81, 88, 94,100,
/*	 p , q , r , s , t , u , v , w , x , y , z , { , | , } , ~ ,DEL,	*/
	};

	if (((c = waitforchar()) < 040) || (c >= '~')) {
		c = controlchar(c);
		return;
	}
	intensity = intents[c-' '];
	intensity = muldiv(intensity, 200-intensity, 100);
	graphmode();
}

intent(p)
Point p;
{
	int i,j,cnt=0;
	for (i=0;i<=smear;i++)
		for (j=0;j<=smear;j++)
			if (dithermode) {
				if ( (16*16*intensity) > 
				     (100*dither[(p.y+i)%16][(p.x+j)%16]) )
					cnt++;
			} else {
				if (intensity > (rand()%100))
					cnt++;
			}
	return cnt;
}

graphmode() {
	register b,c;
	static Point ojcurs;
	static hix, hiy, lox, loy, extra;
	register int numdots;

	if ((c = waitforchar()) < 040) {
		c = controlchar(c);
		return;
	}
	if ((c & 0140) == 040) {	/* new hiy */
		hiy = c & 037;
		do
			if (((c = waitforchar()) < 040) &&
			    ((c = controlchar(c)) == OUTMODED))
				return;
		while (c == 0);
	}
	if ((c & 0140) == 0140) {	/* new loy */
		b = c & 037;
		do
			if (((c = waitforchar()) < 040) &&
			    ((c = controlchar(c)) == OUTMODED))
				return;
		while (c == 0);
		if ((c & 0140) == 0140) {	/* no, it was extra */
			extra = b;
			loy = c & 037;
			do
				if (((c = waitforchar()) < 040) &&
				    ((c = controlchar(c)) == OUTMODED))
					return;
			while (c == 0);
		}
		else
			loy = b;
	}
	if ((c & 0140) == 040) {	/* new hix */
		hix = c & 037;
		do
			if (((c = waitforchar()) < 040) &&
			    ((c = controlchar(c)) == OUTMODED))
				return;
		while (c == 0);
	}
	lox = c & 037;		/* this should be lox */
	if (extra & 020)
		tekmargin = TEKXMAX/2;
	tekcursor.x = (hix<<7) | (lox<<2) | (extra & 03);
	tekcursor.y = (hiy<<7) | (loy<<2) | ((extra & 014)>>2);
	jerqcursor = tektojerq(tekcursor);
	if (dispmode == GRAPH) {
		if (drawallowed) {
			if (zaxis != WRITETHRU) {
				dsegment(&display,ojcurs,jerqcursor,
					 F_DRAW, linestyle);
				point(&display,jerqcursor,F_DRAW);
			}
			saveline(ojcurs,jerqcursor,linestyle);
		}
	} else {
		if ((numdots = intent(tekcursor)) >= 4)
			point(&display,sub(jerqcursor,Pt(0,1)),F_DRAW);
		if (numdots >= 3)
			point(&display,sub(jerqcursor,Pt(1,0)),F_DRAW);
		if (numdots >= 2)
			point(&display,sub(jerqcursor,Pt(1,1)),F_DRAW);
		if (numdots >= 1)
			point(&display,jerqcursor,F_DRAW);
		savepoint(jerqcursor);
	}
	ojcurs = jerqcursor;
	drawallowed = 1;
}

incrementalmode() {
	int c;
	register int numdots;

	if ((c = waitforchar()) == OUTMODED)
		return;
	if ((c < 040) && ((c = controlchar(c)) <= 0))
			return;
	if (c&040)
		penstate = 0;
	if (c&020)
		penstate = 1;
	if (c&04)
		tekcursor.y++;
	if (c&010)
		tekcursor.y--;
	if (c&01)
		tekcursor.x++;
	if (c&02)
		tekcursor.x--;
	jerqcursor = tektojerq(tekcursor);
	if (penstate == 0)
		return;
	if ((numdots = intent(tekcursor)) >= 4)
		point(&display,sub(jerqcursor,Pt(0,1)),F_DRAW);
	if (numdots >= 3)
		point(&display,sub(jerqcursor,Pt(1,0)),F_DRAW);
	if (numdots >= 2)
		point(&display,sub(jerqcursor,Pt(1,1)),F_DRAW);
	if (numdots >= 1)
		point(&display,jerqcursor,F_DRAW);
	savepoint(jerqcursor);
}

alphamode() {
	int c;

	c = waitforchar();
	if (c == OUTMODED)
		return;
	if ((c < 040) && ((c = controlchar(c)) <= 0))
			return;
	if (bypass)
		return;
	switch(c) {
		case DEL:
			/* ignore */
			return;
		case BS:
			if ((tekcursor.x -= width[currfont]) < tekmargin)
				tekcursor.x = TEKXMAX - width[currfont];
			break;
		case NL:
			tekcursor.y -= height[currfont];
			break;
		case CR:
			tekcursor.x = tekmargin;
			break;
		case HT:
			tekcursor.x += width[currfont];
			break;
		case VT:
			if ((tekcursor.y += height[currfont]) >= TEKYMAX)
				tekcursor.y = 0;
			break;
		case ' ':
			tekcursor.x += width[currfont];
			break;
		default:
			if (c <= ' ')
				return;
			if (zaxis != WRITETHRU)
				drawchar(currfont,c,jerqcursor,F_DRAW);
			savechar(currfont,c,jerqcursor);
			tekcursor.x += width[currfont];
			break;
	}
	if (tekcursor.x >= TEKXMAX) {
		tekcursor.x = tekmargin;
		tekcursor.y -= height[currfont];
	}
	if (tekcursor.y < 0) {
		tekcursor.y = TEKYMAX - height[currfont];
		tekcursor.x -= tekmargin;
		tekmargin = (TEKXMAX/2) - tekmargin;
		if ((tekcursor.x += tekmargin) > TEKXMAX)
			tekcursor.x -= tekmargin;
	}
	jerqcursor = tektojerq(tekcursor);
}

drawchar(f,c,p,m)
int f,m;
Point p;
char c;
{
	static char s[2];

	s[0] = c;
	string(font[f], s , &display,
#ifdef X11
		Pt(p.x,p.y-font[f]->max_bounds.ascent), m);
#endif X11
#ifdef SUNTOOLS
		Pt(p.x,p.y+(*font[f])->pf_char[0].pc_home.y), m);
#endif SUNTOOLS

}

clearscreen(){
	savereset();
	flash_state = DISALLOWED;
	dim_state = DISALLOWED;
	rectf(&display,Drect,F_CLR);
	homecursor();
}

homecursor(){
	bypass = 0;
	tekcursor.x = 0;
	tekcursor.y = TEKYMAX - height[currfont];
	jerqcursor = tektojerq(tekcursor);
	tekmargin = 0;
}

int status(){
	int s;
	s = 33;
	if (dispmode == GRAPH)
		s |= 8;
	if (dispmode == ALPHA)
		s |= 4;
	if (tekmargin)
		s |= 2;
	return s;
}

errorabort(s)
char *s; {
	sendnchars(strlen(s),s);
	sleep(5);
	exit();
}

main(argc,argv)
int argc;
char *argv[];
{
	char *s;
	Font savedefont;
	extern int childid;
	static Font sfonts[8];

	kbdmode = REMOTE;
	startshell();
	initdisplay(argc, argv);
	initcursors();
	savedefont = defont;
#ifdef X11
	font[0] = XLoadQueryFont(dpy, "9x15");
	font[1] = &savedefont;
	font[2] = XLoadQueryFont(dpy, "6x12");
	font[3] = XLoadQueryFont(dpy, "6x10");
#endif X11
#ifdef SUNTOOLS
	sfonts[0] = pf_open("/usr/lib/fonts/tekfonts/tekfont0");
	sfonts[1] = pf_open("/usr/lib/fonts/tekfonts/tekfont1");
	sfonts[2] = pf_open("/usr/lib/fonts/tekfonts/tekfont2");
	sfonts[3] = pf_open("/usr/lib/fonts/tekfonts/tekfont3");
	font[0] = &sfonts[0];
	font[1] = &sfonts[1];
	font[2] = &sfonts[2];
	font[3] = &sfonts[3];
#endif SUNTOOLS
	font[4] = font[0];
	font[5] = font[1];
	font[6] = font[2];
	font[7] = font[3];
	xtab = (int *)alloc(TEKXMAX*sizeof(int));
	ytab = (int *)alloc(TEKYMAX*sizeof(int));
	dithermode = 1;
	smear = 1;
	lockfont = 0;
	tekhairs.x = TEKXMAX/2;
	tekhairs.y = TEKYMAX/2;
	while (--argc)
		if ((s = argv[argc]) && (*s) && (*s++ == '-'))
			while (*s)
				switch(*s++) {
					case 'd':
						DELimpliesLOY = 1;
						break;
					case 'l':
						LFeffect = 1;
						CReffect = 0;
						break;
					case 'c':
						CReffect = 1;
						LFeffect = 0;
						break;
					case 'g':
						GINcount = 4;
						break;
					case 'e':
						GINcount = 6;
						break;
					case 'u':
						DIMallowed = DISALLOWED;
						break;
				}
	DIMallowed = DISALLOWED;
	F_DRAW = F_OR;
	statemachine();
	sendchar(NL);
	kill(childid, SIGHUP);
}

initcursors()
{
	menu2 = ToCursor(menu2_bits, menu2_bits, 7, 7);
	sunset = ToCursor(sunset_bits, sunset_bits, 7, 7);
	none = ToCursor(none_bits, none_bits, 7, 7);
	vdots = ToTexture(vdots_bits);
	hdots = ToTexture(hdots_bits);
	grey = ToTexture(grey_bits);
}
