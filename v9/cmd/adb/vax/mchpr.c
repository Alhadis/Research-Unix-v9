/*
 * code to print the name of a signal
 * machine-dependent for less than obvious reasons
 * this for VAX
 */

#include "defs.h"
#include "regs.h"
#include "space.h"
#include <signal.h>

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

static char *illinames[] = {
	"reserved addressing fault",
	"privileged instruction fault",
	"reserved operand fault"
};
static char *fpenames[] = {
	0,
	"integer overflow trap",
	"integer divide by zero trap",
	"floating overflow trap",
	"floating/decimal divide by zero trap",
	"floating underflow trap",
	"decimal overflow trap",
	"subscript out of range trap",
	"floating overflow fault",
	"floating divide by zero fault",
	"floating undeflow fault"
};

sigprint()
{
	if ((signo>=0) && (signo<sizeof signals/sizeof signals[0]))
		prints(signals[signo]);
	switch (signo) {

	case SIGFPE:
		if ((sigcode > 0
		&&  sigcode < sizeof fpenames / sizeof fpenames[0]))
			printf(" (%s)", fpenames[sigcode]);
		break;

	case SIGILL:
		if ((sigcode >= 0
		&&  sigcode < sizeof illinames / sizeof illinames[0]))
			printf(" (%s)", illinames[sigcode]);
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
