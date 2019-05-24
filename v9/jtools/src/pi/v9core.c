#include "process.pub"
#include "frame.pri"
#include "symtab.pri"
#include "symbol.h"
#include "hostcore.h"
#include <a.out.h>
#include "asm.pri"
#include "format.pub"
#include "bpts.pri"
#include "master.pri"
#include <machine/vmparam.h>
SRCFILE("hostcore.c")
#define	TXTOFF(magic)	(magic==ZMAGIC ? 0 : sizeof (struct exec))

Behavs HostCore::behavetype()			{ return behavs(); }
char *HostCore::eventname()			{ return SignalName(event()); }
char *HostCore::run()				{ return ioctl(PIOCRUN);  }
char *HostCore::stop()				{ return sendsig(SIGSTOP); }
int HostCore::event()				{ return pr.p_cursig; }
long HostCore::scratchaddr()			{ return 0x2000; }

long HostCore::regaddr()
{
	return (long)u()->u_ar0;
}

#define SUNBPT 0x4e4f			/* Trap #15 */
char *HostCore::laybpt(Trap *t)
{
	t->saved = peek(t->stmt->range.lo)->sht;
	return poke(t->stmt->range.lo, SUNBPT, 2);
}

Behavs HostCore::behavs()
{
	if( !online() )
		return PENDING;
	char *error = readcontrol();
	if( error ){
		behavs_problem = sf( "readcontrol(): %s", error );
		return ERRORED;
	}
	switch( pr.p_stat ){
	case SSLEEP:
	case SRUN:
		return ACTIVE;
	case SSTOP:
		switch( pr.p_cursig ){
		case SIGSTOP:
			return HALTED;
		case SIGTRAP:
			return BREAKED;
		default:
			return PENDING;
		}
	}
	behavs_problem = sf( "pr.p_stat=%d", pr.p_stat);
	return ERRORED;
}

char *HostCore::problem()
{
	if( corefstat() )
		return "core image has gone away";
	return Core::problem();
}

char *HostCore::readcontrol()
{
	char *error;

	if( corefstat() )
		return "cannot fstat core image";
	if( online() ){
		if( ::ioctl( corefd, PIOCGETPR, &pr ) < 0 )
			return SysErr("fetching proc struct:");
		if( error = read( USRADR, uarea(), ctob(UPAGES)) )
			return error;
	} else {
		if( error = read( USRADR, uarea(), ctob(UPAGES)) )
			return error;
		pr = *(struct proc *)(uarea() + *(int *)u()->u_stack - UADDR);
	}
	return 0;
}

char *HostCore::open()
{
	int mode = 2;
	while( procpath() && corefd<0 && mode>=0 )
		corefd = ::open(procpath(), mode--);
	if( procpath() && corefd<0 ) return SysErr("core image: " );
	if( procpath() && corefstat() )
		return SysErr("core image: " );
	if( corestat.st_mode & S_IEXEC )
		return "executable - should be process (/proc/123) or dump (core)";
	if( stabpath() ){
		stabfd = ::open(stabpath(),0);
		if( stabfd<0 ) return SysErr( "symbol tables: " );
	}
	proc dummy_pr;
	if( ::ioctl(corefd, PIOCGETPR, &dummy_pr) == 0 ){
		_online = 1;
		if( stabpath() == 0 )
			stabfd = ::ioctl(corefd, PIOCOPENT, 0);
	}
	stabfstat();
	_symtab = new Ed8SymTab(this, stabfd, _symtab);
	_symtab->read();
	return procpath() ? readcontrol() : 0;
}

char *HostCore::reopen(char *newprocpath, char *newstabpath)
{
	int mode = 2, newcorefd = -1, compstabfd = -1;

	while( newprocpath && newcorefd<0 && mode>=0 )
		newcorefd = ::open(newprocpath, mode--);
	if( newcorefd<0 )
		return "cannot open process";
	proc dummy_pr;
	if( ::ioctl(newcorefd, PIOCGETPR, &dummy_pr) )
		return "not a live process";
	if( newstabpath )
		compstabfd = ::open(newstabpath, 0);
	else
		compstabfd = ::ioctl(newcorefd, PIOCOPENT, 0);
	struct stat compstabstat;
	if( compstabfd < 0 || ::fstat(compstabfd, &compstabstat) ){
		::close(newcorefd);
		return "symbol table error";
	}
	if( compstabstat.st_mtime != stabstat.st_mtime )
		return "symbol tables differ (modified time)";
	if( compstabstat.st_size != stabstat.st_size )
		return "symbol tables differ (file size)";
	::close(corefd);
	corefd = newcorefd;
	::close(compstabfd);
	return readcontrol();
}

#define ATTEMPTS 9		/* core file i/o */
#define SLEEP	2

char *HostCore::seekto(int &fd, long &addr, int &whence)
{
	long maxadr;

	if( !online() ) {
		if( addr < 0x2000 )
			return "offset below text segment";
		else if( addr < (maxadr = ctob(u()->u_tsize + 1))) {
			addr += TXTOFF(_symtab->magic()) - 0x2000;
			fd = stabfd;
		}
		else if( addr < (maxadr = ctob(stoc(ctos(u()->u_tsize+1)))) )
 			return "offset below data segment";
		else if( addr < (maxadr += ctob(u()->u_dsize)) ) {
			addr = addr - (maxadr - ctob(u()->u_dsize))
				+ ctob(UPAGES);
		}
		else if( addr < (SYSADR - ctob(u()->u_ssize)) )
			return "offset beyond data segment";
		else if( addr < SYSADR )
			{ addr -= SYSADR; whence = 2; }
		else if( addr >= USRADR )
			addr -= USRADR;
		else
			return "offset in system space";
	}
	return 0;
}

char *memcpy(char*,char*,int);

char *HostCore::readwrite(long offset, char *buf, int r, int w)
{
	int fd = corefd, whence = 0, attempts = ATTEMPTS;
	extern int errno;
	char *msg = "core image:";

	if( corefstat() )
		return "core image fstat error";
	char *error = seekto(fd, offset, whence);
	if( error )
		return sf("core image: %s", error);
	while(--attempts>=0){
		if( lseek(fd, offset, whence) == -1 )
			return sf("lseek(%d,0x%X,%d)", fd, offset, whence);
		if( r ){
			int got = ::read(fd, buf, r);
			for( int i = got; i < r; ++i ) buf[i] = 0;
			if( got > 0 ) return 0;
		}
		if( w && ::write(fd, buf, w) == w ) return 0;
		if( errno != EBUSY ) break;
		if( online() ){
			const SPRBUSY = (SLOCK|SSWAP|SPAGE|SKEEP|SDLYU|SWEXIT
					|SVFORK|SVFDONE|SNOVM|SPROCIO);
			msg = sf("core p_flag=0x%x: ", pr.p_flag);
			if( !(pr.p_flag&SPRBUSY) ) break;
			if( pr.p_flag&SPAGE ) ++attempts;
		}
		sleep(SLEEP);
	}
	return SysErr(msg);
}

int HostCore::instack(long curfp, long prevfp )
{
	return curfp < SYSADR && curfp > prevfp;
}

int HostCore::fpvalid(long fp)
{
	return instack(fp, 0x08000000);
}

char *HostCore::ioctl(int p)
{
	if( !online() )
		return "process: not live";
	if( ::ioctl( corefd, p, 0 )< 0 )
		return SysErr( "process control: " );
	return 0;
}

char *HostCore::signalmask(long mask)
{
	if( ::ioctl(corefd,PIOCSMASK,&mask)<0 )
		return SysErr( "setting trace mask:" );
	return 0;
}

#define PSW_T   0x00008000
char *HostCore::pswT()
{
	long ps = ((long *)(uarea() + (long)u()->u_ar0 - UADDR))[PS];
	ps |= PSW_T;
	return regpoke(REG_PS(), ps);
}

static Alarm;
static void SigCatch(int)
{
	extern int errno;
	trace("SigCatch()");
	Alarm = 1;
	errno = 0;
}

const int STEPWAIT = 15;

char *HostCore::waitstop()
{
	int oldalarm;
	char *error = 0;

	oldalarm = alarm(STEPWAIT);
	SIG_TYP oldsig = signal(SIGALRM, (SIG_ARG_TYP)&SigCatch);
	Alarm = 0;
	error = ioctl(PIOCWSTOP);
	signal(SIGALRM, (SIG_ARG_TYP)oldsig);
	alarm(oldalarm);
	if( Alarm ){
		ioctl(PIOCSTOP);
		sleep(2);
		return sf("timeout (%d secs)", STEPWAIT);
	}
	return error;
}

const short M68K_TRAP0=0x4E40, M68K_RTS = 0x4E75;

char *HostCore::dostep(long lo, long hi, int sstep)
{
	char *error = 0;
	long fp0, time0, time(long);

	time0 = ::time(0L);
	fp0 = fp();
	for(;;){
		if( hi && isM68KJSB(peek(pc())->sht) ) {
			error = stepoverM68KJSB();
			goto next;
		}
		if (sstep)
			error =	pswT();
		if( !error && (event()==SIGTRAP || event()==SIGSTOP) )
			error = clrcurrsig();
		if( !error ) error = run();
		if( !error ) error = waitstop();
		if( !error ) error = readcontrol();
		if( !error && event()!=SIGTRAP )
			error = sf( "single step error. signal=%d", event() );
		if( !error ) error = clrcurrsig();
next:
		if( error ) return error;
		if( !hi || pc()<lo || pc()>=hi ||
		    (fp()>fp0 && peek(pc())->sht != M68K_RTS) )
			return 0;
		if( ::time(0L) > time0+STEPWAIT )
			return sf("single step timeout (%d secs)",STEPWAIT);
	}
}

char *HostCore::resources()
{
	static char buf[64];

	sprintf( buf, "%.1fu %.1fs",
		(double)u()->u_vm.vm_utime/50, (double)u()->u_vm.vm_stime/50 );
	return buf;
}

void Wait3()
{
#define WNOHANG		1		/* see <wait.h> */
	int wait3(int*,int,int*);
	for( int i = 0; i<10 && wait3(0,WNOHANG,0)>0; ++i ) {}		/* 10? */
}

char *HostCore::destroy()
{
	clrcurrsig();
	char *error = sendsig(SIGKILL);
	Wait3();
	return error;
}

char *HostCore::clrcurrsig()
{
	char *error = ioctl(PIOCCSIG);
	if( !error )
		pr.p_cursig = 0;
	return error;
}

char *HostCore::sendsig(long sig)
{
	if( !online() ) return "send signal: process not live";
	if( ::ioctl(corefd, PIOCKILL, &sig) >= 0 )
		return 0;
	return SysErr( "send signal (PIOCKILL): " );
}

Context* HostCore::newContext()
{
	HostContext *cc = new HostContext;
	cc->error = 0;
	cc->core = this;
	cc->pending = 0;
	if( pr.p_stat != SSTOP )
		cc->error = "context save: process not stopped";
	else if( peek(pc()-2)->sht == M68K_TRAP0 )
		cc->error = "context save: process in system call";
	else if( cc->pending = pr.p_cursig )
		cc->error = clrcurrsig();
	if( !cc->error )
		for( int i = 0; i < 18; ++i )
			cc->regs[i] = regpeek(i);
	return cc;
}

void HostContext::restore()	// should use only PIOCSSIG eventually
{
	if( pending ){
		if( ::ioctl(core->corefd, PIOCSSIG, &pending) == 0 )
			core->pr.p_cursig = pending;
		else {
			static once = 0;
			if( once++ == 0 )
				PadsWarn("warning: unix out of date?: PIOCSSIG");
			if( error = core->sendsig(pending) )
				return;
			if( pending == SIGTRAP ){
				if( error = core->run() ) return;
				if( error = core->waitstop() ) return;
				error = core->readcontrol();
			} else
				error = core->step();	// other signals
			if( error  )
				return;
		}
	}
	for( int i = 0; i < 18; ++i )
		if( error = core->regpoke(i, regs[i]) )
			return;
}

void WaitForExecHang(char *procpath)
{
	int fd = open(procpath, 0);
	for( int i = 1; i <= 15; ++i ){
		proc p;
		if( ::ioctl(fd, PIOCGETPR, &p) >= 0 ){
			if( p.p_stat == SSTOP ){
				close(fd);
				return;
			}
		}
		sleep(1);
	}
	close(fd);
}

