#ifndef lint
static char rcs_id[] = "$Header: command.c,v 1.13 87/09/11 08:18:00 toddb Exp $";
#endif lint
/*
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT RIGHTS,
 * APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN ADDITION TO THAT
 * SET FORTH ABOVE.
 *
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting documentation,
 * and that the name of Digital Equipment Corporation not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 */

/* command.c -- interface to exec mh commands. */

#include "xmh.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>

#ifndef FD_SET
#define NFDBITS         (8*sizeof(fd_set))
#define FD_SETSIZE      NFDBITS
#define FD_SET(n, p)    ((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define FD_CLR(n, p)    ((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define FD_ISSET(n, p)  ((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)      bzero((char *)(p), sizeof(*(p)))
#endif



/* Return the full path name of the given mh command. */

static char *FullPathOfCommand(str)
  char *str;
{
    static char result[100];
    (void) sprintf(result, "%s/%s", defMhPath, str);
    return result;
}


static int childdone;		/* Gets nonzero when the child process
				   finishes. */
ChildDone()
{
    childdone++;
}

/* Execute the given command, and wait until it has finished.  While the
   command is executing, watch the X socket and cause Xlib to read in any
   incoming data.  This will prevent the socket from overflowing during
   long commands. */

DoCommand(argv, inputfile, outputfile)
  char **argv;			/* The command to execute, and its args. */
  char *inputfile;		/* Input file for command. */
  char *outputfile;		/* Output file for command. */
{
    FILEPTR fid;
    int pid;
    fd_set readfds, fds;
    FD_ZERO(&fds);
    FD_SET(QConnectionNumber(theDisplay), &fds);
if (debug) {(void)fprintf(stderr, "Executing %s ...", argv[0]); (void) fflush(stderr);}
    childdone = FALSE;
    (void) signal(SIGCHLD, ChildDone);
    pid = fork();
    if (pid == -1) Punt("Couldn't fork!");
    if (pid) {			/* We're the parent process. */
	while (!childdone) {
	    readfds = fds;
	    (void) select(QConnectionNumber(theDisplay)+1, (int *) &readfds,
			  (int *) NULL, (int *) NULL, (struct timeval *) NULL);
	    if (FD_ISSET(QConnectionNumber(theDisplay), &readfds)) {
		(void) QXPending(theDisplay);
	    }
	}
	(void) wait((union wait *) NULL);
if (debug) (void)fprintf(stderr, " done\n");
    } else {			/* We're the child process. */
	if (inputfile) {
	    fid = FOpenAndCheck(inputfile, "r");
	    (void) dup2(fileno(fid), fileno(stdin));
	}
	if (outputfile) {
	    fid = FOpenAndCheck(outputfile, "w");
	    (void) dup2(fileno(fid), fileno(stdout));
	}
	if (!debug) {		/* Throw away error messages. */
	    fid = FOpenAndCheck("/dev/null", "w");
	    (void) dup2(fileno(fid), fileno(stderr));
	    if (!outputfile)
		(void) dup2(fileno(fid), fileno(stderr));
	}
	(void) execv(FullPathOfCommand(argv[0]), argv);
	(void) execvp(argv[0], argv);
	Punt("Execvp failed!");
    }
}



/* Execute the given command, waiting until it's finished.  Put the output
   in a newly mallocced string, and return a pointer to that string. */

char *DoCommandToString(argv)
char ** argv;
{
    char *result;
    char *file;
    int fid, length;
    file = DoCommandToFile(argv);
    length = GetFileLength(file);
    result = XtMalloc((unsigned) length + 1);
    fid = myopen(file, O_RDONLY, 0666);
    if (length != read(fid, result, length))
	Punt("Couldn't read result from DoCommandToString");
    result[length] = 0;
if (debug) (void) fprintf(stderr, "('%s')\n", result);
    (void) myclose(fid);
    DeleteFileAndCheck(file);
    return result;
}
    

#ifdef NOTDEF	/* This implementation doesn't work right on null return. */
char *DoCommandToString(argv)
  char **argv;
{
    static char result[1030];
    int fildes[2], pid, l;
if (debug) {(void)fprintf(stderr, "Executing %s ...", argv[0]); (void) fflush(stderr);}
    (void) pipe(fildes);
    pid = vfork();
    if (pid == -1) Punt("Couldn't fork!");
    if (pid) {
	while (wait((union wait *) 0) == -1) ;
	l = read(fildes[0], result, 1024);
	if (l <= 0) Punt("Couldn't read result from DoCommandToString");
	(void) myclose(fildes[0]);
	result[l] = 0;
	while (result[--l] == 0) ;
	while (result[l] == '\n') result[l--] = 0;
if (debug) (void)fprintf(stderr, " done: '%s'\n", result);
	return result;
    } else {
	(void) dup2(fildes[1], fileno(stdout));
	(void) execv(FullPathOfCommand(argv[0]), argv);
	(void) execvp(argv[0], argv);
	Punt("Execvp failed!");
	return NULL;
    }
}
#endif



/* Execute the command to a temporary file, and return the name of the file. */

char *DoCommandToFile(argv)
  char **argv;
{
    char *name;
    name = MakeNewTempFileName();
    DoCommand(argv, (char *) NULL, name);
    return name;
}
