/*
 *	$Source: /u1/X11/clients/xcalc/RCS/xcalc.c,v $
 *	$Header: xcalc.c,v 1.4 87/09/12 19:00:36 rws Exp $
 */

#ifndef lint
static char *rcsid_xcalc_c = "$Header: xcalc.c,v 1.4 87/09/12 19:00:36 rws Exp $";
#endif	lint

/*
 * xcalc.c  -  a hand calculator for the X Window system
 *
 *  Author:    John H. Bradley, University of Pennsylvania
 *                (bradley@cis.upenn.edu)
 *                     March, 1987
 *
 *  RPN mode added and port to X11 by Mark Rosenstein, MIT Project Athena
 */

/*
 * color usage:  ForeColor is used for all frames, BackColor is the color of 
 *                  the calculator body.
 *               NKeyFore, NKeyBack are the colors used for the number keys.
 *               OKeyFore, OKeyBack are used for the operator keys (+-*=/).
 *               FKeyFore, FKeyBack are used for all the other keys.
 *               DispFore, DispBack are used for the display.
 *               IconFore, IconBack are used for the display.
 *
 *               if running on monochrome monitor, or if 'stipple' option
 *                  set, the calculator body is a 50% stipple of ForeColor
 *                  and BackColor.  This looks nice in mono, but can prevent
 *                  you from getting the 'right' colors in color mode, so
 *                  it's an option.
 */

#include <stdio.h>
#include <math.h>
#include <strings.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <sys/time.h>
#include <setjmp.h>

/* program icon */
#include "icon"

extern int errno;


/* constants used for setting up the calculator.  changing them would 
   probably be bad.  If you do, don't forget to change the values in the
   syntax routine */

#define KFONT "helvetica-medium9"
#define DFONT "helvetica-bold12"
#define FFONT "helvetica-medium8"
#define PADDINGW    4
#define PADDINGH    8
#define DEF_BDRWIDE 2
#define MAXDISP     11
#define KEYPADH     3
#define KEYPADW     2
#define EXTRAH      2
#define DISPPADH    4
#define DISPPADW    8
#define FLAGH       10
#define PI          3.14159265358979
#define E           2.71828182845904

/* DRG mode.  used for trig calculations */
#define DEG 0
#define RAD 1
#define GRAD 2


/* fonts */
char *kfont, *dfont, *ffont;

/* colors */
int ForeColor, BackColor, NKeyFore, NKeyBack, OKeyFore, OKeyBack;
int FKeyFore, FKeyBack, DispFore, DispBack, IconFore, IconBack;

int border = DEF_BDRWIDE;


/* objects */
Display	 *dpy;
Window   theWindow, iconWindow, dispwid;
Cursor   arrow;
Font     keyfont, dispfont, flagfont;
XFontStruct *kfontinfo, *dfontinfo, *ffontinfo; 
Pixmap   stipplePix, dimBorder, IconPix;
GC	 dispbgc,dispfgc,keygc,keyigc,flaggc;

/* variables used in setup */
int numkeys, dispwide, disphigh, keywide, keyhigh;


/* RPN or Infix mode */
int rpn	  = 0;	    /* infix syntax as default */

/* display flags */
int flagK, flagINV, flagPAREN, flagM, flagE, drgmode;
char dispstr[32];


/* stuff defining the keyboard layout.  The keypad is a 5x8 matrix of keys */
#define TINUMKEYS 40
#define HPNUMKEYS 39

struct _key {
           char   *st;
	   int	  code;
	   int    (*fun)();
           Window wid;
           short  x,y,width,height;
           int    fore,back;
	   void	  (*func)();
            };

struct _key *key;
	      
#define kRECIP 0
#define kSQR   1
#define kSQRT  2
#define kCLR   3
#define kOFF   4
#define kINV   5
#define kSIN   6
#define kCOS   7
#define kTAN   8
#define kDRG   9
#define kE     10
#define kEE    11
#define kLOG   12
#define kLN    13
#define kPOW   14
#define kPI    15
#define kFACT  16
#define kLPAR  17
#define kRPAR  18
#define kDIV   19
#define kSTO   20
#define kSEVEN 21
#define kEIGHT 22
#define kNINE  23
#define kMUL   24
#define kRCL   25
#define kFOUR  26
#define kFIVE  27
#define kSIX   28
#define kSUB   29
#define kSUM   30
#define kONE   31
#define kTWO   32
#define kTHREE 33
#define kADD   34
#define kEXC   35
#define kZERO  36
#define kDEC   37
#define kNEG   38
#define kEQU   39
#define kENTR  40
#define kXXY   41
#define kEXP   42
#define k10X   43
#define kROLL  44
#define kNOP   45
#define kBKSP  46


int  oneop(),twoop(),clearf(),offf(),invf(),drgf(),eef();
int  lparf(),rparf(),digit(),decf(),negf(),equf();

/* "1/x", "x^2", "SQRT","CE/C", "AC",
   "INV", "sin", "cos", "tan",  "DRG",
   "e",   "EE",  "log", "ln",   "y^x",
   "PI",  "x!",  "(",   ")",    "/",
   "STO", "7",   "8",   "9",    "*",
   "RCL", "4",   "5",   "6",    "-",
   "SUM", "1",   "2",   "3",    "+",
   "EXC", "0",   ".",   "+/-",  "="  */

struct _key tikeys[TINUMKEYS] = {
  {"1/x",kRECIP,oneop}, {"x^2",kSQR,oneop}, {"SQRT",kSQRT,oneop},
  {"CE/C",kCLR,clearf}, {"AC",kOFF,offf},
  {"INV",kINV,invf}, {"sin",kSIN,oneop}, {"cos",kCOS,oneop},
  {"tan",kTAN,oneop}, {"DRG",kDRG,drgf},
  {"e",kE,oneop}, {"EE",kEE,eef}, {"log",kLOG,oneop},
  {"ln",kLN,oneop},{"y^x",kPOW,twoop},
  {"PI",kPI,oneop},{"x!",kFACT,oneop},{"(",kLPAR,lparf},
  {")",kRPAR,rparf},{"/",kDIV,twoop},
  {"STO",kSTO,oneop},{"7",kSEVEN,digit},{"8",kEIGHT,digit},
  {"9",kNINE,digit},{"*",kMUL,twoop},
  {"RCL",kRCL,oneop},{"4",kFOUR,digit},{"5",kFIVE,digit},
  {"6",kSIX,digit},{"-",kSUB,twoop},
  {"SUM",kSUM,oneop},{"1",kONE,digit},{"2",kTWO,digit},
  {"3",kTHREE,digit},{"+",kADD,twoop},
  {"EXC",kEXC,oneop},{"0",kZERO,digit},{".",kDEC,decf},
  {"+/-",kNEG,negf},{"=",kEQU,equf}};

int  twof(),nop(),rollf(),bkspf(),entrf(),memf();

/*  { "SQRT","e^x", "10^x", "y^x", "1/x", "CHS", "7", "8", "9",  "/",
      "x!",  "PI",  "sin",  "cos", "tan", "EEX", "4", "5", "6",  "x",
      "",    "",    "R v",  "x:y", "<-",  "ENTR","1", "2", "3",  "-",
      "ON",  "DRG", "INV",  "STO", "RCL",        "0", ".", "SUM","+"  */

struct _key hpkeys[HPNUMKEYS] = {
  {"SQRT",kSQRT,oneop},{"e^x",kEXP,oneop},{"10^x",k10X,oneop},
  {"y^x",kPOW,twof},{"1/x",kRECIP,oneop},{"CHS",kNEG,negf},
  {"7",kSEVEN,digit},{"8",kEIGHT,digit},{"9",kNINE,digit},{"/",kDIV,twof},

  {"x!",kFACT,oneop},{"PI",kPI,oneop},{"sin",kSIN,oneop},{"cos",kCOS,oneop},
  {"tan",kTAN,oneop},{"EEX",kEE,eef},{"4",kFOUR,digit},{"5",kFIVE,digit},
  {"6",kSIX,digit},{"x",kMUL,twof},

  {"",kNOP,nop},{"",kNOP,nop},{"R v",kROLL,rollf},{"x:y",kXXY,twof},
  {"<-",kBKSP,bkspf},{"ENTR",kENTR,entrf},{"1",kONE,digit},{"2",kTWO,digit},
  {"3",kTHREE,digit},{"-",kSUB,twof},

  {"ON",kOFF,offf},{"DRG",kDRG,drgf},{"INV",kINV,invf},{"STO",kSTO,memf},
  {"RCL",kRCL,memf},{"0",kZERO,digit},{".",kDEC,decf},
  {"SUM",kSUM,memf},{"+",kADD,twof} };


/* checkerboard used in mono mode */
#define check_width 16
#define check_height 16
static char check_bits[] = {
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa};


#ifndef IEEE
    jmp_buf env;
#endif


/**************/
main(argc, argv)
    int   argc;
    char *argv[];
/**************/
{
    int i, status,dpcs;
    char *strind;
#ifndef IEEE
    extern fperr();
#endif

    char *fc, *bc, *nfc, *nbc, *ofc, *obc, *ffc, *fbc, *dfc, *dbc, *ifc, *ibc;
    char *geom    = NULL;
    char *display = NULL;
    int rvflag    = 0;      /* don't use reverse video as a default */
    int stip      = 0;      /* don't stipple background by default */
    int analog	  = 0;
    int invkey    = -1;

    XEvent 	event;
    XGCValues 	gcv;
    XWMHints 	wmhints;
    Colormap 	colors;
    XColor	xcsd, xced;
    int min_width, min_height;
    XSizeHints	szhint;

    char *def;



    /*********************Defaults*********************/
    kfont=XGetDefault(dpy,argv[0],"KeyFont");
    if (!kfont) kfont = KFONT;    

    dfont=XGetDefault(dpy,argv[0],"DisplayFont");
    if (!dfont) dfont = DFONT;    

    ffont=XGetDefault(dpy,argv[0],"FlagFont");
    if (!ffont) ffont = FFONT;    

    if ((def=XGetDefault(dpy,argv[0],"BorderWidth"))!=NULL)
         border=atoi(def);

    if ((def=XGetDefault(dpy,argv[0],"ReverseVideo"))!=NULL)
         if (strcmp(def,"on")==0) rvflag=1;

    if ((def=XGetDefault(dpy,argv[0],"Stipple"))!=NULL)
         if (strcmp(def,"on")==0) stip=1;

    if ((def=XGetDefault(dpy,argv[0],"Mode"))!=NULL) {
         if (strcmp(def,"rpn")==0) rpn=1;
         else if (strcmp(def,"analog")==0) analog=1;
         }

    fc  = XGetDefault(dpy, argv[0], "Foreground");
    bc  = XGetDefault(dpy, argv[0], "Background");
    nfc = XGetDefault(dpy, argv[0], "NKeyFore");
    nbc = XGetDefault(dpy, argv[0], "NKeyBack");
    ofc = XGetDefault(dpy, argv[0], "OKeyFore");
    obc = XGetDefault(dpy, argv[0], "OKeyBack");
    ffc = XGetDefault(dpy, argv[0], "FKeyFore");
    fbc = XGetDefault(dpy, argv[0], "FKeyBack");
    dfc = XGetDefault(dpy, argv[0], "DispFore");
    dbc = XGetDefault(dpy, argv[0], "DispBack");
    ifc = XGetDefault(dpy, argv[0], "IconFore");
    ibc = XGetDefault(dpy, argv[0], "IconBack");

    /*********************Options*********************/

    for (i = 1; i < argc; i++) {

        if (argv[i][0] == '=') {
            geom = argv[i];
            continue;
            }

        strind = index(argv[i], ':');

        if(strind != NULL) {
            display = argv[i];
            continue;
            }

        if (strcmp(argv [i], "-bw") == 0) {
            if (++i >= argc) Syntax(argv[0]);
            border = atoi(argv [i]);
            continue;
            }

        if (strcmp(argv [i], "-stip") == 0) {
            stip=1;
            continue;
            }

        if (strcmp(argv [i], "-help") == 0) {
            Syntax(argv[0]);
            }

        if (strcmp(argv [i], "-rv") == 0) {
            rvflag++;
            continue;
            }

        if (strcmp(argv [i], "-rpn") == 0) {
            rpn++;
            continue;
            }

        if (strcmp(argv [i], "-analog") == 0) {
            analog++;
            continue;
            }

        Syntax(argv[0]);
    }


    /*****************************************************/

    /* Open up the display. */

    if ((dpy = XOpenDisplay(display)) == NULL) {
        fprintf(stderr, "%s: Can't open display '%s'\n",argv[0], display);
        exit(1);
        }

    /* Set up colors and pixmaps. */

    /* Set normal default colors */
    if (!rvflag) {
      ForeColor=BlackPixel(dpy, DefaultScreen(dpy));
      BackColor=WhitePixel(dpy, DefaultScreen(dpy));
    } else {
      BackColor=BlackPixel(dpy, DefaultScreen(dpy));
      ForeColor=WhitePixel(dpy, DefaultScreen(dpy));
    }

    dpcs=DisplayCells(dpy, DefaultScreen(dpy));
    if (dpcs<=2) stip=1;  /* monochrome display */

    colors = DefaultColormap(dpy, DefaultScreen(dpy));

    if (dpcs>2&&(fc !=NULL)&&XAllocNamedColor(dpy, colors, fc, &xcsd, &xced))
      ForeColor=xcsd.pixel;

    if (dpcs>2&&(bc !=NULL)&&XAllocNamedColor(dpy, colors, bc, &xcsd, &xced))
      BackColor=xcsd.pixel;

    NKeyFore = OKeyFore = FKeyFore = DispFore = IconFore = ForeColor;
    NKeyBack = OKeyBack = FKeyBack = DispBack = IconBack = BackColor;

    if (dpcs>2&&(nfc !=NULL)&&XAllocNamedColor(dpy, colors, nfc, &xcsd, &xced))
      NKeyFore=xcsd.pixel;

    if (dpcs>2&&(nbc !=NULL)&&XAllocNamedColor(dpy, colors, nbc, &xcsd, &xced))
      NKeyBack=xcsd.pixel;

    if (dpcs>2&&(ofc !=NULL)&&XAllocNamedColor(dpy, colors, ofc, &xcsd, &xced))
      OKeyFore=xcsd.pixel;

    if (dpcs>2&&(obc !=NULL)&&XAllocNamedColor(dpy, colors, obc, &xcsd, &xced))
      OKeyBack=xcsd.pixel;

    if (dpcs>2&&(ffc !=NULL)&&XAllocNamedColor(dpy, colors, ffc, &xcsd, &xced))
      FKeyFore=xcsd.pixel;

    if (dpcs>2&&(fbc !=NULL)&&XAllocNamedColor(dpy, colors, fbc, &xcsd, &xced))
      FKeyBack=xcsd.pixel;

    if (dpcs>2&&(dfc !=NULL)&&XAllocNamedColor(dpy, colors, dfc, &xcsd, &xced))
      DispFore=xcsd.pixel;

    if (dpcs>2&&(dbc !=NULL)&&XAllocNamedColor(dpy, colors, dbc, &xcsd, &xced))
      DispBack=xcsd.pixel;

    if (dpcs>2&&(ifc !=NULL)&&XAllocNamedColor(dpy, colors, ifc, &xcsd, &xced))
      IconFore=xcsd.pixel;

    if (dpcs>2&&(ibc !=NULL)&&XAllocNamedColor(dpy, colors, ibc, &xcsd, &xced))
      IconBack=xcsd.pixel;

    /* load fonts, figure out sizes of keypad and display */
    kfontinfo = XLoadQueryFont(dpy, kfont);
    if (!kfontinfo) kfontinfo = XLoadQueryFont(dpy, "fixed");
    if (!kfontinfo) XCalcError("Can't open '%s' or 'fixed' font\n",KFONT);
    keyfont = kfontinfo->fid;

    dfontinfo = XLoadQueryFont(dpy, dfont);
    if (!dfontinfo) dfontinfo = XLoadQueryFont(dpy, "fixed");
    if (!dfontinfo) XCalcError("Can't open '%s' or 'fixed' font\n",DFONT);
    dispfont = dfontinfo->fid;

    ffontinfo = XLoadQueryFont(dpy, ffont);
    if (!ffontinfo) ffontinfo = XLoadQueryFont(dpy, "fixed");
    if (!ffontinfo) XCalcError("Can't open '%s' or 'fixed' font\n",FFONT);
    flagfont = ffontinfo->fid;

    keywide = XTextWidth(kfontinfo, "MMM", 3) + KEYPADW;
    keyhigh = kfontinfo->ascent + KEYPADH + (rpn ? 2*EXTRAH : 0);

    dispwide  = (rpn ? 6 : 5)*keywide+4*PADDINGW;
    disphigh = dfontinfo->ascent + DISPPADH + FLAGH;

    /* Create Stipple pattern */
    stipplePix = XCreateBitmapFromData (dpy, DefaultRootWindow(dpy),
        check_bits, check_width, check_height);
       
    dimBorder        = stipplePix;

    /* Create Icon Pixmap */
    IconPix = XCreateBitmapFromData (dpy, DefaultRootWindow(dpy),
        icon_bits, icon_width, icon_height);

    if (analog)
      do_sr(argc, argv, geom, border);

    /* Open the main window. */

    if (rpn) {
	min_width  = 10*keywide + 11*PADDINGW + 2;
	min_height = disphigh + 4*keyhigh + 6*PADDINGH + 2;
    } else {
	min_width  = dispwide + 2*PADDINGW + 2;
	min_height = disphigh + 2 + 8*keyhigh + 10*PADDINGH + 10*EXTRAH;
    }

    theWindow = XCreateSimpleWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)),
				    0, 0, min_width, min_height, border,
				    ForeColor, BackColor);
    if (!theWindow) XCalcError("Can't open calculator window");

    if (stip)
      XSetWindowBackgroundPixmap(dpy, theWindow, stipplePix);

    szhint.flags = PSize|PMinSize;
    szhint.width = szhint.min_width = min_width;
    szhint.height = szhint.min_height = min_height;
    XSetStandardProperties(dpy, theWindow, "Calculator", NULL, IconPix,
			   argv, argc, &szhint);

    gcv.function = GXcopyInverted;
    gcv.foreground = DispFore;
    gcv.background = DispBack;
    gcv.font = dispfont;
    dispbgc = XCreateGC(dpy, theWindow,
			GCForeground|GCBackground|GCFunction|GCFont, &gcv);
    gcv.function = GXcopy;
    dispfgc = XCreateGC(dpy, theWindow,
			GCForeground|GCBackground|GCFunction|GCFont, &gcv);
    gcv.font = flagfont;
    flaggc = XCreateGC(dpy, theWindow,
			GCForeground|GCBackground|GCFunction|GCFont, &gcv);
    gcv.function = GXcopy;
    gcv.foreground = NKeyFore;
    gcv.background = NKeyBack;
    gcv.font = keyfont;
    keygc = XCreateGC(dpy, theWindow,
		      GCForeground|GCBackground|GCFunction|GCFont, &gcv);
    gcv.function = GXinvert;
    keyigc = XCreateGC(dpy, theWindow, GCFunction, &gcv);
			

    if (rpn)
      SetupHPCalc();
    else
      SetupTICalc();

    ResetCalc();

    XSelectInput(dpy, theWindow, ExposureMask|KeyPressMask|EnterWindowMask|
		 		 LeaveWindowMask);
    XSelectInput(dpy, dispwid,   ExposureMask|EnterWindowMask|LeaveWindowMask);

    for (i=0; i<numkeys; i++)
        XSelectInput(dpy,key[i].wid,ExposureMask|ButtonPressMask|
			  ButtonReleaseMask|EnterWindowMask|LeaveWindowMask);
    XMapWindow    (dpy, theWindow);
    XMapSubwindows(dpy, theWindow);
    arrow=XCreateFontCursor(dpy, XC_hand2);
    XDefineCursor(dpy, theWindow,arrow);

    /*********** Set up SIGFPE hander ***********/
#ifndef IEEE
    signal(SIGFPE,fperr);
#endif

    /**************** Main loop *****************/

    while (1) {
        Window wind;

        XNextEvent(dpy, &event);

        switch (event.type) {

        case Expose: {
            XExposeEvent *exp_event = (XExposeEvent *) &event;
            wind = exp_event->window;

	    if (wind==theWindow) XClearArea(dpy,theWindow,exp_event->x,
					    exp_event->y,exp_event->width,
					    exp_event->height,False);
	    else if (wind==dispwid) DrawDisplay();
            else if (wind==iconWindow)
	      ;
            else {
                for (i=0; i<numkeys; i++) {
                   if (key[i].wid==wind) DrawKey(i);
                   }
                }
            }
            break;

        case ButtonPress: {
            XButtonPressedEvent *but_event = (XButtonPressedEvent *) &event;
            wind = but_event->window;
            if ((but_event->button & 0xff) == Button1) {
                for (i=0; i<numkeys; i++) {
                   if (key[i].wid==wind) { InvertKey(i); invkey=i; }
                   }
                }

            else if ((but_event->button & 0xff) == Button3) {
	      for (i=0; i<numkeys; i++) {
		if (key[i].wid==wind && key[i].code==kOFF)
		  Quit();
	      }
	    }
	    else XBell(dpy, 0);
	    }
            break;
                       
        case ButtonRelease: {
            XButtonReleasedEvent *but_event = (XButtonReleasedEvent *) &event;
            wind = but_event->window;
            if ((but_event->button & 0xff) == Button1) {
                for (i=0; i<numkeys; i++) {
                   if (key[i].wid==wind) LetGoKey(invkey);
                   }
                invkey = -1;
                }
            }
            break;

        case KeyPress: {
            XKeyPressedEvent *key_event = (XKeyPressedEvent *) &event;
	    char *st, keybuf[10];
            wind = key_event->window;
            if (wind==theWindow) {
	      i = XLookupString(key_event, keybuf, sizeof(keybuf), NULL, NULL);
	      for (st = keybuf; i > 0; i--)
		TypeChar(*st++);
	    } else 
	      printf("KeyPressed in window %ld\n",wind);
	    }
	    break;

	case EnterNotify:
	case LeaveNotify: {
	    XCrossingEvent *cross_event = (XCrossingEvent *) &event;

	    if ((cross_event->detail != NotifyInferior) &&
		(cross_event->window == theWindow) &&
		(cross_event->mode == NotifyNormal) &&
		cross_event->focus) {
		if (event.type == EnterNotify)
		    XSetWindowBorder(dpy,theWindow,ForeColor);
		else
		    XSetWindowBorderPixmap(dpy,theWindow,dimBorder);
	    }
            break;
	  }
        default:
           printf("event type=%ld\n",event.type); 
           XCalcError("Unexpected X_Event");

        }  /* end of switch */
    }  /* end main loop */}



/***********************************/
Syntax(call)
 char *call;
{
    printf ("Usage: %s [-bw <pixels>] [-stip] [-help] [-rv] [-rpn] [-analog]\n", call);
    printf ("          [[<host>]:[<vs>]] [=geometry]\n\n");
    printf ("Default: %s -bw %d =156x226-16-16\n\n",call, DEF_BDRWIDE);
    exit(0);
}


/***********************************/
XCalcError(identifier,arg1,arg2,arg3,arg4)
       char *identifier,*arg1,*arg2,*arg3,*arg4;
{
    fprintf(stderr, identifier, arg1,arg2,arg3,arg4);
    exit(1);
}



/***********************************/
SetupTICalc()
{
    int i;

    numkeys=TINUMKEYS;
    key = &tikeys[0];

    for (i=0; i<numkeys; i++) {
        key[i].x = PADDINGW+(i%5)*(keywide+PADDINGW);
        key[i].y = disphigh+2+2*PADDINGH+(i/5)*(keyhigh+PADDINGH);
        key[i].width = keywide;
        key[i].height = keyhigh;
        if (i>=15) {
            key[i].height+=(2*EXTRAH);
            key[i].y     += ((i-15)/5)*EXTRAH*2;
            }

        switch (tikeys[i].code) {
            case kZERO:  case kONE:  case kTWO:  case kTHREE:
            case kFOUR:  case kFIVE: case kSIX:  case kSEVEN:
            case kEIGHT: case kNINE: case kDEC:  case kNEG:
                key[i].fore=NKeyFore;
                key[i].back=NKeyBack;
                break;
            case kADD:   case kSUB:  case kMUL:  case kDIV:  case kEQU:
                key[i].fore=OKeyFore;
                key[i].back=OKeyBack;
                break;
            default:
                key[i].fore=FKeyFore;
                key[i].back=FKeyBack;
            }
	key[i].wid=XCreateSimpleWindow(dpy,theWindow,key[i].x,key[i].y,
				       key[i].width,key[i].height,1,
				       key[i].fore,key[i].back);
        }

    dispwid = XCreateSimpleWindow(dpy,theWindow,PADDINGW,PADDINGH,
				  dispwide-2,disphigh,2,DispFore,DispBack);
}

SetupHPCalc()
{
    int i,j;

    numkeys=HPNUMKEYS;
    key = &hpkeys[0];

    for (i=0; i<numkeys; i++) {
        j = (i<35)? i : i + 1;
        key[i].x = PADDINGW+(j%10)*(keywide+PADDINGW);
        key[i].y = disphigh+2+2*PADDINGH+(j/10)*(keyhigh+PADDINGH);
        key[i].width = keywide;
        key[i].height = keyhigh;
        if (i==25) {
            key[i].height = keyhigh*2 + PADDINGH;
            }

        switch (hpkeys[i].code) {
            case kZERO:  case kONE:  case kTWO:  case kTHREE:
            case kFOUR:  case kFIVE: case kSIX:  case kSEVEN:
            case kEIGHT: case kNINE: case kDEC:  case kNEG:
	    case kBKSP: case kEE:
                key[i].fore=NKeyFore;
                key[i].back=NKeyBack;
                break;
            case kADD:   case kSUB:  case kMUL:  case kDIV:  case kENTR:
                key[i].fore=OKeyFore;
                key[i].back=OKeyBack;
                break;
            default:
                key[i].fore=FKeyFore;
                key[i].back=FKeyBack;
            }
	key[i].wid=XCreateSimpleWindow(dpy,theWindow,key[i].x,key[i].y,
				       key[i].width,key[i].height,1,
				       key[i].fore,key[i].back);
        }
    dispwid = XCreateSimpleWindow(dpy,theWindow,PADDINGW+keywide,PADDINGH,
				  dispwide-2,disphigh,2,DispFore,DispBack);
}


/**************/
DrawDisplay()
{
    int strwide;

    if (strlen(dispstr)>12) {       /* strip out some decimal digits */
        char tmp[32];
        char *exp = index(dispstr,'e');  /* search for exponent part */
        if (!exp) dispstr[12]='\0';      /* no exp, just trunc. */
        else {
            if (strlen(exp)<=4) 
                sprintf(tmp,"%.8s",dispstr); /* leftmost 8 chars */
            else
                sprintf(tmp,"%.7s",dispstr); /* leftmost 7 chars */
            strcat (tmp,exp);            /* plus exponent */
            strcpy (dispstr,tmp);
            }
        }

    strwide=XTextWidth(dfontinfo,dispstr,strlen(dispstr));

    XFillRectangle(dpy,dispwid,dispfgc,0,0,10,disphigh);
    XFillRectangle(dpy,dispwid,dispfgc,dispwide-10,0,dispwide,disphigh);
    XFillRectangle(dpy,dispwid,dispbgc,10,0,dispwide-20,disphigh);

    XDrawString(dpy,dispwid,dispfgc,dispwide-10-DISPPADW/2-strwide,
		DISPPADH/2+dfontinfo->ascent,dispstr,strlen(dispstr));

    if (flagM) 	       XDrawString(dpy,dispwid,flaggc,12,10,"M",1);
    if (flagE) 	       XDrawString(dpy,dispwid,flaggc,12,20,"E",1);
    if (flagK)         XDrawString(dpy,dispwid,flaggc,20,disphigh-2,"K",1);
    if (flagINV)       XDrawString(dpy,dispwid,flaggc,30,disphigh-2,"INV",3);
    if (drgmode==DEG)  XDrawString(dpy,dispwid,flaggc,60,disphigh-2,"DEG",3);
    if (drgmode==RAD)  XDrawString(dpy,dispwid,flaggc,80,disphigh-2,"RAD",3);
    if (drgmode==GRAD) XDrawString(dpy,dispwid,flaggc,100,disphigh-2,"GRAD",4);
    if (flagPAREN)     XDrawString(dpy,dispwid,flaggc,130,disphigh-2,"( )",3);
}


/***************/
DrawKey(keynum)
    int keynum;
{
    char *str;
    int strwide,extrapad;
    struct _key *kp;

    kp = &key[keynum];
    str = kp->st;
    strwide = XTextWidth(kfontinfo,str,strlen(str));

    extrapad = (keynum>=15 || rpn) ? EXTRAH : 0;
    if (rpn && kp->code == kENTR)
      extrapad = keyhigh;

    XDrawString(dpy,kp->wid,keygc,(kp->width-strwide)/2,
		KEYPADH/2+extrapad+kfontinfo->ascent,str,strlen(str));
}


/*********************************/
InvertKey(keynum)
    int keynum;
{
    struct _key *kp;

    kp = &key[keynum];
    
    XFillRectangle(dpy, kp->wid, keyigc, 0, 0, kp->width, kp->height);
}


/* This section is all of the state machine that implements the calculator
 * functions.  Much of it is shared between the infix and rpn modes.
 */

static double drg2rad=PI/180.0;  /* Conversion factors for trig funcs */
static double rad2drg=180.0/PI;
static int entered=1;  /* true if display contains a valid number.
                          if==2, then use 'dnum', rather than the string
                          stored in the display.  (for accuracy) 
                          if==3, then error occurred, only CLR & AC work */
static int clear  =0;  /* CLR clears display.  if 1, clears acc, also */
static int off    =0;  /* once clears mem, twice quits */
static int decimal=0;  /* to prevent using decimal pt twice in a # */
static int clrdisp=1;  /* if true clears display before entering # */
static int accset =0;
static int lastop =kCLR;
static int memop  =kCLR;
static int exponent=0;
static double acc =0.0;
static double dnum=0.0;
static double mem[10] = { 0.0 };

/*********************************/
LetGoKey(keynum)
     int keynum;
{
    int i;
    int code;

    if (keynum==-1) return;
 
    errno = 0;			/* for non-IEEE machines */

    InvertKey(keynum);

    if ( (entered==3) && !(key[keynum].code==kCLR || key[keynum].code==kOFF)) {
      if (rpn) {
	clrdisp++;
      } else {
        XBell(dpy, 0);
        return;
      }
    }

    code = key[keynum].code;
    if (code != kCLR) clear=0;
    if (code != kOFF) off=0;


#ifndef IEEE
    i=setjmp(env);
    if (i) {
        switch (i) {
#ifdef FPE_FLTDIV_TRAP
           case FPE_FLTDIV_TRAP:  strcpy(dispstr,"div by zero"); break;
#endif
#ifdef FPE_FLTDIV_FAULT
           case FPE_FLTDIV_FAULT: strcpy(dispstr,"div by zero"); break;
#endif
#ifdef FPE_FLTOVF_TRAP
           case FPE_FLTOVF_TRAP:  strcpy(dispstr,"overflow"); break;
#endif
#ifdef FPE_FLTOVF_FAULT
           case FPE_FLTOVF_FAULT: strcpy(dispstr,"overflow"); break;
#endif
#ifdef FPE_FLTUND_TRAP
           case FPE_FLTUND_TRAP:  strcpy(dispstr,"underflow"); break;
#endif
#ifdef FPE_FLTUND_FAULT
           case FPE_FLTUND_FAULT: strcpy(dispstr,"underflow"); break;
#endif
           default:               strcpy(dispstr,"error");
           }
        entered=3;
        DrawDisplay();
        return;
        }
#endif

    (key[keynum].fun)(keynum);
    memop = code;

#ifndef IEEE
    if (errno) {
        strcpy(dispstr,"error");
        DrawDisplay();
        entered=3;
        errno=0;
        }
#endif
}


digit(keynum)
     int keynum;
{
  flagINV=0;
  if (rpn && (memop == kSTO || memop == kRCL || memop == kSUM)) {
      switch (memop) {
      case kSTO: mem[*(key[keynum].st) - '0'] = dnum; break;
      case kRCL: dnum = mem[*(key[keynum].st) - '0'];
		 sprintf(dispstr, "%.8g", dnum);
	  	 break;
      case kSUM: mem[*(key[keynum].st) - '0'] += dnum; break;
      }
      DrawDisplay();
      entered = 2;
      clrdisp++;
      return;
  }
  if (clrdisp) {
    dispstr[0]='\0';
    exponent=decimal=0;
    if (rpn && entered==2)
      PushNum(dnum);
  }
  if (strlen(dispstr)>=MAXDISP)
    return;
  strcat(dispstr,key[keynum].st);
  DrawDisplay();
  if (clrdisp && key[keynum].code != kZERO)
    clrdisp=0; /*no leading 0s*/
  entered=1;
}

bkspf()
{
  if (entered!=1 || clrdisp)
    return;
  if (strlen(dispstr) > 0)
    dispstr[strlen(dispstr)-1] = 0;
  if (strlen(dispstr) == 0) {
    strcat(dispstr, "0");
    clrdisp++;
  }
  DrawDisplay();
}

decf()
{
  flagINV=0;
  if (clrdisp) {
      if (rpn)
	PushNum(dnum);
      strcpy(dispstr,"0");
  }
  if (!decimal) {
    strcat(dispstr,".");
    DrawDisplay();
    decimal++;
  }
  clrdisp=0;
  entered=1;
}

eef()
{
  flagINV=0;
  if (clrdisp) {
      if (rpn)
	PushNum(dnum);
      strcpy(dispstr, rpn ? "1" : "0");
  }
  if (!exponent) {
    strcat(dispstr,"E+");
    DrawDisplay();
    exponent=strlen(dispstr)-1;  /* where the '-' goes */
  }
  clrdisp=0;
  entered=1;
}

clearf()
{
  flagINV=0;
  if (clear && !rpn) { /* clear all */
    ClearStacks();
    flagPAREN=0;
  }
  clear++;
  exponent=decimal=0;
  clrdisp=1;
  entered=1;
  strcpy(dispstr,"0");
  DrawDisplay();
}

negf()
{
  flagINV=0;
  if (exponent) {       /* neg the exponent */
    if (dispstr[exponent]=='-')
      dispstr[exponent]='+';
    else
      dispstr[exponent]='-';
    DrawDisplay();
    return;
  }

  if (strcmp("0",dispstr)==0)
    return;			/* don't neg a zero */
  if (dispstr[0]=='-')	 	/* already neg-ed */
    strcpy(dispstr,dispstr+1);  /* move str left once */
  else {			/* not neg-ed.  add a '-' */
    char tmp[32];
    sprintf(tmp,"-%s",dispstr);
    strcpy(dispstr,tmp);
  }
  if (entered==2)
    dnum = -1.0 * dnum;
  DrawDisplay();
}

/* Two operand functions for infix calc */
twoop(keynum)
{
  double PopNum();

  if (flagINV) {
    flagINV=0;
    DrawDisplay();
  }

  if (!entered) {		/* something like "5+*" */
    if (!isopempty())
      PopOp();			/* replace the prev op */
    PushOp(key[keynum].code);	/* with the new one */
    return;
  }
  
  if (entered==1)
    sscanf(dispstr,"%lf",&dnum);

  clrdisp=clear=1;
  entered=decimal=exponent=0;

  if (!isopempty()) {  /* there was a previous op */
    lastop=PopOp();   /* get it */

    if (lastop==kLPAR) {  /* put it back */
      PushOp(kLPAR);
      PushOp(key[keynum].code);
      PushNum(dnum);
      return;
    }

    /* now, if the current op (keynum) is of
       higher priority than the lastop, the current
       op and number are just pushed on top 
       Priorities:  (Y^X) > *,/ > +,- */
    
    if (priority(key[keynum].code) > priority(lastop)) {
      PushNum(dnum);
      PushOp(lastop);
      PushOp(key[keynum].code);
    } else {  /* execute lastop on lastnum and dnum, push
	       result and current op on stack */
      acc=PopNum();
      switch (lastop) { /* perform the operation */
      case kADD: acc += dnum;  break;
      case kSUB: acc -= dnum;  break;
      case kMUL: acc *= dnum;  break;
      case kDIV: acc /= dnum;  break;
      case kPOW: acc =  pow(acc,dnum);  break;
	}
      PushNum(acc);
      PushOp(key[keynum].code);
      sprintf(dispstr,"%.8g",acc);
      DrawDisplay();
      dnum=acc;
    }
  }
  else { /* op stack is empty, push op and num */
    PushOp(key[keynum].code);
    PushNum(dnum);
  } 
}                      

/* Two operand functions for rpn calc */
twof(keynum)
{
  double PopNum();

  if (flagINV) {
    flagINV=0;
    DrawDisplay();
  }
  if (!entered)
    return;
  if (entered==1)
    sscanf(dispstr, "%lf", &dnum);
  acc = PopNum();
  switch(key[keynum].code) {
  case kADD: acc += dnum;  break;
  case kSUB: acc -= dnum;  break;
  case kMUL: acc *= dnum;  break;
  case kDIV: acc /= dnum;  break;
  case kPOW: acc =  pow(acc,dnum);  break;
  case kXXY: PushNum(dnum);
  }
  sprintf(dispstr, "%.8g", acc);
  DrawDisplay();
  clrdisp++;
  decimal = exponent = 0;
  entered = 2;
  dnum = acc;
}


entrf()
{
  flagINV=0;
  if (!entered)
    return;

  clrdisp=clear=1;
  decimal=exponent=0;

  if (entered==1)
    sscanf(dispstr,"%lf",&dnum);
  entered=2;

  if (entered==2)
    PushNum(dnum);
}

equf()
{
  double PopNum();

  flagINV=0;
  if (!entered)
    return;

  clrdisp=clear=1;
  decimal=exponent=0;

  if (entered==1)
    sscanf(dispstr,"%lf",&dnum);
  entered=2;

  PushNum(dnum);

  while (!isopempty()) {  /* do all pending ops */
    dnum=PopNum();
    acc=PopNum();
    lastop=PopOp();
    switch (lastop) {
    case kADD:  acc += dnum;
		break;
    case kSUB:  acc -= dnum;
		break;
    case kMUL:  acc *= dnum;
		break;
    case kDIV:  acc /= dnum;
		break;
    case kPOW:  acc = pow(acc,dnum);
		break;
    case kLPAR:	flagPAREN--;
		PushNum(acc);
		break;
    }
    dnum=acc;
    PushNum(dnum);
  }

  sprintf(dispstr,"%.8g",dnum);
  DrawDisplay();
}
        
lparf()
{
  flagINV=0;
  PushOp(kLPAR);
  flagPAREN++;
  DrawDisplay();
}

rollf()
{
  double PopNum();

  if (!entered)
    return;
  if (entered==1)
    sscanf(dispstr, "%lf", &dnum);
  entered = 2;
  RollNum(flagINV);
  flagINV=0;
  clrdisp++;
  sprintf(dispstr, "%.8g", dnum);
  DrawDisplay();
}

rparf()
{
  double PopNum();

  flagINV=0;
  if (!entered)
    return;

  if (!flagPAREN)
    return;
  
  clrdisp++;
  decimal=exponent=0;

  if (entered==1)
    sscanf(dispstr,"%lf",&dnum);
  entered=2;

  PushNum(dnum);
  while (!isopempty() && (lastop=PopOp())!=kLPAR) {
    /* do all pending ops, back to left paren */
    dnum=PopNum();
    acc=PopNum();
    switch (lastop) {
    case kADD:  acc += dnum;
		break;
    case kSUB:  acc -= dnum;
		break;
    case kMUL:  acc *= dnum;
		break;
    case kDIV:  acc /= dnum;
		break;
    case kPOW:  acc = pow(acc,dnum);
		break;
    }
    dnum=acc;
    PushNum(dnum);
  }
  PopNum();
  flagPAREN--;
  entered=2;
  sprintf(dispstr,"%.8g",dnum);
  DrawDisplay();
}

drgf()
{
  if (flagINV) {
    if (entered==1)
      sscanf(dispstr,"%lf",&dnum);
    switch (drgmode) {
    case DEG:  dnum=dnum*PI/180.0;    break;
    case RAD:  dnum=dnum*200.0/PI;    break;
    case GRAD: dnum=dnum*90.0/100.0;  break;
    }
    entered=2;
    clrdisp=1;
    flagINV=0;
    sprintf(dispstr,"%.8g",dnum);
  }
                         
  flagINV=0;
  drgmode = ++drgmode % 3;
  switch (drgmode) {
  case DEG:  drg2rad=PI / 180.0;
	     rad2drg=180.0 / PI;
	     break;
  case RAD:  drg2rad=1.0;
	     rad2drg=1.0;
	     break;
  case GRAD: drg2rad=PI / 200.0;
	     rad2drg=200.0 / PI;
	     break;
  }
  DrawDisplay();
}

invf()
{
  flagINV = ~flagINV;
  DrawDisplay();
}

memf(keynum)
{
    if (entered==1)
      sscanf(dispstr,"%lf",&dnum);
    entered = 2;
    clrdisp++;
}

oneop(keynum)
{
  int i,j;
  double dtmp;

  if (entered==1)
    sscanf(dispstr,"%lf",&dnum);
  entered = 2;

  switch (key[keynum].code) {  /* do the actual math fn. */
  case kE:     if (rpn && memop != kENTR) PushNum(dnum); dnum=E;  break;
  case kPI:    if (rpn && memop != kENTR) PushNum(dnum); dnum=PI;  break;
  case kRECIP: dnum=1.0/dnum;  break;
  case kSQR:   flagINV = !flagINV; /* fall through to */
  case kSQRT:  if (flagINV) dnum=dnum*dnum;
	       else dnum=sqrt(dnum);
	       break;
  case k10X:   flagINV = !flagINV; /* fall through to */
  case kLOG:   if (flagINV) dnum=pow(10.0,dnum);
  	       else dnum=log10(dnum);
	       break;
  case kEXP:   flagINV = !flagINV; /* fall through to */
  case kLN:    if (flagINV) dnum=exp(dnum);
	       else dnum=log(dnum);
	       break;
  case kSIN:   if (flagINV) dnum=asin(dnum)*rad2drg;
	       else dnum=sin(dnum*drg2rad);
	       break;
  case kCOS:   if (flagINV) dnum=acos(dnum)*rad2drg;
	       else dnum=cos(dnum*drg2rad);
	       break;
  case kTAN:   if (flagINV) dnum=atan(dnum)*rad2drg;
	       else dnum=tan(dnum*drg2rad);
	       break;
  case kSTO:   mem[0]=dnum;  flagM=!(mem[0]==0.0);  break;
  case kRCL:   dnum=mem[0];  flagM=!(mem[0]==0.0);  break;
  case kSUM:   mem[0]+=dnum; flagM=!(mem[0]==0.0);  break;
  case kEXC:   dtmp=dnum; dnum=mem[0];  mem[0]=dtmp;
	       flagM=!(mem[0]==0.0);  break;
  case kFACT:  if (floor(dnum)!=dnum || dnum<0.0 || dnum>500.0) {
		 strcpy(dispstr,"error");
		 entered=3;
		 break;
	       }
	       i=(int) (floor(dnum));
	       for (j=1,dnum=1.0; j<=i; j++) 
		 dnum*=(float) j;
	       break;
  }
  
  if (entered==3) {  /* error */
    DrawDisplay();
    return;
  }

  entered=2;
  clrdisp=1;
  flagINV=0;
  sprintf(dispstr,"%.8g",dnum);
  DrawDisplay();
}

offf()
{
  /* full reset */
  ResetCalc();
  entered=clrdisp=1;
  dnum=mem[0]=0.0;
  accset=exponent=decimal=0;
  DrawDisplay();
}


nop()
{
  XBell(dpy, 0);
}


/*******/
Quit()
/*******/
{
    exit(0);
}


#define STACKMAX 32
static int opstack[STACKMAX];
static int opsp;
static double numstack[STACKMAX];
static int numsp;


/*******/
PushOp(op)
   int op;
/*******/
{
  if (opsp==STACKMAX) {strcpy(dispstr,"stack error");  entered=3;}
  else opstack[opsp++]=op;
}

/*******/
int PopOp()
/*******/
{
  if (opsp==0) {
      strcpy(dispstr,"stack error");
      entered=3;
      return(kNOP);
  } else
    return(opstack[--opsp]);
}

/*******/
int isopempty()
/*******/
{
  return( opsp ? 0 : 1 );
}

/*******/
PushNum(num)
 double num;
/*******/
{
  if (rpn) {
      numstack[2] = numstack[1];
      numstack[1] = numstack[0];
      numstack[0] = num;
      return;
  }
  if (numsp==STACKMAX) {
      strcpy(dispstr,"stack error");
      entered=3;
  } else
    numstack[numsp++]=num;
}

/*******/
double PopNum()
/*******/
{
    if (rpn) {
	double tmp = numstack[0];
	numstack[0] = numstack[1];
	numstack[1] = numstack[2];
	return(tmp);
    }
    if (numsp==0) {
	strcpy(dispstr,"stack error");
	entered=3;
    } else
      return(numstack[--numsp]);
}

/*******/
RollNum(dir)
/*******/
{
    double tmp;

    if (!dir) {
	tmp         = dnum;
	dnum        = numstack[2];
	numstack[2] = numstack[1];
	numstack[1] = numstack[0];
	numstack[0] = tmp;
    } else {
	tmp         = dnum;
	dnum        = numstack[0];
	numstack[0] = numstack[1];
	numstack[1] = numstack[2];
	numstack[2] = tmp;
    }
}


/*******/
int isnumempty()
/*******/
{
  return( numsp ? 0 : 1 );
}


/*******/
ClearStacks()
/*******/
{
    if (rpn)
      numstack[0] = numstack[1] = numstack[2] = 0.;
    opsp=numsp=0;
}


/*******/
int priority(op)
         int op;
/*******/
{
    switch (op) {
        case kPOW: return(2);
        case kMUL:
        case kDIV: return(1);
        case kADD:
        case kSUB: return(0);
        }
}


/********/
ResetCalc()
/********/
{
    flagM=flagK=flagINV=flagE=flagPAREN=0;  drgmode=DEG;
    strcpy(dispstr,"0");
    ClearStacks();
    drg2rad=PI/180.0;
    rad2drg=180.0/PI;
}


/*********/
TypeChar(c)
    char c;
/*********/
{
    /* figure out if person typed a valid calculator key.
         If so, press the key, wait a bit, and release the key
         else Feep() */

    int i,code;

    switch (c) {
        case '0':  code=kZERO;   break;
        case '1':  code=kONE;    break;
        case '2':  code=kTWO;    break;
        case '3':  code=kTHREE;  break;
        case '4':  code=kFOUR;   break;
        case '5':  code=kFIVE;   break;
        case '6':  code=kSIX;    break;
        case '7':  code=kSEVEN;  break;
        case '8':  code=kEIGHT;  break;
        case '9':  code=kNINE;   break;
        case '.':  code=kDEC;    break;
        case '+':  code=kADD;    break;
        case '-':  code=kSUB;    break;
        case '*':  code=kMUL;    break;
        case '/':  code=kDIV;    break;
        case '(':  code=kLPAR;   break;
        case ')':  code=kRPAR;   break;
        case '!':  code=kFACT;   break;
        case 'e':  code=kEE;     break;
        case '^':  code=kPOW;    break;
        case 'p':  code=kPI;     break;
        case 'i':  code=kINV;    break;
        case 's':  code=kSIN;    break;
        case 'c':  code=kCOS;    break;
        case 't':  code=kTAN;    break;
        case 'd':  code=kDRG;    break;
        case 'l':  code=kLN;     break;
        case '=':  code=kEQU;    break;
        case 'n':  code=kNEG;    break;
	case 'r':  code=kSQRT;   break;
	case 'x':  code=kXXY;    break;
	case '\r':
	case '\n': code=kENTR;	 break;
        case '\177':
        case '\010': code=kBKSP; break;
	case ' ':  code=kCLR;    break;
        case '\003': Quit();     break;
        default:   code = -1;
        }

    if (!rpn && code == kBKSP)
      code = kCLR;
    for (i=0; i < numkeys; i++)
      if (key[i].code == code) break;

    if (code != -1 && i < numkeys && key[i].code == code) {
        InvertKey(i);
        XFlush(dpy);
        Timer(100000L);
        LetGoKey(i);
        XFlush(dpy);
        }
    else XBell(dpy,0);
}



static int timerdone;

/*******/
onalarm()
/*******/
{
  timerdone=1;
}

/*******/
Timer(val)
 long val;
/*******/
{
        struct itimerval it;

        bzero(&it, sizeof(it));
        it.it_value.tv_usec = val;
        timerdone=0;
        signal(SIGALRM,onalarm);
        setitimer(ITIMER_REAL, &it, (struct itimerval *)0);
        while (!timerdone);
        signal(SIGALRM,SIG_DFL);
}



#ifndef IEEE
/******************/
fperr(sig,code,scp)
  int sig,code;
  struct sigcontext *scp;
/******************/
{
    longjmp(env,code);
}

#endif
