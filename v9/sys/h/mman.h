/*	mman.h	6.1	83/07/29	*/

/* protections are chosen from these bits, or-ed together */
#define	PROT_READ	0x1		/* pages can be read */
#define	PROT_WRITE	0x2		/* pages can be written */
#define	PROT_EXEC	0x4		/* pages can be executed */
