/* $Header: Xrm.c,v 1.2 87/09/03 18:19:16 newman Exp $ */
#ifndef lint
static char *sccsid = "@(#)Xrm.c	1.11	3/20/87";
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
#include	"Quarks.h"
#include	<stdio.h>
#include	<ctype.h>

extern void bcopy();

typedef	void (*DBEnumProc)();

#define HASHSIZE	64
#define HASHMASK	63
#define HashIndex(quark)	(quark & HASHMASK)

/*
typedef struct _XrmHashBucketRec	*XrmHashBucket;
*/
typedef struct _XrmHashBucketRec {
    XrmHashBucket		next;
    XrmQuark		quark;
    XrmResourceDataBase	db;
} XrmHashBucketRec;

/*
typedef XrmHashBucket	*XrmHashTable;
*/

/*
typedef XrmHashTable XrmSearchList[];
*/

typedef struct _XrmResourceDataBase {
    XrmRepresentation	type;
    XrmValue		val;
    XrmHashBucket	*hashTable;
    } XrmResourceDataBaseRec;


static XrmResourceDataBase rdb = NULL;

static Bool FindName(quark, hashTable, ppBucket)
    register	XrmQuark		quark;
    		XrmHashTable	hashTable;
    		XrmHashBucket	**ppBucket;
{
/*  search hashTable (which must be non-NULL) for quark.
    If found, *ppBucket is the address of the XrmHashBucket that points to it.
    If not,   *ppBucket is the address of the hashTable entry that should 
    	      point to a new XrmHashBucket; **ppBucket is the XrmHashBucket it 
	      should point to.
*/

    register XrmHashBucket *pBucket, *pStartBucket;
    
    pBucket = pStartBucket = &hashTable[HashIndex(quark)];

    while ((*pBucket) != NULL) {
	if ((*pBucket)->quark == quark) {
	    *ppBucket = pBucket;
	    return True;
	}
	pBucket = &((*pBucket)->next);
    }
    *ppBucket = pStartBucket;
    return False;
}


static void SetValue(type, val, pdb)
    XrmRepresentation	type;
    XrmValue		val;
    XrmResourceDataBase	*pdb;
{
    register	XrmResourceDataBase db = *pdb;

   if (db == NULL) {
	*pdb = db = (XrmResourceDataBase) Xmalloc (sizeof(XrmResourceDataBaseRec));
	db->type = NULLQUARK;
	db->val.addr = NULL;
	db->val.size = 0;
	db->hashTable = NULL;
    }

    db->type = type;
    if (db->val.addr != NULL)
        Xfree((char *)db->val.addr);
    db->val.addr = (caddr_t) Xmalloc(val.size);
    bcopy((char *)val.addr, (char *)db->val.addr, (int) val.size);
    db->val.size = val.size;
}

static void MakeNewDb(quarks, type, val, pdb)
    register XrmQuarkList	quarks;
	     XrmRepresentation	type;
    	     XrmValue		val;
    	     XrmResourceDataBase *pdb;
{
    register 	XrmResourceDataBase db;
    register	XrmHashBucket       *pBucket, bucket;

    /* make a new database tree rooted at *pdb, initialized with quark/val */
    /* quarks[0] can be NULLQUARK, in which case just set the value */

    for (; *quarks != NULLQUARK; quarks++) {
	db = *pdb = (XrmResourceDataBase) Xmalloc(sizeof(XrmResourceDataBaseRec));
	db->type = NULLQUARK;
	db->val.size = 0;
	db->val.addr = NULL;
	db->hashTable = (XrmHashTable) Xmalloc(sizeof(XrmHashBucket) * HASHSIZE);
	bzero((char *) db->hashTable, sizeof(XrmHashBucket) * HASHSIZE);
	pBucket = &(db->hashTable[HashIndex(*quarks)]);
	*pBucket = bucket = (XrmHashBucket) Xmalloc(sizeof(XrmHashBucketRec));
	bucket->next = NULL;
	bucket->quark = *quarks;
	bucket->db = NULL;
	pdb = &(bucket->db);
    }

    SetValue(type, val, pdb);
}

static void AddNameToLevel(quarks, pBucket, type, val)
    XrmQuarkList		quarks;
    XrmHashBucket		*pBucket;
    XrmRepresentation	type;
    XrmValue		val;
{
    /* add a new bucket to this level at pBucket */

    register XrmHashBucket bucket;

    /* Prepend new bucket to front of list */
    bucket = (XrmHashBucket) Xmalloc(sizeof(XrmHashBucketRec));
    bucket->next = *pBucket;
    *pBucket = bucket;
    bucket->quark = *quarks;
    bucket->db = NULL;

    MakeNewDb(&quarks[1], type, val, &(bucket->db));

}

static void PutEntry(quarks, type, val, db)
    register XrmQuarkList	quarks;
	     XrmRepresentation	type;
    	     XrmValue		val;
    register XrmResourceDataBase	*db;
{
    XrmHashBucket *pBucket;

    for (; *quarks != NULLQUARK; quarks++) {
	if (*db == NULL) {
	    MakeNewDb(quarks, type, val, db);
	    return;
	}
	if (! FindName(*quarks, (*db)->hashTable, &pBucket)) {
	    AddNameToLevel(quarks, pBucket, type, val);
	    return;
	    }
	db = &((*pBucket)->db);
	}

    /* update value for entry i */
    SetValue(type, val, db);

}

static Bool GetEntry(names, classes, type, val, hashTable)
    register XrmNameList        names;
    register XrmClassList       classes;
    	     XrmRepresentation  *type;
    	     XrmValue	       *val;
    register XrmHashTable         hashTable;
{
    register XrmHashBucket	 bucket;
    register XrmHashTable	 nextHashTable;

    for (; *names != NULLQUARK; names++, classes++) {
    	bucket = hashTable[HashIndex(*names)];
	while (bucket != NULL) {
	    if (bucket->quark == *names) {
	    	if (names[1] == NULLQUARK) {
		    *val = bucket->db->val;
		    /* Must be leaf node with data, else doesn't really match */
		    if ((*val).addr) {
			*type = bucket->db->type;
		    	return True;
		    } else
			return False;
	    	} else if ((nextHashTable = bucket->db->hashTable)
		 && GetEntry(names+1, classes+1, type, val, nextHashTable)) {
	    	    return True;
	    	} else {
		    break;
		}
	    }
	    bucket = bucket->next;
    	}
    	bucket = hashTable[HashIndex(*classes)];
	while (bucket != NULL) {
	    if (bucket->quark == *classes) {
	    	if (classes[1] == NULLQUARK) {
		    *val = bucket->db->val;
		    /* Must be leaf node with data, else doesn't really match */
		    if ((*val).addr) {
			*type = bucket->db->type;
		    	return True;
		    } else
			return False;
	    	} else if ((nextHashTable = bucket->db->hashTable)
		 && GetEntry(names+1, classes+1, type, val, nextHashTable)) {
	    	    return True;
	    	} else {
		    break;
		}
	    }
	    bucket = bucket->next;
    	}
    }
    return False;
}


static int searchListCount;

static void GetSearchList(names, classes, searchList, hashTable)
    register XrmNameList  names;
    register XrmClassList classes;
    XrmSearchList		 searchList;
    register XrmHashTable	 hashTable;
{
    register XrmHashBucket	 bucket;
    register XrmHashTable	 nextHashTable;

    for (; *names != NULLQUARK; names++, classes++) {
    	bucket = hashTable[HashIndex(*names)];
	while (bucket != NULL) {
	    if (bucket->quark == *names) {
		nextHashTable = bucket->db->hashTable;
	    	if (nextHashTable) {
		    if (names[1] != NULLQUARK)
			GetSearchList(names+1,classes+1, 
			 searchList,nextHashTable);
		    searchList[searchListCount++] = nextHashTable;
		}
		break;
	    }
	    bucket = bucket->next;
    	}
    	bucket = hashTable[HashIndex(*classes)];
	while (bucket != NULL) {
	    if (bucket->quark == *classes) {
		nextHashTable = bucket->db->hashTable;
	    	if (nextHashTable) {
		    if (classes[1] != NULLQUARK)
			GetSearchList(names+1,classes+1,
			 searchList,nextHashTable);
		    searchList[searchListCount++] = nextHashTable;
		}
		break;
	    }
	    bucket = bucket->next;
    	}
    }
}

void XrmGetSearchList(names, classes, searchList)
    XrmNameList  names;
    XrmClassList classes;
    XrmSearchList	searchList;	/* RETURN */
{
    searchListCount = 0;
    if (rdb && rdb->hashTable) {
        GetSearchList(names, classes, searchList, rdb->hashTable);
        searchList[searchListCount++] = rdb->hashTable;
    }
    searchList[searchListCount] = NULL;
}

void XrmGetSearchResource(dpy, searchList, name, class, type, pVal)
	     Display	*dpy;
    register XrmSearchList	searchList;
    register XrmName	name;
    register XrmClass	class;
    	     XrmAtom	type;
    	     XrmValue	*pVal;	/* RETURN */
{
    register XrmHashBucket	bucket;
    register int	nameHash = HashIndex(name);
    register int	classHash = HashIndex(class);

    for (; (*searchList) != NULL; searchList++) {
	bucket = (*searchList)[nameHash];
	while (bucket != NULL) {
	    if (bucket->quark == name) {
		if (bucket->db->val.addr != NULL) {
		    /* Leaf node, it really matches */
		    _XrmConvert(dpy, bucket->db->type, bucket->db->val,
		               XrmAtomToRepresentation(type), 	pVal);
		    return;
		}
		break;
	    }
	    bucket = bucket->next;
    	}
	bucket = (*searchList)[classHash];
	while (bucket != NULL) {
	    if (bucket->quark == class) {
		if (bucket->db->val.addr != NULL) {
		    /* Leaf node, it really matches */
		    _XrmConvert(dpy, bucket->db->type, bucket->db->val,
		               XrmAtomToRepresentation(type), 	pVal);
		    return;
		}
		break;
	    }
	    bucket = bucket->next;
    	}
    }
    (*pVal).addr = NULL;
    (*pVal).size = 0;
}


void XrmPutResource(quarks, type, value)
    XrmQuarkList		quarks;
    XrmRepresentation	type;
    XrmValue		value;
{
    PutEntry(quarks, type, value, &rdb);
}

void XrmSetCurrentDataBase(db)
    XrmResourceDataBase	db;
{
    rdb = db;
}

void XrmGetCurrentDataBase(db)
    XrmResourceDataBase	*db;
{
    if (db != NULL)
	*db = rdb;
}

void XrmGetDataBase(magicCookie, db)
    FILE		*magicCookie;
    XrmResourceDataBase	*db;
{
    char		buf[1000], *s, *valStr;
    XrmQuark		nl[100];
    int			i;
    XrmResourceDataBase	odb = rdb;
    XrmValue		val;

    *db = NULL;
    if (magicCookie == NULL)
    	return;

    rdb = NULL;
    for (;;) {
	s = fgets(buf, sizeof(buf), magicCookie);
	if (s == NULL) break;
	for (; isspace(s[0]); s++) ;
	if ((s[0] == '\0') || (s[0] == '#')) continue;
	i = strlen(s);
	if (s[i-1] == '\n') s[i-1] = '\0';
	for (i=0 ; ; i++) {
	    if (s[i] == '\0') {
		valStr = "";
		break;
	    }
	    if ((s[i] == ':') || isspace(s[i])) {
		valStr = &s[i+1];
		for (; isspace(valStr[0]); valStr++) ;
		s[i] = '\0';
		break;
	    }
	}
	XrmStringToQuarkList(s, nl);
	val.size = strlen(valStr)+1;
	val.addr = (caddr_t) valStr;
	XrmPutResource(&nl[0], XrmQString, val);
    }
    *db = rdb;
    rdb = odb;
}

static void Enum(quarks, count, db, cd, proc)
    XrmQuarkList	quarks;
    unsigned	count;
    XrmResourceDataBase db;
    unspecified cd;
    DBEnumProc  proc;
{
    unsigned int	i;
    XrmHashBucket bucket;

    if (db == NULL) return;
    if (db->hashTable != NULL) {
	quarks[count+1] = NULLQUARK;
	for (i=0; i < HASHSIZE; i++) {
	    bucket = db->hashTable[i];
	    while (bucket != NULL) {
	    	quarks[count] = bucket->quark;
	    	Enum(quarks, count+1, bucket->db, cd, proc);
		bucket = bucket->next;
	    }
	}
    }
    quarks[count] = NULLQUARK;
    if (db->val.addr != NULL) proc(quarks, db->type, db->val, cd);
    }

static void EnumerateDataBase(db, cd, proc)
    XrmResourceDataBase	db;
    unspecified	cd;
    DBEnumProc	proc;
    {
    XrmQuark	nl[100];
    Enum(nl, 0, db, cd, proc);
}

void PrintQuark(quark)
    XrmQuark	quark;
{
    (void) printf("%s", XrmQuarkToAtom(quark));
}

void PrintQuarkList(quarks)
    XrmQuarkList	quarks;
{
    Bool	firstNameSeen;

    for (firstNameSeen = False; (*quarks) != NULLQUARK; quarks++) {
        if (firstNameSeen) (void) printf(".");
	firstNameSeen = True;
	PrintQuark(*quarks);
    }
}

static void DumpEntry(quarks, type, val, stream)
    XrmQuarkList	     quarks;
    XrmRepresentation type;
    XrmValue	     val;
    FILE	     *stream;
{

    register unsigned int	i;

    for (i=0; quarks[i] != NULLQUARK; i++) {
	if (i != 0) (void) fprintf(stream, ".");
	(void) fprintf(stream, "%s", XrmQuarkToAtom(*quarks));
    }
    if (type == XrmQString) {
	(void) fprintf(stream, ":\t%s\n", val.addr);
    } else {
	(void) fprintf(stream, "!%s:\t", XrmRepresentationToAtom(type));
	for (i = 0; i < val.size; i++)
	    (void) fprintf(stream, "%02x", (int) val.addr[i]);
        (void) fprintf(stream, "\n");
    }
}

void XrmPutDataBase(magicCookie, db)
    FILE		*magicCookie;
    XrmResourceDataBase	db;
{
    EnumerateDataBase(db, (unspecified) magicCookie, DumpEntry);
}

void XrmMergeDataBases(new, into)
    XrmResourceDataBase	new, *into;
{
    EnumerateDataBase(new, (unspecified) into, PutEntry);
}

void XrmGetResource(dpy, names, classes, destType, val)
    Display		*dpy;
    XrmNameList		names;
    XrmClassList 	classes;
    XrmRepresentation	destType;
    XrmValue		*val;
    {
    XrmRepresentation	fromType;
    XrmValue 		from;

    if (rdb && rdb->hashTable
     && GetEntry(names, classes, &fromType, &from, rdb->hashTable)) {
	_XrmConvert(dpy, fromType, from, destType, val);
    } else {
	(*val).addr = NULL;
	(*val).size = 0;
    }
}
