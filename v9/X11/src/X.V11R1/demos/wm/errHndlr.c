/* $Header: errHndlr.c,v 1.1 87/09/11 08:23:29 toddb Exp $ */
#include <stdio.h>
#include <X11/Xlib.h>

int errorStatus = False;

int ErrorHandler (dpy, event)
    Display *dpy;
    XErrorEvent *event;
{
#ifdef debug
    char *buffer[BUFSIZ];
    XGetErrorText(dpy, event->error_code, buffer, BUFSIZ);
    (void) fprintf(stderr, "X Error: %s\n", buffer);
    (void) fprintf(stderr, "  Request Major code: %d\n", event->request_code);
    (void) fprintf(stderr, "  Request Minor code: %d\n", event->minor_code);
    (void) fprintf(stderr, "  ResourceId 0x%x\n", event->resourceid);
    (void) fprintf(stderr, "  Error Serial #%d\n", event->serial);
    (void) fprintf(stderr, "  Current Serial #%d\n", dpy->request);
#endif
    errorStatus = True;
    return 0;
}
