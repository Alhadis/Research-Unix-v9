#include <a.out.h>
#include "symtab.pri"
#include "dtype.pri"
#include "symbol.h"
#include "stab.h"
#include "core.pub"
SRCFILE("bsdsymtab.c")

char *strchr(char *, int);
char *memset(char *, int, int);
char *memcpy(char *, char *, int);
int nccdemangle(char **, char *);
void SymbolStats();

BsdSymTab::BsdSymTab(Core* c, int fd, SymTab *i, long r):(c, fd, i, r)
{
	hdr = new exec;
	bsdshare = 0;
}

BsdSymTab::~BsdSymTab()
{
	delete hdr;
	if (bsdshare)
		delete bsdshare;
}

char *BsdSymTab::gethdr()
{
	if( lseek( fd, 0L, 0 ) == -1 || !ReadOK(fd, (char*)hdr, sizeof *hdr) )
		return SysErr( "symbol table: " );
	_magic = hdr->a_magic;
//	if( N_BADMAG(*hdr) || hdr->a_trsize || hdr->a_drsize )
	if( N_BADMAG(*hdr) )
		return "symbol table: not executable text";
	entries = hdr->a_syms/sizeof(nlist);
	return 0;
}

int BsdSymTab::endtext()
{
	exec h = *hdr;
	return N_DATADDR(h);
}

Block *BsdSymTab::gatherfunc(Func *func)
{
	register struct nlist *f, *n;
	register Block *ablk, *lblk;
	Var *arg = 0, *lcl = 0;
	register Stmt *stmt = 0;
	long bfun = func->begin, size = func->size, i;
	char *so = func->source()->_text;
	BsdType *bsd = func->source()->bsdp;
	char *subtype;

	++FunctionGathered;
	SymbolStats();
	IF_LIVE( bfun < 0 || bfun+size > entries ) return 0;
	if (size == 0)
		return fakeblk();
	if( !(n = f = nlistvector(bfun,size+1)) ) return 0;
	IF_LIVE( n->n_type != N_FUN ) return 0;
	ablk = new Block( this, 0, 0, sf("%s().arg_blk",n->n_un.n_name) );
	lblk = new Block( this, ablk, 0, sf("%s().lcl_blk",n->n_un.n_name) );
	ablk->child = lblk;
	for( i = 0; i < size; ++i, ++n ){
		switch( n->n_type ){
		case N_PSYM:
			gathervar( n, &arg, ablk, U_ARG, bsd );
			break;
		case N_LSYM:
			subtype = strchr(n->n_un.n_name, ':');
			if (!subtype || *++subtype != '(')
				break;
			gathervar( n, &lcl, lblk, U_AUT, bsd );
			break;
		case N_STSYM:
		case N_LCSYM:
			subtype = strchr(n->n_un.n_name, ':');
			if (!subtype || *++subtype != 'V')
				break;
			n->n_value += relocation;
			gathervar( n, &lcl, lblk, U_STA, bsd );
			break;
		case N_RSYM:
			gathervar( n, &lcl, lblk, U_REG, bsd );
			break;
		case N_FUN:
		case N_SLINE:
			n->n_value += relocation;
			if( stmt ) {
				if ( n->n_desc > stmt->lineno )
					func->lines.hi = n->n_desc;
				stmt->range.hi = n->n_value;
			}
			stmt = new Stmt(this,lblk,stmt);
			if( !ablk->stmt ) ablk->stmt = stmt;
			stmt->lineno = n->n_desc;
			stmt->range.lo = n->n_value;
			if( !ablk->range.lo )
				ablk->range.lo = n->n_value;
			ablk->range.hi = n->n_value;
			break;
		}
	}
	if( stmt )
		stmt->range.hi = n->n_value + relocation;
	delete f;
	uncfront( ablk->var, (char *)0 );
	uncfront( lblk->var, (char *)0 );
	return ablk;
}

void BsdSymTab::gathervar( nlist *n, Var **v, Block *b, UDisc d, BsdType *bt )
{
	IF_LIVE( !v ) return;
	if( *n->n_un.n_name == ' ')
		return;
	char *marker = strchr( n->n_un.n_name, ':');
	if (!marker)
		marker = n->n_un.n_name + strlen(n->n_un.n_name);
	else if (marker[1] == ':') {	//Special Ncc symbol
		marker = strchr( &marker[2], ':');
		if (!marker) {
			marker = n->n_un.n_name + strlen(n->n_un.n_name);
			while (*++marker == 0)
				;
			marker--;
		}
	}
	*marker++ = 0;
	*v = new Var( this, b, *v, d, n->n_un.n_name );
	if( b && !b->var ) b->var = *v;
	(*v)->range.lo = n->n_value;
	(*v)->type = bt->gettype(marker);
}

void BsdSymTab::gathervar( nlist *n, Var **v, Block *b, UDisc d)
{
	DType dt;

	IF_LIVE( !v ) return;
	*v = new Var( this, b, *v, d, n->n_un.n_name );
	if( b && !b->var ) b->var = *v;
	(*v)->range.lo = n->n_value;
	(*v)->type = dt;
	(*v)->type.pcc = n->n_desc;
}

char *BsdSymTab::gettbl()
{
	base = new nlist[entries];
	symoff = (nlist*)N_SYMOFF(*hdr);
	if( lseek(fd, (long)symoff, 0) == 1
	 || !ReadOK(fd, (char*)base, hdr->a_syms)
	 || !ReadOK(fd, (char*)&strsize, 4) ){
		delete base; base = 0;
		return SysErr( "symbol table: " );
	}
	strings = new char[strsize];
	if( lseek( fd, -4, 1 ) == -1 || !ReadOK(fd, strings, strsize) ){
		delete strings; strings = 0;
		delete base; base = 0;
		return SysErr( "strings table: " );
	}
	strcpy( strings, "???" );	/* zero string index */
	return 0;
}

Source *BsdSymTab::tree()
{
	register nlist	*n, *base_entries;
	Source		*src = 0;
	Func		*func = 0;
	Block		*fake = fakeblk();
	Var		*glb = 0, *fst = 0, *resolve;
//	UType		*u;
	register long	regstrings, inafunc = 0, inasrc = 0;
	long		lastline, funcstart = 0, lgfound = 0;
	char		*equal, *subtype;
	BsdType		*bsd = 0;
	DType		*t;
	
	if( _warn = gettbl() ) return 0;
	base_entries = base+entries;
	regstrings = (long) strings;
	glb = globregs(_blk, _core->nregs() );
	bsdshare = new BsdTShare;
	for( n = base; n < base_entries; ++n ){
		n->n_un.n_strx += (long) regstrings;
		if( inasrc && n->n_un.n_name && (equal = strchr(n->n_un.n_name, '=')))
			bsd->parsetype(equal, n->n_un.n_name);
		switch( n->n_type ){
		case N_ABS|N_EXT:
			if( n->n_un.n_name[0] != '_' ) break;	/* ? */
		case N_BSS|N_EXT:
		case N_DATA|N_EXT:
			if( *n->n_un.n_name == '_' ) ++n->n_un.n_name;
			nccdemangle(&n->n_un.n_name, (char *)0);
			resolve = (Var*)idtosym(U_GLB, n->n_un.n_name, 0);
			n->n_value += relocation;
			if( resolve ){
				if( !resolve->range.lo )
					resolve->range.lo = n->n_value;
			} else {
				n->n_desc = LONG;
				gathervar( n, &glb, _blk, U_GLB );
				break;
			}
			break;
		case N_GSYM:
			if( !inasrc ) break;
			subtype = strchr(n->n_un.n_name, ':');
			if (!subtype)
				break;
			*subtype = 0;
			nccdemangle(&n->n_un.n_name, (char *)0);
			if( !idtosym(U_GLB, n->n_un.n_name, 0) )
				gathervar( n, &glb, _blk, U_GLB, bsd );
			break;
		case N_LCSYM:
		case N_STSYM:
			if( !inasrc || !src || !src->blk ) break;
			subtype = strchr(n->n_un.n_name, ':') + 1;
			if (*subtype != 'S')
				break;
			n->n_value += relocation;
			gathervar( n, &fst, src->blk, U_FST, bsd );
			break;
		case N_SLINE:
			lastline = n->n_desc;
			if (funcstart) {
				funcstart = 0;
				func->lines.lo = lastline;
			}
			break;
		case N_BINCL:	// Include files for type indexing
			bsd->addinclude(n->n_un.n_name, 1);
			break;
		case N_EXCL:
			bsd->addinclude(n->n_un.n_name, 0);
			break;
		case N_SO:
			if (inafunc) {
				inafunc = 0;
				func->lines.hi = lastline;
				func->size = n-base - func->begin;
			}
			// Directory entry in 4.0?
			if (n->n_un.n_name[strlen(n->n_un.n_name)-1] == '/') {
				inasrc = 0;
				break;
			}
			if (!strcmp(n->n_un.n_name, "libg.s")) {
				lgfound = 1;
				inasrc = 0;
				break;
			}
			inasrc = 1;
			src = new Source(this,src,n->n_un.n_name,0);
			bsd = src->bsdp = new BsdType(src, bsdshare);
			func = 0;
			inafunc = 0;
			fst = 0;
			n += 12;	// Through away type info
			break;
		case N_TEXT|N_EXT:
			if( *n->n_un.n_name == '_' ) ++n->n_un.n_name;
			nccdemangle(&n->n_un.n_name, (char *)0);
			if( idtosym(U_FUNC, n->n_un.n_name, 0) ) break;
			func = new Func(this,0,0,0,n->n_un.n_name);
			func->range.lo = n->n_value + relocation;
			func->_blk = fake;
			t = new DType;
			t->pcc = LONG;
			func->type = t->incref();
			func->type.pcc = FTN;
			break;
		case N_FUN:
			if (inafunc) {
				inafunc = 0;
				func->lines.hi = lastline;
				func->size = n-base - func->begin;
			}
			if( !inasrc || !src ) break;
			inafunc = 1;
			++FunctionStubs;
			subtype = strchr(n->n_un.n_name, ':');
			*subtype++ = 0;
			nccdemangle(&n->n_un.n_name, (char *)0);
			func = new Func(this,src,func,n-base,n->n_un.n_name);
			if( !src->child ) src->child = src->linefunc = func;
			funcstart = 1;
			func->range.lo = n->n_value + relocation;
			t = new DType;
			*t = bsd->gettype(subtype);
			func->type = t->incref();
			func->type.pcc = FTN;
			break;
		}
	}
	while( src && src->lsib ) src = (Source*) src->lsib;
	if( base ) { delete base; base = 0; }
	if (!lgfound)
		_warn = "should be linked with ld -g";
	return src;
}

Var *BsdSymTab::gatherutype(UType *u)
{
	Var *first = 0, *v = 0;
	char *memname, *cp;
	int file, offset;
	int bitsize, bitoffset;
	int isenum = (u->type.pcc == ENUMTY);

	memname = u->encode;
	if (!memname)
		return first;
	++UTypeGathered;
	SymbolStats();
	while (*memname != ';') {
		cp = strchr(memname, ':');
		*cp++ = 0;
		if (isenum) {
			v = new Var( this, 0, v, U_MOT, memname );
			v->range.lo = u->bsdp->toint(cp);
			v->type.pcc = MOETY;
			if( !first ) first = v;
			memname = strchr(cp, ',') + 1;
			continue;
		}
		u->bsdp->toindices(cp, file, offset);
		cp = strchr(cp, ')') + 2;
		bitoffset = u->bsdp->toint(cp);
		cp = strchr(cp, ',') + 1;
		bitsize = u->bsdp->toint(cp);
		v = new Var( this, 0, v, U_MOT, memname );
		v->type = u->bsdp->gettype(file, offset);
		if ((bitsize & 0x7) || (bitoffset & 0x7)) {
			v->range.lo = ((bitoffset >> 5) << 5) +
				      32 - bitsize - (bitoffset & 0x1f);
			v->type.pcc = UBITS;
			v->type.dim = bitsize;
		} else
			v->range.lo = bitoffset >> 3;
		if( !first ) first = v;
		memname = strchr(cp, ';') + 1;
	}
	uncfront(first, u->_text);
	return first;
}

nlist *BsdSymTab::nlistvector(long start, long size )
{
	struct nlist *n;
	int i;

	lseek(fd, (long) (symoff + start), 0);
	n = new nlist[size];
	IF_LIVE( !ReadOK(fd, (char*) n, size * sizeof *n) ){
		delete n;
		return 0;
	}
	for( i = 0; i < size; ++i )
		n[i].n_un.n_strx += (long) strings;
	return n;
}

BsdTShare::BsdTShare()
{
}

BsdTShare::~BsdTShare()
{
	for(BsdTFile *f = file; f < &file[used]; f++) {
		for (int i = 0; i < f->ntypes; i++)
			delete f->type[i];
		delete f->type;
	}
	delete file;
}

int BsdTShare::addfile(char *fname, int flag)
{
	BsdTFile *f;
	int i;

	if (used == nfiles) {		// Grow the number of files
		nfiles += 10;
		f = new BsdTFile[nfiles];
		for (i = 0; i < used; i++)
			f[i] = file[i];
		delete file;
		file = f;
	}
	f = &file[used];
	f->fname = fname;		// Initialize
	f->ntypes = 20;
	f->type = new DTypep[20];
	if (flag) {			// add builtins
		static int typeindex[] = {
			0, INT, CHAR, LONG, SHORT, UCHAR, USHORT,
			ULONG, UNSIGNED, FLOAT, DOUBLE, INT, UNDEF
		};
		for(i = 1; i < sizeof(typeindex)/sizeof(int); i++) {
			f->type[i] = new DType;
			f->type[i]->pcc = typeindex[i];
		}
	}
	return used++;
}

int BsdTShare::findfile(char *fname)
{
	for(BsdTFile *f =  &file[used-1]; f >= file; f--)
		if (!strcmp(f->fname, fname))
			return f - file;
	return -1;
}

DType *BsdTShare::findtype(int fnum, int offset)
{
	if (offset >= file[fnum].ntypes)
		return 0;
	return file[fnum].type[offset];
}

char *BsdTShare::filename(int fnum)
{
	return file[fnum].fname;
}

void BsdTShare::entertype(int fnum, int offset, DTypep dp)
{
	BsdTFile *f = &file[fnum];
	if (offset >= f->ntypes) {
		int n = offset + 5;	// Room to spare
		DTypepar nda = new DTypep[n];
		for(int i = 0; i < f->ntypes; i++)
			nda[i] = f->type[i];
		delete f->type;
		f->type = nda;
		f->ntypes = n;
	}
	f->type[offset] = dp;
}

BsdType::BsdType(Source *sp, BsdTShare *sh)
{
	share = sh;
	src = sp;
	utypeindex = 0;
	nfiles = 10;
	filemap = new int[nfiles];
	filemap[0] = share->addfile(src->text(), 1);
	used = 1;
}

BsdType::~BsdType()
{
	delete filemap;
}

void BsdType::addinclude(char *fname, int flg)
{
	if (used == nfiles) {	// More mapping space needed
		nfiles += 10;
		int *map = new int[nfiles];
		for(int i = 0; i < used; i++)
			map[i] = filemap[i];
		delete filemap;
		filemap = map;
	}
	if (flg)
		filemap[used] = share->addfile(fname);
	else
		filemap[used] = share->findfile(fname);
	used++;
}

/*
 * Parse the yucky '=' symbol table information
 * Possiblities:
 *	(a,b)=(x,y)				usually typedefs
 *	(a,b)=*(x,y)				pointers
 *	(a,b)=f(x,y)				functions
 *	(a,b)=ar(0,1);lo;hi;(x,y)		arrays
 *	(a,b)=r(x,y);lo;hi;			range - who cares!
 *	(a,b)=eN:size,...;			enum encoding
 *	(a,b)=sSN:(x,y),off,size;...;;		structure encoding
 *	(a,b)=uSN:(x,y),off,size;...;;		union encoding
 *	(a,b)=x{esu}name:			ptr to undefined utype
 */
void BsdType::parsetype(char *equalptr, char *name)
{
	int file, offset;
	int pfile, poffset;
	struct DType *dp, *dp1;
	int ssize;
	char stype;
	char sname[100], *utypename;

	char *nextequal = strchr(equalptr+1, '=');
	if (nextequal)
		parsetype (nextequal, name);
	register char *tp = equalptr;
	while (*--tp != '(')
		;
	toindices(tp, file, offset);
	register char *cp = equalptr + 1;
	switch ( *cp ) {
		case '(':
			toindices(cp, pfile, poffset);
			dp1 = share->findtype(pfile,poffset);
			dp = new DType;
			dp->pcc = dp1->pcc;
			dp->dim = dp1->dim;
			dp->univ = dp1->univ;
			share->entertype(file, offset, dp);
			cp = strchr(cp, ')') + 1;
			break;
		case '*':
			toindices(++cp, pfile, poffset);
			cp = strchr(cp, ')') + 1;
			dp = new DType;
			dp->univ = share->findtype(pfile,poffset);
			if (!dp->univ)
				dp->over = (pfile << 16) | poffset;
			dp->pcc = PTR;
			share->entertype(file, offset, dp);
			break;
		case 'f':
			toindices(++cp, pfile, poffset);
			cp = strchr(cp, ')') + 1;
			dp = new DType;
			dp->univ = share->findtype(pfile,poffset);
			if (!dp->univ)
				dp->over = (pfile << 16) | poffset;
			dp->pcc = FTN;
			share->entertype(file, offset, dp);
			break;
		case 'a':
			cp = strchr(cp, ';') + 1;
			cp = strchr(cp, ';') + 1;
			dp = new DType;
			dp->dim = toint(cp) + 1;
			dp->pcc = ARY;
			cp = strchr(cp, ';') + 1;
			toindices(cp, pfile, poffset);
			cp = strchr(cp, ')') + 1;
			dp->univ = share->findtype(pfile,poffset);
			if (!dp->univ)
				dp->over = (pfile << 16) | poffset;
			share->entertype(file, offset, dp);
			break;
		case 'r':
			cp = strchr(cp, ';') + 1;
			cp = strchr(cp, ';') + 1;
			cp = strchr(cp, ';') + 1;
			break;
		case 'x':
			stype = *++cp;
			ssize = 0;
			cp++;
			for(char *to = utypename = sname; *cp != ':'; )
				*to++ = *cp++;
			*to = 0;
			cp++;
			goto xentry;
		case 's': 
		case 'u':
		case 'e':
			if (*--tp == 'T' || *tp == 't') {
				*strchr(name, ':') = 0;
				utypename = name;
			} else {
				sprintf(sname, "%s.%d",
					basename(share->filename(file)),
					offset);
				utypename = sname;
			}
			stype = *cp++;
			ssize = toint(cp);
			if (stype == 'e')
				ssize = 1;
xentry:
			UType *u = (UType*) src->symtab->idtosym(
						U_UTYPE, utypename, 0);
			if( u ){
				if( u->range.lo < ssize ){
					if( u->encode )
						delete u->encode;
					u->encode = toutypestr(cp, stype);
					u->range.lo = ssize;
					u->bsdp = this;
				} else if (ssize)
					delete toutypestr(cp, stype);
			} else {
				++UTypeStubs;
				if (utypename == sname)
					utypename = sf("%s", sname);
				u = new UType(src->symtab,
				    ssize ? toutypestr(cp, stype) : (char *)0,
				    utypename, this);
				u->range.lo = ssize;
				switch(stype) {
					case 'e':
						u->type.pcc = ENUMTY;
						break;
					case 's':
						u->type.pcc = STRTY;
						break;
					case 'u':
						u->type.pcc = UNIONTY;
						break;
				}
				u->type.univ = u;
				u->rsib = src->symtab->utype;
				src->symtab->utype = u;
			}
			if (!share->findtype(file, offset)) {
				dp = new DType;
				dp->univ = u;
				dp->pcc = u->type.pcc;
				share->entertype(file, offset, dp);
			}
			break;
		default:
			break;
	}
	memset(equalptr, ' ', cp - equalptr);
}

DType BsdType::gettype(char *string)
{
	int file, offset;
	string = strchr(string, '(');
	toindices(string, file, offset);
	return gettype(file, offset);
}

DType BsdType::gettype(int fnum, int offset)
{
	return chain(share->findtype(fnum, offset));
}

DType BsdType::chain(DType *dp)
{
	DType d;

	if (!dp) {		// Shouldn't happen - fix this if ever figure
		d.pcc = INT;	// out how the include file references are
		return d;	// shared. I give up ....
	}
	d.pcc = dp->pcc;
	d.dim = dp->dim;
	d.univ = dp->univ;
	if( d.pcc & TMASK ){
		if (!dp->univ)
			dp->univ = share->findtype(dp->over>>16,
					dp->over&0xffff);
		d.univ = new DType;
		*(d.ref()) = chain(dp->ref());
	}
	return d;
}

void BsdType::toindices(char *cp, int &fnum, int &offset)
{
	while (*cp++ != '(')
		;
	fnum = filemap[toint(cp)];
	while (*cp++ != ',')
		;
	offset = toint(cp);
}

int BsdType::toint(char *cp)
{
	int cnt = 0;
	int sign = 1;

	if (*cp == '-') {
		sign = -1;
		cp++;
	}
	while (*cp >= '0' && *cp <= '9')
		cnt = cnt * 10 + *cp++ - '0';
	return cnt * sign;
}

char *BsdType::toutypestr(char *cp, int c)
{
	char *bp;
	int size;
	char *cstart;
	register char *to, *from;

	if (c == 'e') {
		cstart = cp;
		for(size = 0; ; cp++) {
			if (*cp == '\\') {
				*cp++ = ' ';
				*cp = ' ';
			}
			if (*cp != ' ')
				size++;
			if (*cp == ';')	/* Enum delimiter */
				break;
		}
	} else {
		while (*cp >= '0' && *cp <= '9')
			*cp++ = ' ';
		cstart = cp;
		for(size = 0; ; cp++) {
			if (*cp == '\\') {
				*cp++ = ' ';
				*cp = ' ';
				char *nextequal = strchr(cp+1, '=');
				if (nextequal)
					parsetype (nextequal, nextequal);
			}
			if (*cp != ' ')
				size++;
			if (*cp == ';' && *(cp-1) == ';')
				break;
		}
	}
	to = bp = new char[size + 1];
	from = cstart;
	for (register i = size; size--; ) {
		while (*from == ' ')
			from++;
		*to++ = *from;
		*from++ = ' ';
	}
	*to = 0;
	return bp;
}
