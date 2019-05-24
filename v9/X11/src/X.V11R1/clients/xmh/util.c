#ifndef lint
static char rcs_id[] = "$Header: util.c,v 1.14 87/09/11 08:18:35 toddb Exp $";
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

/* util.c -- little miscellaneous utilities. */

#include "xmh.h"
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <ctype.h>

/* Something went wrong; panic and quit. */

Punt(str)
  char *str;
{
    extern void abort();
    (void) fprintf(stderr, "%s\nerrno = %d\007\n", str, errno);
    (void) fflush(stderr);
    abort();
}


int myopen(path, flags, mode)
char *path;
int flags, mode;
{
    int fid;
    fid = open(path, flags, mode);
    if (debug && fid >= 0) fprintf(stderr, "# %d : %s\n", fid, path);
    return fid;
}


FILE *myfopen(path, mode)
char *path, *mode;
{
    FILE *result;
    result = fopen(path, mode);
    if (debug && result)  fprintf(stderr, "# %d : %s\n", result->_file, path);
    return result;
}



int myclose(fid)
{
    if (close(fid) < 0) Punt("Error in myclose!");
    if (debug) fprintf(stderr, "# %d : <Closed>\n", fid);
}


int myfclose(file)
FILE *file;
{
    int fid = file->_file;
    if (fclose(file) < 0) Punt("Error in myfclose!");
    if (debug) fprintf(stderr, "# %d : <Closed>\n", fid);
}



/* Return a unique file name. */

char *MakeNewTempFileName()
{
    static char name[60];
    static int  uniqueid = 0;
    do {
	(void) sprintf(name, "%s/xmh_%ld_%d", tempDir, getpid(), uniqueid++);
    } while (FileExists(name));
    return name;
}


/* Make an array of string pointers big enough to hold n+1 entries. */

char **MakeArgv(n)
  int n;
{
    char **result;
    result = ((char **) XtMalloc((unsigned) (n+1) * sizeof(char *)));
    result[n] = 0;
    return result;
}


char **ResizeArgv(argv, n)
  char **argv;
  int n;
{
    argv = ((char **) XtRealloc((char *) argv, (unsigned) (n+1) * sizeof(char *)));
    argv[n] = 0;
    return argv;
}

/* Open a file, and punt if we can't. */

FILEPTR FOpenAndCheck(name, mode)
  char *name, *mode;
{
    FILEPTR result;
    result = myfopen(name, mode);
    if (result == NULL)
	Punt("Error in FOpenAndCheck");
    return result;
}


/* Read one line from a file. */

static char *DoReadLine(fid, lastchar)
  FILEPTR fid;
  char lastchar;
{
    static char *buf;
    static int  maxlength = 0;
    char   *ptr, c;
    int     length = 0;
    ptr = buf;
    c = ' ';
    while (c != '\n' && !feof(fid)) {
	c = getc(fid);
	if (length++ > maxlength - 5) {
	    if (maxlength)
		buf = XtRealloc(buf, (unsigned) (maxlength *= 2));
	    else
		buf = XtMalloc((unsigned) (maxlength = 512));
	    ptr = buf + length - 1;
	}
	*ptr++ = c;
    }
    if (!feof(fid) || length > 1) {
	*ptr = 0;
	*--ptr = lastchar;
	return buf;
    }
    return NULL;
}


char *ReadLine(fid)
  FILEPTR fid;
{
    return DoReadLine(fid, 0);
}


/* Read a line, and keep the CR at the end. */

char *ReadLineWithCR(fid)
  FILEPTR fid;
{
    return DoReadLine(fid, '\n');
}



/* Delete a file, and Punt if it fails. */

DeleteFileAndCheck(name)
  char *name;
{
    if (strcmp(name, "/dev/null") != 0 && unlink(name) == -1)
	Punt("DeleteFileAndCheck failed!");
}

CopyFileAndCheck(from, to)
  char *from, *to;
{
    int fromfid, tofid, n;
    char buf[512];
    fromfid = myopen(from, O_RDONLY, 0666);
    tofid = myopen(to, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (fromfid < 0 || tofid < 0) Punt("CopyFileAndCheck failed!");
    do {
	n = read(fromfid, buf, 512);
	if (n) (void) write(tofid, buf, n);
    } while (n);
    (void) myclose(fromfid);
    (void) myclose(tofid);
}


RenameAndCheck(from, to)
  char *from, *to;
{
    if (rename(from, to) == -1) {
	if (errno != EXDEV) Punt("RenameAndCheck failed!");
	CopyFileAndCheck(from, to);
	DeleteFileAndCheck(from);
    }
}


char *MallocACopy(str)
  char *str;
{
    return strcpy(XtMalloc((unsigned) strlen(str)+1),str);
}



char *CreateGeometry(gbits, x, y, width, height)
  int gbits;
  Position x, y;
  Dimension width, height;
{
    char   *result, str1[10], str2[10], str3[10], str4[10];
    if (gbits & WidthValue)
	(void) sprintf(str1, "=%d", width);
    else
	(void) strcpy(str1, "=");
    if (gbits & HeightValue)
	(void) sprintf(str2, "x%d", height);
    else
	(void) strcpy(str2, "x");
    if (gbits & XValue)
	(void) sprintf(str3, "%c%d", (gbits & XNegative) ? '-' : '+', abs(x));
    else
	(void) strcpy(str3, "");
    if (gbits & YValue)
	(void) sprintf(str4, "%c%d", (gbits & YNegative) ? '-' : '+', abs(y));
    else
	(void) strcpy(str4, "");
    result = XtMalloc(22);
    (void) sprintf(result, "%s%s%s%s", str1, str2, str3, str4);
    return result;
}


FileExists(file)
  char *file;
{
    return (access(file, F_OK) == 0);
}

LastModifyDate(file)
  char *file;
{
    struct stat buf;
    if (stat(file, &buf)) return -1;
    return buf.st_mtime;
}

CurrentDate()
{
    struct timeval time;
    struct timezone zone;
    (void) gettimeofday(&time, &zone);
    return time.tv_sec;
}

GetFileLength(file)
char *file;
{
    struct stat buf;
    if (stat(file, &buf)) return -1;
    return buf.st_size;
}



#ifdef NOTDEF
SetValues(dpy, window, arglist, argcount)
Display *dpy;
Window window;
ArgList arglist;
int argcount;
{
    (void) XtSendMessage(dpy, window, messageSETVALUES, arglist, argcount);
}


GetValues(dpy, window, arglist, argcount)
Display *dpy;
Window window;
ArgList arglist;
int argcount;
{
    (void) XtSendMessage(dpy, window, messageGETVALUES, arglist, argcount);
}
#endif


ChangeLabel(window, str)
Window window;
char *str;
{
    labelarglist[0].value = (XtArgVal)MallocACopy(str);
    XtLabelSetValues(DISPLAY window, labelarglist, XtNumber(labelarglist));
}




Window CreateTextSW(scrn, position, name, options)
Scrn scrn;
int position;
char *name;
int options;
{
    Window result;
    static Arg arglist[] = {
	{XtNname, NULL},
	{XtNtextOptions, NULL},
	{XtNfile, (XtArgVal)"/dev/null"}
    };
    arglist[0].value = (XtArgVal)name;
    arglist[1].value = (XtArgVal) (scrollVertical | options);
    result = XtTextDiskCreate(DISPLAY scrn->window,
			      arglist, XtNumber(arglist));
    XtVPanedWindowAddPane(DISPLAY scrn->window,
			  result, position, 50, 1000, TRUE);
    return result;
}



Window CreateTitleBar(scrn, position)
Scrn scrn;
int position;
{
    Window result;
    int width, height;
    static Arg arglist[] = {
	{XtNname, (XtArgVal)"titlebar"},
	{XtNlabel, NULL},
    };
    arglist[1].value = (XtArgVal) Version();
    result = XtLabelCreate(DISPLAY scrn->window, arglist, XtNumber(arglist));
    GetWindowSize(result, &width, &height);
    XtVPanedWindowAddPane(DISPLAY scrn->window, result, position,
			  height, height, TRUE);
    return result;
}


GetWindowSize(window, width, height)
Window window;
int *width, *height;		/* RETURN */
{
    WindowBox bbox, rbox;
    if (XtMakeGeometryRequest(DISPLAY window,
			      XtgeometryGetWindowBox, &bbox, &rbox) ==
	    XtgeometryYes) {
	*width = rbox.width;
	*height = rbox.height;
    } else {
#ifdef X11
	Drawable root;
	int x, y;
	unsigned int borderwidth, depth;
	(void) XGetGeometry(theDisplay, window, &root, &x, &y,
			    (unsigned int *)width, (unsigned int *)height,
			    &borderwidth, &depth);
#endif
#ifdef X10
	WindowInfo info;
	XQueryWindow(window, &info);
	*width = info.width;
	*height = info.height;
#endif
    }
}


Feep()
{
#ifdef X11
    XBell(theDisplay, 50);
#endif
#ifdef X10
    XFeep(0);
#endif
}



MsgList CurMsgListOrCurMsg(toc)
  Toc toc;
{
    MsgList result;
    Msg curmsg;
    result = TocCurMsgList(toc);
    if (result->nummsgs == 0 && (curmsg = TocGetCurMsg(toc))) {
	FreeMsgList(result);
	result = MakeSingleMsgList(curmsg);
    }
    return result;
}



Toc SelectedToc(scrn)
Scrn scrn;
{
    return TocGetNamed(BBoxNameOfButton(scrn->curfolder));
}



int strncmpIgnoringCase(str1, str2, length)
char *str1, *str2;
int length;
{
    int i, diff;
    for (i=0 ; i<length ; i++, str1++, str2++) {
        diff = ((*str1 >= 'A' && *str1 <= 'Z') ? (*str1 + 'a' - 'A') : *str1) -
	       ((*str2 >= 'A' && *str2 <= 'Z') ? (*str2 + 'a' - 'A') : *str2);
	if (diff) return diff;
    }
    return 0;
}
