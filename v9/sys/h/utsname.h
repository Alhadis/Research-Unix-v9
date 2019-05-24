/*
 *	structure for the "uname" system call.  This file is
 *	not used by the operating system proper, but is included
 *	here for compatibility with the BTL development UNIX systems.
 */

struct utsname {
	char sysname[15];
	char nodename[15];
	char release[15];
	char version[15];
};
