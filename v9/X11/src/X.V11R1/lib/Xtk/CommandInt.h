/*
* $Header: CommandInt.h,v 1.5 87/09/13 23:05:09 newman Exp $
*/

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.  
 * 
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
/*
 *
 *  A few definitions to make Command.c easier to read.
 *   
 */

  /* Yes, this is gross, but the code will be easier to read.
     Trust me.  */
#define ComWx                            cbw->core.x
#define ComWy                            cbw->core.y
#define ComWdepth                        cbw->core.depth
#define ComWwidth                        cbw->core.width
#define ComWheight                       cbw->core.height
#define ComWborder_width                 cbw->core.border_width;
#define ComWbackground                   cbw->core.background_pixel
#define ComWforeground                   cbw->label.foreground
#define ComWfont                         cbw->label.font
#define ComWlabel                        cbw->label.label
#define ComWjustify                      cbw->label.justify
#define ComWinternalWidth                cbw->label.internal_width
#define ComWinternalHeight               cbw->label.internal_height
#define ComWlabelX                       cbw->label.label_x
#define ComWlabelY                       cbw->label.label_y
#define ComWlabelWidth                   cbw->label.label_width
#define ComWlabelHeight                  cbw->label.label_height
#define ComWlabelLen                     cbw->label.label_len
#define ComWnormalGC                     cbw->label.normal_GC
#define ComWgrayGC                       cbw->label.gray_GC
#define ComWgrayPixmap                   cbw->label.gray_pixmap
#define ComWsensitive                 cbw->core.sensitive 
#define ComWdisplaySensitive          cbw->label.display_sensitive
#define ComWcallbackList                 cbw->command.callback_list
#define ComWcallback                     cbw->command.callback
#define ComWclosure                      cbw->command.closure
#define ComWhighlightGC                  cbw->command.highlight_GC
#define ComWinverseGC                    cbw->command.inverse_GC
#define ComWinverseTextGC                cbw->command.inverse_text_GC
#define ComWhighlightThickness           cbw->command.highlight_thickness
#define ComWset                          cbw->command.set
#define ComWhighlighted                  cbw->command.highlighted
#define ComWdisplaySet                   cbw->command.display_set
#define ComWdisplayHighlighted           cbw->command.display_highlighted

#define XtCBField(cbw,field)  cbw->command.field
#define XtLField(cbw,field)   cbw->label.field
#define XtCField(cbw,field)   cbw->core.field


static void Initialize();
static void Realize();
static void Resize();
static void Redisplay();
static Boolean SetValues();
static void Set();
static void Notify();
static void Unset();
static void Highlight();
static void Unhighlight();
static void Unset();
static void Destroy();
static void ClassInitialize();
