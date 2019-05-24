#include <jerq.h>
#include <sys/ioctl.h>
#include "frame.h"

extern struct sgttyb	sttymodes;
extern struct tchars	tcharmodes;

/*
 * Keyboard-dependent things
 */

#define	CTRLW		0x17
#define	NSCRL	2
int	HIWATER;	/* #chars max to save off top of screen */
int	LOWATER;
int doubleclickOK;

#define BLOCKED	1
int Pstate;

typedef struct Type{
	Frame	*frame;		/* Frame being typed at */
	short	start;		/* start pos. of text to be sent to host */
	short	nchars;		/* number of chars typed */
	String	old;		/* old text scrolled off top */
	char	scroll;
}Type;

String		buf;
#define		M	1	/* margin around frame */

windowproc(){
	register Frame *f;
	register got;
	String rcvstr;
	static String zerostr;
	Type current;
	bufinit();
	if((current.frame=f=fralloc(Drect, M))==0)
		exit(1);
	frinit(f);
	current.start=0;
	current.nchars=0;
	current.scroll=1;
	current.old=zerostr;
	insure(&current.old, 128);
	rcvstr=zerostr;
	insure(&rcvstr, 128);
	request(RCV|KBD|MOUSE);
	cursswitch((Texture *)0);
	setscroll(&current);
	for(;;){
		wait(MOUSE|KBD|RCV);
		if(P->state&RESHAPED){
			frsetrects(f, Drect);
			drawframe(f);	/* sets complete */
			if(!complete && current.scroll)
				advance(&current);
			P->state&=~RESHAPED;
		} else
			linecurse(f);	/* turn it off */
		if(button123())
			wbuttons(&current);
		else if(P->state & KBD)
			kbd(&current, kbdchar());
		else if(P->state & RCV)
			rcv(&current, &rcvstr);
		linecurse(f);	/* cursor is off, so turn it on */
	}
}
bufinit(){
	strzero(&buf);
	HIWATER=10000, LOWATER=8000;
}

rcv(t, rcvstr)
	Type *t;
	register String *rcvstr;
{
	register char *p;
	register Frame *f=t->frame;
	register i, posn, c;
	int compl;
	/* Read the string */
	c=rcvchar();
loop:
	while(c=='\b'){
		if(t->start > 0)
			deltype(t, t->start-1, t->start);
		c=rcvchar();
	}
	p=rcvstr->s;
	for(i=0; c!=-1; c=rcvchar()){	/*aplterm1*/
		c&=0x7F;
		if(c == '\b') break;
		if(c=='\7')
			ringbell();	/* smashes keyclick? */
		if(c && c!='\r' && c<=fontnchars(&defont)){
			*p++=c;
			if(++i==rcvstr->size)
				break;
		}
	}
	rcvstr->n=i;
	posn=t->start;
	if(f->s1<posn && posn<f->s2){
		t->start=posn=f->s1;	/* before selected text; avoids problems */
		t->nchars=0;
	}
	/* Undraw selection if necessary */
	if(posn<f->s2 && f->s2>f->s1)
		selectf(f, F_XOR);
	/* Find where it goes */
	instext(f, rcvstr, posn);
	compl=inscomplete;
	/* Adjust the selection and typing location */
	if(posn<f->s2 || (f->s1==f->s2 && posn==f->s2)){
		f->s1+=i;
		f->s2+=i;
	}
	t->start+=i;
	if(posn+i<f->s2 && f->s2>f->s1)
		selectf(f, F_XOR);
	if(!compl){
		if(t->scroll)
			advance(t);
		else
			Pstate|=BLOCKED;	/* so cat hugefile is safe */
	}
	if(c=='\b')
		goto loop;
}
linecurse(t)
	register Frame *t;
{
	Point p;
	if(t && t->str.s && t->s1==t->s2){
		p=ptofchar(t, t->s1);
		Rectf(Rpt(p, Pt(p.x+1, p.y+newlnsz)), F_XOR);
	}
}

kbd(t, ac)
	register Type *t;
	char ac;
{
	static int raw, echo, cbreak, crmod; /* can be static; we don't sleep here */
	register Frame *f=t->frame;
	register struct tchars *tp= &tcharmodes;
	int tounix;
	unsigned char c;
	c=ac;
	if(f->s2>f->s1)
		cut(t, 1);
	if(f->s2>=t->start){
		tounix=TRUE;
		raw= sttymodes.sg_flags & RAW;
		cbreak= sttymodes.sg_flags & CBREAK;
		crmod= sttymodes.sg_flags & CRMOD;
		echo= sttymodes.sg_flags & ECHO;
	}else{
		tounix=FALSE;
		raw=FALSE;
		cbreak=FALSE;
		crmod=TRUE;
		echo=TRUE;
	}
	if(raw){
		/* flush input */
		t->start+=t->nchars;
		t->nchars=0;
    Noecho:
		sendchar(c);
		return;		/* no echo in raw mode */
	}
	if(c==tp->t_intrc || c==tp->t_quitc){	/* always kill */
		/* flush input */
		t->start=f->str.n;
		t->nchars=0;
		dosig(c==tp->t_intrc? 2 : 3);
		return;
	}
	/* never raw mode */
	if(!echo && t->nchars>0){	/* send off what's already there */
		sendsubstr(f->str.s, t->start, t->nchars-t->start);
		t->nchars=0;
		t->start=f->str.n;
	}
	c&=0x7F;	/* strip off sign bit from keypad */	/*aplterm2*/
	if(c==tp->t_eofc && !cbreak){
		if(tounix)
			goto Send;	/* throw away eofc */
		return;
	}
	if(c=='\r' && crmod)
		c='\n';
	if(!echo)
		goto Noecho;
	if(f->s2>0 && f->str.s[f->s2-1]=='\\' &&
	  (c==sttymodes.sg_erase || (c==sttymodes.sg_kill&&tounix) || c==CTRLW ||
	   c==tp->t_stopc || c==tp->t_startc)){
		deltype(t, f->s2-1, f->s2);	/* throw away \ */
		goto Ordinary;
	}
	if(c==sttymodes.sg_erase || c==CTRLW){
		if(f->s1 > (tounix? t->start : 0))
			deltype(t, f->s1-nback(f, c, tounix? t->start : 0), f->s1);
		return;
	}
	if(c==sttymodes.sg_kill && tounix){
		f->s2=f->str.n;
		inschar(t, '@');
		inschar(t, '\n');
		t->start=f->s2;
		t->nchars=0;
		return;
	}
	if(c==tp->t_stopc){
		Pstate|=BLOCKED;
		return;
	}
	if(c==tp->t_startc){
		Pstate&=~BLOCKED;
		return;
	}
    Ordinary:
	inschar(t, c);
	if(tounix && (Pstate&BLOCKED))
		Pstate&=~BLOCKED;
	if(tounix && f->s2>t->start && (c=='\n' || (cbreak && c=='\r'))){
    Send:
		sendsubstr(f->str.s, t->start, f->s2);
		t->nchars-=f->s2-t->start;
		t->start+=f->s2-t->start;
	}
}
nback(f, c, lim)
	Frame *f;
{
	register n=0, s1=f->s1;
	register char *s=f->str.s+s1;
	static char alphanl[]=
	    "\n0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
#define	alphanum	alphanl+1
	if(s1 <= 0)
		return 0;
	if(c!=CTRLW || *--s=='\n')
		return 1;
	/* else it's ^W. first, get to an alphanumeric (or newline) */
	while(n<s1-lim && notin(*s, alphanl))
		--s, ++n;
	/* *s is alphanumeric or space; now back up to non-alphanumeric */
	while(n<s1 && !notin(*s, alphanum))
		--s, ++n;
	return n;
}
notin(c, s)
	register c;
	register char *s;
{
	c&=0xFF;
	while(*s)
		if(c == *s++)
			return FALSE;
	return TRUE;
}
inschar(t, ac)
	register Type *t;
{
	static char c;
	static String kbdstr={
		&c,
		1,
		1
	};
	register Frame *f=t->frame;
	c=ac;
	instype(t, &kbdstr, f->s2);
	if(!inscomplete)
		scrlf(t, getto(f, f->s2));
	setsel(f, f->s2+1);
}
getto(f, end)
	register Frame *f;
{
	register nl, nc, i;
	nl=max(NSCRL, f->nlines/5);
	nc=charofpt(f, f->rect.corner);
	while(nc<end)
		if(f->str.s[nc++]=='\n')
			nl++;
	/* nl is now #lines; find how many chars from start of frame */
	for(i=nc=0; i<end; )
		if(f->str.s[i++]=='\n'){
			nc=i;
			if(--nl<=0)
				return nc;
		}
	return end;
}
advance(t)
	Type *t;
{
	/* should work first time unless lines are folded */
	do; while(!scrlf(t, getto(t->frame, t->frame->str.n)));
}
insertstring(p, i, s, n)	/* warning: must call insure() before this! */
	String *p;
	char *s;
{
	String str;
	str.s=s, str.n=n;
	insstring(p, i, &str);
}
inserttype(t, s, n)		/* warning: must call insure() before this! */
	Type *t;
	char *s;
{
	register Frame *f=t->frame;
	String str;
	str.s=s, str.n=n;
	selectf(f, F_XOR);
	instext(f, &str, 0);
	f->s1+=n;
	f->s2+=n;
	t->start+=n;
	selectf(f, F_XOR);
}
scrlf(t, n)
	register Type *t;
	register n;
{
	register Frame *f=t->frame;
	if(n>0){
		if(n>f->str.n)
			n=f->str.n;
		sendoldtext(t, n);
		/* quick hack; don't worry about order of allocation */
		insure(&t->old, t->old.n+n);
		insertstring(&t->old, t->old.n, f->str.s, n);
		if(t->old.n>HIWATER)
			delstring(&t->old, 0, t->old.n-LOWATER);
		if(n=deltype(t, 0, n))	/* all text in frame now visible */
			Pstate&=~BLOCKED;
	}
	setscroll(t);
	return n;
}
scrlb(t, n)
	register Type *t;
	register n;
{
	register Frame *f=t->frame;
	register i;
	if(n>0){
		if(n>t->old.n)
			n=t->old.n;
		i=t->old.n-n;
		while(i>0 && t->old.s[i-1]!='\n')
			i--, n++;
		insure(&t->old, t->old.n+n);
		inserttype(t, t->old.s+i, n);
		delstring(&t->old, i, t->old.n);
	}
	setscroll(t);
}
deltype(t, s1, s2)
	register Type *t;
	register s1, s2;
{
	register Frame *f=t->frame;
	int compl;
	if(s2<=t->start)
		t->start-=s2-s1;
	else if(s1>=t->start)
		t->nchars-=s2-s1;
	else if(s2>t->start){	/* deletion overlaps start */
		t->nchars-=s2-t->start;
		t->start=s1;
	}
	selectf(f, F_XOR);
	deltext(f, s1, s2);
	compl=complete;
	f->s1-=max(0, min(f->s1, s2)-s1);
	f->s2-=max(0, min(f->s2, s2)-s1);
	selectf(f, F_XOR);
	return compl;
}
instype(t, s, s1)
	register Type *t;
	String *s;
	register s1;
{
	if(s->n>0){
		if(s1<t->start)
			t->start+=s->n;
		else if(s1>=t->start)
			t->nchars+=s->n;
		instext(t->frame, s, s1);
	}
}

#define	CUT		0
#define	PASTE		1
#define	SNARF		2
#define	SENDIT		3
#define	SCROLL		4

#define	UP		0
#define	DOWN		1

static char *editstrs[]={
	"cut",
	"paste",
	"snarf",
	"send",
	0,
	0,
};
static	Menu	editmenu={editstrs};

wbuttons(t)
	register Type *t;
{
	register Frame *f=t->frame;
	static char *scrollstrs[]={ "scroll", "noscroll" };
	if(!ptinrect(mouse.xy, f->totalrect)){	/* not for us anyway */
		doubleclickOK=0;
		return;
	}
	if(ptinrect(mouse.xy, f->scrollrect))
		scroll(t, mouse.buttons);
	else if(button1()){
		if(ptinrect(mouse.xy, f->rect))
			frselect(f, mouse.xy);
	}else if(button2()){
		doubleclickOK=0;
		editstrs[SCROLL]=scrollstrs[t->scroll];
		switch(menuhit(&editmenu, 2)){
		case CUT:
			cut(t, 1);
			break;
		case PASTE:
			paste(t, &buf);
			break;
		case SNARF:
			if(f->s2>f->s1)
				snarf(&f->str, f->s1, f->s2);
			break;
		case SENDIT:
			send(t);
			break;
		case SCROLL:
			if((t->scroll^=1) &&
			    (frameop(f, opnull, f->rect.origin,
			      f->str.s, f->str.n), !complete))
				advance(t);
			break;
		}
	}else{
		doubleclickOK=0;
	}
}
snarf(p, i, j)
	register String *p;
	register short i, j;
{
	register n = j-i;
	insure(&buf, n);
	movstring(n, p->s+i, buf.s);
	buf.n = n;
	setmuxbuf(&buf);
}
cut(t, save)
	register Type *t;
{
	register n;
	register Frame *f=t->frame;
	if((n=f->s1) != f->s2){
		if(save)
			snarf(&f->str, n, f->s2);
		deltype(t, f->s1, f->s2);
		f->s1=f->s2;
		setsel(f, n);
		Pstate&=~BLOCKED;	/* in case we are suspended */
	}
}
paste(t, s)
	register Type *t;
	String *s;
{
	register Frame *f=t->frame;
	getmuxbuf(s);
	if(s->n==0)
		return;
	cut(t, 0);
	instype(t, s, f->s1);
	f->s2=f->s1+s->n;
	selectf(f, F_XOR);
}
send(t)
	register Type *t;
{
	register Frame *f=t->frame;
	if(f->s1!=f->s2)
		snarf(&f->str, f->s1, f->s2);
	getmuxbuf(&buf);
	if(buf.n==0)
		return;
	if(t->nchars)
		deltype(t, t->start, t->start+t->nchars);
	if(sttymodes.sg_flags&ECHO){
		instext(f, &buf, f->str.n);
		if(!inscomplete)
			advance(t);
	}
	selectf(f, F_XOR);
	f->s1=f->s2=f->str.n;
	sendsubstr(buf.s, 0, buf.n);
	if(buf.s[buf.n-1]!='\n'){
		sendnchars(1, "\n");
		if((sttymodes.sg_flags&(RAW|CBREAK|ECHO))==ECHO)
			inschar(t, '\n');
		delim();
	}
	/* else the delim's already been sent */
	t->start=f->s2;
	t->nchars=0;
}
sendsubstr(s, beg, end)
	register char *s;
{
	register m, n;
	if(beg==end){
		delim();
		return;
	}
	for(m=beg; m<end; m=n+1){
		/* invariant: n is the last character we are going to send */
		for(n=m; n<end && s[n]!='\n'; n++)
			if(n==end-1)
				break;
		sendnchars(n-m+1, s+m);
		if(n<end && s[n]=='\n')
			delim();
	}
}
sendoldtext(t, n)
	register Type *t;
{
	if(t->start<n && t->nchars>0){
		if(n>t->start+t->nchars)
			n=t->start+t->nchars;
		sendsubstr(t->frame->str.s, t->start, n);
	}
}

short grey_bits[]={
	0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA,
	0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA,
};
Texture grey;

initcursors()
{
	grey = ToTexture(grey_bits);
}

Point
scrollclip(p)
	Point p;
{
	if(p.x<0){
		p.y-=p.x;
		p.x=0;
	}
	if(p.y>SCROLLRANGE)
		p.y=SCROLLRANGE;
	return p;
}
Rectangle
scrollmark(f, p)
	register Frame *f;
	Point p;
{
	Rectangle r;
	p=scrollclip(p);
	r=f->scrollrect;
	r.origin.y=clixtopix(f, p.x);
	r.corner.y=clixtopix(f, p.y);
	return r;
}
clixtopix(f, y)
	register Frame *f;
{
	return f->scrollrect.origin.y+muldiv(f->nlines*newlnsz, y, SCROLLRANGE);
}
pixtoclix(f, y)
	register Frame *f;
{
	return muldiv(y-f->rect.origin.y, SCROLLRANGE, f->nlines*newlnsz);
}
drawscrollbar(f)
	register Frame *f;
{
	Rectangle r;
	register Bitmap *b;
	r=inset(f->scrollrect, -1);
	b=(Bitmap *)D;
	rectf(b, r, F_OR);
	rectf(b, scrollmark(f, f->scroll), F_XOR);
	bitblt(b, r, D, r.origin, F_STORE);
}
setscroll(t)
	register Type *t;
{
	register Frame *f=t->frame;
	Point new;
	register x, y;
	x=f->rect.origin.y+1;	/* a little margin at the top */
	y=f->rect.corner.y-1;	/* and bottom */
	if(t->old.n+f->str.n>0)
		x+=muldiv(f->nlines*newlnsz, t->old.n, t->old.n+f->str.n);
	if(f->cpl[f->nlines-1]>0){
		int n;
		n=charofpt(f, Pt(Drect.corner.x, Drect.corner.y));
		y-=muldiv(f->nlines*newlnsz, f->str.n-n, t->old.n+f->str.n);
	}
	if(x>y-(f->rect.corner.y-f->rect.origin.y)/30)
		x=y-(f->rect.corner.y-f->rect.origin.y)/30;
	if(y<x+(f->rect.corner.y-f->rect.origin.y)/30)
		y=x+(f->rect.corner.y-f->rect.origin.y)/30;
	new.x=pixtoclix(f, x);
	new.y=pixtoclix(f, y);
	if(abs(f->scroll.x-new.x)>20 || abs(f->scroll.y-new.y)>20){
		f->scroll=new;
		drawscrollbar(f);
	}
}
Point
checkmouse(f, mousep, p)
	Frame *f;
	Point mousep, p;
{
	if(!ptinrect(mousep, f->scrollrect)){
		extern Rectangle Null;
		return Null.origin;
	}
	return p;
}
Point
but1func(f, p)
	register Frame *f;
	Point p;
{
	register delta=muldiv(p.y-f->rect.origin.y, f->scroll.y-f->scroll.x, f->nlines*newlnsz);
	return checkmouse(f, p, sub(f->scroll, Pt(delta, delta)));
}
Point
but2func(f, p)
	register Frame *f;
	Point p;
{
	Point scroll;
	register size=(f->scroll.y-f->scroll.x)/2;
	scroll.x=pixtoclix(f, p.y)-size;
	scroll.y=pixtoclix(f, p.y)+size;
	return checkmouse(f, p, scroll);
}
Point
but3func(f, p)
	register Frame *f;
	Point p;
{
	register delta=muldiv(p.y-f->rect.origin.y, f->scroll.y-f->scroll.x, f->nlines*newlnsz);
	return checkmouse(f, p, add(f->scroll, Pt(delta, delta)));
}
typedef Point (*ptrfpoint)();
ptrfpoint butfunc[]={
	0,
	but1func,
	but2func,
	but3func,
};
scrollbar(f, but)
	register Frame *f;
{
	ptrfpoint fp;
	Point pt;
	fp=butfunc[but];
	pt=mouse.xy;
	while(button(but)){
		texture(D, scrollmark(f, (*fp)(f, pt)), &grey, F_XOR);
		do jnap(3); while(eqpt(mouse.xy, pt) && button(but));
		texture(D, scrollmark(f, (*fp)(f, pt)), &grey, F_XOR);
		pt = mouse.xy;
	}
	if(ptinrect(pt, f->scrollrect)){
		f->scroll=(*fp)(f, pt);
		if(f->scroll.x<0){
			f->scroll.y-=f->scroll.x;
			f->scroll.x=0;
		}
		if(f->scroll.x>SCROLLRANGE){
			f->scroll.y-=f->scroll.x-SCROLLRANGE;
			f->scroll.x=SCROLLRANGE;
		}
		return 1;
	}
	return 0;
}
scroll(t, but)
	register Type *t;
{
	register Frame *f=t->frame;
	Point old, new;
	register y, b;
	old=f->scroll;
	if(scrollbar(f, b=whichbutton())){
		new=f->scroll;
		f->scroll=old;	/* ugh */
		y=mouse.xy.y;
		if(b==2){
			y=muldiv(f->str.n+t->old.n, new.x, SCROLLRANGE);
			if(y>=t->old.n){
				y-=t->old.n;
				do; while(y<f->str.n && f->str.s[y++]!='\n');
				scrlf(t, y);
			}else
				scrlb(t, t->old.n-y);
		}else if(new.x>=old.x)
				scrlf(t, charofpt(f, Pt(f->rect.origin.x, y)));
		else
			scrlb(t, oldlinepos(t, (y-f->scrollrect.origin.y)/newlnsz));
	}
}
oldlinepos(t, n)
	register Type *t;
	register n;
{
	register i=t->old.n;
	while(n>0 && i>0)
		if(t->old.s[--i]=='\n')
			--n;
	return t->old.n-i;
}
