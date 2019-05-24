#include <signal.h>
#include <errno.h>

extern	int	(*_signal())();
extern	int	_sigtramp();
int		(*_sigfunc[NSIG])();
extern	int	errno;

int (*signal(sig, func))()
	int sig;
	int (*func)();
{
	int (*ofunc)();

	if (sig <= 0 || sig >= NSIG) {
		errno = EINVAL;
		return (BADSIG);
	}
	ofunc = _sigfunc[sig];
	_sigfunc[sig] = func;
	if (func != SIG_DFL && func != SIG_IGN)
		func = _sigtramp;
	if (_signal(sig, func) == BADSIG ) {
		_sigfunc[sig] = ofunc;
		return (BADSIG);
	}
	return(ofunc);
}
