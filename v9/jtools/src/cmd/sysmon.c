#include <jerq.h>
#include <nlist.h>
#include <ctype.h>

typedef int bool;

#undef	TRUE
#define TRUE -1
#define FALSE 0

struct ld {
    float l_runq;
    long l_cp[5];
};

#define TIMELEN 6
#define LOADLEN 13

struct nodedef {
	Rectangle bar;		/* the bar graph of system use */
	char load[LOADLEN];		/* load numbers */
	int vec[5];			/* current use vectors */
	int oldvec[5];		/* old use vectors */
	struct ld	m_old;	/* previous set of poal numbers */
	struct ld	m_new;	/* current set of load numbers */
} node0;

/* user nice sys queue idle */
short black_bits[]={
	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	
	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	
};
Texture black;
short white_bits[]={
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
};
Texture white;
short darkgrey_bits[] = {
	0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777,
	0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777, 0xDDDD, 0x7777,
};
Texture darkgrey;
short lightgrey_bits[] = {
	0x2222, 0x8888, 0x2222, 0x8888, 0x2222, 0x8888, 0x2222, 0x8888,
	0x2222, 0x8888, 0x2222, 0x8888, 0x2222, 0x8888, 0x2222, 0x8888,
};
Texture lightgrey;
short grey_bits[] = {
	0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555,
	0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555,
};
Texture grey;

#ifdef BSD
#include <sys/param.h>
#define NUMLOADBAR	4
Texture *txt[]={
	&black, &lightgrey, &darkgrey, &white
};
#else
#define NUMLOADBAR	5
Texture *txt[]={
	&black, &lightgrey, &darkgrey, &grey, &white
};
#endif BSD

/* size contraints */
#define BARHEIGHT 18
#define MINBARHEIGHT 4
#define MAXBARHEIGHT 24
#define TIMEORIGIN 2
#define NAMELEN 9
int CHARHEIGHT;
int CHARWIDTH;
int TIMECORNER;
int LOADORIGIN;
int LOADCORNER;

int size;
int barheight;

main(ac, av)
int ac;
char *av[];
{
	register i, state;
	int ticks = 5;
	int permin = 60;
	char *p;

	/* initialize load gathering */
	initload();
	gettime();
	getload();
	docpu();

	request(ALARM|KBD|MOUSE);
	initdisplay(ac, av);
	inittextures();

	CHARHEIGHT = fontheight(&defont) - 2;
	CHARWIDTH = strwidth(&defont, "m");
	TIMECORNER = NAMELEN*CHARWIDTH;
	LOADORIGIN = TIMECORNER;
	LOADCORNER = TIMECORNER + LOADLEN*CHARWIDTH;
	barheight = MAXBARHEIGHT;

	/* read the tick value and arguments */
	if (ac > 1 && av[1][0] == '-') {
		if (isdigit(av[1][1])) {
			ticks = -atoi(av[1]);
			if (ticks < 2)
				ticks = 2;
		}
		av++, ac--;
	}

	reshape();
	alarm(ticks * 60);
	while (TRUE) {
		state = wait(ALARM|KBD|MOUSE);
		if(P->state&RESHAPED)
			reshape();
		if(state&KBD)
			while(kbdchar() != -1)	/* dump any keyboard input */
				;
		if(state&ALARM) {
			getinfo();
			docpu();
			drawload();
			getload();
			drawload();
			drawbar();
			if(permin++ >= 60/ticks){
				drawtime();
				gettime();
				drawtime();
				permin = 0;
			}
			alarm(ticks * 60);
		}
	}
}

inittextures()
{
	black = ToTexture(black_bits);
	white = ToTexture(white_bits);
	darkgrey = ToTexture(darkgrey_bits);
	lightgrey = ToTexture(lightgrey_bits);
	grey = ToTexture(grey_bits);
}

reshape()
{
	register int i;

	P->state&=~RESHAPED;
	rectf(&display, Drect, F_CLR);

	/* resize objects */
	size = Drect.corner.x-Drect.origin.x;
	barheight = (Drect.corner.y-Drect.origin.y-2) - CHARHEIGHT;
	if (barheight < MINBARHEIGHT)
		barheight = MINBARHEIGHT;

	/* redraw */
	nodeinit();
	drawload();
	drawtime();
}


/* clear out the bar and write in the node name */
nodeinit()
{
	register struct nodedef *n = &node0;
	register int i;

	/* make the bar */
	n->bar.origin.x = Drect.origin.x;
	n->bar.origin.y = Drect.origin.y + 0*(barheight+CHARHEIGHT);
	n->bar.corner.x = Drect.origin.x + size;
	n->bar.corner.y = n->bar.origin.y + barheight;
	n->bar = inset(n->bar, 4);
	rectf(&display, inset(n->bar, -2), F_OR);
	rectf(&display, n->bar, F_CLR);

	/* init the node */
	for(i = 0; i<NUMLOADBAR - 1; i++)
		n->vec[i] = n->oldvec[i] = 0;
	n->vec[i]=n->oldvec[i]=size;
};

/* draw the bar graph for a system's CPU time */
drawbar ()
{
	struct nodedef *n = &node0;
	Point pt[2];
	register i;

	rectf(&display, n->bar, F_CLR);
	pt[0].y = n->bar.origin.y;
	pt[0].x = n->bar.origin.x;
	pt[1].y =  n->bar.corner.y;
	for(i=0; i< NUMLOADBAR - 1; i++){
		if (!n->vec[i])
			continue;
		pt[1].x = pt[0].x + n->vec[i];
		if (pt[1].x > n->bar.corner.x)
			pt[1].x = n->bar.corner.x;
		texture(&display, Rpt(pt[0], pt[1]), txt[i], F_XOR);
		pt[0].x = pt[1].x;
	}
}

static char *timestr;

drawtime()
{
	if (size < TIMECORNER)
		return;
	string(&defont, timestr, &display,
		Pt (Drect.origin.x+TIMEORIGIN, Drect.origin.y+barheight-2), F_XOR);
}

gettime()
{
	char *p, *ctime();
	long l;

	l = time ((long *)0);
	p = ctime(&l);
	timestr = p+11;
	*(p+11+TIMELEN-1) = 0;
}

drawload()
{
	struct nodedef *n = &node0;

	if (size < LOADCORNER)
		return;
	string(&defont, n->load, &display, 
		Pt (Drect.origin.x+LOADORIGIN, n->bar.origin.y+barheight-6), F_XOR);
}

getload()
{
	double fabs();

	sprintf(node0.load, " %.2f %c%.2f", node0.m_new.l_runq,
		"-+"[node0.m_new.l_runq>node0.m_old.l_runq],
				fabs(node0.m_new.l_runq-node0.m_old.l_runq));
	node0.m_old.l_runq = node0.m_new.l_runq;
}

/* CPU percentages */
docpu()
{
	register long *ln, *lo;
	register int sum;
	long diff[5];
	register long i;

#ifndef BSD
	i = node0.m_new.l_cp[3];
	node0.m_new.l_cp[3] = node0.m_new.l_cp[4];
	node0.m_new.l_cp[4] = i;
#endif BSD
	ln = node0.m_new.l_cp;
	lo = node0.m_old.l_cp;
	for(sum=i=0; i < NUMLOADBAR; i++) {
		diff[i] = *ln - *lo;
		sum += diff[i];
		*lo++ = *ln++;
	}
	sum = sum ? sum : 1;
	ln = node0.vec;
	lo = diff;
	for (i=0; i<NUMLOADBAR; i++) {
		*ln = (*lo * size) / sum;
		*ln++; *lo++;
	}
}

/* globals */
struct nlist nl[] ={
    {"_intrtime",0},
    {"_cp_time",0},
    {"_avenrun",0},
    { 0,0 },
};

#ifdef BSD
char *sys = "/vmunix";
#else
char *sys = "/unix";
#endif BSD
char *core = "/dev/kmem";
int mem;

/* imported */
extern long lseek();

/* initialize */
initload()
{
	nlist(sys, nl);

	mem = open(core, 0);
	if (mem<0) {
		printf("can't open %s\n", core);
		exit(1);
	}
	getinfo();
	node0.m_old = node0.m_new;
}

getinfo()
{
	long avenrun;

	lseek(mem, (long)nl[1].n_value, 0);
	read(mem, (char *)node0.m_new.l_cp, sizeof(node0.m_new.l_cp));
	lseek(mem, (long)nl[2].n_value, 0);
#ifdef BSD
	read(mem, (char *)&avenrun, sizeof(avenrun));
	node0.m_new.l_runq = (float)avenrun/FSCALE;
#else
	read(mem, (char *)&(node0.m_new.l_runq), sizeof(node0.m_new.l_runq));
#endif
}
