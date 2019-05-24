#include "flayer.h"
#include "../mesg.h"

#define	MAXFILES	256

enum{
	Up,
	Down
};
#define	NL	10
typedef struct Section Section;
struct Section{
	long	nbytes;
	uchar	*text;		/* if null, we haven't got it */
	Section	*next;
};
typedef struct Rasp{
	long	nbytes;
	Section	*sect;
}Rasp;
#define	Untagged	((unsigned short)65535)
typedef struct Text{
	Rasp	rasp;
	short	nwin;
	short	front;		/* input window */
	ushort	tag;
	char	lock;
	Flayer	l[NL];		/* screen storage */
}Text;
extern Text	*text[];
extern uchar	*name[];
extern ushort	tag[];
extern int	nname;
/* SUN: changed following Cursor's from Texture's */
extern Cursor	bullseye;
extern Cursor	coffeecup;
extern Cursor	sweep;
extern Cursor	deadmouse;
extern Cursor	lockarrow;
extern Cursor	*cursor;
extern Flayer	*which;
extern Flayer	*work;
extern Text	cmd;
extern uchar	*scratch;
extern long	nscralloc;
extern char	lock;
extern long	snarflen;

Flayer	*flwhich();
uchar	**gettext();
uchar	**rload();
uchar	*ALLOC();
