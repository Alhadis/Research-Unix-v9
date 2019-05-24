#include <jerq.h>

#define kbdcurs(p)	rectf(&display,Rect(p.x,p.y,p.x+1,p.y+fontheight(&defont) ),F_XOR)

static echo(c,p)
Point p;
{
	static char buf[2];

	buf[0] = c;
	string(&defont, buf, &display, p, F_XOR);
}


kbdstring(str,nchmax,p)	/* read string from keyboard with echo at p */
register char *str; int nchmax; Point p;
{
	register int kbd, nchars = 0;

	*str = 0; kbdcurs(p);
	for (;;) {
		wait(KBD);
		kbdcurs(p);
		switch (kbd = kbdchar()) {
		case 0:
			break;
		case '\r':
		case '\n':
			return nchars;
		case '\b':
			if (nchars <= 0) break;
			kbd = *--str; *str = 0; nchars--;
			p.x -= fontwidth(&defont);
			echo(kbd, p);
			break;
		default:
			if (nchars >= nchmax) break;
			*str++ = kbd; *str = 0; nchars++;
			echo(kbd, p);
			p.x += fontwidth(&defont);
		}
		kbdcurs(p);
	}
}
