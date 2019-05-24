#include "master.pri"
#include "format.pri"
#include <CC/stdio.h>
SRCFILE("main.c")

void ErrExit(char *e)
{
	fprintf(stderr, "%s\n", e);
	exit(1);
}

char *getenv(char*), *Getwd();
char *CD;
char *DEVKMEM = "/dev/kmemr";
char *PADSTERM;
char *TAPTO;
char *UNIX = "";

void PadsRemInit();
char *PadsTermInit(char*);

void LoadTerm(char **av)
{
	if (av[0] && !strcmp(av[0],"-R")) {
		PadsRemInit();
		return;
	}
	if (av[0] && !strcmp(av[0],"-r") && av[1])
		ErrExit(PadsTermInit(av[1]));
	if( PadsInit(PADSTERM) )
		ErrExit("cannot load terminal");
}

void mainbatch(char **av)
{
	char *core = "core", *aout = "a.out";
	if( *av ) core = *av++;
	if( *av ) aout = *av++;
	new BatchMaster(core, aout);
	exit(0);
}

void mainpi(char **av)
{
	if(av[0] && !strcmp(av[0],"-t") ) {
		mainbatch(av+1);
		av += 2;
	}
	LoadTerm(av);
	extern char *TapTo;
	TapTo = TAPTO;
	NewHelp();
	NewWd();
	new HostMaster();
	PadsServe();
}

void main(int, char **av)
{
	char *e;
	if( e = getenv("PADSTERM") ) PADSTERM = e;
	if( e = getenv("UNIX") ) UNIX = e;
	if( e = getenv("DEVKMEM") ) DEVKMEM = e;
	CD = sf( "builtin cd %s;", Getwd() );

	av++;
	mainpi(av);
}
