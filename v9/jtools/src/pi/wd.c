#include "univ.h"
#include <CC/sys/types.h>
#include <sys/param.h>
#include <CC/sys/stat.h>
#include <dirent.h>
#include "wd.h"
SRCFILE("wd.c")

void NewWd() { new Wd; }

Wd::Wd()
{
	pad = new Pad((PadRcv*) this);
	pad->name("pwd/cd");
	pad->banner("Working Directory:");
	prevwd = 0;
	pwd();
}

Index Wd::carte()
{
	struct stat stbuf;
	Menu m;
	struct dirent *dp;
	DIR *dirp;

	dirp = ::opendir(".");
	if (dirp == NULL)
		return m.index();
	for ( dp = ::readdir(dirp); dp!=NULL; dp=::readdir(dirp) ) {
		char dn[25];
		sprintf(dn, "%0.25s", dp->d_name);
		int len = strlen(dn);
		if( len>=3 && dn[len-2]=='.' ) continue;
		if( dp->d_ino == 0 
		 || ::stat(dn, &stbuf)== -1
		 || (stbuf.st_mode&S_IFMT)!=S_IFDIR )
			continue;
		long opand = (long) sf("%s/%s", getwd, dn);
		m.sort(sf("%s\240", dn), (Action)&Wd::kbd, opand);
	}
	::closedir(dirp);
	return m.index();
}

char *Getwd()
{
	char pathname[MAXPATHLEN];
	char *getwd(char*);

	if (getwd(pathname) == 0)
		return "getwd error";
	return sf("%s",pathname);
}

char *Wd::help()
{
	trace( "%d.help()", this );
	return "[cd] <path> {change working directory}";
}

char *Wd::kbd(char *s)
{
	trace( "%d.kbd(%s)", this, s );		OK("kbd");
	if( s[0]=='c' && s[1]=='d' && (s[2]==' '||s[2]==0) ) s += 2;
	while( *s == ' ' ) ++s;
	if( *s==0 ){
		char *getenv(char*), *e = getenv("HOME");
		if( e ) s = e;
	}
	if( chdir(s) == -1 ){
		pad->insert(key++, "cannot cd %s", s);
		prevwd = 0;
	}
	pwd(SELECTLINE);
	return 0;
}

void Wd::pwd(Attrib a)
{
	if( prevwd )
		pad->insert(key, a, (PadRcv*)this, ix, "%s", getwd);
	getwd = Getwd();
	ix = carte();
	pad->menu(ix);
	pad->insert(++key, a|DONT_CUT, (PadRcv*)this, ix, "wd=%s", getwd);
	prevwd = getwd;
}
