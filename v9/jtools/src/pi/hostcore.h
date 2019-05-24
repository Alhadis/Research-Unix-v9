#include "core.pri"
#include "m68kcore.h"
#ifdef V9
#include <sys/param.h>
#include <sys/dir.h>
#include <machine/pte.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/pioctl.h>
#else
#include <sys/types.h>
#include <sys/reg.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
int ptrace(int,int,...);
#endif V9

#define SYSADR		KERNELBASE	/* address of system seg. */
#define USRADR		UADDR		/* start of User page */

char *SignalName(int);
void Wait3();

class HostCore : public M68kCore {
	friend SigMask; friend KernCore;
	friend  HostProcess; friend HostContext;
	int	kmemfd;
#ifdef V9
	proc	pr;
	union {
		user	struct_u;
		char	char_u[ctob(UPAGES)];
	} union_u;
	char	*uarea()		{ return union_u.char_u; }	
	user	*u()			{ return &union_u.struct_u; }
	char	*exechang(long e) 	{ return ioctl(e?PIOCSEXEC:PIOCREXEC); }
	char	*waitstop();
	short	ppid()			{ return pr.p_ppid; }
	char	*pswT();
#else
	friend	WaitList;
	Bsdcore	bsdcore;
	int	pid;
	int 	cursig;
	int	sigmask;
	int	state;
	int	endtext;
	int	startdata;
	int	enddata;
	int	startstack;
	char	*exechang(long);
	short	ppid()			{ return 0; }
	char	*pswT(long,int);
	char	*waitstop(int);
	int	ptrace(int,int=0,int=0,int=0);
	char	*regrw(long,char*,int,int);
#endif V9
	long	regaddr();
	int	fpvalid(long);
	long	scratchaddr();
	Behavs	behavetype();
	char	*ioctl(int);
	char	*signalmask(long);
	char	*clrcurrsig();
	char	*sendsig(long);
	char	*dostep(long,long,int);
	char	*readwrite(long,char*,int,int);
virtual char	*seekto(int&,long&,int&);
virtual int	instack(long,long);
public:
		HostCore(Process *p, Master *m):(p, m) { kmemfd = -1; }
	Context	*newContext();
	Behavs	behavs();
	char	*eventname();
	char	*destroy();
	char	*laybpt(Trap*);
	char	*open();
#ifdef V9
	char	*resources();
#else
	void	close();
#endif V9
	char	*problem();
	char	*readcontrol();
	char	*reopen(char*,char*);
	char	*run();
	char	*stop();
virtual	int	event();
};

class HostContext : public Context { friend HostCore;
	long	regs[18];
	HostCore *core;
	int	pending;
public:
		HostContext()		{}
	void	restore();
};

class KernCore : public HostCore {
	long	sbr;
	long	slr;
	long	intstack;
	pcb	pcb_copy;
	long	pcb_loc;
	char	*seekto(int&,long&,int&);
	int	instack(long,long);
public:
		KernCore(Process *p, Master *m):(p,m) {}
	long	cs_fp;
	long	regloc(int,int=0);
	long	pc();
	long	fp();
	char	*open();
	char	*readcontrol();
	Behavs	behavs();
	char	*eventname();
	int	event();
	char	*getpcb(long);
	char	*specialop(char*);
	char	*special(char*,long);
};

#ifndef V9
class WaitMem {
	friend WaitList;
	HostCore	*core;
	WaitMem		*next;
	int		changed;
	_wait		status;
};

class WaitList {
	WaitMem		*head;
public:
	void		add(HostCore*);
	void		remove(HostCore*);
	int		wait(HostCore*, int);
			WaitList()		{ head = 0; }
};

#define WAIT_POLL	0x1
#define	WAIT_PCFIX	0x2
#define	WAIT_DISCARD	0x4
#endif V9

void WaitForExecHang(char*);
