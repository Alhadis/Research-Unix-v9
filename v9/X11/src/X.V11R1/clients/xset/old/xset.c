/* 
 * $header: xset.c,v 1.18 87/07/11 08:47:46 dkk Locked $ 
 * $Locker:  $ 
 */
#include <X11/copyright.h>

/* Copyright    Massachusetts Institute of Technology    1985	*/

#ifndef lint
static char *rcsid_xset_c = "$Header: xset.c,v 1.22 87/09/12 23:14:10 toddb Exp $";
#endif

#include <X11/X.h>      /*  Should be transplanted to X11/Xlibwm.h     %*/
#include <X11/Xlib.h>
/*  #include <X11/Xlibwm.h>  [Doesn't exist yet  5-14-87]  %*/
#include <X11/keysym.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <ctype.h>

#define ON 1
#define OFF 0

#define DONT_CHANGE -2

#define ALL -1
#define TIMEOUT 1
#define INTERVAL 2
#define PREFER_BLANK 3
#define ALLOW_EXP 4

#define	nextarg(i, argv) \
	argv[i]; \
	if (i >= argc) \
		break; \

char *progName;

main(argc, argv)
int argc;
char **argv;
{
register char *arg;
register int i;
int percent;
int acc_num, acc_denom, threshold;
XKeyboardControl values;
unsigned long pixels[512];
caddr_t colors[512];
XColor def;
int numpixels = 0;
char *disp = '\0';
Display *dpy;
if (argc == 1)  usage(argv[0]); /* To be replaced by window-interface */
progName = argv[0];
for (i = 1; i < argc; ) {
  arg = argv[i++];
  if (index(arg, ':')) {     /*  Set display name if given by user.  */
    disp = arg;
  } 
}
dpy = XOpenDisplay(disp);  /*  Open display and check for success */
if (dpy == NULL) {
  fprintf(stderr, "%s: Can't open display '%s'\n",
	  argv[0], XDisplayName(disp ? disp : NULL ));
  exit(1);
}
for (i = 1; i < argc; ) {
  arg = argv[i++];
  if (index(arg, ':')) {     /*  Set display name if given by user.  */
	; /* forget this */
  } else if (*arg == '-' && *(arg + 1) == 'c'){ /* Does arg start with "-c"? */
    set_click(dpy, 0);           /* If so, turn click off and  */
  } 
  else if (*arg == 'c') {         /* Well, does it start with "c", then? */
    percent = -1;   /* Default click volume.      */
    arg = nextarg(i, argv);
    if (strcmp(arg, "on") == 0) {               /* Let click be default. */
      i++;
    } 
    else if (strcmp(arg, "off") == 0) {  
      percent = 0;       /* Turn it off.          */
      i++;
    } 
    else if (isnumber(arg, 100)) {
      percent = atoi(arg);  /* Set to spec. volume */
      i++;
    }
    set_click(dpy, percent);
  } 
  else if (*arg == '-' && *(arg + 1) == 'b') {  /* Does arg start w/ "-b" */
    set_bell_vol(dpy, 0);           /* Then turn off bell.    */
  } 
  else if (*arg == 'b') {                       /* Does it start w/ "b".  */
    percent = -1;             /* Set bell to default.   */
    arg = nextarg(i, argv);
    if (strcmp(arg, "on") == 0) {               /* Let it stay that way.  */
      set_bell_vol(dpy, percent);
      i++;
    } 
    else if (strcmp(arg, "off") == 0) {
      percent = 0;            /* Turn the bell off.     */
      set_bell_vol(dpy, percent);
      i++;
    } 
    else if (isnumber(arg, 100)) {              /* If volume is given:    */
      percent = atoi(arg);    /* set bell appropriately.*/
      set_bell_vol(dpy, percent);
      i++;
      arg = nextarg(i, argv);

      if (isnumber(arg, 20000)) {               /* If pitch is given:     */
	set_bell_pitch(dpy, atoi(arg));    /* set the bell.           */
	i++;

	arg = nextarg(i, argv);
	if (isnumber(arg, 1000)) {              /* If duration is given:  */
	  set_bell_dur(dpy, atoi(arg));  /*  set the bell.      */
	  i++;
	}
      }
    }
  }
  else if (strcmp(arg, "fp") == 0) {	       /* set font path */
    arg = nextarg(i, argv);
    set_font_path(dpy, arg);
    i++;
  }
  else if (strcmp(arg, "-led") == 0) {         /* Turn off one or all LEDs */
    values.led_mode = OFF;
    values.led = ALL;        /* None specified */
    arg = nextarg(i, argv);
    if (isnumber(arg, 32) && atoi(arg) > 0) {
      values.led = atoi(arg);
      i++;
    }
    set_led(dpy, values.led, values.led_mode);
  } 
  else if (strcmp(arg, "led") == 0) {         /* Turn on one or all LEDs  */
    values.led_mode = ON;
    values.led = ALL;
    arg = nextarg(i, argv);
    if (strcmp(arg, "on") == 0) {
      i++;
    } 
    else if (strcmp(arg, "off") == 0) {       /*  ...except in this case. */
       values.led_mode = OFF;
      i++;
    }
    else if (isnumber(arg, 32) && atoi(arg) > 0) {
      values.led = atoi(arg);
      i++;
    }
    set_led(dpy, values.led, values.led_mode);
  }
/*  Set pointer (mouse) settings:  Acceleration and Threshold. */
  else if (strcmp(arg, "m") == 0 || strcmp(arg, "mouse") == 0) {
    acc_num = -1;
    acc_denom = -1;     /*  Defaults */
    threshold = -1;
    if (i >= argc){
      set_mouse(dpy, acc_num, acc_denom, threshold);
      break;
    }
    arg = argv[i];
    if (strcmp(arg, "default") == 0) {
      i++;
    } 
    else if (*arg >= '0' && *arg <= '9') {
      acc_num = atoi(arg);  /* Set acceleration to user's tastes.  */
      i++;
      if (i >= argc) {
	set_mouse(dpy, acc_num, acc_denom, threshold);
	break;
      }
      arg = argv[i];
      if (*arg >= '0' && *arg <= '9') {
	threshold = atoi(arg);  /* Set threshold as user specified.  */
	i++;
      }
    }
      set_mouse(dpy, acc_num, acc_denom, threshold);
  } 
  else if (*arg == 's') {   /*  If arg starts with "s".  */
    if (i >= argc) {
      set_saver(dpy, ALL, 0);  /* Set everything to default  */
      break;
    }
    arg = argv[i];
    if (strcmp(arg, "blank") == 0) {       /* Alter blanking preference. */
      set_saver(dpy, PREFER_BLANK, PreferBlanking);
      i++;
    }
    else if (strcmp(arg, "noblank") == 0) {     /*  Ditto.  */
      set_saver(dpy, PREFER_BLANK, DontPreferBlanking);
      i++;
    }
    else if (strcmp(arg, "off") == 0) {
      set_saver(dpy, TIMEOUT, 0);   /*  Turn off screen saver.  */
      i++;
      if (i >= argc)
	break;
      arg = argv[i];
      if (strcmp(arg, "off") == 0) {
	set_saver(dpy, INTERVAL, 0);
	i++;
      }
    }
    else if (strcmp(arg, "default") == 0) {    /*  Leave as default.       */
      set_saver(dpy, ALL, 0);
      i++;
    } 
    else if (*arg >= '0' && *arg <= '9') {  /*  Set as user wishes.   */
      set_saver(dpy, TIMEOUT, atoi(arg));
      i++;
      if (i >= argc)
	break;
      arg = argv[i];
      if (*arg >= '0' && *arg <= '9') {
	set_saver(dpy, INTERVAL, atoi(arg));
	i++;
      }
    }
  } 
  else if(*arg == '-' && *(arg + 1) == 'r'){ /* If arg starts w/ "-r" */
    set_repeat(dpy, ALL, OFF);
  } 
  else if (*arg == 'r') {            /*  If it starts with "r"        */
    if (i >= argc) {
      set_repeat(dpy, ALL, ON);
      break;
    }
    arg = argv[i];                   /*  Check next argument.         */
    if (strcmp(arg, "on") == 0) {
      set_repeat(dpy, ALL, ON);
      i++;
    } 
    else if (strcmp(arg, "off") == 0) {
      set_repeat(dpy, ALL, OFF);
      i++;
    }
  } 
  else if (*arg == 'p') {           /*  If arg starts with "p"       */
    if (i + 1 >= argc)
      usage(argv[0]);
    arg = argv[i];
    if (*arg >= '0' && *arg <= '9')
      pixels[numpixels] = atoi(arg);
    else
      usage(argv[0]);
    i++;
    colors[numpixels] = argv[i];
    i++;
    numpixels++;
    set_pixels(dpy, pixels, colors, numpixels);
  }
  else if (*arg == '-' && *(arg + 1) == 'k') {
    set_lock(dpy, OFF);
  }
  else if (*arg == 'k') {         /*  Set modifier keys.               */
    set_lock(dpy, ON);
  }
  else if (*arg == 'q') {         /*  Give status to user.             */
    query(dpy);
  }
  else
    usage(argv[0]);
}

XFlush(dpy);

exit(0);    /*  Done.  We can go home now.  */
}


isnumber(arg, maximum)
	char *arg;
	int maximum;
{
	register char *p;

	if (arg[0] == '-' && arg[1] == '1' && arg[2] == '\0')
		return(1);
	for (p=arg; isdigit(*p); p++);
	if (*p || atoi(arg) > maximum)
		return(0); 
	return(1);
}

/*  These next few functions do the real work (xsetting things).
 */
set_click(dpy, percent)
Display *dpy;
int percent;
{
XKeyboardControl values;
values.key_click_percent = percent;
XChangeKeyboardControl(dpy, KBKeyClickPercent, &values);
return;
}

set_bell_vol(dpy, percent)
Display *dpy;
int percent;
{
XKeyboardControl values;
values.bell_percent = percent;
XChangeKeyboardControl(dpy, KBBellPercent, &values);
return;
}

set_bell_pitch(dpy, pitch)
Display *dpy;
int pitch;
{
XKeyboardControl values;
values.bell_pitch = pitch;
XChangeKeyboardControl(dpy, KBBellPitch, &values);
return;
}

set_bell_dur(dpy, duration)
Display *dpy;
int duration;
{
XKeyboardControl values;
values.bell_duration = duration;
XChangeKeyboardControl(dpy, KBBellDuration, &values);
return;
}

set_font_path(dpy, path)
    Display *dpy;
    char *path;
{
    char **directoryList = NULL; int ndirs = 0;
    char *directories;
    char *pDir;

    if (strcmp(path, "default")!=0) {
        if (((directories = (char *)malloc( strlen(path) )) == NULL) ||
	    ((directoryList = (char **)malloc(sizeof(char *))) == NULL))
	    error( "out of memory" );

	strcpy( directories, path );
	*directoryList = pDir = directories;
	ndirs++;
	while( (pDir = index(pDir, ',')) != NULL) {
	    *pDir++ = '\0';
	    directoryList = (char **)realloc(directoryList, 
					     (ndirs+1)*sizeof(char *));
	    if (directoryList == NULL) error( "out of memory" );
	    directoryList[ndirs++] = pDir;
	}
    }

    XSetFontPath( dpy, directoryList, ndirs );
}

set_led(dpy, led, led_mode)
Display *dpy;
int led, led_mode;
{
  XKeyboardControl values;
  values.led_mode = led_mode;
  if (led != ALL) {
    values.led = led;
    XChangeKeyboardControl(dpy, KBLed | KBLedMode, &values);
  }
  else {
    XChangeKeyboardControl(dpy, KBLedMode, &values);
  }
  return;
}

set_mouse(dpy, acc_num, acc_denom, threshold)
Display *dpy;
int acc_num, acc_denom, threshold;
{
int do_accel = True, do_threshold = True;
if (acc_num == DONT_CHANGE)
  do_accel = False;
if (threshold == DONT_CHANGE)
  do_threshold = False;
XChangePointerControl(dpy, do_accel, do_threshold, acc_num,
		      acc_denom, threshold);
return;
}

set_saver(dpy, mask, value)
Display *dpy;
int mask, value;
{
  int timeout, interval, prefer_blank, allow_exp;
  XGetScreenSaver(dpy, &timeout, &interval, &prefer_blank, 
		  &allow_exp);
  if (mask == TIMEOUT) timeout = value;
  if (mask == INTERVAL) interval = value;
  if (mask == PREFER_BLANK) prefer_blank = value;
  if (mask == ALLOW_EXP) allow_exp = value;
  if (mask == ALL) {  /* "value" is ignored in this case.  (defaults) */
    timeout = -1;
    interval = -1;
    prefer_blank = DefaultBlanking;
    allow_exp = DefaultExposures;
  }
      XSetScreenSaver(dpy, timeout, interval, prefer_blank, 
		      allow_exp);
  return;
}

set_repeat(dpy, key, auto_repeat_mode)
Display *dpy;
int key, auto_repeat_mode;
{
  XKeyboardControl values;
  values.auto_repeat_mode = auto_repeat_mode;
  if (key != ALL) {
    values.key = key;
    XChangeKeyboardControl(dpy, KBKey | KBAutoRepeatMode, &values);
  }
  else {
    XChangeKeyboardControl(dpy, KBAutoRepeatMode, &values);
  }
  return;
}

set_pixels(dpy, pixels, colors, numpixels)
Display *dpy;
unsigned long pixels[512];
caddr_t colors[512];
int numpixels;
{
  char *spec;   /*%%%%%*/
  XColor def;
  if(DisplayCells(dpy, DefaultScreen(dpy)) >= 2) {
    while (--numpixels >= 0) {
      def.pixel = pixels[numpixels];
      if (XParseColor(dpy, colors[numpixels], spec, &def))
	XStoreColor(&def);
      else
	fprintf(stderr, "%s: No such color\n", colors[numpixels]);
    }
  }
  return;
}

set_lock(dpy, onoff)
Display *dpy;
Bool onoff;
{
  XModifierKeymap *mods;
  mods = XGetModifierMapping(dpy);

  if (onoff)
    mods = XInsertModifiermapEntry(mods, XK_Caps_Lock, LockMapIndex);
  else
    mods = XDeleteModifiermapEntry(mods, XK_Caps_Lock, LockMapIndex);
  XSetModifierMapping(dpy, mods);
  return;
}

/*  This is the information-getting function for telling the user what the
 *  current "xsettings" are.
 */
query(dpy)
Display *dpy;
{
XKeyboardState values;
int acc_num, acc_denom, threshold;
int timeout, interval, prefer_blank, allow_exp;
char **font_path; int npaths;

XGetKeyboardControl(dpy, &values);
XGetPointerControl(dpy, &acc_num, &acc_denom, &threshold);
XGetScreenSaver(dpy, &timeout, &interval, &prefer_blank, &allow_exp);
font_path = XGetFontPath(dpy, &npaths);

printf ("Keyboard Control Values:\n");
/*printf ("Auto Repeat: %d \t\t", values.auto_repeat_mode);    %%*/
/*printf ("Key: %d \n\n", values.key);     %%*/
printf ("Key Click Volume (%%): %d \n", values.key_click_percent);
printf ("Bell Volume (%%): %d \t", values.bell_percent);
printf ("Bell Pitch (Hz): %d \t", values.bell_pitch);
printf ("Bell Duration (msec): %d \n", values.bell_duration);
/*printf ("LED: %d \t\t\t", values.led);
printf ("LED Mode: %o \t\t", values.led_mode);         %%*/

printf ("Pointer (Mouse) Control Values:\n");
printf ("Acceleration: %d \t", acc_num / acc_denom);
printf ("Threshold: %d \n", threshold);
printf ("Screen Saver: (yes = %d, no = %d, default = %d)\n",
	PreferBlanking, DontPreferBlanking, DefaultBlanking);
printf ("Prefer Blanking: %d \t", prefer_blank);
printf ("Time-out: %d \t Cycle: %d\n", timeout, interval);
if (npaths) {
    printf( "Font Path: %s", *font_path++ );
    for( --npaths; npaths; npaths-- )
        printf( ",%s", *font_path++ );
    printf( "\n" );
}
return;
}


/*  This is the usage function */

usage(prog)
char *prog;
{
	printf("usage: %s option [option ...] [host:vs]\n", prog);
	printf("    To turn bell off:\n");
	printf("\t-b                b off               b 0\n");
	printf("    To set bell volume, pitch and duration:\n");
	printf("\t b [vol [pitch [dur]]]          b on\n");
	printf("    To turn keyclick off:\n");
	printf("\t-c                c off               c 0\n");
	printf("    To set keyclick volume:\n");
	printf("\t c [0-100]        c on\n");
	printf("    To set the font path:\n" );
	printf("\t fp path[,path...]\n" );
	printf("    To restore the default font path:\n" );
	printf("\t fp default\n" );
	printf("    To set LED states off or on:\n");
	printf("\t-led [1-32]         led off\n");
	printf("\t led [1-32]         led on\n");
	printf("    To set mouse acceleration and threshold:\n");
	printf("\t m [acc [thr]]    m default\n");
	printf("    To set pixel colors:\n");
	printf("\t p pixel_value color_name\n");
	printf("    To turn auto-repeat off or on:\n");
	printf("\t-r     r off        r    r on\n");
	printf("    For screen-saver control:\n");
	printf("\t s [timeout [cycle]]  s default\n");
	printf("\t s blank              s noblank\n");
	printf("    For status information:  q   or  query\n");
	exit(0);
}

error( message )
    char *message;
{
    fprintf( stderr, "%s: %s\n", progName, message );
    exit( 1 );
}
