#include "defs.h"
#include <signal.h>

#define CAT(x) if (x) for(cp=x;bp-buf<sizeof(buf)-1&&*cp;) *bp++= *cp++; *bp='\0'

char *
ipcpath(machine, defdialer, service)
	char *machine;
	char *defdialer;
	char *service;
{
	static char buf[256];
	char *bp;
	char *cp;

	bp = buf;
	if (machine!=NULL && *machine!='\0') {
		CAT(CSROOT);
		if (strchr(machine, '!')==NULL){
			CAT(defdialer);
			CAT("!");
		}
	}
	CAT(machine);
	if (buf[0]=='\0') {
		CAT(CSROOT);
	} else if (service!=NULL && *service!='\0') {
		CAT("!");
	}
	CAT(service);
	return buf;
}
