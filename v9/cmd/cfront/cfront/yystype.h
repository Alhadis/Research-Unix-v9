/*ident	"@(#)cfront:src/yystype.h	1.4" */
typedef union {
	char*	s;
	TOK	t;
	int	i;
	loc	l;
	Pname	pn;
	Ptype	pt;
	Pexpr	pe;
	Pstmt	ps;
	PP	p;
} YYSTYPE;
extern YYSTYPE yylval;
