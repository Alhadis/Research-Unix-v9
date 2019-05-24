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
SRCFILE("hostcore.c")
#define	TXTOFF(magic)	(magic==ZMAGIC ? 0 : sizeof (struct exec))
#include <setjmp.h>

static WaitList waitlist;
int kill(int,int);

const int STOPPED=0, RUNNING=1, EXITED=2, DIED=3;
const REGADDR = USRADR - 18 * 4;

Behavs HostCore::behavetype()		{ return behavs(); }
char *HostCore::eventname()		{ return SignalName(event()); }
char *HostCore::stop()			{ ::kill(pid, SIGSTOP); return 0; }
int HostCore::event()			{ return cursig; }
long HostCore::regaddr()		{ return REGADDR; }
long HostCore::scratchaddr()		{ return 0x2000; }

extern int errno;

int HostCore::ptrace(int cmd, int a, int d, int a2)
{
	errno = 0;
	int ret = ::ptrace(cmd, pid, a, d, a2);
	switch (cmd) {
		case PTRACE_CONT:
		case PTRACE_KILL:
		case PTRACE_SINGLESTEP:
		case PTRACE_ATTACH:
		case PTRACE_DETACH:
			break;
		default:
			if (!errno)
				waitlist.wait(this, WAIT_DISCARD);
			break;
	}
	return ret;
}

char *HostCore::run()
{
	if (cursig == SIGSTOP || cursig == SIGTRAP)
		cursig = 0;
	ptrace(PTRACE_CONT, 1, cursig);
	state = RUNNING;
	cursig = 0;
	return 0;
}

#define SUNBPT 0x4e4f			/* Trap #15 */
char *HostCore::laybpt(Trap *t)
{
	Cslfd *cp = peek(t->stmt->range.lo, 0);
	if (!cp)
		return "text not readable (probably shared)";
	t->saved = cp->sht;
	return poke(t->stmt->range.lo, SUNBPT, 2);
}

Behavs HostCore::behavs()
{
	if( !online() )
		return PENDING;
	// Confirm it is still running
	if ( state == EXITED || state == DIED)
		return ERRORED;
	if ( state == RUNNING ) {
		if (waitlist.wait(this, WAIT_POLL|WAIT_PCFIX)) {
			if (state != STOPPED)
				return ERRORED;
			if (!(sigmask&(1<<(cursig-1)))) {
				run();
				return ACTIVE;
			}
		} else
			return ACTIVE;
	} 
	switch( cursig ){
	case SIGSTOP:
		return HALTED;
	case SIGTRAP:
		return BREAKED;
	default:
		return PENDING;
	}
}

char *HostCore::problem()
{
	static char buf[64];

	if( !online() ) {
		if( corefstat() )
			return "core image has gone away";
		return Core::problem();
	}
	if (state == EXITED)
		sprintf( buf, "exited with status %d", cursig);
	else if (state == DIED)
		sprintf( buf, "died from signal %d", cursig);
	return buf;
}

char *HostCore::readcontrol()
{
	if( online() )
		return "online readcontrol";
	if( corefstat() )
		return "cannot fstat core image";
	int got = ::read(corefd, (char *)&bsdcore, sizeof(bsdcore));
	if (got != sizeof(bsdcore))
		return "cannot read bsdcore";
	state = STOPPED;
	cursig = bsdcore.c_signo;
	endtext = N_TXTADDR(bsdcore.c_aouthdr) + bsdcore.c_tsize;
	startdata = N_DATADDR(bsdcore.c_aouthdr);
	enddata = startdata + bsdcore.c_dsize;
	startstack = SYSADR - bsdcore.c_ssize;
	return 0;
}

void HostCore::close()
{
	if (online()){
		if (state == RUNNING) {
			stop();
			waitstop(WAIT_PCFIX);
			ptrace(PTRACE_DETACH, 1);
		} else if (state == STOPPED) {
			::kill(pid, SIGSTOP);
			ptrace(PTRACE_DETACH, 1);
		}
		waitlist.remove(this);
	}
	Core::close();
}

char *HostCore::open()
{
	int mode = 2;
	if( procpath() && !strcmp(basename(procpath()), "core") ) {
		while( corefd<0 && mode>=0 )
			corefd = ::open(procpath(), mode--);
		if( corefd<0 )
			return SysErr("core image: " );
		if( corefstat() )
			return SysErr("core image: " );
		if( corestat.st_mode & S_IEXEC )
			return "executable - should be dump (core)";
	} else if( procpath() ) {
		sscanf(procpath(), "%d", &pid);
		_online = 1;
		waitlist.add(this);
		waitlist.wait(this, 0);
		cursig = SIGSTOP;
	}
	stabfd = ::open(stabpath(),0);
	if( stabfd<0 ) {
		if( online() ) {
			::kill(pid, SIGSTOP);
			ptrace(PTRACE_DETACH, 1);
		}
		return SysErr( "symbol tables: " );
	}
	stabfstat();
	BsdSymTab *bsdsp = new BsdSymTab(this, stabfd, _symtab);
	_symtab = bsdsp;
	_symtab->read();
	// Can't find endtext from U structure in 4.0 release, this is a kludge
	endtext = bsdsp->endtext();
	if( online() || !procpath() )
		return 0;
	return readcontrol();
}

char *HostCore::reopen(char *newprocpath, char *newstabpath)
{
	int compstabfd = -1;
	char *error = 0;
	int retstat;

	if( !online() || (newprocpath && !strcmp(basename(newprocpath), "core")) )
		return "reopen core not implemented";
	int opid = pid;
	int ostate = state;
	int ocursig = cursig;
	sscanf(newprocpath, "%d", &pid);
	waitlist.wait(this, 0);
	
	compstabfd = ::open(newstabpath, 0);
	struct stat compstabstat;
	if( compstabfd < 0 ) {
		error = "symbol table error";
		goto out;
	}
	retstat = ::fstat(compstabfd, &compstabstat);
	::close(compstabfd);
	if (retstat)
		error = "symbol table error";
	else if( compstabstat.st_mtime != stabstat.st_mtime )
		error = "symbol tables differ (modified time)";
	else if( compstabstat.st_size != stabstat.st_size )
		error = "symbol tables differ (file size)";
	cursig = ocursig;
	state = ostate;
	if (error) {
out:
		::kill(pid, SIGSTOP);
		ptrace(PTRACE_DETACH, 1);
		pid = opid;
		return error;
	}
	// Must disconnect  from the old process
	int npid = pid;
	pid = opid;
	switch (state) {
		case RUNNING:
			stop();
			waitstop(WAIT_PCFIX);
			ptrace(PTRACE_DETACH, 1);
			break;
		case STOPPED:
			::kill(pid, SIGSTOP);
			ptrace(PTRACE_DETACH, 1);
			break;
		case EXITED:
		case DIED:
			break;
	}
	pid = npid;
	state = STOPPED;
	cursig = SIGSTOP;
	// Can't find endtext from U structure in 4.0 release, this is a kludge
	BsdSymTab *bsdsp = (BsdSymTab *)_symtab;
	endtext = bsdsp->endtext();
	return 0;
}

const int REGFD = -2, DATAFD = -3, TEXTFD = -4, USERFD = -5;
char *HostCore::seekto(int &fd, long &addr, int &whence)
{
	if( !online() ) {
		if ( (u_long)addr >= USRADR) {
			addr -= USRADR + ctob(UPAGES);
			whence = 2;
		} else if ( (u_long)addr >= REGADDR) {
			addr -= REGADDR;
			fd = REGFD;
		} else if ( (u_long)addr < 0x2000)
			return "offset below text segment";
		else if ( (u_long)addr <= endtext) {
			addr += N_TXTOFF(bsdcore.c_aouthdr)
				- N_TXTADDR(bsdcore.c_aouthdr);
			fd = stabfd;
		} else if ( (u_long)addr < startdata)
			return "offset beyond text segment";
		else if ( (u_long)addr <= enddata)
			addr -= startdata - bsdcore.c_len;
		else if ( (u_long)addr < startstack)
			return "offset beyond data segment";
		else if ( (u_long)addr < SYSADR) {
			addr -= SYSADR + ctob(UPAGES);
			whence = 2;
		} else
			return "offset in system space";
	} else {
		if ( (u_long)addr >= USRADR) {
			addr -= USRADR;
			fd = USERFD;
		} else if ( (u_long)addr >= REGADDR) {
			addr -= REGADDR;
			fd = REGFD;
		} else if ( (u_long)addr >= SYSADR ) {
			extern char *DEVKMEM;
			if( kmemfd == -1 ) {
				kmemfd = ::open(DEVKMEM, 0);
				if (kmemfd == -1)
					return "can't read kernel";
			}
			fd = kmemfd;
		} else if ( (u_long)addr <= endtext)
			fd = TEXTFD;
		else
			fd = DATAFD;
	}
	return 0;
}

void bcopy(char*,char*,int);

char *HostCore::readwrite(long offset, char *buf, int r, int w)
{
	int fd = corefd, whence = 0;
	int rv;
	char *msg = "core image:";

	char *error = seekto(fd, offset, whence);
	if( error )
		return sf("core image: %s", error);
	if( fd >= 0 ) {
		if( lseek(fd, offset, whence) == -1 )
			return sf("lseek(%d,0x%X,%d)", fd, offset, whence);
		if( r ){
			int got = ::read(fd, buf, r);
			for( int i = got; i < r; ++i ) buf[i] = 0;
			if( got > 0 ) return 0;
		}
		if( w && ::write(fd, buf, w) == w ) return 0;
		return SysErr(msg);
	}
	int wasrunning = 0;
	if (state == RUNNING) {
		stop();
		if (error = waitstop(WAIT_PCFIX))
			return error;
		if (cursig != SIGSTOP) {
			int savesig = cursig;
			/* The STOP signal still has to be eaten */
			run();
			waitstop(WAIT_PCFIX);
			cursig = savesig;
		} else
			wasrunning = 1;
	}
	if (fd == REGFD) {
		error = regrw(offset, buf, r, w);
		if (wasrunning && !error)
			run();
		return error;
	} else if (fd == DATAFD) {
		if ( r )
			rv = ptrace(PTRACE_READDATA, offset, r, (int)buf);
		else
			rv = ptrace(PTRACE_WRITEDATA, offset, w, (int)buf);
	}
	else if (fd == TEXTFD) {
		if ( r )
			rv = ptrace(PTRACE_READTEXT, offset, r, (int)buf);
		else {
			/* Damn bug in Sun's kernel */
			if (w < 4){
				int tmp;
				ptrace(PTRACE_READTEXT, offset, 4, (int)&tmp);
				::bcopy(buf, (char*)&tmp, w);
				if (rv = ptrace(PTRACE_WRITETEXT, offset, 4, (int)&tmp))
					return "text not writable (probably shared)";
			} else
				rv = ptrace(PTRACE_WRITETEXT, offset, w, (int)buf);
		}
	}
	else if (fd == USERFD) {
		int *ip = (int *)buf;
		if ( r ) {
			do {
				*ip++ = ptrace(PTRACE_PEEKUSER, offset);
				offset += 4;
				r -= 4;
			} while (r > 0 && errno == 0);
		} else {
			do {
				ptrace(PTRACE_POKEUSER, offset, *ip++);
				offset += 4;
				w -= 4;
			} while (w > 0 && errno == 0);
		}
		rv = errno;
	}
	if (wasrunning && !rv)
		run();
	return rv == 0 ? 0 : "bad ptrace call";
}

char *HostCore::regrw(long offset, char *buf, int r, int w)
{
	if ( online() ) {
		regs rg;
		if ( ptrace(PTRACE_GETREGS, (int)&rg) )
			return "Can't read registers";
		if ( r )
			::bcopy( (char*)&rg + offset, buf, r);
		else {
			::bcopy( buf, (char*)&rg + offset, w);
			ptrace(PTRACE_SETREGS, (int)&rg);
		}
	} else {
		if ( r )
			::bcopy( (char*)&bsdcore.c_regs + offset, buf, r);
		else if ( w )
			::bcopy( buf, (char*)&bsdcore.c_regs + offset, w);
	}
	return 0;
}

int HostCore::instack(long curfp, long prevfp )
{
	return ((curfp&0xf8000000)==0x08000000) && ((curfp&0xff000000)!=0x0f000000)
		 && (curfp>prevfp);
}

int HostCore::fpvalid(long fp)
{
	return (instack(fp, 0x08000000) && fp < 0x0f000000);
}

char *HostCore::signalmask(long mask)
{
	sigmask = mask;
	return 0;
}

char	*HostCore::exechang(long ehang)
{
	if (!ehang)
		return "Must hang on exec"; 
	return 0;
}

#define PSW_T   0x00008000
char *HostCore::pswT(long psw_loc, int t)
{
	long ps;

	char *error = read(psw_loc, (char*) &ps, 4 );
	if( error ) return error;
	if( t )
		ps |= PSW_T;
	else
		ps &= ~PSW_T;
	return write(psw_loc, (char*)&ps, 4);
}

static jmp_buf saveenv;
void longjmp(jmp_buf, int);
int setjmp(jmp_buf);

static void SigCatch(int)
{
	longjmp(saveenv, 1);
}

const int STEPWAIT = 15;

char *HostCore::waitstop(int flags)
{
	int wret;

	SIG_TYP oldsig = ::signal(SIGALRM, (SIG_TYP)&SigCatch);
	int oldalarm = ::alarm(STEPWAIT);
	if( ::setjmp(saveenv) ){
		::alarm(oldalarm);
		::signal(SIGALRM, (SIG_TYP)oldsig);
		::kill(pid, SIGSTOP);
		return sf("timeout (%d secs)", STEPWAIT);
	}
	wret = waitlist.wait(this, flags);
	::alarm(oldalarm);
	::signal(SIGALRM, (SIG_TYP)oldsig);
	if (!wret )
		return "Unexpected wait error";
	if (state != STOPPED)
		return "Process exited";
	return 0;
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
			ptrace(PTRACE_SINGLESTEP, 1);
		else
			error = run();
		if( !error ) error = waitstop(sstep ? 0 : WAIT_PCFIX);
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

char *HostCore::destroy()
{
	char *error;

	if ( state == EXITED || state == DIED)
		return 0;
	clrcurrsig();
	if (state == RUNNING) {
		stop();
		if (error = waitstop(WAIT_PCFIX))
			return error;
	}
	ptrace(PTRACE_KILL);
	waitlist.wait(this, WAIT_DISCARD|WAIT_POLL);
	state = EXITED;
	cursig = SIGKILL;
	return 0;
}

char *HostCore::clrcurrsig()
{
	cursig = 0;
	return 0;
}

char *HostCore::sendsig(long sig)
{
	if( !online() ) return "send signal: process not live";
	::kill(pid, sig);
	return 0;
}

Context* HostCore::newContext()
{
	HostContext *cc = new HostContext;
	cc->error = 0;
	cc->core = this;
	cc->pending = 0;
	if( state != STOPPED )
		cc->error = "context save: process not stopped";
	else if( peek(pc()-2)->sht == M68K_TRAP0 )
		cc->error = "context save: process in system call";
	else if( cc->pending = cursig )
		cc->error = clrcurrsig();
	if( !cc->error )
		for( int i = 0; i < 18; ++i )
			cc->regs[i] = regpeek(i);
	return cc;
}

void HostContext::restore()
{
	if( pending )
		core->cursig = pending;
	for( int i = 0; i < 18; ++i )
		if( error = core->regpoke(i, regs[i]) )
			return;
}
