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

/* $Header: atom.c,v 1.21 87/09/11 07:18:24 toddb Exp $ */

#include "X.h"
#include "Xatom.h"
#include "misc.h"

#define InitialTableSize 100

char *Xalloc();

typedef struct _Node {
    struct _Node   *left,   *right;
    Atom a;
    unsigned int fingerPrint;
    char   *string;
} NodeRec, *NodePtr;

static Atom lastAtom = None;
static NodePtr atomRoot = (NodePtr)NULL;
static int tableLength;
static NodePtr *nodeTable;

Atom 
MakeAtom(string, len, makeit)
    char *string;
    int len;
    Bool makeit;
{
    register    NodePtr * np;
    int     comp, i;
    register unsigned int   fp = 0;

    np = &atomRoot;
    for (i = 0; i < (len+1)/2; i++)
    {
	fp = fp * 27 + string[i];
	fp = fp * 27 + string[len - 1 - i];
    }
    while (*np != (NodePtr) NULL)
    {
	if (fp < (*np)->fingerPrint)
	    np = &((*np)->left);
	else if (fp > (*np)->fingerPrint)
	    np = &((*np)->right);
	else
	{			       /* now start testing the strings */
	    comp = strncmp(string, (*np)->string, len);
	    if ((comp < 0) || ((comp == 0) && (len < strlen((*np)->string))))
		np = &((*np)->left);
	    else if (comp > 0)
		np = &((*np)->right);
	    else
		return(*np)->a;
	    }
    }
    if (makeit)
    {
	*np = (NodePtr) Xalloc(sizeof(NodeRec));
	(*np)->left = (*np)->right = (NodePtr) NULL;
	(*np)->fingerPrint = fp;
	(*np)->a = (++lastAtom);
	strncpy((*np)->string = Xalloc(len + 1), string, len);
	(*np)->string[len] = 0;
	if (lastAtom >= tableLength)
	{
	    tableLength *= 2;
	    nodeTable = (NodePtr *) Xrealloc(
		    nodeTable, tableLength * sizeof(NodePtr));
	}
	nodeTable[lastAtom] = *np;
	return(*np)->a;
    }
    else
	return None;
}

ValidAtom(atom)
    Atom atom;
{
    return (atom != None) && (atom <= lastAtom);
}

char *
NameForAtom(atom)
    Atom atom;
{
    NodePtr node;
    if (atom > lastAtom) return 0;
    if ((node = nodeTable[atom]) == (NodePtr)NULL) return 0;
    return node->string;
}

AtomError()
{
    FatalError("initializing atoms");
}

InitAtoms()
{
    FreeAllAtoms();
    tableLength = InitialTableSize;
    nodeTable = (NodePtr *)Xalloc(InitialTableSize*sizeof(NodePtr));
    nodeTable[None] = (NodePtr)NULL;
    MakePredeclaredAtoms();
    if (lastAtom != XA_LAST_PREDEFINED)
	AtomError ();
}

FreeAllAtoms()
{
    if(atomRoot == (NodePtr)NULL)
	return;
    FreeAtom(atomRoot);
    atomRoot = (NodePtr)NULL;
    Xfree(nodeTable);
    nodeTable = (NodePtr *)NULL;
    lastAtom = None;
}

FreeAtom(patom)
    NodePtr patom;
{
    if(patom->left)
	FreeAtom(patom->left);
    if(patom->right)
	FreeAtom(patom->right);
    Xfree(patom->string);
    Xfree(patom);
}
    
