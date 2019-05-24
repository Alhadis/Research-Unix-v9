/* $Header: tocutil.h,v 1.1 87/09/11 08:19:32 toddb Exp $ */
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

#ifndef _tocutil_h
#define _tocutil_h

extern Toc TUMalloc();
extern int TUScanFileOutOfDate();
extern void TUScanFileForToc();
extern int TUGetMsgPosition();
extern void TUResetTocLabel();
extern void TURedisplayToc();
extern void TULoadSeqLists();
extern void TURefigureWhatsVisible();
extern void TULoadTocFile();
extern void TUSaveTocFile();
extern void TUEnsureScanIsValidAndOpen();
extern void TURefigureTocPositions();
extern void TUGetFullFolderInfo();
extern Msg TUAppendToc();

#endif _tocutil_h
