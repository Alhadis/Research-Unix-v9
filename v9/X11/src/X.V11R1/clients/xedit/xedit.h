/*
 *	rcs_id[] = "$Header: xedit.h,v 1.8 87/09/11 08:22:22 toddb Exp $";
 */

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


#include <stdio.h>
#include <strings.h>
#include <sys/file.h>

#ifdef X11
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xtlib.h>
#include <X11/TextDisp.h>
#endif X11

#ifdef X10
#include <Xlib.h>
#include <Xtlib.h>
#include <TextDisp.h>
#endif X10

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
/* Handle enums that are in process of being changed ... */

#ifdef X11
#define QRootWindow   		RootWindow
#define Feep()			XBell(CurDpy, 50)
#define QXFetchBuffer		XFetchBuffer
#define QXDefineCursor		XDefineCursor
#define QXStoreName		XStoreName
#define QXtButtonBoxCreate	XtButtonBoxCreate
#define QXtButtonBoxAddButton	XtButtonBoxAddButton
#define QXtButtonBoxGetValues	XtButtonBoxGetValues
#define QXtLabelCreate		XtLabelCreate
#define QXtLabelSetValues	XtLabelSetValues
#define QXtTextSourceCreate	XtTextSourceCreate
#define QXtVPanedRefigureMode	XtVPanedRefigureMode
#define QXtVPanedWindowAddPane	XtVPanedWindowAddPane
#define QXtVPanedWindowCreate	XtVPanedWindowCreate
#define QXtTextGetInsertionPoint XtTextGetInsertionPoint
#define QXtTextReplace		XtTextReplace	
#define QXtTextUnsetSelection	XtTextUnsetSelection
#define QXtTextSetNewSelection	XtTextSetNewSelection
#define QXtTextSetInsertionPoint XtTextSetInsertionPoint
#define QXtTextNewSource	XtTextNewSource
#define QXtButtonBoxDeleteButton XtButtonBoxDeleteButton
#define QXtCommandCreate	XtCommandCreate
#define QXtTextStringCreate	XtTextStringCreate
#define QXtTextInvalidate	XtTextInvalidate
#define QXMapWindow		XMapWindow
#define QXUnmapWindow		XUnmapWindow
#define QXNextEvent		XNextEvent
#define QXtGetResources		XtGetResources
#endif X11

#ifdef X10
#define XtsdRight			sdRIGHT
#define XtsdLeft			sdLEFT
#define XtstPositions			stPOSITIONS
#define XtstEOL				stEOL
#define XtstParagraph			stPARAGRAPH
#define XtstWhiteSpace			stWHITESPACE
#define XtstFile			stFILE
#define Feep()				XFeep(0)
#define QRootWindow(dpy, scrn)		RootWindow
#define QXFetchBuffer(disp, buf, num)	XFetchBuffer(buf, num)
#define QXMapWindow(dpy, win)		XMapWindow(win)
#define QXUnmapWindow(dpy, win)		XUnmapWindow(win)
#define QXNextEvent(dpy, event)		XNextEvent(event)
#define QXDefineCursor(dpy, win, cur)	XDefineCursor(win, cur)
#define QXStoreName(dpy, win, name)	XStoreName(win, name)
#define QXtButtonBoxCreate(dpy,win,arg,num) XtButtonBoxCreate(win, arg, num)
#define QXtButtonBoxAddButton(d,w,a,n)	XtButtonBoxAddButton(w,a,n)
#define QXtButtonBoxGetValues(d,w,a,n)	XtButtonBoxGetValues(w,a,n)
#define QXtLabelCreate(d,w,a,n)		XtLabelCreate(w,a,n)
#define QXtLabelSetValues(d,w,a,n)	XtLabelSetValues(w,a,n)
#define QXtTextSourceCreate(d,w,a,n,s)	XtTextSourceCreate(w,a,n,s)
#define QXtVPanedRefigureMode(d,w,r) 	XtVPanedRefigureMode(w,r)
#define QXtVPanedWindowAddPane(d,p,w,i,x,y,r) XtVPanedWindowAddPane(p,w,i,x,y,r)
#define QXtVPanedWindowCreate(d,w,a,n)	XtVPanedWindowCreate(w,a,n)
#define QXtTextGetInsertionPoint(d,w)	XtTextGetInsertionPoint(w)
#define QXtTextSetInsertionPoint(d,w,i)	XtTextSetInsertionPoint(w,i)
#define QXtTextReplace(d,w,b,e,t)	XtTextReplace(w,b,e,t)
#define QXtTextUnsetSelection(d,w)	XtTextUnsetSelection(w)
#define	QXtTextSetNewSelection(d,w,b,e)	XtTextSetNewSelection(w,b,e)	
#define QXtTextNewSource(d,w,s,i)	XtTextNewSource(w,s,i)
#define QXtButtonBoxDeleteButton(d,w,a,n) XtButtonBoxDeleteButton(w,a,n) 
#define QXtCommandCreate(d,w,a,n)	XtCommandCreate(w,a,n)
#define QXtTextStringCreate(d,w,a,n)	XtTextStringCreate(w,a,n)
#define QXtTextInvalidate(d,w,f,t)	XtTextInvalidate(w,f,t)
#define QXtGetResources(dpy,a,b,c,d,e,f,g,h,i) XtGetResources(a,b,c,d,e,f,g,h,i)
#define XrmResourceDataBase		ResourceDataBase
#define XrmRInt				XtRInt	
#define XrmRBoolean			XtRBoolean
#define XrmRString			XtRString
#define  XrmNameList			XtNameList
#define  XrmClassList			XtClassList
#define XrmOptionDescRec		OptionDescRec
#define XrmoptionStickyArg		XtoptionStickyArg
#define XrmGetDataBase			XtGetDataBase
#define XrmSetCurrentDataBase		XtSetCurrentDataBase
#define XrmParseCommand			XtParseCommand
#endif X10


#define MakeArg(n, v){  	args[numargs].name = n; 	\
			        args[numargs].value = v;	\
			        numargs++; 			\
		      }

/*	misc externs 	*/
extern XtTextSource *TCreateISSource();
extern XtTextSource *CreatePSource();
extern XtTextSource *TCreateApAsSource();
extern DestroyPSource();
extern PSchanges();
extern TDestroyApAsSource();
extern char *malloc();
extern char *realloc();
extern char *calloc();


/*	externs in xedit.c	*/
extern Window  searchstringwindow;
extern Window editbutton;
extern char *filename;
extern char *savedfile;
extern char *loadedfile;
extern Editable;
extern backedup;
extern saved;
extern lastChangeNumber;
extern Window Row1;
extern char *searchstring;
extern char *replacestring;
extern Window master;
extern Window textwindow;
extern Window messwindow;
extern Window labelwindow;
extern XtTextSource *source, *asource, *dsource, *psource, *messsource;
extern editInPlace;
extern enableBackups;
extern char *backupNamePrefix;
extern char *backupNameSuffix;

extern Display *CurDpy;


/*	externals in util.c 	*/
extern DoLine();
extern DoJump();
extern XeditPrintf();
extern setWidgetValue();
extern getWidgetValue();
extern Window makeCommandButton();
extern Window makeBooleanButton();
extern Window makeStringBox();
extern FixScreen();

/*	externs in commands.c 	*/
extern DoQuit();
extern DoReplaceOne();
extern DoReplaceAll();
extern DoSearchRight();
extern DoSearchLeft();
extern DoUndo();
extern DoUndoMore();
extern DoSave();
extern DoLoad();
extern DoEdit();
