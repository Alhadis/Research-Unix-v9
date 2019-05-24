// <string.h>: the C string functions, UNIX Programmer's Manual vol 1 section 3

extern char* strcat(char*, const char*);
extern char* strncat(char*, const char*, int);
extern int strcmp(const char*, const char*);
extern int strncmp(const char*, const char*, int);
extern char* strcpy(char*, const char*);
extern char* strncpy(char*, const char*, int);
extern int strlen(const char*);
// bsd:
//extern char* index(char*, int);
//extern char* rindex(char*, int);
// system V:
//extern char* strchr(char*, char);
//extern char* strrchr(char*, char);
//extern char* strpbrk(char*, char*);
//extern int strspn(char*, char*);
//extern int strcspn(char*, char*);
//extern char* strtok(char*, char*);
