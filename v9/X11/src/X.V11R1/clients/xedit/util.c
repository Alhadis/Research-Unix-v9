#ifndef lint
static char rcs_id[] = "$Header: util.c,v 1.6 87/09/11 08:22:16 toddb Exp $";
#endif

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

#include "xedit.h"

XeditPrintf(fmt, arg1, arg2, arg3, arg4)
  char *fmt;
{
  char buf[1024];
  XtTextBlock text;
  
  XtTextPosition pos = (*messsource->scan)(messsource, 0, XtstFile, XtsdRight,1,0);
    sprintf(buf, fmt, arg1, arg2, arg3, arg4);
    text.length = strlen(buf);
    text.ptr = buf;
    QXtTextReplace(CurDpy, messwindow, pos, pos, &text);
    QXtTextSetInsertionPoint(CurDpy, messwindow, pos + text.length);
}

Window makeCommandButton(box, name, function)
  Window box;
  char *name;
  int (*function)();
{
  int numargs;
  Arg args[3];
    numargs = 0;
    MakeArg(XtNlabel, (caddr_t)name);
    MakeArg(XtNfunction, (caddr_t)function);
    MakeArg(XtNname, (caddr_t)name);
    return(QXtCommandCreate(CurDpy, box, args, numargs));
}
/*
Window makeBooleanButton(box, name, value)
  Window box;
  char *name;
  int *value;
{
  Arg args[2];
  int numargs;
    numargs = 0;
    MakeArg(XtNlabel, (caddr_t)name);
    MakeArg(XtNvalue, (caddr_t)value);
    return(QXtBooleanCreate(CurDpy, box, args, numargs));
}
*/

Window makeStringBox(parentBox, string, length)
  Window parentBox;
  char *string;
{
  XtEditType edittype;
  Arg args[4];
  Window StringW;
  int numargs;
    numargs = 0;
    MakeArg(XtNtextOptions, (caddr_t)(/* editable |*/ resizeWidth));
    MakeArg(XtNeditType, (caddr_t)XttextEdit);
    MakeArg(XtNstring,(caddr_t)string); 
    MakeArg(XtNwidth,  (caddr_t)length);
    StringW = QXtTextStringCreate(CurDpy, parentBox, args, numargs);
    return(StringW);  
}
 
FixScreen(from)
    XtTextPosition from;
{
    XtTextPosition to;
    if(from >= 0){
        to = (*source->getLastPos)(source) + 10;
	QXtTextInvalidate(CurDpy,textwindow, (from > 0 ) ? from -1 : from, to); 
	QXtTextSetInsertionPoint(CurDpy, textwindow, from); 
    } else {
	Feep();
    }
}

