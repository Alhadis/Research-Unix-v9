/* 
 * $Locker:  $ 
 */ 
static char     *rcsid = "$Header: mkPixmap.c,v 1.2 87/06/17 16:20:05 swick Exp $";
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "test.h"

extern Display *dpy;
extern RootInfoRec *findRootInfo();

Pixmap 
MakePixmap(root, data, width, height) Drawable root; short *data; int width, height;
{
    XImage ximage;
    GC pgc;
    XGCValues gcv;
    Pixmap pid;
    int scr = findRootInfo(root) - RootInfo;

    pid = XCreatePixmap(dpy, root, width, height, DefaultDepth(dpy, scr));

    gcv.foreground = BlackPixel(dpy, scr);
    gcv.background = WhitePixel(dpy, scr);
    pgc = XCreateGC(dpy, pid, GCForeground | GCBackground, &gcv);
    pid = XCreatePixmap(dpy, root, width, height, DefaultDepth(dpy, scr));

    ximage.height = height;
    ximage.width = width;
    ximage.xoffset = 0;
    ximage.format = XYBitmap;
    ximage.data = (char *)data;
    ximage.byte_order = LSBFirst;
    ximage.bitmap_unit = 16;
    ximage.bitmap_bit_order = LSBFirst;
    ximage.bitmap_pad = 16;
    ximage.bytes_per_line = (width+15)/16 * 2;
    ximage.depth = 1;

    XPutImage(dpy, pid, pgc, &ximage,0, 0,  0, 0, width, height);
    XFreeGC(dpy, pgc);
    return(pid);
}
