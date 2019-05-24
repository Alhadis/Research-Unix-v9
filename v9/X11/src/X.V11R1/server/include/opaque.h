/* $Header: opaque.h,v 1.3 87/09/07 13:09:51 swick Exp $ */

extern char *defaultFontPath;
extern char *defaultTextFont;
extern char *defaultCursorFont;
extern char *rgbPath;
extern long MaxClients;
extern int isItTimeToYield;

extern void CloseFont();
extern unsigned long *Xalloc();
extern unsigned long *Xrealloc();
extern void Xfree();
extern void AddResource();
extern void FreeResource();
extern pointer LookupID();
extern int	TimeOutValue;
extern long ScreenSaverTime;
extern long ScreenSaverInterval;
extern int  ScreenSaverBlanking;
extern int  ScreenSaverAllowExposures;
extern int argcGlobal;
extern char **argvGlobal;
