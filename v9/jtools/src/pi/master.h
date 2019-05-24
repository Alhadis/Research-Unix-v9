#ifndef MASTER_H
#define MASTER_H
#ifndef UNIV_H
#include "univ.h"
#endif

void NewWd();

class Master : public PadRcv {
	friend HostMaster; friend KernMaster;
	friend RtRawMaster; friend RtNrtxMaster;

	Process	*child;
virtual	char	*kbd(char*s);
virtual	char	*help();
virtual	Process	*domakeproc(char*, char*, char*)	{ return 0; }
PUBLIC(Master, U_MASTER)
	Core	*core;
	Pad	*pad;
	void	insert(Process*);
	Process	*search(char*);
	Process	*makeproc(char*, char* =0, char* =0);
		Master();
};

>pri
class KernMaster : public Master {
	Process	*domakeproc(char*, char*, char*);
	char	*kbd(char*);
	char	*help();
	void	refresh();
	void	findcores(char*);
public:
		KernMaster(SymTab*);
};

class HostMaster : public Master {
	KernMaster
		*kernmaster;
	Process	*domakeproc(char*, char*, char*);
	char	*dopscmd(char*);
	void	kpi();
	char	*kbd(char*);
	char	*help();
	void	refresh(char*);
	void	exit();
public:
		HostMaster();
};

class BatchMaster : public Master {
public:
		BatchMaster(char*, char*);
};
>
#endif
