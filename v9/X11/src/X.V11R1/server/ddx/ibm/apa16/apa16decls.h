/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Header: apa16decls.h,v 5.4 87/09/13 03:11:06 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/apa16/RCS/apa16decls.h,v $ */

#if !defined(lint) && !defined(LOCORE) && defined(RCS_HDRS)
static char *rcsidapa16decls = "$Header: apa16decls.h,v 5.4 87/09/13 03:11:06 erik Exp $";
#endif

/* apa16bitblt.c */
extern	void		apa16CopyArea();
extern	int		apa16DoBitblt();

/* apa16cursor.c */
extern	int		apa16CursorInit();
extern	Bool		apa16RealizeCursor();
extern	Bool		apa16UnrealizeCursor();
extern	int		apa16DisplayCursor();

/* apa16fillsp.c */
extern	void		apa16SolidFS();
extern	void		apa16StippleFS();

/* apa16gc.c */
extern	Bool		apa16CreateGC();
extern	void		apa16ValidateGC();
extern	void		apa16DestroyGC();

/* apa16hdwr.c */
extern	int		apa16_qoffset;
extern	CARD16		apa16_rop2stype[];

/* apa16imggblt.c */
extern	void		apa16ImageGlyphBlt();

/* apa16io.c */
extern	Bool		apa16ScreenInit();
extern	Bool		apa16ScreenClose();
extern	Bool		apa16SaveScreen();

/* apa16line.c */
extern	void		apa16LineSS();

/* apa16plygblt.c */
extern	void		apa16PolyGlyphBlt();

/* apa16pntwin.c */
extern	void		apa16PaintWindowSolid();

/* apa16pnta.c */
extern	void		apa16SolidFillArea();
extern	void		apa16StippleFillArea();

/* apa16window.c */
extern	Bool		apa16CreateWindow();
extern	void		apa16CopyWindow();
extern	Bool		apa16ChangeWindowAttributes();
