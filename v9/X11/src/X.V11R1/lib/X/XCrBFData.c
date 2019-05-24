#include "copyright.h"


#include "Xlib.h"

/*
 * MakeBitmap: Routine to make a pixmap from user supplied data.
 *             D is the window the pixmap will be used in (i.e, where
 *             to get depth from).  Data is a pointer to the bit data, and
 *             width & height give the size in bits of the pixmap.
 *
 * The following format is assumed for data:
 *
 *    format=XYPixmap
 *    bit_order=LSBFirst
 *    byte_order=LSBFirst
 *    padding=8
 *    bitmap_unit=8
 *    xoffset=0
 *    no extra bytes per line
 */  
Pixmap XCreateBitmapFromData(display, d, data, width, height)
     Display *display;
     Drawable d;
     char *data;
     int width, height;
{
    XImage ximage;
    GC gc;
    XGCValues gcv;
    Pixmap pix;

    pix = XCreatePixmap(display, d, width, height, 1);
    if (!pix)
      return(0);
    gcv.foreground = 1;
    gcv.background = 0;
    gc = XCreateGC(display, pix, GCForeground|GCBackground, &gcv);
    ximage.height = height;
    ximage.width = width;
    ximage.depth = 1;
    ximage.xoffset = 0;
    ximage.format = XYBitmap;
    ximage.data = data;
    ximage.byte_order = LSBFirst;
    ximage.bitmap_unit = 8;
    ximage.bitmap_bit_order = LSBFirst;
    ximage.bitmap_pad = 8;
    ximage.bytes_per_line = (width+7)/8;

    XPutImage(display, pix, gc, &ximage, 0, 0, 0, 0, width, height);
    XFreeGC(display, gc);
    return(pix);
}
