#include "univ.h"
#include "process.pri"
#include "sigmask.h"
#include "hostcore.h"
#include "expr.pub"
#include "master.pub"
#include "bpts.pri"
#include "frame.pri"
#include "memory.pub"
#include "symtab.pub"
#include "symbol.h"
#include "srcdir.h"
#include "asm.pub"
#include <CC/stdio.h>
SRCFILE("hostproc.c")

int access(char*,int);
int kill(int,int);

static char *pathexpand(char *f)
{
	static char file[128];
	extern char *PATH;
	register char *pa, *p;

	if (*f != '/' && strncmp(f, "./", 2) && strncmp(f, "../", 3) && 
	    (pa=PATH)!=0){
		while(*pa){
			for(p=file; *pa && *pa!=':'; p++,pa++)
				*p= *pa;
			if(p!=file)
				*p++='/';
			if(*pa)
				pa++;
			(void)strcpy(p, f);
			if (access(file, 5) != -1)
				return file;
		}
	}
	if (access(f, 5) != -1 )
		return f;
	return 0;
}

void HostProcess::takeoverstopped()
{
	if( pad ){
		openstopped();
		insert(ERRORKEY, "take over: already open");
		return;
	}
	Pick( "take over", (Action)&HostProcess::substitute, (long) this );
}

void HostProcess::reopendump()
{
	char *error;

	insert(ERRORKEY, 0);
	if( error = core->reopen(procpath, stabpath) ){
		insert(ERRORKEY, error);
		return;
	}
	docycle();
}

void HostProcess::substitute(HostProcess *t)
{
	char *error, *oldprocpath, *oldstabpath, *oldcomment;

	insert(ERRORKEY, 0);
	if( !core ){
		insert(ERRORKEY, "that ought to work - but it doesn't");
		return;
	}
	if( !core->online() ){
		insert(ERRORKEY, "cannot take over a coredump");
		return;
	}
	_bpts->lift();
	if( error = core->reopen(t->procpath, t->stabpath) ){
		_bpts->lay();
		insert(ERRORKEY, error);
		return;
	}
					/*	t->kill();	*/
	oldprocpath = procpath;
	oldstabpath = stabpath;
	oldcomment = comment;
	procpath = t->procpath;
	stabpath = t->stabpath;
	comment = t->comment;
	master->makeproc( oldprocpath, oldstabpath, oldcomment );
	master->insert(t);
	master->insert(this);
	banner();
	if( _asm ) _asm->banner();
	if( _bpts ) _bpts->banner();
	if( memory ) memory->banner();
	if( globals ) globals->banner();
	if( sigmsk ){
		sigmsk->banner();
		sigmsk->updatecore();
	}
	if( srcdir ) srcdir->banner();
	core->symtab()->banner();
	pad->clear();
	_bpts->lay();
	docycle();
}

int HostProcess::accept( Action a )
{
	return a == (Action)&HostProcess::substitute;
}

void HostProcess::imprint()
{
	char *parentpath = sf("%s%d", slashname(procpath), hostcore()->ppid() );
	insert(ERRORKEY, "parent=%s", parentpath);
	Process *p = master->search(parentpath);
	if( !p ){
		insert(ERRORKEY, "parent (%d) not opened", hostcore()->ppid());
		return;
	}
	_bpts->liftparents(p->_bpts);
}

void HostProcess::userclose()
{
	if( sigmsk ){
		sigmsk->hostclose();
		delete sigmsk;
		sigmsk = 0;
	}
	Process::userclose();
}

int HostProcess::openstopped(long ischild)
{
	Menu m, s;
	char *error;

	Process::openpad();
	if( core ) return 1;
	insert(ERRORKEY, "Checking process and symbol table...");
	core = (Core*) new HostCore(this, master);
	if( error = core->open() ){
		delete core;
		core = 0;
		if( ischild )
			m.last( "open child", (Action)&HostProcess::open, 1 );
		else
			m.last( "open process", (Action)&HostProcess::open, 0 );
		pad->menu( m );
		insert(ERRORKEY, error);
		return 0;
	}
	insert(ERRORKEY, core->symtab()->warn());
	globals = new Globals(core);
	_asm = core->newAsm();
	if( core->online() ){
		m.last( "stop", (Action)&HostProcess::stop );
		m.last( "run",  (Action)&Process::go  );
	}
	m.last( "src text",  (Action)&Process::srcfiles    );	/* should check */
	m.last( "Globals",   (Action)&Process::openglobals );
	m.last( "RawMemory", (Action)&Process::openmemory  );
	s.last( "Assembler", (Action)&Process::openasm     );
	s.last( "User Types",(Action)&Process::opentypes   );
	if( core->online() ){
		s.last("Journal", (Action)&Process::openjournal);
		s.last("Signals", (Action)&HostProcess::opensigmask);
		s.last("Bpt List", (Action)&HostProcess::openbpts);
		_bpts = new Bpts(core);
		_bpts->lay();
		if( ischild ) imprint();
		sigmsk = new SigMask(hostcore());
		m.last("kill?",   (Action)&HostProcess::destroy     );
	}
	m.last(s.index("more"));
	pad->menu(m);
	pad->makecurrent();
	docycle();
	return 1;
}

void HostProcess::opensigmask()
{
	if( sigmsk ) sigmsk->open();
}

void HostProcess::destroy()
{
	IF_LIVE( !core->online() ) return;
	insert(ERRORKEY, core->destroy());
	docycle();
}

void HostProcess::stop()
{
	IF_LIVE( !core->online() ) return;
	if( !(sigmsk->mask&sigmsk->bit(SIGSTOP)) ) sigmsk->setsig(SIGSTOP);
	Process::stop();
}

Index HostProcess::carte()
{
	Menu m;
	if( !strcmp(procpath,"!") ){
		m.last( "hang & open proc", (Action)&HostProcess::hangopen );
		m.last( "hang & take over", (Action)&HostProcess::hangtakeover );
	} else if( !strcmp(basename(procpath), "core") )
		m.last( "open coredump",(Action)&HostProcess::openstopped );
	else {
		m.last( "open process",  (Action)&HostProcess::open, 0 );
		m.last( "take over",    (Action)&HostProcess::takeover );
		m.last( "open child", (Action)&HostProcess::open, 1 );
	}
	return m.index();
}

void HostProcess::hang()
{
	char program[128];
	char *argv[10];
	
	char *from = stabpath;
	char *to = argv[0] = program;
	int i = 1;
	for(;;) {
		while(*from && *from != ' ')
			*to++ = *from++;
		*to++ = 0;
		if (*from == 0) {
			argv[i] = 0;
			break;
		} else {
			while (*++from == ' ')
				;
			if (*from)
				argv[i++] = to;
		}
	}
	char *ssave = stabpath;
	stabpath  = sf("%s", argv[0]);
	int pid = ::fork();
	if( !pid ){
		int fd;
		for( fd = 0; fd < _NFILE; ++fd ) ::close(fd);
		::open("/dev/null", 2);
		::dup2(0, 1);
		::dup2(0, 2);
		::setpgrp(0, ::getpid());
                ::ptrace(PTRACE_TRACEME, 0, 0, 0, 0);
                ::execvp(argv[0], argv);
		::exit(0);
	}		
	procpath = sf("%d", pid);
	master->makeproc("!", ssave);
	master->insert(this);
}

void HostProcess::hangopen()
{
	hang();
	openstopped();
}

void HostProcess::hangtakeover()
{
	hang();
	takeoverstopped();
}

void HostProcess::open(long ischild)
{
	char *error;
	int pid = 0;

	if( core )
		return;
	if (!stabpath) {
		char *nstab;
		char file[80];
		sscanf(comment, " %*s %*s %*s %s", file);
		if ((nstab = pathexpand(file)) == 0) {
			error = "Can't find symbol table file";
			goto err;
		}
		stabpath = sf("%s", nstab);
		comment = 0;
		master->insert(this);
	}
	sscanf(procpath, "%d", &pid);
	::kill(pid, SIGCONT);
	if (::ptrace(PTRACE_ATTACH, pid, 0, 0, 0)) {
		error = "Can't attach to process";
		goto err;
	}
	openstopped(ischild);
	return;
err:
	Process::openpad();
	insert(ERRORKEY, error);
	return;
	
}

void HostProcess::takeover()
{
	if (!stabpath) {
		char *nstab;
		char file[80];
		sscanf(comment, " %*s %*s %*s %s", file);
		if ((nstab = pathexpand(file)) == 0)
			return;
		stabpath = sf("%s", nstab);
		comment = 0;
		master->insert(this);
	}
	int pid = 0;
	sscanf(procpath, "%d", &pid);
	::kill(pid, SIGCONT);
	if (::ptrace(PTRACE_ATTACH, pid, 0, 0, 0))
		return;
	takeoverstopped();
}
