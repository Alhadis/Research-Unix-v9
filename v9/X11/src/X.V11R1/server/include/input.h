/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/
#ifndef INPUT_H
#define INPUT_H

#include "misc.h"

#define DEVICE_INIT	0
#define DEVICE_ON	1
#define DEVICE_OFF	2
#define DEVICE_CLOSE	3

#define MAP_LENGTH	256
#define DOWN_LENGTH	32	/* 256/8 => number of bytes to hold 256 bits */
#define OTHERCLIENTS(win) ((OtherClientsPtr)(win)->otherClients)
#define NullGrab ((GrabPtr)NULL)
#define PASSIVEGRABS(win) ((GrabPtr)(win)->passiveGrabs)
#define PointerRootWin ((WindowPtr)PointerRoot)
#define NoneWin ((WindowPtr)None)
#define NullDevice ((DevicePtr)NULL)

typedef unsigned long Leds;
typedef struct _OtherClients *OtherClientsPtr;
typedef struct _GrabRec *GrabPtr;

typedef int (*DeviceProc)();
typedef void (*ProcessInputProc)();

typedef struct _DeviceRec {
    pointer	devicePrivate;
    ProcessInputProc processInputProc;
    Bool	on;			/* used by DDX to keep state */
} DeviceRec, *DevicePtr;

typedef struct {
	int		count;
	DevicePtr	*devices;
} DevicesDescriptor;

typedef struct {
    int			click, bell, bell_pitch, bell_duration;
    Bool		autoRepeat;
    unsigned char	autoRepeats[32];
    Leds		leds;
} KeybdCtrl;

typedef struct {
    KeySym  *map;
    KeyCode minKeyCode,
	    maxKeyCode;
    int     mapWidth;
} KeySymsRec, *KeySymsPtr;

typedef struct {
    int		num, den, threshold;
} PtrCtrl;

extern KeybdCtrl	defaultKeyboardControl;
extern PtrCtrl		defaultPointerControl;

extern int DeliverEvents(/* WindowPtr, xEvent*, int, WindowPtr */);
extern int MaybeDeliverEventsToClient(/*
	WindowPtr, xEvent *, int, Mask, ClientPtr */);

extern DevicePtr AddInputDevice(/* DeviceProc, Bool */);
extern void RegisterPointerDevice();
extern void RegisterKeyboardDevice();

extern void ProcessPointerEvent();
extern void ProcessKeyboardEvent();
extern void ProcessOtherEvent();

extern void InitPointerDeviceStruct();
extern void InitKeyboardDeviceStruct();
extern void InitOtherDeviceStruct();
extern GrabPtr SetDeviceGrab();

extern DevicesDescriptor GetInputDevices();
extern DevicePtr LookupInputDevice();
extern DevicePtr LookupKeyboardDevice();
extern DevicePtr LookupPointerDevice();

extern void CloseDownDevices();
extern int InitAndStartDevices();
extern int NumMotionEvents();

extern void WriteEventsToClient();
extern int EventSelectForWindow();
extern int EventSupressForWindow();

#endif /* INPUT_H */
