/* $Header: Xrm.c,v 1.1 87/09/12 12:27:21 toddb Exp $ */
/* $Header: Xrm.c,v 1.1 87/09/12 12:27:21 toddb Exp $ */
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


static void SetValue(pdb, type, val)
    XrmResourceDataBase	*pdb;
    XrmRepresentation	type;
    XrmValuePtr		val;
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
    db->val.addr = (caddr_t) Xmalloc(val->size);
    bcopy((char *)val->addr, (char *)db->val.addr, (int) val->size);
    db->val.size = val->size;
}

static void MakeNewDb(pdb, quarks, type, val)
    	     XrmResourceDataBase *pdb;
    register XrmQuarkList	quarks;
	     XrmRepresentation	type;
    	     XrmValuePtr	val;
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

    SetValue(pdb, type, val);
}

static void AddNameToLevel(quarks, pBucket, type, val)
    XrmQuarkList		quarks;
    XrmHashBucket		*pBucket;
    XrmRepresentation	type;
    XrmValuePtr		val;
{
    /* add a new bucket to this level at pBucket */

    register XrmHashBucket bucket;

    /* Prepend new bucket to front of list */
    bucket = (XrmHashBucket) Xmalloc(sizeof(XrmHashBucketRec));
    bucket->next = *pBucket;
    *pBucket = bucket;
    bucket->quark = *quarks;
    bucket->db = NULL;

    MakeNewDb(&(bucket->db), &quarks[1], type, val);

}

static void PutEntry(db, quarks, type, val)
    register XrmResourceDataBase	*db;
    register XrmQuarkList	quarks;
	     XrmRepresentation	type;
    	     XrmValue		val;
{
    XrmHashBucket *pBucket;

    for (; *quarks != NULLQUARK; quarks++) {
	if (*db == NULL) {
	    MakeNewDb(db, quarks, type, &val);
	    return;
	}
	if (! FindName(*quarks, (*db)->hashTable, &pBucket)) {
	    AddNameToLevel(quarks, pBucket, type, &val);
	    return;
	    }
	db = &((*pBucket)->db);
	}

    /* update value for entry i */
    SetValue(db, type, &val);

}

static Bool GetEntry(names, classes, type, val, hashTable)
    register XrmNameList        names;
    register XrmClassList       classes;
    	     XrmRepresentation  *type;
    	     XrmValuePtr        val;
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

void XrmGetSearchList(rdb, names, classes, searchList)
    XrmResourceDataBase rdb;
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

void XrmGetSearchResource(screen, searchList, name, class, type, pVal)
	     Screen	*screen;
    register XrmSearchList	searchList;
    register XrmName	name;
    register XrmClass	class;
    	     XrmAtom	type;
    	     XrmValuePtr pVal;	/* RETURN */
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
		    _XrmConvert(screen, bucket->db->type, &bucket->db->val,
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
		    _XrmConvert(screen, bucket->db->type, &bucket->db->val,
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

void XrmPutResource(rdb, quarks, type, value)
    XrmResourceDataBase *rdb;
    XrmQuarkList		quarks;
    XrmRepresentation	type;
    XrmValuePtr		value;
{
    PutEntry(rdb, quarks, type, *value);
}

static char *getstring(buf, nchars, dp)
	char *buf;
	register int nchars;
	char **dp;
{
	register char *p = *dp;
	register char c;
	if (*p == '\0') return NULL;
	while (--nchars > 0) {
		*buf++ = c = *p++;
		if (c == '\n' || c == '\0') {
			*dp = p;
			return (buf);
		}
	}
	return buf;
}

XrmResourceDataBase XrmLoadDataBase(data)
    char 		*data;
{
    char		buf[1000];
    register char	*s, *valStr;
    XrmQuark		nl[100];
    register int	i;
    XrmValue		val;
    char	 	*dp = data;
    XrmResourceDataBase	db;

    if (data == NULL || data[0] == '\0')
    	return NULL;

    for (;;) {
	s = getstring(buf, sizeof(buf), &dp);
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
	XrmPutResource(&db, &nl[0], XrmQString, &val);
    }
    return db;
}

XrmResourceDataBase XrmGetDataBase(magicCookie)
    char 		*magicCookie;
{
    char		buf[1000];
    register char	*s, *valStr;
    XrmQuark		nl[100];
    register int	i;
    XrmValue		val;
    FILE		*magicCookieFile;
    XrmResourceDataBase	db;

    if (magicCookie == NULL)
    	return NULL;

    if((magicCookieFile = fopen(magicCookie, "r")) == NULL){
	return NULL;
    }
    for (;;) {
	s = fgets(buf, sizeof(buf), magicCookieFile);
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
	XrmPutResource(&db, &nl[0], XrmQString, &val);
    }
    fclose(magicCookieFile);
    return db;
}

static void Enum(db, quarks, count, cd, proc)
    XrmResourceDataBase db;
    XrmQuarkList	quarks;
    unsigned	count;
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
	    	Enum(bucket->db, quarks, count+1, cd, proc);
		bucket = bucket->next;
	    }
	}
    }
    quarks[count] = NULLQUARK;
    if (db->val.addr != NULL) proc(cd, quarks, db->type, db->val);
    }

static void EnumerateDataBase(db, cd, proc)
    XrmResourceDataBase	db;
    unspecified	cd;
    DBEnumProc	proc;
    {
    XrmQuark	nl[100];
    Enum(db, nl, 0, cd, proc);
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

void XrmPutDataBase(db, magicCookie)
    XrmResourceDataBase	db;
    char 		*magicCookie;
{
    FILE *magicCookieFile;
    
    if((magicCookieFile = fopen(magicCookie, "w")) == NULL) return;
    EnumerateDataBase(db, (unspecified) magicCookieFile, DumpEntry);
}

void XrmMergeDataBases(new, into)
    XrmResourceDataBase	new, *into;
{
    EnumerateDataBase(new, (unspecified) into, PutEntry);
}

void XrmGetResource(screen, rdb, names, classes, destType, val)
    Screen		*screen;
    XrmResourceDataBase rdb;
    XrmNameList		names;
    XrmClassList 	classes;
    XrmRepresentation	destType;
    XrmValuePtr		val;
    {
    XrmRepresentation	fromType;
    XrmValue 		from;

    if (rdb && rdb->hashTable
     && GetEntry(names, classes, &fromType, &from, rdb->hashTable)) {
	_XrmConvert(screen, fromType, &from, destType, val);
    } else {
	(*val).addr = NULL;
	(*val).size = 0;
    }
}
