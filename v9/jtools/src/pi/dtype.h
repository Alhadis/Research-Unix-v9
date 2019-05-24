#ifndef DTYPE_H
#define DTYPE_H
#ifndef UNIV_H
#include "univ.h"
#endif
class DType : public PadRcv {
	PadRcv	*univ;
>pri
	friend CoffSymTab; friend BsdSymTab; friend Ed8SymTab;
	friend Block; friend Func; friend SymTab; friend Var;
	friend BsdType;
	int	formatset();
	void	free();
>
PUBLIC(DType,U_DTYPE)
		DType();
	int	dim;
	short	pcc;
	int	over;
class	UType	*utype();
	DType	*ref();
	DType	incref();
	DType	*decref();
	char	*text();
	int	format();
struct	Index	carte();
	int	size_of();
	int	isary();
	int	isaryorptr();
	int	isftn();
	int	isintegral();
	int	isptr();
	int	isreal();
	int	isscalar();
	int	isstrun();
	void	reformat(int,int=0);
};
char *PccName(int);

#ifndef V9
typedef class DType *DTypep;
typedef DTypep *DTypepar;
class BsdTShare;

class BsdTFile {
	friend		BsdTShare;
	char		*fname;
	DTypepar	type;
	int		ntypes;
};

class BsdTShare {
	friend		BsdType;
	int		used;
	int		nfiles;
	BsdTFile	*file;
	int		findfile(char*);
	DType		*findtype(int,int);
	char		*filename(int);
	void		entertype(int, int, DTypep);
	int		addfile(char*, int=0);
public:
			~BsdTShare();
			BsdTShare();
};


class BsdType {
	int		utypeindex;
	Source		*src;
	int		*filemap;
	int		nfiles;
	int		used;
	BsdTShare	*share;
public:
	void		addinclude(char*, int);
	DType		chain(DType*);
	DType		gettype(int, int);
	DType		gettype(char*);
	void 		parsetype(char*, char*);
	void		toindices(char*, int&, int&);
	int		toint(char*);
	char		*toutypestr(char*, int);
			~BsdType();
			BsdType(Source*, BsdTShare*);
};
#endif V9
#endif
