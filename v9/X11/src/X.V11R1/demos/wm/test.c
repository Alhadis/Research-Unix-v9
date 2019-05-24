/* 
 * $Locker:  $ 
 */ 
static char     *rcsid = "$Header: test.c,v 1.2 87/06/17 16:08:52 swick Locked ";
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/X10.h>
#include "test.h"
#include "externs.h"
#include "dblArrow.h"
#include "growBox.h"
#include "cross.h"

XAssocTable *frameInfo;
XFontStruct *font;
Display *dpy;
int ErrorFunc();

extern char *malloc();

void MoveOutline();

RootInfoRec *
findRootInfo(win)
    Window win;
{
    int	i;

    for (i = 0; i < ScreenCount(dpy); i++) {
	if (RootInfo[i].root == win)
	    return (&(RootInfo[i]));
    }
    return (NULL);
}

#ifdef	debug
static char *eventtype[] = {
	"zero",
	"one",
	"KeyPress",
	"KeyRelease",
	"ButtonPress",
	"ButtonRelease",
	"MotionNotify",
	"EnterNotify",
	"LeaveNotify",
	"FocusIn",
	"FocusOut",
	"KeymapNotify",
	"Expose",
	"GraphicsExpose",
	"NoExpose",
	"VisibilityNotify",
	"CreateNotify",
	"DestroyNotify",
	"UnmapNotify",
	"MapNotify",
	"MapRequest",
	"ReparentNotify",
	"ConfigureNotify",
	"ConfigureRequest",
	"GravityNotify",
	"ResizeRequest",
	"CirculateNotify",
	"CirculateRequest",
	"PropertyNotify",
	"SelectionClear",
	"SelectionRequest",
	"SelectionNotify",
	"ColormapNotify",
	"ClientMessage",
	"MappingNotify",
};
#endif	debug

main(argc, argv) int argc; char *argv[];
{
    Window      parent;
    Window     *children;
    int         nchildren;
    int         i;
    Window      dragWindow = None;
    Window      resizeWindow = None;
    Window      iconifyWindow = None;
    int         dragx, dragy;
    int         iconPressx, iconPressy;
    int         dragHeight, dragWidth;
    int         origx, origy, origWidth, origHeight;
    int         clampTop, clampBottom, clampLeft, clampRight;
    int         buttonsPressed = 0;
    int         buttonsDown[128]; /* XXX */
    int         hdrWidth;

    XSetWindowAttributes attributes;
    XGCValues   gcv;

    for (i = 0; i < 128; i++) buttonsDown[i] = 0; /* XXX */
    StartConnectionToServer(argc, argv);
    XSetErrorHandler(ErrorHandler);
#ifdef debug
    XSynchronize(dpy, 1);
#endif
    frameInfo = XCreateAssocTable(ASSOCTABLESZ);
    if (!frameInfo) {
	Error(1, "XCreateAssocTable failed\n");
    }

    font = XLoadQueryFont(dpy, "fixed");
    if (!font)
	Error(1, "XFont failed\n");
#ifdef debug
    printf("descent=%d, ascent=%d, width=%d\n",
	   font->max_bounds.descent, font->max_bounds.ascent,
	   font->max_bounds.width);
#endif

    RootInfo = (RootInfoRec *) malloc(ScreenCount(dpy) * sizeof(RootInfoRec));
    for (i = 0; i < ScreenCount(dpy); i++) {
	RootInfo[i].root = RootWindow(dpy, i);
	RootInfo[i].doubleArrow = MakePixmap(RootInfo[i].root,
					     dblArrow_bits,
					     dblArrow_width,
					     dblArrow_height);
	RootInfo[i].growBox = MakePixmap(RootInfo[i].root,
					 growBox_bits,
					 growBox_width,
					 growBox_height);
	RootInfo[i].cross = MakePixmap(RootInfo[i].root,
				       cross_bits,
				       cross_width,
				       cross_height);
	gcv.font = font->fid;
	gcv.foreground = WhitePixel(dpy, i);
	gcv.background = BlackPixel(dpy, i);
	RootInfo[i].headerGC = XCreateGC(dpy, RootInfo[i].root,
					 GCForeground | GCBackground | GCFont, &gcv);
	gcv.function = GXinvert;
	gcv.subwindow_mode = IncludeInferiors;
	RootInfo[i].xorGC = XCreateGC(dpy, RootInfo[i].root,
				      GCFunction | GCSubwindowMode, &gcv);
	{
	    Window      retroot;
	    int         bw, depth;
	    XGetGeometry(dpy, (Drawable) RootInfo[i].root, &retroot,
			 &RootInfo[i].rootx, &RootInfo[i].rooty,
	    &RootInfo[i].rootwidth, &RootInfo[i].rootheight, &bw, &depth);
	}
	/*
	 * select Substructure Redirect on the root to have MapRequest
	 * events generated instead of windows getting mapped 
	 */
	errorStatus = False;
	attributes.event_mask = (SubstructureRedirectMask|
			ButtonPressMask|ButtonReleaseMask|FocusChangeMask);
	XChangeWindowAttributes(dpy, RootInfo[i].root, CWEventMask, &attributes);
	/* XSelectInput(dpy, root, SubstructureRedirectMask); */
	XSync(dpy, False);
	if (errorStatus == True) {
	    fprintf(stderr, "Are you running another window manager?\n");
	    exit(1);
	}
	if (XQueryTree(dpy, RootInfo[i].root, &RootInfo[i].root, &parent,
		       &children, &nchildren)) {
	    int         i;

	    for (i = 0; i < nchildren; i++) {
		if (MappedNotOverride(children[i]))
		    AddGizmos(children[i]);
	    }
	    if (children)
		free((char *)children);
	}
	XFlush(dpy);
    }
    XSetErrorHandler(ErrorFunc);
    while (1) {
	XEvent      ev;
	XNextEvent(dpy, &ev);
#ifdef debug
	printf("event %s ", eventtype[ev.type]);
#endif
	switch (ev.type) {
	case MapRequest:{
		Window      w;
		GenericAssoc *ga;
		w = ev.xmaprequest.window;
		ga = (GenericAssoc *) XLookUpAssoc(dpy, frameInfo, (XID) w);
		if (ga) {
		    XMapWindow(dpy, ga->frame);
		}
		else {
		    AddGizmos(w);
#ifdef debug
		    printf("delivered to %s of frame %d\n",
			   WindowType(w), Frame(w));
#endif
		}
		break;
	    }
	case UnmapNotify:{
		GenericAssoc *ga;
		Window      w;
		w = ev.xunmap.window;
#ifdef debug
		printf("delivered to %s of frame %d\n",
		       WindowType(w), Frame(w));
#endif
		ga = (GenericAssoc *) XLookUpAssoc(dpy, frameInfo, (XID) w);
		if (ga && (ga->client == w))
		    XUnmapWindow(dpy, ga->frame);
		break;
	    }
	case DestroyNotify:{
		GenericAssoc *ga;
		Window      w;
		w = ev.xdestroywindow.window;
#ifdef debug
		printf("delivered to %s of frame %d\n",
		       WindowType(w), Frame(w));
#endif
		ga = (GenericAssoc *) XLookUpAssoc(dpy, frameInfo, (XID) w);
		if (ga && (ga->client == w))
		    RemoveGizmos(ga);
		break;
	    }
	case PropertyNotify:{
		GenericAssoc *ga;
		Window      w;
		Window	    root;
		int	    x, y, wd, ht, bw, d;

		w = ev.xproperty.window;
		XGetGeometry(dpy, (Drawable) w, &root, &x, &y, &wd, &ht, &bw, &d);
		ga = (GenericAssoc *) XLookUpAssoc(dpy, frameInfo, (XID) w);
		if (!ga || w != ga->client)
		    break;
		switch (ev.xproperty.atom) {
		case XA_WM_NAME:
		    ProcessNewName(root, ga);
		    break;
		case XA_WM_ICON_NAME:
		    ProcessNewIconName(root, ga);
		    break;
		default:
		    break;
		}
		break;
	    }
	case ButtonPress:{
		GenericAssoc *ga;
		Window      w;
		w = ev.xbutton.window;
#ifdef debug
		printf("delivered to %s of frame %d\n",
		       WindowType(w), Frame(w));
#endif
		if (buttonsDown[ev.xbutton.button])
		   break;
		buttonsDown[ev.xbutton.button] = 1;
		if (buttonsPressed++) {
		    iconifyWindow = None;
		    dragWindow = None;
		    resizeWindow = None;
		    MoveOutline(ev.xbutton.root,
				ev.xbutton.x_root - dragx - BORDERWIDTH,
				ev.xbutton.y_root - dragy - BORDERWIDTH,
				0, 0);
		    XUngrabServer(dpy);
		    break;
		}
		ga = (GenericAssoc *) XLookUpAssoc(dpy, frameInfo, (XID) w);
		if (ga && (ga->rbox == w) && resizeWindow == None) {
		    Window      junkRoot;
		    int         junkbw, junkDepth;
		    resizeWindow = ga->frame;
		    XGrabServer(dpy);
		    XGetGeometry(dpy, (Drawable) resizeWindow, &junkRoot,
			&dragx, &dragy, &dragWidth, &dragHeight, &junkbw,
				 &junkDepth);
		    dragx += BW;
		    dragy += BW;
		    origx = dragx;
		    origy = dragy;
		    origWidth = dragWidth;
		    origHeight = dragHeight;
		    clampTop = clampBottom = clampLeft = clampRight = 0;
		}
		else if (ga && (ga->iconWindow == w)) {
		    iconifyWindow = w;
		    if (ev.xbutton.state & ShiftMask) {
			Window      junkRoot;
			int         junkX, junkY, junkbw, junkDepth;
			dragWindow = w;
			dragx = ev.xbutton.x;
			dragy = ev.xbutton.y;
			XGrabServer(dpy);
			XGetGeometry(dpy, (Drawable) dragWindow,
				     &junkRoot, &junkX,
				&junkY, &dragWidth, &dragHeight, &junkbw,
				     &junkDepth);
#ifdef debug
			printf("From: x=%d, y=%d, w=%d, h=%d\n",
			       junkX, junkY, dragWidth, dragHeight);
#endif
			MoveOutline(ev.xbutton.root,
				 ev.xbutton.x_root - dragx - BORDERWIDTH,
				 ev.xbutton.y_root - dragy - BORDERWIDTH,
				    dragWidth + 2 * BORDERWIDTH,
				    dragHeight + 2 * BORDERWIDTH);
		    }
		}
		else if (ga && (ga->upbox == w)) {
		    XWindowChanges wc;
		    wc.stack_mode = Opposite;
		    XConfigureWindow(dpy, ga->frame, CWStackMode, &wc);
		}
		else if (ga && (ga->iconify == w) && (iconifyWindow == None)) {
		    Window      junkRoot;
		    int         junkX, junkY, junkbw, junkDepth;
		    iconifyWindow = w;
		    iconPressx = ev.xbutton.x_root;
		    iconPressy = ev.xbutton.y_root;
		    dragx = 0;
		    dragy = 0;
		    dragWindow = ga->iconWindow;
		    XGrabServer(dpy);
		    XGetGeometry(dpy, (Drawable) dragWindow,
				 &junkRoot, &junkX,
				 &junkY, &dragWidth, &dragHeight, &junkbw,
				 &junkDepth);
#ifdef debug
		    printf("From: x=%d, y=%d, w=%d, h=%d\n",
			   junkX, junkY, dragWidth, dragHeight);
#endif
		    MoveOutline(ev.xbutton.root,
				ev.xbutton.x_root - dragx - BORDERWIDTH,
				ev.xbutton.y_root - dragy - BORDERWIDTH,
				dragWidth + 2 * BORDERWIDTH,
				dragHeight + 2 * BORDERWIDTH);
		}
		else if (ga && (ga->header == w) && dragWindow == None) {
		    Window      junkRoot;
		    int         junkX, junkY, junkbw, junkDepth;
		    dragWindow = ga->frame;
		    dragx = ev.xbutton.x + UPBOXWIDTH + ICONIFYWIDTH + BORDERWIDTH;
		    dragy = ev.xbutton.y;
		    hdrWidth = ga->headerWidth;
		    XGrabServer(dpy);
		    XGetGeometry(dpy, (Drawable) dragWindow, &junkRoot, &junkX,
				 &junkY, &dragWidth, &dragHeight, &junkbw,
				 &junkDepth);
#ifdef debug
		    printf("From: x=%d, y=%d, w=%d, h=%d\n",
			   junkX, junkY, dragWidth, dragHeight);
#endif
		    MoveOutline(ev.xbutton.root,
				ev.xbutton.x_root - dragx - BORDERWIDTH,
				ev.xbutton.y_root - dragy - BORDERWIDTH,
				dragWidth + 2 * BORDERWIDTH,
				dragHeight + 2 * BORDERWIDTH);
		}
		else {
		    int newx, newy;
		    Window newRoot;
		    RootInfoRec *ro = findRootInfo(ev.xbutton.root);
		    RootInfoRec *rn = ro;

		    if (++rn >= (RootInfo + ScreenCount(dpy)))
			rn = RootInfo;
#ifdef	debug
		    printf("Roots 0x%x: old 0x%x, new 0x%x\n", ev.xbutton.root, ro, rn);
#endif
		    if (rn != ro) {
#ifdef	debug
			printf("Warp from %d to %d new root is 0x%x\n",
			       (ro - RootInfo), (rn - RootInfo), rn->root);
#endif
		        newx = (((ev.xbutton.x_root - ro->rootx)*100/(ro->rootwidth))
			    *(rn->rootwidth))/100 + rn->rootx;
		        newy = (((ev.xbutton.y_root - ro->rooty)*100/(ro->rootheight))
			    *(rn->rootheight))/100 + rn->rooty;
		        XWarpPointer(dpy, None, rn->root, 0, 0, 0, 0, newx, newy);
		    }
		}
		break;
	    }
	case MotionNotify:{
		Window      junkRoot, junkChild;
		int         junkx, junky, junkrx, junkry;
		unsigned int junkMask;
		Window 	    root;
#ifdef debug
		printf("\n");
#endif
		if (QLength(dpy)) {
		    XEvent      rete;
		    XPeekEvent(dpy, &rete);
		    if (rete.type == MotionNotify &&
			rete.xmotion.window == ev.xmotion.window) {
			break;
		    }
		}
		root = ev.xmotion.root;
		if (dragWindow != None) {
		    int         x, y;
		    x = ev.xmotion.x_root - dragx;
		    y = ev.xmotion.y_root - dragy;
		    if (iconifyWindow != None)
			MakeReachable(root, &x, &y, dragWidth,
				      dragHeight);
		    else {
			x += UPBOXWIDTH + ICONIFYWIDTH + BW;
			MakeReachable(root, &x, &y, hdrWidth,
				      TITLEHEIGHT);
			x -= UPBOXWIDTH + ICONIFYWIDTH + BW;
		    }
		    MoveOutline(root,
				x - BORDERWIDTH, y - BORDERWIDTH,
				dragWidth + 2 * BORDERWIDTH,
				dragHeight + 2 * BORDERWIDTH);
		    XQueryPointer(dpy, root, &junkRoot, &junkChild,
				  &junkrx, &junkry,
				  &junkx, &junky, &junkMask);
		}
		else if (resizeWindow != None) {
		    int         action = 0;
		    if (clampTop) {
			int         delta = ev.xmotion.y_root - dragy;
			if (dragHeight - delta < MINHEIGHT) {
			    delta = dragHeight - MINHEIGHT;
			    clampTop = 0;
			}
			dragy += delta;
			dragHeight -= delta;
			action = 1;
		    }
		    else if (ev.xmotion.y_root <= dragy ||
			     ev.xmotion.y_root == findRootInfo(root)->rooty) {
			dragy = ev.xmotion.y_root;
			dragHeight = origy + origHeight -
			    ev.xmotion.y_root;
			clampBottom = 0;
			clampTop = 1;
			action = 1;
		    }
		    if (clampLeft) {
			int         delta = ev.xmotion.x_root - dragx;
			if (dragWidth - delta < MINWIDTH) {
			    delta = dragWidth - MINWIDTH;
			    clampLeft = 0;
			}
			dragx += delta;
			dragWidth -= delta;
			action = 1;
		    }
		    else if (ev.xmotion.x_root <= dragx ||
			     ev.xmotion.x_root == findRootInfo(root)->rootx) {
			dragx = ev.xmotion.x_root;
			dragWidth = origx + origWidth -
			    ev.xmotion.x_root;
			clampRight = 0;
			clampLeft = 1;
			action = 1;
		    }
		    if (clampBottom) {
			int         delta = ev.xmotion.y_root - dragy - dragHeight;
			if (dragHeight + delta < MINHEIGHT) {
			    delta = MINHEIGHT - dragHeight;
			    clampBottom = 0;
			}
			dragHeight += delta;
			action = 1;
		    }
		    else if (ev.xmotion.y_root >= dragy + dragHeight - 1 ||
			   ev.xmotion.y_root == findRootInfo(root)->rooty
			   + findRootInfo(root)->rootheight - 1) {
			dragy = origy;
			dragHeight = 1 + ev.xmotion.y_root - dragy;
			clampTop = 0;
			clampBottom = 1;
			action = 1;
		    }
		    if (clampRight) {
			int         delta = ev.xmotion.x_root - dragx - dragWidth;
			if (dragWidth + delta < MINWIDTH) {
			    delta = MINWIDTH - dragWidth;
			    clampRight = 0;
			}
			dragWidth += delta;
			action = 1;
		    }
		    else if (ev.xmotion.x_root >= dragx + dragWidth - 1 ||
			     ev.xmotion.x_root == findRootInfo(root)->rootx +
			     findRootInfo(root)->rootwidth - 1) {
			dragx = origx;
			dragWidth = 1 + ev.xmotion.x_root - origx;
			clampLeft = 0;
			clampRight = 1;
			action = 1;
		    }
		    if (action) {
			MoveOutline(root,
				    dragx - BORDERWIDTH,
				    dragy - BORDERWIDTH,
				    dragWidth + 2 * BORDERWIDTH,
				    dragHeight + 2 * BORDERWIDTH);
			XQueryPointer(dpy, root, &junkRoot, &junkChild,
				      &junkrx, &junkry,
				      &junkx, &junky, &junkMask);
		    }
		}
		break;
	    }
	case ButtonRelease:{
		Window      w;
		GenericAssoc *ga;
		if (!buttonsDown[ev.xbutton.button])
		   break;
		buttonsDown[ev.xbutton.button] = 0;
		buttonsPressed--;
		w = ev.xbutton.window;
		ga = (GenericAssoc *) XLookUpAssoc(dpy, frameInfo,
						   (XID) w);

		if (ga && (ga->header == w)
		    && (ga->frame == dragWindow)) {
		    XWindowChanges wc;
		    wc.x = ev.xbutton.x_root - dragx;
		    wc.y = ev.xbutton.y_root - dragy;
		    wc.x += UPBOXWIDTH + ICONIFYWIDTH + BW;
		    MakeReachable(ev.xbutton.root, &wc.x, &wc.y, ga->headerWidth,
				  TITLEHEIGHT);
		    wc.x -= UPBOXWIDTH + ICONIFYWIDTH + 2 * BW;
		    /*
		     * the extra BW is because XReconfigureWindow includes
		     * the border width in x and y, but not in width and
		     * height 
		     */
		    wc.y -= BW;
#ifdef debug
		    printf("delivered to %s of frame %d\n",
			   WindowType(w), Frame(w));
#endif
		    MoveOutline(ev.xbutton.root, 0, 0, 0, 0);
		    XUngrabServer(dpy);
#ifdef debug
		    printf("To x=%d, y=%d\n",
			   ev.xbutton.x_root - dragx,
			   ev.xbutton.y_root - dragy);
#endif
		    XConfigureWindow(dpy, dragWindow,
				       CWX | CWY, &wc);
		    dragWindow = None;
		}
		else if (ga && (ga->rbox == w)
			 && (ga->frame == resizeWindow)) {
#ifdef debug
		    printf("delivered to %s of frame %d\n",
			   WindowType(w), Frame(w));
#endif
		    MoveOutline(ev.xbutton.root, 0, 0, 0, 0);
		    XUngrabServer(dpy);
		    ReconfigureFrameAndClient(ev.xbutton.root, ga, dragx,
			dragy + TITLEHEIGHT + BORDERWIDTH, dragWidth,
				 dragHeight - TITLEHEIGHT - BORDERWIDTH);
		    resizeWindow = None;
		}
		else if (ga && (ga->iconify == w) && (iconifyWindow == w)) {
		    XWindowChanges wc;
#ifdef debug
		    printf("delivered to %s of frame %d\n",
			   WindowType(w), Frame(w));
#endif
		    if (ga->iconknown && iconPressx == ev.xbutton.x_root
			&& iconPressy == ev.xbutton.y_root) {
			wc.x = ga->iconx;
			wc.y = ga->icony;
		    }
		    else {
			ga->iconknown = True;
			wc.x = ga->iconx = ev.xbutton.x_root;
			wc.y = ga->icony = ev.xbutton.y_root;
		    }
		    MakeReachable(ev.xbutton.root, &wc.x, &wc.y,
				  dragWidth, dragHeight);
		    wc.x -= BW;
		    wc.y -= BW;
		    MoveOutline(ev.xbutton.root, 0, 0, 0, 0);
		    XUngrabServer(dpy);
		    dragWindow = None;
		    iconifyWindow = None;
		    XConfigureWindow(dpy, ga->iconWindow, CWX | CWY, &wc);
		    XUnmapWindow(dpy, ga->frame);
		    XMapWindow(dpy, ga->iconWindow);
		}
		else if (ga && (ga->iconWindow == w) && (iconifyWindow == w)) {
		    if (dragWindow == w) {
			XWindowChanges wc;
			wc.x = ev.xbutton.x_root - dragx;
			wc.y = ev.xbutton.y_root - dragy;
#ifdef debug
			printf("delivered to %s of frame %d\n",
			       WindowType(w), Frame(w));
#endif
			MoveOutline(ev.xbutton.root, 0, 0, 0, 0);
			XUngrabServer(dpy);
			ga->iconx = wc.x;
			ga->icony = wc.y;
#ifdef debug
			printf("To x=%d, y=%d\n",
			       ev.xbutton.x_root - dragx,
			       ev.xbutton.y_root - dragy);
#endif
			MakeReachable(ev.xbutton.root, &wc.x, &wc.y, dragWidth,
				      dragHeight);
			wc.x -= BW;
			wc.y -= BW;
			XConfigureWindow(
					dpy, dragWindow, CWX | CWY, &wc);
			dragWindow = None;
		    }
		    else {
			XUnmapWindow(dpy, ga->iconWindow);
			XMapWindow(dpy, ga->frame);
		    }
		    iconifyWindow = None;
		}
		else {
#ifdef debug
		    printf("delivered to ???\n");
#endif
		}
		break;
	    }
	case ConfigureRequest:{
		Window      w;
		GenericAssoc *ga;
		XWindowChanges wc;
		XConfigureRequestEvent evc;
		evc = ev.xconfigurerequest;
		w = evc.window;
		ga = (GenericAssoc *) XLookUpAssoc(dpy, frameInfo, (XID) w);
		if (!ga) {
#ifdef debug
		    printf("delivered to unknown client\n");
#endif
		    wc.x = evc.x;
		    wc.y = evc.y;
		    wc.width = evc.width;
		    wc.height = evc.height;
		    wc.border_width = evc.border_width;
		    wc.sibling = evc.above;
		    wc.stack_mode = Above;
		    XConfigureWindow(dpy, w, CWX | CWY | CWWidth |
				       CWBorderWidth | CWSibling | CWStackMode,
				       &wc);
		}
		else if (ga->client == w) {
		    int         x, y;
		    Window      root, jc;
		    int         ht, wd, bw, d, x1, y1;
#ifdef debug
		    printf("delivered to %s of frame %d\n",
			   WindowType(w), Frame(w));
#endif
		    XGetGeometry(dpy,
				 (Drawable) w, &root, &x1, &y1, &wd, &ht, &bw, &d);
		    XTranslateCoordinates(dpy, w, root, 0, 0, &x, &y, &jc);
		    ReconfigureFrameAndClient
			(root, ga, x, y, evc.width, evc.height);
		}
		else {
#ifdef debug
		    printf("delivered to %s of frame %d\n",
			   WindowType(w), Frame(w));
#endif
		}
		break;
	    }
	case Expose:{
		Window      w = ev.xexpose.window;
		GenericAssoc *ga;
		Window	    root;
		int	    x, y, wd, ht, bw, d;

#ifdef debug
		printf("delivered to %s of frame %d\n",
		       WindowType(w), Frame(w));
#endif
		ga = (GenericAssoc *) XLookUpAssoc(dpy, frameInfo, (XID) w);
		XGetGeometry(dpy, (Drawable) w, &root, &x, &y, &wd, &ht, &bw, &d);
		if (ga && ga->header == w) {
		    int         x;
		    x = (ga->headerWidth -
			 XTextWidth(font, ga->name, ga->namelen)) / 2;
		    XDrawString(dpy, w, findRootInfo(root)->headerGC, x,
			  font->max_bounds.ascent + 5,
			  ga->name, ga->namelen);
		}
		else if (ga && ga->iconWindow == w) {
		    if (ga->iconnamelen == 0)
			XDrawString(dpy, w, findRootInfo(root)->headerGC, 2,
			      font->max_bounds.ascent + 5,
			      ga->name, ga->namelen);
		    else
			XDrawString(dpy, w, findRootInfo(root)->headerGC, 2,
			      font->max_bounds.ascent + 5,
			      ga->iconname, ga->iconnamelen);
		}		/* else if (ga && (ga->upbox == w)) {
				 * XCopyArea(dpy, doubleArrow, w,
				 * headerGC, 0, 0, UPBOXWIDTH,
				 * TITLEHEIGHT, 0, 0); } else if (ga &&
				 * (ga->rbox == w)) { XCopyArea(dpy,
				 * growBox, w, headerGC, 0, 0, UPBOXWIDTH,
				 * TITLEHEIGHT, 0, 0); } else if (ga &&
				 * (ga->iconify == w)) { XCopyArea(dpy,
				 * cross, w, headerGC, 0, 0, UPBOXWIDTH,
				 * TITLEHEIGHT, 0, 0); } */
		break;
	    }
	case FocusIn: break;    /* just ignore */
	case FocusOut: {
		if (ev.xfocus.detail == NotifyPointerRoot)
		    XSetInputFocus(dpy, PointerRoot, None, CurrentTime);
		break;
	    }
	default:
#ifdef debug
	    printf("\n");
#endif
	    ;
	}
    }
}

int
ErrorFunc(dpy, event)
	Display *dpy;
	XErrorEvent *event;
{
    switch (event->error_code) {
	case BadWindow:
	case BadDrawable:
		(void) fprintf(stderr, "wm: Window #0x%x disappeared!\n",
			event->resourceid);
		break;
	default:
		_XDefaultError(dpy, event);
    }
    return (0);
}

Bool
MappedNotOverride(w) Window w;
{
XWindowAttributes wa;
	XGetWindowAttributes(dpy, w, &wa);
	return ((wa.map_state != IsUnmapped) && (wa.override_redirect != True));
}

static void
MoveOutline(root, x, y, width, height)
    Window root;
    int x, y, width, height;
{
    static int  lastx = 0;
    static int  lasty = 0;
    static int  lastWidth = 0;
    static int  lastHeight = 0;
    XRectangle  outline[8];
    XRectangle *r = outline;

    if (x == lastx && y == lasty && width == lastWidth && height == lastHeight)
	return;
    if (lastWidth || lastHeight) {
	r->x = lastx;
	r->y = lasty;
	r->width = lastWidth;
	r++->height = BORDERWIDTH;
	r->x = lastx;
	r->y = lasty + lastHeight - BORDERWIDTH;
	r->width = lastWidth;
	r++->height = BORDERWIDTH;
	r->x = lastx;
	r->y = lasty + BORDERWIDTH;
	r->width = BORDERWIDTH;
	r++->height = lastHeight - 2 * BORDERWIDTH;
	r->x = lastx + lastWidth - BORDERWIDTH;
	r->y = lasty + BORDERWIDTH;
	r->width = BORDERWIDTH;
	r++->height = lastHeight - 2 * BORDERWIDTH;
    }
    lastx = x;
    lasty = y;
    lastWidth = width;
    lastHeight = height;
    if (lastWidth || lastHeight) {
	r->x = lastx;
	r->y = lasty;
	r->width = lastWidth;
	r++->height = BORDERWIDTH;
	r->x = lastx;
	r->y = lasty + lastHeight - BORDERWIDTH;
	r->width = lastWidth;
	r++->height = BORDERWIDTH;
	r->x = lastx;
	r->y = lasty + BORDERWIDTH;
	r->width = BORDERWIDTH;
	r++->height = lastHeight - 2 * BORDERWIDTH;
	r->x = lastx + lastWidth - BORDERWIDTH;
	r->y = lasty + BORDERWIDTH;
	r->width = BORDERWIDTH;
	r++->height = lastHeight - 2 * BORDERWIDTH;
    }
    if (r != outline) {
	XFillRectangles(dpy, root, findRootInfo(root)->xorGC, outline, r - outline);
    }
}
		
void
AddGizmos(w)
    Window w;
{
    XWindowAttributes wa;
    Window      root, fw, gw, lw, uw, iw, iconw, hpw;
    int         hw;
    Bool        resize = False;
    int		x, y, wd, h, bw, d;

    /*
     * When we reparent w to fw, below, make sure w gets reparented back when
     * fw goes away.  We do this early, in case we die, otherwise the map
     * request in the client will hang forever. 
     */
    XChangeSaveSet(dpy, w, SetModeInsert);

    /*
     * Don't try to fiddle with InputOnly windows.
     */
    XGetWindowAttributes(dpy, w, &wa);
    if (wa.class == InputOnly) {
        XClientMessageEvent ev;

	ev.type = ClientMessage;
	ev.display = (Display *)NULL;
	ev.window = w;
	ev.message_type = XA_WM_HINTS;
	ev.format = 8;
	strcpy( ev.data.b, "bad window class" ); /* fix */
        XSendEvent( dpy, w, False, -1, &ev );
	return;
    }

    XGetGeometry(dpy, (Drawable) w, &root, &x, &y, &wd, &h, &bw, &d);
    /* set the border of the original window to 0 */
    ChangeBorderWidth(w, 0);
    /* make sure this window is big enough */
    if (wa.width < MINWIDTH) {
	resize = True;
	wa.width = MINWIDTH;
    }
    if (wa.height + TITLEHEIGHT + 2 * BW < MINHEIGHT) {
	resize = True;
	wa.height = MINHEIGHT;
    }
    if (resize == True) {
	XResizeWindow(dpy, w, wa.width, wa.height);
    }
    /* MakeFrame makes sure part of the header is on-screen */
    fw = MakeFrame(root, wa.x, wa.y - TITLEHEIGHT - BW, wa.width,
		   wa.height + TITLEHEIGHT + BW);
    if (!fw)
	return;
    hw = wa.width - GROWBOXWIDTH - UPBOXWIDTH - ICONIFYWIDTH - 2 * BW;
    hpw = MakeHeaderParent(root, fw, hw);
    gw = MakeHeader(root, hpw, hw);
    lw = MakeGrowBox(root, fw, wa.width - GROWBOXWIDTH);
    iw = MakeIconBox(root, fw, UPBOXWIDTH);
    uw = MakeUpBox(root, fw);
    iconw = MakeIcon(root, fw);
    XSync(dpy, False);
    errorStatus = False;
    {
	XWindowChanges wc;

	wc.sibling = w;
	wc.stack_mode = Above;
	XConfigureWindow(dpy, fw, CWSibling|CWStackMode, &wc);
    }
    XReparentWindow(dpy, w, fw, 0, TITLEHEIGHT + BW);
    XSync(dpy, False);
    if (errorStatus == True) {
	XDestroyWindow(dpy, fw);
	return;
    }
    /* register fw, w, gw, and lw in the association table */
    RegisterCompleteWindow(fw, w, gw, hpw, lw, iw, uw, iconw, hw);
    MapCompleteWindow(fw);
}

void
ReconfigureFrameAndClient(root, ga, x, y, width, height)
    Window root;
	GenericAssoc *ga;
	int x, y, width, height;
{
    XWindowAttributes wa;
    XWindowChanges wc;
    unsigned long mask;

    if (width < MINWIDTH)
	width = MINWIDTH;
    if (height < TITLEHEIGHT)
	height = TITLEHEIGHT;

    /* let's unmap all the inferiors, so we dont' see all the interim
     * exposure events of the inferior windows in the old sizes.
     * XXX I suspect this might actually be hiding a real bug in the
     * server, and should be reexamined someday.
     */
    XUnmapSubwindows(dpy,ga->frame);

    /* Reconfigure frame */
    wc.x = x + UPBOXWIDTH + ICONIFYWIDTH + BW;
    wc.y = y - TITLEHEIGHT - BW;
    MakeReachable(root, &wc.x, &wc.y, width - UPBOXWIDTH - GROWBOXWIDTH -
		  ICONIFYWIDTH - 2 * BW, TITLEHEIGHT);
    wc.x -= UPBOXWIDTH + ICONIFYWIDTH + 2 * BW;
    /* the extra BW is because XReconfigureWindow includes the border */
    /* width in x and y, but not in width and height */
    wc.y -= BW;
    wc.width = width;
    wc.height = height + TITLEHEIGHT + BW;
    mask = CWX | CWY | CWWidth | CWHeight;
    XConfigureWindow(dpy, ga->frame, mask, &wc);
    /* Reconfigure client */
    wc.x = 0;
    wc.y = TITLEHEIGHT + BW;
    wc.width = width;
    wc.height = height;
    wc.border_width = 0;
    mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
    XConfigureWindow(dpy, ga->client, mask, &wc);
    /* Reconfigure raise button */
    wc.x = 0;
    wc.y = 0;
    wc.width = UPBOXWIDTH;
    wc.height = TITLEHEIGHT;
    mask = CWX | CWY | CWWidth | CWHeight;
    XConfigureWindow(dpy, ga->upbox, mask, &wc);
    /* Reconfigure header parent */
    wc.x = UPBOXWIDTH + BW + ICONIFYWIDTH;
    wc.y = 0;
    wc.width = width - UPBOXWIDTH - GROWBOXWIDTH - 2 * BW - ICONIFYWIDTH;
    ga->headerWidth = wc.width;
    wc.height = TITLEHEIGHT;
    mask = CWX | CWY | CWWidth | CWHeight;
    XConfigureWindow(dpy, ga->headerParent, mask, &wc);
    /* Reconfigure header */
    wc.x = 0;
    wc.y = 0;
    wc.width = width - UPBOXWIDTH - GROWBOXWIDTH - 2 * BW - ICONIFYWIDTH;
    wc.height = TITLEHEIGHT;
    mask = CWX | CWY | CWWidth | CWHeight;
    XConfigureWindow(dpy, ga->header, mask, &wc);
    /* Reconfigure resize button */
    wc.x = width - GROWBOXWIDTH;
    wc.y = 0;
    wc.width = GROWBOXWIDTH;
    wc.height = TITLEHEIGHT;
    mask = CWX | CWY | CWWidth | CWHeight;
    XConfigureWindow(dpy, ga->rbox, mask, &wc);
    /* Reconfigure iconify button */
    wc.x = UPBOXWIDTH;
    wc.y = 0;
    wc.width = ICONIFYWIDTH;
    wc.height = TITLEHEIGHT;
    mask = CWX | CWY | CWWidth | CWHeight;
    XConfigureWindow(dpy, ga->iconify, mask, &wc);
    XGetWindowAttributes(dpy, ga->frame, &wa);

    XMapSubwindows(dpy,ga->frame);
}

Window
MakeGrowBox(root, fw, lx)
    Window root;
    Window fw;
    int lx;
{
    unsigned long valuemask;
    XSetWindowAttributes attributes;
    RootInfoRec *ri = findRootInfo(root);
    int scr = ri - RootInfo;

    valuemask = CWEventMask | CWBackPixmap;
    attributes.background_pixmap = ri->growBox;
    attributes.event_mask =
	ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
	PointerMotionHintMask | ExposureMask;
    return XCreateWindow(dpy, fw, lx, 0, GROWBOXWIDTH, TITLEHEIGHT, 0,
		   DefaultDepth(dpy, scr), CopyFromParent,
		   DefaultVisual(dpy, scr), valuemask, &attributes);
}

Window
MakeUpBox(root, fw)
    Window root;
    Window fw;
{
    unsigned long valuemask;
    XSetWindowAttributes attributes;
    RootInfoRec *ri = findRootInfo(root);
    int scr = ri - RootInfo;

    valuemask = CWEventMask | CWBackPixmap;
    attributes.background_pixmap = ri->doubleArrow;
    attributes.event_mask =
	ButtonPressMask | ButtonReleaseMask | ExposureMask;
    return XCreateWindow(dpy, fw, 0, 0, UPBOXWIDTH, TITLEHEIGHT, 0,
	     DefaultDepth(dpy, scr), CopyFromParent, DefaultVisual(dpy, scr),
		   valuemask, &attributes);
}

Window
MakeIconBox(root, fw, ix)
    Window root;
    Window fw;
    int ix;
{
    unsigned long valuemask;
    XSetWindowAttributes attributes;
    RootInfoRec *ri = findRootInfo(root);
    int         scr = ri - RootInfo;

    valuemask = CWEventMask | CWBackPixmap;
    attributes.background_pixmap = ri->cross;
    attributes.event_mask =
	ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
	PointerMotionHintMask | ExposureMask;
    return XCreateWindow(dpy, fw, ix, 0, UPBOXWIDTH, TITLEHEIGHT, 0,
	     DefaultDepth(dpy, scr), CopyFromParent, DefaultVisual(dpy, scr),
		   valuemask, &attributes);
}
	
void
ChangeBorderWidth(w, newWidth) Window w; int newWidth;
{
XWindowChanges wch;
	wch.border_width = newWidth;
	XConfigureWindow(dpy, w, CWBorderWidth, &wch);
}

void
MapCompleteWindow(frame) Window frame;
{
	XMapSubwindows(dpy, frame);
	XMapWindow(dpy, frame);
}

void
RegisterCompleteWindow(frame, client, header, hp, rbox, iconify, upbox, iconw, headerWidth)
	Window frame, client, header, hp, rbox, upbox, iconify, iconw;
	int headerWidth;
{
    GenericAssoc *fa;
    XWMHints   *wmhints;
    Window      root;
    int         x, y, wd, ht, bw, d;

    XGetGeometry(dpy, (Drawable) client, &root, &x, &y, &wd, &ht, &bw, &d);

    XSelectInput(dpy, client, PropertyChangeMask);
    fa = GimmeAssocStruct();
    fa->frame = frame;
    fa->header = header;
    fa->headerParent = hp;
    fa->client = client;
    fa->rbox = rbox;
    fa->iconify = iconify;
    fa->upbox = upbox;
    fa->headerWidth = headerWidth;
    fa->iconWindow = iconw;
    fa->iconnamelen = fa->namelen = 0;
    ProcessNewIconName(root, fa);
    ProcessNewName(root, fa);
    if ((wmhints = XGetWMHints(dpy, client)) &&
	(wmhints->flags & IconPositionHint)) {
	fa->iconx = wmhints->icon_x;
	fa->icony = wmhints->icon_y;
	fa->iconknown = True;
    }
    else
	fa->iconknown = False;
    if (wmhints)
	free((char *) wmhints);
    XMakeAssoc(dpy, frameInfo, (XID) frame, (char *) fa);
    XMakeAssoc(dpy, frameInfo, (XID) header, (char *) fa);
    XMakeAssoc(dpy, frameInfo, (XID) hp, (char *) fa);
    XMakeAssoc(dpy, frameInfo, (XID) client, (char *) fa);
    XMakeAssoc(dpy, frameInfo, (XID) rbox, (char *) fa);
    XMakeAssoc(dpy, frameInfo, (XID) iconify, (char *) fa);
    XMakeAssoc(dpy, frameInfo, (XID) upbox, (char *) fa);
    XMakeAssoc(dpy, frameInfo, (XID) iconw, (char *) fa);
    /* Xfree(pd); */
}

Window
MakeHeaderParent(root, fw, width)
    Window root;
    Window fw;
    int width;
{
    unsigned long valuemask;
    XSetWindowAttributes attributes;
    Window      result;
    int scr = findRootInfo(root) - RootInfo;

    valuemask = CWBackPixel;
    attributes.background_pixel = BlackPixel(dpy, scr);
    result = XCreateWindow(dpy, fw, UPBOXWIDTH + ICONIFYWIDTH + BW, 0,
		     width, TITLEHEIGHT, 0,
		     DefaultDepth(dpy, scr), CopyFromParent,
		     DefaultVisual(dpy, scr), valuemask,
		     &attributes);
    return result;
}

Window
MakeHeader(root, hw, width)
    Window root;
    Window hw;
    int width;
{
    unsigned long    valuemask;
    XSetWindowAttributes attributes;
    Window      result;
    int         scr = findRootInfo(root) - RootInfo;

    valuemask = CWEventMask | CWBackPixel;
    attributes.event_mask =
	ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
	ExposureMask | PointerMotionHintMask;
    attributes.background_pixel = BlackPixel(dpy, scr);
    result = XCreateWindow(dpy, hw, 0, 0, width, TITLEHEIGHT, 0,
		     DefaultDepth(dpy, scr), CopyFromParent,
		     DefaultVisual(dpy, scr), valuemask,
		     &attributes);
    return result;
}

/* XXX fw unused? */
MakeIcon(root, fw)
    Window root;
    Window fw;
{
    unsigned long  valuemask;
    XSetWindowAttributes attributes;
    Window      result;
    int         scr = findRootInfo(root) - RootInfo;

    valuemask = CWEventMask | CWBackPixel | CWBorderPixel;
    attributes.event_mask =
	ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
	ExposureMask | PointerMotionHintMask;
    attributes.background_pixel = BlackPixel(dpy, scr);
    attributes.border_pixel = WhitePixel(dpy, scr);
    result = XCreateWindow(dpy, root, 0, 0, ICONWIDTH, TITLEHEIGHT, BW,
	     DefaultDepth(dpy, scr), CopyFromParent, DefaultVisual(dpy, scr),
		     valuemask, &attributes);
    return result;
}


Window
MakeFrame(root, x, y, width, height)
    Window root;
	int x, y, width, height;
{
    unsigned long valuemask;
    XSetWindowAttributes attributes;
    int         hx, hy;
    int         scr;
    RootInfoRec *ri = findRootInfo(root);

    if (!ri)
	return (NULL);
    scr = ri - RootInfo;
    hx = x + ICONIFYWIDTH + UPBOXWIDTH + BW;
    hy = y;
    MakeReachable(root, &hx, &hy, width - UPBOXWIDTH - GROWBOXWIDTH - ICONIFYWIDTH
		  - 2 * BW, TITLEHEIGHT);
    x = hx - (ICONIFYWIDTH + UPBOXWIDTH + BW) - BW;
    y = hy - BW;
    valuemask = CWEventMask | CWBorderPixel | CWBackPixel;
    attributes.event_mask = SubstructureRedirectMask
	| SubstructureNotifyMask;
    attributes.background_pixel = WhitePixel(dpy, scr);
    attributes.border_pixel = WhitePixel(dpy, scr);
    attributes.bit_gravity = NorthWestGravity;
    return XCreateWindow(dpy, root, x, y, width, height, BORDERWIDTH,
	     DefaultDepth(dpy, scr), CopyFromParent, DefaultVisual(dpy, scr),
		   valuemask, &attributes);
}

void
RemoveGizmos(ga) GenericAssoc *ga;
{
	XDeleteAssoc(dpy, frameInfo, ga->frame);
	XDeleteAssoc(dpy, frameInfo, ga->header);
	XDeleteAssoc(dpy, frameInfo, ga->rbox);
	XDeleteAssoc(dpy, frameInfo, ga->iconify);
	XDeleteAssoc(dpy, frameInfo, ga->upbox);
	XDeleteAssoc(dpy, frameInfo, ga->iconWindow);
	XDeleteAssoc(dpy, frameInfo, ga->client);
	XDestroyWindow(dpy, ga->frame);
	XDestroyWindow(dpy, ga->iconWindow);
	free((char *) ga);
}


StartConnectionToServer(argc, argv)
	int	argc;
	char	*argv[];
{
    char       *display;
    int         i;
    extern char *index();

    display = NULL;
    for (i = 1; i < argc; i++) {
	if (index(argv[i], ':') != NULL)
	    display = argv[i];
    }
    if (!(dpy = XOpenDisplay(display))) {
	Error(1, "Cannot open display\n");
    }
}

Error(status, message) int status; char *message;
{
	printf("%s", message);
	if (!status) return;
	else exit(1);
}

GenericAssoc *
GimmeAssocStruct(){
	return (GenericAssoc *) malloc(sizeof(GenericAssoc));
}


char *
WindowType(w) Window w;
{
GenericAssoc *ga;
	ga = (GenericAssoc *) XLookUpAssoc(dpy, frameInfo, w);
	if (ga && w == ga->frame) return "frame";
	if (ga && w == ga->header) return "header";
	if (ga && w == ga->client) return "client";
	if (ga && w == ga->rbox) return "resize box";
	if (ga && w == ga->iconify) return "iconify box";
	if (ga && w == ga->upbox) return "raise box";
	if (ga && w == ga->iconWindow) return "icon";
	if (ga) {
		Error(0, "WindowType: error in association table routines\n");
		return "error";
	}
	Error(0, "WindowType: window %d not found in assoc table\n", w);
	return "error";
}

Window
Frame(w) Window w;
{
GenericAssoc *ga;
	ga = (GenericAssoc *) XLookUpAssoc(dpy, frameInfo, w);
	if (ga) return ga->frame;
	Error(0, "Frame: window %d not found in assoc table\n");
	return None;
}

void
MakeReachable(root, x, y, width, height)
    Window root;
	int *x, *y;
	int width, height;
{
    RootInfoRec *ri = findRootInfo(root);

    if (!ri)
	return;
    if (*x <= MARGIN + ri->rootx - width)
	*x = MARGIN + ri->rootx - width + 1;
    if (*y <= MARGIN + ri->rooty - height)
	*y = MARGIN + ri->rooty - height + 1;
    if (*x >= ri->rootx + ri->rootwidth - MARGIN)
	*x = ri->rootx + ri->rootwidth - MARGIN - 1;
    if (*y >= ri->rooty + ri->rootheight - MARGIN)
	*y = ri->rooty + ri->rootheight - MARGIN - 1;
}

Bool
FillName(ga) GenericAssoc *ga;
{
    Status      status;
    char       *name;
    Bool        noname = False;
    int         len;
    Bool        res;
    extern char *strncpy();

    status = XFetchName(dpy, ga->client, &name);
    if (!status || !name || !(*name)) {
	name = "No name";
	noname = True;
    }

    len = strlen(name);
    if (len >= MAXNAME)
	len = MAXNAME;
    if (len != ga->namelen || strncmp(name, ga->name, len)) {
	ga->namelen = len;
	(void) strncpy(ga->name, name, ga->namelen);
	res = True;
    }
    else
	res = False;
    if (!noname)
	free(name);
    return res;
}

void
FillIconName(ga) GenericAssoc *ga;
{
Status status;
long len2;
char *pd;
Atom at;
int af;
long ba;
	status = XGetWindowProperty(dpy, ga->client, XA_WM_ICON_NAME,
		0,  MAXNAME, False,
		XA_STRING, &at, &af, &len2, &ba, &pd);
	if (status != Success) {
		ga->iconnamelen = 0;
	} else {
		switch (af) {
		case 8:
			ga->iconnamelen = len2;
			if (ga->iconnamelen >= MAXNAME) ga->iconnamelen = MAXNAME;
			strncpy(ga->iconname, pd, ga->iconnamelen);
			free (pd);
			break;
		default:
			ga->iconnamelen = 0;
			if (pd) free(pd);
		}
	} 
}

void
ProcessNewName(root, ga)
    Window root;
    GenericAssoc *ga;
{
	if (!FillName(ga)) return;
	if (ga->iconnamelen == 0) {
		Bool wasMapped;
		int width;
		wasMapped = MappedNotOverride(ga->iconWindow);
		width = XTextWidth(font, ga->name, ga->namelen) + 2*BW;
		MakeReachable(root, &ga->iconx, &ga->icony, width, TITLEHEIGHT);
		if (wasMapped) XUnmapWindow(dpy, ga->iconWindow); 
		XMoveResizeWindow(dpy, ga->iconWindow, ga->iconx - BW,
			ga->icony - BW, width, TITLEHEIGHT);
		if (wasMapped) XMapWindow(dpy, ga->iconWindow);
	}
	XUnmapWindow(dpy, ga->header);
	XMapWindow(dpy, ga->header);
}

void
ProcessNewIconName(root, ga)
    Window root;
    GenericAssoc *ga;
{
    int         oldIconLen;
    Bool        wasMapped;
    int         width;

    wasMapped = MappedNotOverride(ga->iconWindow);
    oldIconLen = ga->iconnamelen;
    FillIconName(ga);
    if (ga->iconnamelen == 0) {
	if (oldIconLen == 0)
	    return;
	width = XTextWidth(font, ga->name, ga->namelen) + 2 * BW;
	MakeReachable(root, &ga->iconx, &ga->icony, width, TITLEHEIGHT);
	if (wasMapped)
	    XUnmapWindow(dpy, ga->iconWindow);
	XMoveResizeWindow(dpy, ga->iconWindow, ga->iconx - BW,
			 ga->icony - BW, width, TITLEHEIGHT);
	if (wasMapped)
	    XMapWindow(dpy, ga->iconWindow);
	return;
    }
    width = XTextWidth(font, ga->iconname, ga->iconnamelen) + 2 * BW;
    MakeReachable(root, &ga->iconx, &ga->icony, width, TITLEHEIGHT);
    if (wasMapped)
	XUnmapWindow(dpy, ga->iconWindow);
    XMoveResizeWindow(dpy, ga->iconWindow, ga->iconx - BW,
		     ga->icony - BW, width, TITLEHEIGHT);
    if (wasMapped)
	XMapWindow(dpy, ga->iconWindow);
}
