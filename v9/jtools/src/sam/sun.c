#include "sam.h"

static char	*samterm="xsamterm";
static char	*termargv[20]={"xsamterm", 0};
static int	targc=1;

sunarg(pargv, pargc)
	uchar **pargv[];
	int *pargc;
{
	switch((*pargv)[1][1]){
	case '=':	/* X geometry argument */
		termargv[targc++]=(char *)((*pargv)[1]+1);
		break;
	}
	termargv[targc]=0;
}
char *memcpy(s1, s2, n)
char *s1;
register char *s2;
register int n;
{
	register char *cs1;
	cs1=s1;
	while(n--)
		*cs1++ = *s2++;
	return s1;
}
bootterm(zflag)
{
	int afildes[2], bfildes[2], pid;
	if((pipe(afildes)==-1)||(pipe(bfildes)==-1)){
		dprint("sam: can't create pipe to terminal process\n");
		return 0;
	}
	if((pid=fork())==0){
		close(0);
		dup(afildes[0]);
		close(1);
		dup(bfildes[1]);
		execvp(samterm, termargv);
		exit(127);
	}
	if(pid==-1){
		dprint("sam: can't fork samterm\n");
		return 0;
	}
#ifndef DIST
	sleep(3);	/* for dbx: allow time for child to get out */
#endif
	close(0);
	dup(bfildes[0]);
	close(1);
	dup(afildes[1]);
	return 1;
}

/* start the terminal part, then execute sam on remote machine */
connectboot(machine,zflag)
	char *machine;
{
	bootterm(zflag);
	rawmode(1);
	execlp("rsh", machine, "exec /usr/jerq/bin/sam -R", 0);
	exit(127);
}

/* connectboot means these two aren't called */
connectto(machine)
	char *machine;
{	return;
}

join()
{	return;
}
