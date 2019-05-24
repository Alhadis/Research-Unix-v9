/*
 * $Header: def.h,v 1.2 87/08/20 12:13:09 toddb Exp $
 *
 * $Log:	def.h,v $
 * Revision 1.2  87/08/20  12:13:09  toddb
 * add define for u_char for the sake of USG.
 * 
 * Revision 1.1  87/04/08  16:40:37  rich
 * Initial revision
 * 
 * Revision 1.1  87/04/08  16:40:37  rich
 * Initial revision
 * 
 * Revision 1.2  86/04/18  14:05:40  toddb
 * Added a new field to struct inclist: "i_marked".  This is used
 * by recursive_pr() to tell if it is traversing a loop of include
 * files.
 * 
 * Revision 1.1  86/04/15  08:34:21  toddb
 * Initial revision
 * 
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef USG
#define u_char		unchar
#endif

#define MAXDEFINES	512
#define MAXFILES	512
#define MAXDIRS		10
#define SYMTABINC	10	/* must be > 1 for define() to work right */
#define	TRUE		1
#define	FALSE		0
#define	IF		0
#define	IFDEF		1
#define	IFNDEF		2
#define	ELSE		3
#define	ENDIF		4
#define	DEFINE		5
#define	UNDEF		6
#define	INCLUDE		7
#define	LINE		8
#define	IFFALSE		9	/* pseudo value --- never matched */
#define	INCLUDEDOT	10	/* pseudo value --- never matched */

#ifdef DEBUG
extern int	debug;
#define	debug0	((debug&0x0001)==0) ? debug : log /* show ifn*(def)*,endif */
#define	debug1	((debug&0x0002)==0) ? debug : log /* trace defined/!defined */
#define	debug2	((debug&0x0004)==0) ? debug : log /* show #include */
#define	debug3	((debug&0x0008)==0) ? debug : log /* unused */
#define	debug4	((debug&0x0010)==0) ? debug : log /* unused */
#define	debug5	((debug&0x0020)==0) ? debug : log /* unused */
#define	debug6	((debug&0x0040)==0) ? debug : log /* unused */
#else DEBUG
#define	debug0
#define	debug1
#define	debug2
#define	debug3
#define	debug4
#define	debug5
#define	debug6
#endif DEBUG

typedef	u_char	boolean;

struct symtab {
	char	*s_name;
	char	*s_value;
};

struct	inclist {
	char		*i_incstring;	/* string from #include line */
	char		*i_file;	/* path name of the include file */
	struct inclist	**i_list;	/* list of files it itself includes */
	int		i_listlen;	/* length of i_list */
	struct symtab	*i_defs;	/* symbol table for this file */
	struct symtab	*i_lastdef;	/* last symbol defined */
	int		i_deflen;	/* number of defines */
	boolean		i_defchecked;	/* whether defines have been checked */
	boolean		i_notified;	/* whether we have revealed includes */
	boolean		i_marked;	/* whether it's in the makefile */
	boolean		i_searched;	/* whether we have read this */
};

struct filepointer {
	char	*f_p;
	char	*f_base;
	char	*f_end;
	long	f_len;
	long	f_line;
};

char			*copy();
char			*malloc();
char			*realloc();
char			*basename();
char			*getline();
struct symtab		*slookup();
struct symtab		*defined();
struct symtab		*fdefined();
struct filepointer	*getfile();
struct inclist		*newinclude();
struct inclist		*inc_path();
