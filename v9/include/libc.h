/* system calls */
extern unsigned alarm();
extern void nap(), pause();
extern char *sbrk();
extern void exit(), _exit();
extern long lseek();
extern void nice();
extern void profil();
extern long settod();
extern void sync();
extern long time();

/* libc et al */
extern long lcm();
extern double atof(), strtod();
extern long atol();
extern char *crypt();
extern char *ctime();
extern char *ecvt(), *fcvt(), *gcvt();
extern char *galloc();
extern char *getenv();
extern char *getlogin();
extern char *getpass();
extern char *getwd(), *getcwd();
extern char *malloc(), *realloc(), *calloc();
extern char *memcpy(), *memchr(), *memccpy(), *memset();
extern char *mktemp();
extern double frand();
extern char *strcpy(), *strncpy(), *strcat(), *strncat(), *strchr(), *strrchr();
extern char *strpbrk(), *strtok(), *strdup();
extern char *tgetstr(), tgoto();
extern char *ttyname(), *cttyname();
extern char *getline();

#define	NONEXIT		33
