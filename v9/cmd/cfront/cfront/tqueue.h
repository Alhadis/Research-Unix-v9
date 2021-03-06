/*ident	"@(#)cfront:src/tqueue.h	1.4" */
#ifndef EOF
#include <stdio.h>
#else
extern printf(const char* ...);
extern fprintf(const FILE*, const char* ...);

#endif
struct toknode {
	TOK      tok;			/* token for parser */
	YYSTYPE  retval;			/* $arg */
	toknode* next;
	toknode* last;
		toknode(TOK,YYSTYPE);
		inline ~toknode();
};
extern toknode* front;
extern toknode* rear;

extern void addtok(TOK,YYSTYPE);	/* add tok to rear of Q */
extern TOK  deltok();			/* take tok from front of Q */

// extern char* image(TOK);
extern void tlex();
extern TOK lalex();
extern void* malloc(unsigned);

extern YYSTYPE yylval;
extern TOK tk;				// last token returned;

extern char* image(TOK);
