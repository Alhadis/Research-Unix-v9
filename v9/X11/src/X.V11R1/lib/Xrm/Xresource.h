/* $Header: Xresource.h,v 1.1 87/09/12 12:27:11 toddb Exp $ */
/* $Header: Xresource.h,v 1.1 87/09/12 12:27:11 toddb Exp $ */
/*
 *	sccsid:	%W%	%G%
 */

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

#ifndef _Xresource_h
#define _Xresource_h

/****************************************************************
 ****************************************************************
 ***                                                          ***
 ***                                                          ***
 ***          X Resource Manager Intrinsics                   ***
 ***                                                          ***
 ***                                                          ***
 ****************************************************************
 ****************************************************************/



/****************************************************************
 *
 * Miscellaneous definitions
 *
 ****************************************************************/

#include	<sys/types.h>

#ifndef NULL
#define NULL 0
#endif

/****************************************************************
 *
 * Quark Management
 *
 ****************************************************************/

typedef int     XrmQuark, *XrmQuarkList;
#define NULLQUARK ((XrmQuark) 0)

typedef char *XrmAtom;
#define NULLATOM ((XrmAtom) 0)

/* find quark for atom, create new quark if none already exists */
extern XrmQuark XrmAtomToQuark(); /* name */
    /* XrmAtom name; */

/* find atom for quark */
extern XrmAtom XrmQuarkToAtom(); /* quark */
    /* XrmQuark name; */

extern XrmQuark XrmUniqueQuark();

#define XrmAtomsEqual(a1, a2) (strcmp(a1, a2) == 0)


/****************************************************************
 *
 * Quark Lists
 *
 ****************************************************************/

extern void XrmStringToQuarkList(); /* name, quarks */
    /* char	    *name;  */
    /* XrmQuarkList  quarks; */

extern XrmQuarkList XrmNewQuarkList();

extern XrmQuarkList XrmFreeQuarkList(); /* list */
    /* XrmQuarkList list; */

extern int XrmQuarkListLength(); /* list */
    /* XrmQuarkList list   */

extern XrmQuarkList XrmCopyQuarkList(); /* list */
    /* XrmQuarkList	list; */


/****************************************************************
 *
 * Name and Class lists.
 *
 ****************************************************************/

/* ||| Should be opaque types */

typedef XrmQuark     XrmName;
typedef XrmQuarkList XrmNameList;
#define XrmNameToAtom(name)		(XrmQuarkToAtom((XrmQuark) (name)))
#define XrmAtomToName(name)		((XrmName) XrmAtomToQuark((XrmAtom) (name)))
#define XrmNameListLength(names) 	XrmQuarkListLength((XrmQuarkList)(names))
#define XrmStringToNameList(str, name)	XrmStringToQuarkList(str, (XrmQuarkList)(name))
#define XrmFreeNameList(name)		XrmFreeQuarkList((XrmQuarkList)(name))

typedef XrmQuark     XrmClass;
typedef XrmQuarkList XrmClassList;
#define XrmClassToAtom(class)		(XrmQuarkToAtom((XrmQuark) (class)))
#define XrmAtomToClass(class)		((XrmClass) XrmAtomToQuark((XrmAtom) (class)))
#define XrmClassListLength(classes) 	XrmQuarkListLength((XrmQuarkList)(classes))
#define XrmStringToClassList(str,class)	XrmStringToQuarkList(str, (XrmQuarkList)(class))
#define XrmFreeClassList(class)		XrmFreeQuarkList((XrmQuarkList)(class))



/****************************************************************
 *
 * Resource Types and Conversions
 *
 ****************************************************************/

typedef struct {
    unsigned int	size;
    caddr_t		addr;
} XrmValue, *XrmValuePtr;

typedef	void (*XrmTypeConverter)(); /* from, to */
    /* XrmValue    from; */
    /* XrmValue    *to; */

extern void XrmRegisterTypeConverter(); /*fromType, toType, converter*/
    /* XrmAtom		fromType, toType; */
    /* XrmTypeConverter  converter; */

extern void XrmConvert(); /* screen, fromType, from, toType, to*/
    /* Screen	    *screen;		*/
    /* XrmAtom       fromType		*/
    /* XrmValue     from;		*/
    /* XrmAtom       toType;		*/
    /* XrmValue     *to;      /* RETURN */

/****************************************************************
 *
 * Resource Manager Functions
 *
 ****************************************************************/

typedef struct _XrmHashBucketRec *XrmHashBucket;
typedef XrmHashBucket *XrmHashTable;
typedef XrmHashTable XrmSearchList[];

extern void XrmInitialize();

extern void XrmPutResource(); /*quarks, type, val*/
    /* XrmQuarkList      quarks;     */
    /* XrmRepresentation type;       */
    /* XrmValue		val;	    */

extern void XrmGetResource(); /* screen, rdb, names, classes, destType, val*/
    /* Screen		*screen;    */
    /* XrmResourceDataBase rdb	    */
    /* XrmNameList	names;      */
    /* XrmClassList 	classes;    */
    /* XrmRepresentation destType;   */
    /* XrmValue		*val;       */

extern void XrmGetSearchList(); /* rdb, names, classes, searchList */
    /* XrmResourceDataBase rdb	    */
    /* XrmNameList   names;		    */
    /* XrmClassList  classes;		    */
    /* SearchList   searchList;   /* RETURN */

extern void XrmGetSearchResource();
/* screen, searchList, name, class, type, pVal */
    /* Screen	    *screen;		    */
    /* SearchList   searchList;		    */
    /* XrmName       name;		    */
    /* XrmClass      class;		    */
    /* XrmAtom       type;		    */
    /* XrmValue     *pVal;        /* RETURN */

/****************************************************************
 *
 * Resource Database Management
 *
 ****************************************************************/

typedef struct		_XrmResourceDataBase *XrmResourceDataBase;
typedef int		unspecified;

extern XrmResourceDataBase XrmGetDataBase(); /* filename*/
    /* char *filename; /*  file name */

extern XrmResourceDataBase XrmLoadDataBase(); /* data */
    /* char *data; 		/*  null terminated string. */

extern void XrmPutDataBase(); /* db, magicCookie */
    /* XrmResourceDataBase db;				*/
    /* unspecified magicCookie;     /*  *FILE, actually */

extern void XrmMergeDataBases(); /* new, into */
    /* XrmResourceDataBase new;		    */
    /* XrmResourceDataBase *into;    /* RETURN */




/****************************************************************
 *
 * Command line option mapping to resource entries
 *
 ****************************************************************/

typedef enum {
    XrmoptionNoArg,      /* Value is specified in OptionDescRec.value	    */
    XrmoptionIsArg,      /* Value is the option string itself		    */
    XrmoptionStickyArg,  /* Value is characters immediately following option */
    XrmoptionSepArg,     /* Value is next argument in argv		    */
    XrmoptionSkipArg,    /* Ignore this option and the next argument in argv */
    XrmoptionSkipLine    /* Ignore this option and the rest of argv	    */
} XrmOptionKind;

typedef struct {
    char	*option;	/* Option abbreviation in argv		    */
    char	*resourceName;  /* Resource name (sans application name)    */
    XrmOptionKind  argKind;	/* Which style of option it is		    */
    caddr_t     value;		/* Value to provide if XrmoptionNoArg    */
} XrmOptionDescRec, *XrmOptionDescList;

extern void XrmParseCommand(); /* table, prependName, argc, argv */
    /* XrmResourceDataBase *rdb;*/
    /* XrmOptionDescList   table;					    */
    /* int		tableCount;					    */
    /* XrmAtom		prependName; (NULLATOM means don't prepend)	    */
    /* int		*argc;						    */
    /* char		**argv;						    */


/****************************************************************
 *
 * Predefined atoms
 *
 ****************************************************************/

/* Representation types */

#define XrmRBoolean		"Boolean"
#define XrmRColor		"Color"
#define XrmRCursor		"Cursor"
#define XrmRDims		"Dims"
#define XrmRDisplay		"Display"
#define XrmRFile		"File"
#define XrmRFloat		"Float"
#define XrmRFont		"Font"
#define XrmRFontStruct		"FontStruct"
#define XrmRGeometry		"Geometry"
#define XrmRInt			"Int"
#define XrmRPixel		"Pixel"
#define XrmRPixmap		"Pixmap"
#define XrmRPointer		"Pointer"
#define XrmRString		"String"
#define XrmRWindow		"Window"


/* Boolean enumeration constants */

#define XrmEoff			"off"
#define XrmEfalse		"false"
#define XrmEno			"no"

#define XrmEon			"on"
#define XrmEtrue		"true"
#define XrmEyes			"yes"




#endif _Xresource_h
/* DON'T ADD STUFF AFTER THIS #endif */
