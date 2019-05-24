
typedef union  {
	char*	s;
	TOK	t;
	int	i;
	loc	l;
	Pname	pn;
	Ptype	pt;
	Pexpr	pe;
	Pstmt	ps;
	Pbase	pb;
	Pnlist	nl;
	Pslist	sl;
	Pelist	el;
	PP	p;	// fudge: pointer to all class node objects
} YYSTYPE;
extern YYSTYPE yylval;
# define EOFTOK 0
# define ASM 1
# define BREAK 3
# define CASE 4
# define CONTINUE 7
# define DEFAULT 8
# define DELETE 9
# define DO 10
# define ELSE 12
# define ENUM 13
# define FOR 16
# define FORTRAN 17
# define GOTO 19
# define IF 20
# define NEW 23
# define OPERATOR 24
# define RETURN 28
# define SIZEOF 30
# define SWITCH 33
# define THIS 34
# define WHILE 39
# define LP 40
# define RP 41
# define LB 42
# define RB 43
# define REF 44
# define DOT 45
# define NOT 46
# define COMPL 47
# define MUL 50
# define AND 52
# define PLUS 54
# define MINUS 55
# define ER 64
# define OR 65
# define ANDAND 66
# define OROR 67
# define QUEST 68
# define COLON 69
# define ASSIGN 70
# define CM 71
# define SM 72
# define LC 73
# define RC 74
# define CAST 113
# define ENDCAST 122
# define MEMPTR 173
# define ID 80
# define STRING 81
# define ICON 82
# define FCON 83
# define CCON 84
# define ZERO 86
# define ASOP 90
# define RELOP 91
# define EQUOP 92
# define DIVOP 93
# define SHIFTOP 94
# define ICOP 95
# define TYPE 97
# define TNAME 123
# define EMPTY 124
# define NO_ID 125
# define NO_EXPR 126
# define ELLIPSIS 155
# define AGGR 156
# define MEM 160
# define PR 175
