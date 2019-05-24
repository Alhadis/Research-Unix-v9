#include "univ.h"

ALARMServe()
{
	if( own()&ALARM ){
		Cycle();
		alarm( 60 );
	}
}

main(argc, argv)
char *argv[];
{
	request(KBD|MOUSE|SEND|RCV|ALARM);
	initdisplay(argc, argv);
	initcursors();
	cursswitch(&Coffee);
	Configuration |= NOVICEUSER;
	Configuration |= BIGMEMORY;
	PadClip();
	alarm( 60 );
	for( ;; ){
		wait(RCV|MOUSE|KBD|ALARM);
		LayerReshaped();
		MOUSEServe();
		KBDServe();
		RCVServe();
		ALARMServe();
		if( !(P->state&RCV) ) Dirty((Pad*)0);
	}
}
