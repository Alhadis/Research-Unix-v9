
typedef union  {
    char *sval;
    int ival;
    short shval;
    struct _menuline *mlval;
    struct _menuinfo *mival;
    char **cval;
} YYSTYPE;
extern YYSTYPE yylval;
# define NL 257
# define STRING 258
# define COMMENT 259
