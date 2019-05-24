#include "copyright.h"

/* $Header: XCrAssoc.c,v 10.15 87/09/11 07:56:32 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985	*/

#include "Xlibint.h"
#include "X10.h"

/*
 * XCreateAssocTable - Create an XAssocTable.  The size argument should be
 * a power of two for efficiency reasons.  Some size suggestions: use 32
 * buckets per 100 objects;  a reasonable maximum number of object per
 * buckets is 8.  If there is an error creating the XAssocTable, a NULL
 * pointer is returned.
 */
XAssocTable *XCreateAssocTable(size)
	register int size;		/* Desired size of the table. */
{
	register XAssocTable *table;	/* XAssocTable to be initialized. */
	register XAssoc *buckets;	/* Pointer to the first bucket in */
					/* the bucket array. */
	
	/* XMalloc the XAssocTable. */
	if ((table = (XAssocTable *)Xmalloc(sizeof(XAssocTable))) == NULL) {
		/* XMalloc call failed! */
		errno = ENOMEM;
		return(NULL);
	}
	
	/* XMalloc the buckets (actually just their headers). */
	buckets = (XAssoc *)Xcalloc((unsigned)size, (unsigned)sizeof(XAssoc));
	if (buckets == NULL) {
		/* XCalloc call failed! */
		errno = ENOMEM;
		return(NULL);
	}

	/* Insert table data into the XAssocTable structure. */
	table->buckets = buckets;
	table->size = size;

	while (--size >= 0) {
		/* Initialize each bucket. */
		buckets->prev = buckets;
		buckets->next = buckets;
		buckets++;
	}

	return(table);
}
