/* $Header: XrmConvert.c,v 1.2 87/09/13 20:07:28 jg Exp $ */
#ifndef lint
static char *sccsid = "@(#)XrmConvert.c	1.6	2/25/87";
#endif lint

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.  
 * 
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */


#include	"Xlib.h"
#include	"Xlibos.h"
#include	"Xresource.h"
#include	"XrmConvert.h"
#include	"Conversion.h"

/* ||| */
#include	<stdio.h>

extern int	bcmp();
extern void	bcopy();

/* Conversion procedure hash table */

typedef struct _ConverterRec *ConverterPtr;

typedef struct _ConverterRec {
    ConverterPtr	next;
    XrmRepresentation	from, to;
    XrmTypeConverter	proc;
} ConverterRec;

#define CONVERTERHASHSIZE	256
#define CONVERTERHASHMASK	255
#define ProcHash(from, to) (2 * (from) + to)

typedef ConverterPtr ConverterTable[CONVERTERHASHSIZE];

static ConverterTable	converterTable;


/* Data cache hash table */

typedef struct _CacheRec *CachePtr;

typedef struct _CacheRec {
    CachePtr		next;
    int			hash;
    Screen		*screen;
    XrmRepresentation	fromType;
    XrmValue		from;
    XrmRepresentation	toType;
    XrmValue		to;
} CacheRec;

#define CACHEHASHSIZE	256
#define CACHEHASHMASK	255
#define CacheHash(from, to, data) (2 * (from) + to + (int)(data))

typedef CachePtr CacheHashTable[CACHEHASHSIZE];

static CacheHashTable	cacheHashTable;

void _XrmRegisterTypeConverter(fromType, toType, convertProc)
    XrmRepresentation		fromType, toType;
    XrmTypeConverter		convertProc;
{
    register ConverterPtr	*pHashEntry;
    register ConverterPtr	p;

    pHashEntry = &converterTable[ProcHash(fromType,toType) & CONVERTERHASHMASK];
    p 		= (ConverterPtr) Xmalloc(sizeof(ConverterRec));
    p->next 	= *pHashEntry;
    *pHashEntry	= p;
    p->from	= fromType;
    p->to	= toType;
    p->proc	= convertProc;
}

void XrmRegisterTypeConverter(fromType, toType, convertProc)
    register XrmAtom	fromType, toType;
    XrmTypeConverter	convertProc;
{
    _XrmRegisterTypeConverter(XrmAtomToRepresentation(fromType),
			     XrmAtomToRepresentation(toType), convertProc);
}

Bool LookupConverter(fromType, toType, convertProc)
    register XrmRepresentation	fromType, toType;
    XrmTypeConverter		*convertProc;
{
    register ConverterPtr	p;

    p = converterTable[ProcHash(fromType, toType) & CONVERTERHASHMASK];
    while (p != NULL) {
	if ((fromType == p->from) && (toType == p->to)) {
	    *convertProc = p->proc;
	    return True;
	}
	p = p->next;
    }
    return False;
}

Bool CacheLookup(screen, fromType, from, toType, to, pHash)
    register Screen		*screen;
    register XrmRepresentation	fromType;
    XrmValuePtr			from;
    register XrmRepresentation	toType;
    XrmValuePtr			to;
    int				*pHash;
{
    register CachePtr		p;
    register int		hash;

    hash = CacheHash(fromType, toType, *((char *)from->addr));
    for (p = cacheHashTable[hash & CACHEHASHMASK]; p != NULL; p = p->next) {
	if ((p->hash == hash)
	 && (p->toType == toType)
	 && (p->fromType == fromType)
	 && (p->screen == screen)
	 && (p->from.size == from->size)
	 && (! bcmp((char *)p->from.addr, (char *)from->addr, (int)from->size))) {
	    /* Perfect match */
	    *to = p->to;
	    return True;
	}
    }
    (*to).size = 0;
    (*to).addr = NULL;
    (*pHash) = hash;
    return False;
}

static void CacheEnter(screen, fromType, from, toType, to, hash)
    Screen		*screen;
    XrmRepresentation	fromType;
    XrmValuePtr		from;
    XrmRepresentation	toType;
    XrmValuePtr		to;
    register int	hash;
    {
    register	CachePtr *pHashEntry;
    register	CachePtr p;

    pHashEntry = &cacheHashTable[hash & CACHEHASHMASK];

    p = (CachePtr) Xmalloc(sizeof(CacheRec));
    p->next     = *pHashEntry;
    *pHashEntry = p;
    p->hash     = hash;
    p->screen  = screen;
    p->fromType = fromType;
    p->from     = *from;
    p->from.addr = (caddr_t) Xmalloc(from->size);
    bcopy((char *) from->addr, (char *)p->from.addr, (int) from->size);

    p->toType	= toType;
    p->to       = *to;
    p->to.addr = (caddr_t) Xmalloc(to->size);
    bcopy((char *) to->addr, (char *) p->to.addr, (int) to->size);
    }

void _XrmConvert(screen, fromType, from, toType, to)
    register Screen		*screen;
    register XrmRepresentation	fromType;
	     XrmValuePtr	from;
    register XrmRepresentation	toType;
    register XrmValuePtr	to;
    {
    XrmTypeConverter	proc;
    int			hash;
    char		msg[100];

    if (fromType != toType) {
	if (CacheLookup(screen, fromType, from, toType, to, &hash))
	    return;
	if (LookupConverter(fromType, toType, &proc)) {
	    (*proc)(screen, from, to);
	    if ((*to).addr != NULL) {
	        CacheEnter(screen, fromType, from, toType, to, hash);
	    } else {
#ifdef notdef
	        (void)sprintf(msg, "Bad source value for '%s' to '%s' conversion.\n",
		    XrmRepresentationToAtom(fromType),
		    XrmRepresentationToAtom(toType));
/* ||| Some sort of pluggable warning call */
	    	(void) fprintf(stderr, "%s", msg);
#endif
	    }
	} else {
	    (void)sprintf(msg, "No type converter registered for '%s' to '%s'.\n",
	    	XrmRepresentationToAtom(fromType),
		XrmRepresentationToAtom(toType));
/* ||| Some sort of pluggable warning call */
	    (void) fprintf(stderr, "%s", msg);
	}
    } else {
	(*to).size = from->size;
	(*to).addr = from->addr;
    }
}

void XrmConvert(screen, fromType, from, toType, to)
    Screen	*screen;
    XrmAtom	fromType;
    XrmValuePtr	from;
    XrmAtom	toType;
    XrmValuePtr	to;
{
    _XrmConvert(screen, XrmAtomToRepresentation(fromType), from,
	       XrmAtomToRepresentation(toType),   to);
}
