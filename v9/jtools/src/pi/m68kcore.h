#ifndef M68KCORE_H
#define M68KCORE_H
#include "core.pri"

class M68kCore : public Core {
	friend  HostCore; friend HostProcess;
	friend	M68kContext;
	friend	RtRawCore;
	friend	RtNrtxCore;
	char	*stepoverM68KJSB();
	int	isM68KJSB(int);
	char	*stepprolog();
	char	*read(long,char*,int);
	char	*write(long,char*,int);
virtual	char	*dostep(long,long,int);
virtual	char	*readwrite(long,char*,int,int);
virtual	int	instack(long,long);
virtual	int	fpvalid(long);
virtual	long	regaddr();
virtual	Behavs	behavetype();
virtual	long	scratchaddr();
public:
		M68kCore(Process *p, Master *m):(p, m) {}
	Asm	*newAsm();
	Cslfd	*peek(long,Cslfd* =PEEKFAIL);
	CallStk	*callstack();
	Frame	frameabove(long);
	char	*liftbpt(Trap*);
	char	*peekstring(long,char* =0);
	char	*poke(long,long,int);
	char	*pokedbl(long,double,int);
	char	*regname(int);
	char	*step(long=0,long=0);
	char	*popcallstack();
	int	REG_AP();
	int	REG_FP();
	int	REG_PC();
	int	REG_SP();
	int	REG_PS();
	long	regloc(int,int=0);
	long	saved(Frame*,int,int=0);
	long	apforcall(int);
	char	*docall(long,int);
	long	returnregloc();
virtual	int	nregs();
virtual	Context	*newContext();
};

class M68kContext : public Context {
	friend M68kCore;
	long		regs[18];
	M68kCore	*core;
public:
		M68kContext()		{}
virtual	void	restore();
};
#endif M68KCORE_H
