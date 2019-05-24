/*
#ifdef	mc68020
#define	GNOT
#endif
*/

#define	FIOBSIZE	4096

typedef struct Fbuffer
{
	unsigned char *next;		/* next char to be used */
	unsigned char *end;		/* first invalid char */
	unsigned char *lnext;		/* previous value of next */
	char rdlast;			/* true if last input was rdline */
	long offset;			/* seek of next char to be read */
	unsigned char buf[FIOBSIZE];
} Fbuffer;
extern Fbuffer *Ffb[];

#define	FIORESET(f)	((f)->next=(f)->lnext=(f)->end=(f)->buf, (f)->rdlast=0)
#define	FIOSET(f, fd)	if((f=Ffb[fd&=0x7f]) == 0){Finit(fd,(char *)0);f=Ffb[fd];}
/* FIOLINELEN is length of last input */
#define	FIOLINELEN(fd)	(((int)(Ffb[fd]->next - Ffb[fd]->lnext))-1)
/* FIOSEEK is lseek of next char to be processed */
#define	FIOSEEK(fd)	(Ffb[fd]->offset - (Ffb[fd]->end - Ffb[fd]->next))

extern void Finit();
extern char *Frdline();
extern void Fundo();
extern int Fgetc();
extern long Fread();
extern long Fseek();

/* COUNT is the type of counts to things like read, write, memcpy etc */
/*
#ifdef	GNOT
#define	COUNT	long
#define	FIOMALLOC(n)	sbrk(n)
#define	SEEK(a,b,c)	seek(a,b,c)
#else
*/
#define	COUNT	int
#define	FIOMALLOC(n)	malloc(n)
#define	SEEK(a,b,c)	lseek(a,b,c)
/*#endif*/
extern COUNT read(), write();
