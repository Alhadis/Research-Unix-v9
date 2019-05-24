/***********************************************************
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

******************************************************************/
/* $Header: utils.c,v 1.34 87/09/14 02:09:06 toddb Exp $ */
#include <stdio.h>
#include <sys/time.h>
#include "misc.h"
#include "X.h"
#include "input.h"
#include "opaque.h"
#include "site.h"
#include <strings.h>
extern char *display;

extern long defaultScreenSaverTime;	/* for parsing command line */
extern long defaultScreenSaverInterval;
extern int defaultScreenSaverBlanking;

extern long ScreenSaverTime;		/* for forcing reset */

Bool clientsDoomed = FALSE;
extern void KillServerResources();

/* Force connections to close on SIGHUP from init */

AutoResetServer ()
{
    clientsDoomed = TRUE;
    /* force an immediate timeout (and exit) in WaitForSomething */
    ScreenSaverTime = TimeSinceLastInputEvent();
#ifdef GPROF
    chdir ("/tmp");
    exit (0);
#endif
}

/* Force connections to close and then exit on SIGTERM, SIGINT */

GiveUp()
{
    KillServerResources();
    exit(0);
}


void
ErrorF( f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9) /* limit of ten args */
    char *	f;
    char *s0, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9;
{
    fprintf( stderr, f, s0, s1, s2, s3, s4, s5, s6, s7, s8, s9);
}

void
AbortServer()
{
    abort();
}

int
Error(str)
    char *str;
{
    perror(str);
}

int
Notice()
{
}

int
GetTimeInMillis()
{
    struct timeval  tp;

    gettimeofday(&tp, 0);
    return(tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

/*
 * This function parses the command line. Handles device-independent fields
 * only.  It is not allowed to modify argc or any of the strings pointed to
 * by argv.
 */
ProcessCommandLine ( argc, argv )
int	argc;
char	*argv[];

{
    int i;

    for ( i = 1; i < argc; i++ )
    {
	/* initialize display */
	if(argv[i][0] ==  ':')  
	{
	    display = argv[i];
	    display++;
	}
	else if ( strcmp( argv[i], "-a") == 0)
	{
	    if(++i < argc)
	        defaultPointerControl.num = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "c") == 0)
	{
	    if(++i < argc)
	        defaultKeyboardControl.click = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-c") == 0)
	{
	    defaultKeyboardControl.click = 0;
	}
	else if ( strcmp( argv[i], "-co") == 0)
	{
	    if(++i < argc)
	        rgbPath = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-f") == 0)
	{
	    if(++i < argc)
	        defaultKeyboardControl.bell = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fc") == 0)
	{
	    if(++i < argc)
	        defaultCursorFont = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fn") == 0)
	{
	    if(++i < argc)
	        defaultTextFont = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-fp") == 0)
	{
	    if(++i < argc)
	        defaultFontPath = argv[i];
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-help") == 0)
	{
	    UseMsg();
	    exit(0);
	}
	else if ( strcmp( argv[i], "-p") == 0)
	{
	    if(++i < argc)
	        defaultScreenSaverInterval = atoi(argv[i]) * MILLI_PER_MIN;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "r") == 0)
	    defaultKeyboardControl.autoRepeat = TRUE;
	else if ( strcmp( argv[i], "-r") == 0)
	    defaultKeyboardControl.autoRepeat = FALSE;
	else if ( strcmp( argv[i], "-s") == 0)
	{
	    if(++i < argc)
	        defaultScreenSaverTime = atoi(argv[i]) * MILLI_PER_MIN;
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-t") == 0)
	{
	    if(++i < argc)
	        defaultPointerControl.threshold = atoi(argv[i]);
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "-to") == 0)
	{
	    if(++i < argc)
	    {
	        if((TimeOutValue = atoi(argv[i])) < 0)
		    TimeOutValue = DEFAULT_TIMEOUT;
	    }
	    else
		UseMsg();
	}
	else if ( strcmp( argv[i], "v") == 0)
	    defaultScreenSaverBlanking = PreferBlanking;
	else if ( strcmp( argv[i], "-v") == 0)
	    defaultScreenSaverBlanking = DontPreferBlanking;
	else if ( strcmp( argv[i], "-x") == 0)
	{
	    if(++i >= argc)
		UseMsg();
	    /* For U**x, which doesn't support dynamic loading, there's nothing
	     * to do when we see a -x.  Either the extension is linked in or
	     * it isn't */
	}
    }
}

UseMsg()
{
    ErrorF("use: X <display> [option] <tty>\n");
    ErrorF("-a #                   mouse acceleration (pixels)\n");
    ErrorF("-bp<:screen> color     BlackPixel for screen\n");
    ErrorF("-c                     turns off key-click\n");
    ErrorF("c #                    key-click volume (0-8)\n");
    ErrorF("-co string             color database file\n");
    ErrorF("-fc string             cursor font\n");
    ErrorF("-fn string             default text font name\n");
    ErrorF("-fp string             default text font path\n");
    ErrorF("-p #                   screen-saver pattern duration (seconds)\n");
    ErrorF("-r                     turns off auto-repeat\n");
    ErrorF("r                      turns on auto-repeat \n");
    ErrorF("-f #                   bell base (0-100)\n");
    ErrorF("-x string              loads named extension at init time \n");
    ErrorF("-help                  prints message with these options\n");
    ErrorF("-s #                   screen-saver timeout (seconds)\n");
    ErrorF("-t #                   mouse threshold (pixels)\n");
    ErrorF("-to #                  connection time out\n");
    ErrorF("v                      video blanking for screen-saver\n");
    ErrorF("-v                     screen-saver without video blanking\n");
    ErrorF("-wp<:screen> color     WhitePixel for screen\n");
    ErrorF("There may be other device-dependent options as well\n");
}
/*
 * malloc wrap-around, to take care of the "no memory" case, since
 * it would be difficult in many places to "back out" on failure.
 */
#ifdef DEBUG
#define FIRSTMAGIC 0x11aaaa11
#define SECONDMAGIC 0x22aaaa22
#define FREEDMAGIC  0x33aaaa33
#endif

/* XALLOC -- X's internal memory allocator.  Why does it return unsigned
 * int * instead of the more common char *?  Well, if you read K&R you'll
 * see they say that alloc must return a pointer "suitable for conversion"
 * to whatever type you really want.  In a full-blown generic allocator
 * there's no way to solve the alignment problems without potentially
 * wasting lots of space.  But we have a more limited problem. We know
 * we're only ever returning pointers to structures which will have to
 * be long word aligned.  So we are making a stronger guarantee.  It might
 * have made sense to make Xalloc return char * to conform with people's
 * expectations of malloc, but this makes lint happier.
 */

unsigned long * 
Xalloc (amount)
    int amount;
{
    char		*malloc();
    register pointer  ptr;
	
    if(amount == 0)
	return( (unsigned long *)NULL);
#ifdef DEBUG
        /* aligned extra on long word boundary */
    amount = (amount + 3) & ~3;  
    if (ptr =  (pointer) malloc( ((unsigned) amount) + 12))
    {
        *(unsigned long *)ptr = FIRSTMAGIC;
        *((unsigned long *)(ptr + 4)) = amount;
        *((unsigned long *)(ptr + 8 + amount)) = SECONDMAGIC;
	return (unsigned long *) (ptr + 8);
    }
    FatalError("Error in Xalloc\n");
    /* NOTREACHED */
#else
    ptr =  (pointer) malloc( ((unsigned) amount));
    return ((unsigned long *)ptr);
#endif /* DEBUG */
    /* NOTREACHED */
}

/*****************
 * Xrealloc
 *     realloc wrap-around, to take care of the "no memory" case, since
 *     it would be difficult in many places to "back out" on failure.
 *****************/

extern unsigned long *minfree;           /* DEBUG */

unsigned long *
Xrealloc (ptr, amount)
register pointer ptr;
int amount;
{
    char *malloc();
    char *realloc();
    char *foo;

    amount = (amount + 3) & ~3;  
    if (ptr)
    {
        if (ptr < (pointer) minfree)
        {
	    ErrorF("Xrealloc: trying to free static storage\n");
	    /* Force a core dump */
	    AbortServer();
	}
#ifdef DEBUG
	if (!CheckNode(ptr - 8))
	    AbortServer();
	ptr = (pointer) realloc ((ptr - 8), (unsigned) amount + 12);
#else
        ptr = (pointer) realloc (ptr, amount);
#endif /* DEBUG */
    }
    else
    {
#ifdef DEBUG
	ptr =  (pointer) malloc (((unsigned) amount) + 12);
#else
	ptr =  (pointer) malloc ((unsigned) amount);
#endif 
    }
#ifdef DEBUG
    if (ptr)
    {
        *(unsigned long *)ptr = FIRSTMAGIC;
	*((unsigned long *)(ptr + 4)) = amount;
	*((unsigned long *)(ptr + 8 + amount)) = SECONDMAGIC;
	return ((unsigned long *) (ptr + 8));
    }
    FatalError ("Error in Xrealloc\n");
    /*NOTREACHED*/
#else
        return((unsigned long *)ptr);
#endif /* DEBUG */
}
                    
/*****************
 *  Xfree
 *    calls free 
 *****************/    

void
Xfree(ptr)
    register pointer ptr;
{
    if (ptr == (pointer) NULL)
	return;
#ifdef DEBUG
    if (ptr < (pointer) minfree)
	ErrorF("Xfree: trying to free static storage\n");
    else if (CheckNode(ptr - 8))
    {
        *(unsigned long *)(ptr - 8) = FREEDMAGIC;
	free(ptr - 8); 
    }
#else
    if (ptr >= (pointer) minfree)
        free(ptr); 
#endif /* DEBUG */
}

#ifdef DEBUG
static Bool
CheckNode(ptr)
    pointer ptr;
{
    int    amount;

    amount = *((unsigned long *)(ptr + 4));
    if (*((unsigned long *) ptr) == FREEDMAGIC)
    {
        ErrorF("Freeing something already freed!\n");
	return FALSE;
    }
    if( *((unsigned long *) ptr) != FIRSTMAGIC ||
        *((unsigned long *) (ptr + amount + 8)) != SECONDMAGIC)
	FatalError("Heap bug!\n");
    return TRUE;
}
#endif /* DEBUG */
