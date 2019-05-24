/*
 * code to print the name of a signal
 * machine-dependent for less than obvious reasons
 * this for mc68000
 */

#include "defs.h"
#include "space.h"
#include <signal.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>

static char *signals[] = {
	"",
	"hangup",
	"interrupt",
	"quit",
	"illegal instruction",
	"trace/BPT",
	"IOT",
	"EMT",
	"floating exception",
	"killed",
	"bus error",
	"memory fault",
	"bad system call",
	"broken pipe",
	"alarm call",
	"terminated",
	"signal 16",
	"stopped",
	"stop (tty)",
	"continue (signal)",
	"child termination",
	"stop (tty input)",
	"stop (tty output)",
	"input available (signal)",
	"cpu timelimit",
	"file sizelimit",
	"signal 26",
	"signal 27",
	"signal 28",
	"signal 29",
	"signal 30",
	"signal 31",
};

struct faultcode {
	int fcode;
	char *fname;
};

static struct faultcode illinames[] = {
	{ 0x10,	"illegal instruction fault" },
	{ 0x20,	"privilege violation fault" },
	{ 0x34,	"coprocesser protocol error fault" },
	{ 0x84,	"trap #1 fault" },
	{ 0x88,	"trap #2 fault" },
	{ 0x8c,	"trap #3 fault" },
	{ 0x90,	"trap #4 fault" },
	{ 0x94,	"trap #5 fault" },
	{ 0x98,	"trap #6 fault" },
	{ 0x9c,	"trap #7 fault" },
	{ 0xa0,	"trap #8 fault" },
	{ 0xa4,	"trap #9 fault" },
	{ 0xa8,	"trap #10 fault" },
	{ 0xac,	"trap #11 fault" },
	{ 0xb0,	"trap #12 fault" },
	{ 0xb4,	"trap #13 fault" },
	{ 0xb8,	"trap #14 fault" },
	{ 0 }
};

static struct faultcode emtnames[] = {
	{ 0x28,	"line 1010 emulator trap" },
	{ 0x2c,	"line 1111 emulator trap" },
	{ 0 }
};

static struct faultcode fpenames[] = {
	{ 0x14,	"integer divide by zero" },
	{ 0x18,	"CHK CHK2 instruction" },
	{ 0x1c,	"TRAPV cpTRAPcc TRAPcc instr" },
	{ 0xc0,	"branch or set on unordered cond" },
	{ 0xc4,	"floating inexact result" },
	{ 0xc8,	"floating divide by zero" },
	{ 0xcc,	"floating underflow" },
	{ 0xd0,	"floating operand error" },
	{ 0xd4,	"floating overflow" },
	{ 0xd8,	"floating Not-A-Number" },
	{ 0 }
};

sigprint()
{
	register struct faultcode *fp;

	if ((signo>=0) && (signo<sizeof signals/sizeof signals[0]))
		prints(signals[signo]);
	switch (signo) {

	case SIGFPE:
		for (fp = fpenames; fp->fcode; fp++)
			if (sigcode == fp->fcode) {
				printf(" (%s)", fp->fname);
				return;
			}
		break;

	case SIGEMT:
		for (fp = emtnames; fp->fcode; fp++)
			if (sigcode == fp->fcode) {
				printf(" (%s)", fp->fname);
				return;
			}
		break;

	case SIGILL:
		for (fp = illinames; fp->fcode; fp++)
			if (sigcode == fp->fcode) {
				printf(" (%s)", fp->fname);
				return;
			}
		break;

	}
}

printpc()
{

	dot = (ADDR)rtow(rget(PC));
	psymoff((WORD)dot, INSTSP, "?%16t");
	printins(SYMF|INSTSP);
	printc(EOR);
}
