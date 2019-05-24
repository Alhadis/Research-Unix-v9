/******************************************************************
Copyright 1987 by Apollo Computer Inc., Chelmsford, Massachusetts.

                        All Rights Reserved

Permission to use, duplicate, change, and distribute this software and
its documentation for any purpose and without fee is granted, provided
that the above copyright notice appear in such copy and that this
copyright notice appear in all supporting documentation, and that the
names of Apollo Computer Inc. or MIT not be used in advertising or publicity
pertaining to distribution of the software without written prior permission.
******************************************************************/

#include "apollo.h"

#define MOTION_BUFFER_SIZE 0
#define NUMSCREENS 1
#define NUMFORMATS 0

PixmapFormatRec *formats;

InitOutput(screenInfo, argc, argv)
    ScreenInfo *screenInfo;
    int argc;
    char **argv;
{
    int i;

    screenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    screenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    screenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    screenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    screenInfo->numPixmapFormats = NUMFORMATS;
    for (i=0; i< NUMFORMATS; i++)
    {
	screenInfo->formats[i].depth = formats[i].depth;
	screenInfo->formats[i].bitsPerPixel = formats[i].bitsPerPixel;
	screenInfo->formats[i].scanlinePad = formats[i].scanlinePad;
    }

    AddScreen(apScreenInit, argc, argv);
}

void
InitInput(argc, argv)
    int argc;
    char *argv[];
{
    DevicePtr ptr_dev, kbd_dev;
    
    ptr_dev = AddInputDevice(apMouseProc, TRUE);

    kbd_dev = AddInputDevice(apKeybdProc, TRUE);

    RegisterPointerDevice(ptr_dev, MOTION_BUFFER_SIZE);
    RegisterKeyboardDevice(kbd_dev);
}
