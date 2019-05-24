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

/* $Header: os.h,v 1.18 87/09/10 18:00:15 toddb Exp $ */

#ifndef OS_H
#define OS_H
#include "misc.h"

#define NullFID ((FID) 0)

#define SCREEN_SAVER_ON   0
#define SCREEN_SAVER_OFF  1
#define SCREEN_SAVER_FORCER 2

#define MAX_REQUEST_SIZE 16384

typedef pointer	FID;
typedef struct _FontPathRec *FontPathPtr;
typedef struct _NewClientRec *NewClientPtr;

/*
 * os-dependent definition of local allocation and deallocation
 * If you need something other than malloc/free for ALLOCATE/DEALLOCATE
 * LOCAL then you add that to the beginning of this set.  Note that
 * some machines do not return a valid pointer for malloc(0), in
 * which case we provide an alternate under the control of the
 * define MALLOC_0_RETURNS_NULL.  This is necessary because some
 * server code expects malloc(0) to return a valid pointer to storage.
 */
#if defined(ibm032) && !defined(_pcc_)

#define ALLOCATE_LOCAL(size) (alloca(size))
#define DEALLOCATE_LOCAL(ptr)
pragma on(alloca);

#else /* everyone else */

# ifdef MALLOC_0_RETURNS_NULL
# define ALLOCATE_LOCAL(size) (malloc(max(size,1)))
# else
# define ALLOCATE_LOCAL(size) (malloc(size))
# endif

#define DEALLOCATE_LOCAL(ptr) (free(ptr))

#endif

extern WaitForSomething();
extern char *ReadRequestFromClient();   /* should be xReq but then 
					   include Xproto.h */
extern Bool CloseDownConnection();
extern void CreateWellKnownSockets();
extern FontPathPtr GetFontPath();
extern FontPathPtr ExpandFontNamePattern();
extern FID FiOpenForRead();
extern int FiClose();

#endif /* OS_H */
