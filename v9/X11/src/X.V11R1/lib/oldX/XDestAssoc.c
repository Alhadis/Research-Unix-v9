#include "copyright.h"

/* $Header: XDestAssoc.c,v 10.15 87/09/11 07:56:38 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985	*/

#include "Xlibint.h"
#include "X10.h"

/*
 * XDestroyAssocTable - Destroy (free the memory associated with)
 * an XAssocTable.  
 */
XDestroyAssocTable(table)
	register XAssocTable *table;
{
	register int i;
	register XAssoc *bucket;
	register XAssoc *Entry, *entry_next;

	/* Free the buckets. */
	for (i = 0; i < table->size; i++) {
		bucket = &table->buckets[i];
		for (
	        	Entry = bucket->next;
			Entry != bucket;
			Entry = entry_next
		) {
		        entry_next = Entry->next;
			Xfree((char *)Entry);
		}
	}

	/* Free the bucket array. */
	Xfree((char *)table->buckets);

	/* Free the table. */
	Xfree((char *)table);
}

