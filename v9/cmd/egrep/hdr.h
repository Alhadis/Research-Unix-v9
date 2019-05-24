#include	<fio.h>
#include	<ctype.h>
#include	<libc.h>

#define BLKSIZE 1024	/* size of reported disk blocks */
#define MAXLIN 10000
#define MAXPOS 20000
#define NCHARS 256
#define NSTATES 128
#define FINAL -1
#define LEFT '\177'	/* serves as ^ */
#define RIGHT '\n'	/* serves as record separator and as $ */

typedef struct State
{
	struct State *gotofn[NCHARS];
	int out;
} State;
extern State states[];
State *nxtst();
int state[];
int line;
int name[];
int left[];
int right[];
int parent[];
int foll[];
int positions[];
char chars[];
int nxtpos;
int nxtfoll;
int inxtpos;
int nxtfoll;
int nxtchar;
int tmpstat[];
State *istat;
int nstate;
int xstate;
int count;
char *input;

char reinit;

int begout;
int begcnt;
int begstat[];

int colpos[];
int cntpos;

long	lnum;
int	bflag;
int	cflag;
int	fflag;
int	hflag;
int	iflag;
int	lflag;
int	nflag;
int	sflag;
int	vflag;
int	nfile;
long	tln;
int	nsucc;
int	badbotch;

int	expfile;

int bmegrep;
